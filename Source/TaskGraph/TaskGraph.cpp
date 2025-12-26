#include "TaskGraph.h"

// ============================================================================
// FTaskEvent Implementation
// ============================================================================

FTaskEvent::FTaskEvent()
    : bComplete(false)
{
}

FTaskEvent::~FTaskEvent()
{
}

void FTaskEvent::Signal()
{
    {
        std::lock_guard<std::mutex> Lock(Mutex);
        bComplete = true;
    }
    Condition.notify_all();
}

void FTaskEvent::Wait()
{
    std::unique_lock<std::mutex> Lock(Mutex);
    Condition.wait(Lock, [this]() { return bComplete.load(); });
}

bool FTaskEvent::IsComplete() const
{
    return bComplete.load();
}

// ============================================================================
// FTask Implementation
// ============================================================================

FTask::FTask()
    : Event(std::make_unique<FTaskEvent>())
{
}

FTask::~FTask()
{
}

// ============================================================================
// FLambdaTask Implementation
// ============================================================================

FLambdaTask::FLambdaTask(std::function<void()> InLambda)
    : Lambda(std::move(InLambda))
{
}

FLambdaTask::~FLambdaTask()
{
}

void FLambdaTask::Execute()
{
    if (Lambda)
    {
        Lambda();
    }
    Event->Signal();
}

// ============================================================================
// FTaskGraph Implementation
// ============================================================================

std::unique_ptr<FTaskGraph> FTaskGraph::Singleton;

FTaskGraph::FTaskGraph(uint32 InNumWorkerThreads)
    : bShutdown(false)
    , bInitialized(false)
    , NumThreads(InNumWorkerThreads)
{
    // Auto-detect number of threads if not specified
    if (NumThreads == 0)
    {
        NumThreads = std::thread::hardware_concurrency();
        if (NumThreads == 0)
        {
            NumThreads = 4;  // Fallback to 4 threads
        }
        // Reserve some threads for named threads (Game, Render, RHI)
        if (NumThreads > 3)
        {
            NumThreads -= 3;
        }
        else
        {
            NumThreads = 1;  // At least one worker thread
        }
    }
}

FTaskGraph::~FTaskGraph()
{
    Shutdown();
}

void FTaskGraph::Initialize()
{
    if (bInitialized)
    {
        return;
    }
    
    FLog::Log(ELogLevel::Info, std::string("TaskGraph initializing with ") + std::to_string(NumThreads) + " worker threads");
    
    // Create worker threads
    for (uint32 i = 0; i < NumThreads; ++i)
    {
        WorkerThreads.emplace_back(&FTaskGraph::WorkerThreadLoop, this);
    }
    
    bInitialized = true;
    FLog::Log(ELogLevel::Info, "TaskGraph initialized");
}

void FTaskGraph::Shutdown()
{
    if (!bInitialized)
    {
        return;
    }
    
    FLog::Log(ELogLevel::Info, "TaskGraph shutting down");
    
    // Signal shutdown
    bShutdown = true;
    QueueCondition.notify_all();
    
    // Wait for all worker threads to finish
    for (auto& Thread : WorkerThreads)
    {
        if (Thread.joinable())
        {
            Thread.join();
        }
    }
    WorkerThreads.clear();
    
    // Clean up remaining tasks
    {
        std::lock_guard<std::mutex> Lock(QueueMutex);
        while (!TaskQueue.empty())
        {
            TaskQueue.pop();
        }
    }
    
    // Clean up owned tasks
    {
        std::lock_guard<std::mutex> Lock(OwnedTasksMutex);
        OwnedTasks.clear();
    }
    
    bInitialized = false;
    FLog::Log(ELogLevel::Info, "TaskGraph shutdown complete");
}

FTaskEvent* FTaskGraph::CreateTask(std::function<void()> Lambda)
{
    auto Task = std::make_unique<FLambdaTask>(std::move(Lambda));
    FTaskEvent* Event = Task->GetEvent();
    FTask* TaskPtr = Task.get();  // Get pointer before moving
    
    // Store the task for lifetime management
    {
        std::lock_guard<std::mutex> Lock(OwnedTasksMutex);
        OwnedTasks.push_back(std::move(Task));
    }
    
    // Queue the task (using the pointer we captured before moving)
    QueueTask(TaskPtr);
    
    return Event;
}

