#include "RenderCommands.h"
#include "../Renderer/Renderer.h"
#include "../RHI/RHI.h"

// ============================================================================
// FRenderCommandQueue Implementation
// ============================================================================

std::unique_ptr<FRenderCommandQueue> FRenderCommandQueue::Singleton;

FRenderCommandQueue::FRenderCommandQueue()
    : bShutdown(false)
{
}

FRenderCommandQueue::~FRenderCommandQueue()
{
    SignalShutdown();
}

void FRenderCommandQueue::EnqueueCommand(std::unique_ptr<FRenderCommandBase> Command)
{
    if (bShutdown)
    {
        return;
    }
    
    {
        std::lock_guard<std::mutex> Lock(QueueMutex);
        Commands.push(std::move(Command));
    }
    QueueCondition.notify_one();
}

uint32 FRenderCommandQueue::ProcessCommands()
{
    uint32 ProcessedCount = 0;
    
    std::queue<std::unique_ptr<FRenderCommandBase>> LocalCommands;
    
    // Quickly swap the queue to minimize lock time
    {
        std::lock_guard<std::mutex> Lock(QueueMutex);
        std::swap(LocalCommands, Commands);
    }
    
    // Process all commands
    while (!LocalCommands.empty())
    {
        auto& Command = LocalCommands.front();
        if (Command)
        {
            try
            {
                Command->Execute();
            }
            catch (const std::exception& e)
            {
                FLog::Log(ELogLevel::Error, std::string("Render command exception (") + 
                    Command->GetName() + "): " + e.what());
            }
            catch (...)
            {
                FLog::Log(ELogLevel::Error, std::string("Render command exception (") + 
                    Command->GetName() + "): Unknown error");
            }
        }
        LocalCommands.pop();
        ++ProcessedCount;
    }
    
    return ProcessedCount;
}

bool FRenderCommandQueue::ProcessOneCommand()
{
    std::unique_ptr<FRenderCommandBase> Command;
    
    {
        std::lock_guard<std::mutex> Lock(QueueMutex);
        if (Commands.empty())
        {
            return false;
        }
        Command = std::move(Commands.front());
        Commands.pop();
    }
    
    if (Command)
    {
        try
        {
            Command->Execute();
        }
        catch (const std::exception& e)
        {
            FLog::Log(ELogLevel::Error, std::string("Render command exception (") + 
                Command->GetName() + "): " + e.what());
        }
    }
    
    return true;
}

bool FRenderCommandQueue::WaitAndProcessCommand()
{
    std::unique_ptr<FRenderCommandBase> Command;
    
    {
        std::unique_lock<std::mutex> Lock(QueueMutex);
        QueueCondition.wait(Lock, [this]() {
            return !Commands.empty() || bShutdown;
        });
        
        if (bShutdown && Commands.empty())
        {
            return false;
        }
        
        if (!Commands.empty())
        {
            Command = std::move(Commands.front());
            Commands.pop();
        }
    }
    
    if (Command)
    {
        try
        {
            Command->Execute();
        }
        catch (const std::exception& e)
        {
            FLog::Log(ELogLevel::Error, std::string("Render command exception (") + 
                Command->GetName() + "): " + e.what());
        }
    }
    
    return true;
}

void FRenderCommandQueue::SignalShutdown()
{
    bShutdown = true;
    QueueCondition.notify_all();
}

bool FRenderCommandQueue::HasPendingCommands() const
{
    std::lock_guard<std::mutex> Lock(QueueMutex);
    return !Commands.empty();
}

uint32 FRenderCommandQueue::GetPendingCommandCount() const
{
    std::lock_guard<std::mutex> Lock(QueueMutex);
    return static_cast<uint32>(Commands.size());
}

FRenderCommandQueue& FRenderCommandQueue::Get()
{
    if (!Singleton)
    {
        Singleton = std::make_unique<FRenderCommandQueue>();
    }
    return *Singleton;
}

// ============================================================================
// FRenderThread Implementation
// ============================================================================

std::unique_ptr<FRenderThread> FRenderThread::Singleton;

FRenderThread::FRenderThread()
    : bRunning(false)
    , bShouldStop(false)
    , Renderer(nullptr)
    , RHI(nullptr)
    , bFrameReady(false)
    , bFrameComplete(true)
{
}

