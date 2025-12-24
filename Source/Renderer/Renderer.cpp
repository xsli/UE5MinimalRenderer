#include "Renderer.h"
#include "../RHI/RHI.h"
#include <iostream>
#include <chrono>

Renderer::Renderer()
    : bIsRunning(false)
    , bRenderThreadActive(false)
    , GameThreadFrameNumber(0)
    , RenderThreadFrameNumber(0)
{
}

Renderer::~Renderer()
{
    if (bIsRunning)
    {
        Shutdown();
    }
}

bool Renderer::Initialize()
{
    std::cout << "[Renderer] Initializing Renderer..." << std::endl;

    // Create RHI instance
    RHI = CreateRHI();
    if (!RHI)
    {
        std::cerr << "[Renderer] Failed to create RHI" << std::endl;
        return false;
    }

    std::cout << "[Renderer] Using RHI: " << RHI->GetAPIName() << std::endl;

    // Initialize RHI (on render thread in real implementation, but simplified here)
    if (!RHI->Initialize())
    {
        std::cerr << "[Renderer] Failed to initialize RHI" << std::endl;
        return false;
    }

    bIsRunning = true;

    // Start the render thread
    StartRenderThread();

    std::cout << "[Renderer] Renderer initialized successfully" << std::endl;
    return true;
}

void Renderer::Shutdown()
{
    std::cout << "[Renderer] Shutting down Renderer..." << std::endl;

    // Stop render thread
    StopRenderThread();

    // Shutdown RHI
    if (RHI)
    {
        RHI->Shutdown();
        RHI.reset();
    }

    bIsRunning = false;

    std::cout << "[Renderer] Renderer shut down" << std::endl;
}

void Renderer::StartRenderThread()
{
    std::cout << "[Renderer] Starting Render Thread..." << std::endl;
    
    bRenderThreadActive = true;
    RenderThread = std::make_unique<std::thread>(&Renderer::RenderThreadMain, this);
    
    std::cout << "[Renderer] Render Thread started" << std::endl;
}

void Renderer::StopRenderThread()
{
    std::cout << "[Renderer] Stopping Render Thread..." << std::endl;

    bRenderThreadActive = false;

    // Wake up render thread if it's waiting
    CommandQueueCV.notify_all();

    // Wait for render thread to finish
    if (RenderThread && RenderThread->joinable())
    {
        RenderThread->join();
    }

    std::cout << "[Renderer] Render Thread stopped" << std::endl;
}

void Renderer::EnqueueRenderCommand(RenderCommand&& Command)
{
    std::lock_guard<std::mutex> Lock(CommandQueueMutex);
    CommandQueue.push(std::move(Command));
    CommandQueueCV.notify_one();
}

void Renderer::BeginFrame()
{
    // Called from game thread
    uint32_t CurrentFrame = GameThreadFrameNumber.load();
    std::cout << "[Renderer::GameThread] Begin Frame " << CurrentFrame << std::endl;
}

void Renderer::EndFrame()
{
    // Called from game thread
    uint32_t CurrentFrame = GameThreadFrameNumber.load();
    std::cout << "[Renderer::GameThread] End Frame " << CurrentFrame << std::endl;
    
    // Increment game thread frame counter
    GameThreadFrameNumber++;
}

void Renderer::RenderThreadMain()
{
    std::cout << "[Renderer::RenderThread] Render thread started" << std::endl;

    while (bRenderThreadActive)
    {
        std::unique_lock<std::mutex> Lock(CommandQueueMutex);
        
        // Wait for commands or shutdown signal
        CommandQueueCV.wait(Lock, [this]() {
            return !CommandQueue.empty() || !bRenderThreadActive;
        });

        // Process all pending commands
        while (!CommandQueue.empty())
        {
            RenderCommand Cmd = std::move(CommandQueue.front());
            CommandQueue.pop();
            
            // Unlock while executing command to allow more commands to be enqueued
            Lock.unlock();
            
            if (Cmd.Execute)
            {
                Cmd.Execute();
            }
            
            Lock.lock();
        }
    }

    std::cout << "[Renderer::RenderThread] Render thread stopped" << std::endl;
}