void FTaskGraph::QueueTask(FTask* Task)
{
    if (!Task || bShutdown)
    {
        return;
    }
    
    {
        std::lock_guard<std::mutex> Lock(QueueMutex);
        TaskQueue.push(Task);
    }
    QueueCondition.notify_one();
}

FTaskGraph& FTaskGraph::Get()
{
    if (!Singleton)
    {
        Singleton = std::make_unique<FTaskGraph>();
        Singleton->Initialize();
    }
    return *Singleton;
}

void FTaskGraph::WorkerThreadLoop()
{
    while (!bShutdown)
    {
        FTask* Task = nullptr;
        
        // Wait for a task
        {
            std::unique_lock<std::mutex> Lock(QueueMutex);
            QueueCondition.wait(Lock, [this]() {
                return !TaskQueue.empty() || bShutdown;
            });
            
            if (bShutdown && TaskQueue.empty())
            {
                break;
            }
            
            if (!TaskQueue.empty())
            {
                Task = TaskQueue.front();
                TaskQueue.pop();
            }
        }
        
        // Execute the task
        if (Task)
        {
            try
            {
                Task->Execute();
            }
            catch (const std::exception& e)
            {
                FLog::Log(ELogLevel::Error, std::string("Task exception: ") + e.what());
            }
            catch (...)
            {
                FLog::Log(ELogLevel::Error, "Task exception: Unknown error");
            }
        }
    }
}

// ============================================================================
// FRenderFence Implementation
// ============================================================================

FRenderFence::FRenderFence()
    : bFenceSet(false)
{
}

FRenderFence::~FRenderFence()
{
    // Ensure we wait for any pending fence before destruction
    if (bFenceSet && Event && !Event->IsComplete())
    {
        Wait();
    }
}

void FRenderFence::BeginFence()
{
    Event = std::make_shared<FTaskEvent>();
    bFenceSet = true;
}

void FRenderFence::Wait()
{
    if (bFenceSet && Event)
    {
        Event->Wait();
        bFenceSet = false;
    }
}

bool FRenderFence::IsFenceComplete() const
{
    if (bFenceSet && Event)
    {
        return Event->IsComplete();
    }
    return true;  // No fence means complete
}

// ============================================================================
// FThreadManager Implementation
// ============================================================================

std::unique_ptr<FThreadManager> FThreadManager::Singleton;

FThreadManager::FThreadManager()
{
    // Initialize all thread IDs to invalid
    for (int i = 0; i < static_cast<int>(ENamedThreads::NumThreads); ++i)
    {
        ThreadIds[i] = std::thread::id();
    }
}

FThreadManager::~FThreadManager()
{
}

void FThreadManager::Initialize()
{
    FLog::Log(ELogLevel::Info, "ThreadManager initialized");
}

void FThreadManager::Shutdown()
{
    FLog::Log(ELogLevel::Info, "ThreadManager shutdown");
}

bool FThreadManager::IsCurrentThread(ENamedThreads ThreadType) const
{
    std::lock_guard<std::mutex> Lock(ThreadIdMutex);
    return std::this_thread::get_id() == ThreadIds[static_cast<int>(ThreadType)];
}

std::thread::id FThreadManager::GetThreadId(ENamedThreads ThreadType) const
{
    std::lock_guard<std::mutex> Lock(ThreadIdMutex);
    return ThreadIds[static_cast<int>(ThreadType)];
}

void FThreadManager::SetCurrentThread(ENamedThreads ThreadType)
{
    std::lock_guard<std::mutex> Lock(ThreadIdMutex);
    ThreadIds[static_cast<int>(ThreadType)] = std::this_thread::get_id();
    
    const char* ThreadNames[] = { "Game", "Render", "RHI" };
    FLog::Log(ELogLevel::Info, std::string("Thread registered as ") + ThreadNames[static_cast<int>(ThreadType)] + " thread");
}

FThreadManager& FThreadManager::Get()
{
    if (!Singleton)
    {
        Singleton = std::make_unique<FThreadManager>();
        Singleton->Initialize();
    }
    return *Singleton;
}