FRenderThread::~FRenderThread()
{
    Stop();
}

void FRenderThread::Start()
{
    if (bRunning)
    {
        return;
    }
    
    FLog::Log(ELogLevel::Info, "Starting render thread");
    
    bShouldStop = false;
    bRunning = true;
    
    Thread = std::thread(&FRenderThread::ThreadLoop, this);
}

void FRenderThread::Stop()
{
    if (!bRunning)
    {
        return;
    }
    
    FLog::Log(ELogLevel::Info, "Stopping render thread");
    
    bShouldStop = true;
    
    // Signal to wake up the thread
    {
        std::lock_guard<std::mutex> Lock(FrameMutex);
        bFrameReady = true;
    }
    FrameReadyCondition.notify_all();
    FRenderCommandQueue::Get().SignalShutdown();
    
    if (Thread.joinable())
    {
        Thread.join();
    }
    
    bRunning = false;
    FLog::Log(ELogLevel::Info, "Render thread stopped");
}

void FRenderThread::SignalFrameReady()
{
    {
        std::lock_guard<std::mutex> Lock(FrameMutex);
        bFrameReady = true;
        bFrameComplete = false;
    }
    FrameReadyCondition.notify_one();
}

void FRenderThread::WaitForFrameComplete()
{
    std::unique_lock<std::mutex> Lock(FrameMutex);
    FrameCompleteCondition.wait(Lock, [this]() {
        return bFrameComplete.load() || bShouldStop;
    });
}

FRenderThread& FRenderThread::Get()
{
    if (!Singleton)
    {
        Singleton = std::make_unique<FRenderThread>();
    }
    return *Singleton;
}

void FRenderThread::ThreadLoop()
{
    // Register this as the render thread
    FThreadManager::Get().SetCurrentThread(ENamedThreads::RenderThread);
    
    FLog::Log(ELogLevel::Info, "Render thread loop started");
    
    while (!bShouldStop)
    {
        // Wait for a frame to be ready
        {
            std::unique_lock<std::mutex> Lock(FrameMutex);
            FrameReadyCondition.wait(Lock, [this]() {
                return bFrameReady.load() || bShouldStop;
            });
            
            if (bShouldStop)
            {
                break;
            }
            
            bFrameReady = false;
        }
        
        // Process all render commands for this frame
        FRenderCommandQueue::Get().ProcessCommands();
        
        // Signal frame complete
        {
            std::lock_guard<std::mutex> Lock(FrameMutex);
            bFrameComplete = true;
        }
        FrameCompleteCondition.notify_all();
    }
    
    // Process any remaining commands before shutdown
    FRenderCommandQueue::Get().ProcessCommands();
    
    FLog::Log(ELogLevel::Info, "Render thread loop ended");
}

// ============================================================================
// FRHIThread Implementation
// ============================================================================

std::unique_ptr<FRHIThread> FRHIThread::Singleton;

FRHIThread::FRHIThread()
    : bRunning(false)
    , bShouldStop(false)
    , RHI(nullptr)
    , bFrameReady(false)
    , bFrameComplete(true)
{
}

FRHIThread::~FRHIThread()
{
    Stop();
}

void FRHIThread::Start()
{
    if (bRunning)
    {
        return;
    }
    
    FLog::Log(ELogLevel::Info, "Starting RHI thread");
    
    bShouldStop = false;
    bRunning = true;
    
    Thread = std::thread(&FRHIThread::ThreadLoop, this);
}

void FRHIThread::Stop()
{
    if (!bRunning)
    {
        return;
    }
    
    FLog::Log(ELogLevel::Info, "Stopping RHI thread");
    
    bShouldStop = true;
    
    // Signal to wake up the thread
    {
        std::lock_guard<std::mutex> Lock(FrameMutex);
        bFrameReady = true;
    }
    FrameReadyCondition.notify_all();
    WorkCondition.notify_all();
    
    if (Thread.joinable())
    {
        Thread.join();
    }
    
    bRunning = false;
    FLog::Log(ELogLevel::Info, "RHI thread stopped");
}

void FRHIThread::EnqueueWork(std::function<void()> Work)
{
    {
        std::lock_guard<std::mutex> Lock(WorkMutex);
        WorkQueue.push(std::move(Work));
    }
    WorkCondition.notify_one();
}

void FRHIThread::SignalFrameReady()
{
    {
        std::lock_guard<std::mutex> Lock(FrameMutex);
        bFrameReady = true;
        bFrameComplete = false;
    }
    FrameReadyCondition.notify_one();
}

void FRHIThread::WaitForFrameComplete()
{
    std::unique_lock<std::mutex> Lock(FrameMutex);
    FrameCompleteCondition.wait(Lock, [this]() {
        return bFrameComplete.load() || bShouldStop;
    });
}

FRHIThread& FRHIThread::Get()
{
    if (!Singleton)
    {
        Singleton = std::make_unique<FRHIThread>();
    }
    return *Singleton;
}

void FRHIThread::ThreadLoop()
{
    // Register this as the RHI thread
    FThreadManager::Get().SetCurrentThread(ENamedThreads::RHIThread);
    
    FLog::Log(ELogLevel::Info, "RHI thread loop started");
    
    while (!bShouldStop)
    {
        // Wait for frame ready or work available
        {
            std::unique_lock<std::mutex> Lock(FrameMutex);
            FrameReadyCondition.wait(Lock, [this]() {
                return bFrameReady.load() || bShouldStop;
            });
            
            if (bShouldStop)
            {
                break;
            }
            
            bFrameReady = false;
        }
        
        // Process all work for this frame
        {
            std::lock_guard<std::mutex> Lock(WorkMutex);
            while (!WorkQueue.empty())
            {
                auto Work = std::move(WorkQueue.front());
                WorkQueue.pop();
                
                try
                {
                    Work();
                }
                catch (const std::exception& e)
                {
                    FLog::Log(ELogLevel::Error, std::string("RHI work exception: ") + e.what());
                }
            }
        }
        
        // Signal frame complete
        {
            std::lock_guard<std::mutex> Lock(FrameMutex);
            bFrameComplete = true;
        }
        FrameCompleteCondition.notify_all();
    }
    
    FLog::Log(ELogLevel::Info, "RHI thread loop ended");
}

// ============================================================================
// FFrameSyncManager Implementation
// ============================================================================

std::unique_ptr<FFrameSyncManager> FFrameSyncManager::Singleton;

FFrameSyncManager::FFrameSyncManager()
    : GameFrameNumber(0)
    , RenderFrameNumber(0)
    , RHIFrameNumber(0)
{
}

FFrameSyncManager::~FFrameSyncManager()
{
    Shutdown();
}

void FFrameSyncManager::Initialize()
{
    FLog::Log(ELogLevel::Info, "Frame sync manager initialized");
}

void FFrameSyncManager::Shutdown()
{
    // Wake up any waiting threads
    GameCanProceedCondition.notify_all();
    FLog::Log(ELogLevel::Info, "Frame sync manager shutdown");
}

void FFrameSyncManager::GameThread_BeginFrame()
{
    // Check if game is too far ahead of render
    // If game frame > render frame + MaxFrameLead, wait
    {
        std::unique_lock<std::mutex> Lock(SyncMutex);
        GameCanProceedCondition.wait(Lock, [this]() {
            return GameFrameNumber.load() < RenderFrameNumber.load() + MaxFrameLead + 1;
        });
    }
    
    ++GameFrameNumber;
}

void FFrameSyncManager::GameThread_EndFrame()
{
    // Game thread has finished enqueueing render commands for this frame
    // Signal render thread that a frame is ready
    FRenderThread::Get().SignalFrameReady();
}

void FFrameSyncManager::RenderThread_BeginFrame()
{
    ++RenderFrameNumber;
}

void FFrameSyncManager::RenderThread_EndFrame()
{
    // Signal game thread that it can proceed
    GameCanProceedCondition.notify_all();
    
    // Signal RHI thread
    FRHIThread::Get().SignalFrameReady();
}

void FFrameSyncManager::RHIThread_BeginFrame()
{
    ++RHIFrameNumber;
}

void FFrameSyncManager::RHIThread_EndFrame()
{
    // RHI frame complete - nothing specific to do here
}

FFrameSyncManager& FFrameSyncManager::Get()
{
    if (!Singleton)
    {
        Singleton = std::make_unique<FFrameSyncManager>();
        Singleton->Initialize();
    }
    return *Singleton;
}
