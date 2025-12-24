#pragma once

#include <memory>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <functional>

class IRHI;

// Command that can be enqueued to the render thread
struct RenderCommand
{
    std::function<void()> Execute;
};

// Renderer class - Manages the render thread and RHI
class Renderer
{
public:
    Renderer();
    ~Renderer();

    // Initialization and shutdown
    bool Initialize();
    void Shutdown();

    // Thread management
    void StartRenderThread();
    void StopRenderThread();

    // Command submission from game thread to render thread
    void EnqueueRenderCommand(RenderCommand&& Command);

    // Frame synchronization
    void BeginFrame();
    void EndFrame();

    // Render thread main loop
    void RenderThreadMain();

private:
    std::unique_ptr<IRHI> RHI;
    std::unique_ptr<std::thread> RenderThread;
    
    std::atomic<bool> bIsRunning;
    std::atomic<bool> bRenderThreadActive;

    // Command queue for parallel rendering
    std::queue<RenderCommand> CommandQueue;
    std::mutex CommandQueueMutex;
    std::condition_variable CommandQueueCV;

    // Frame synchronization
    std::mutex FrameMutex;
    std::condition_variable FrameCV;
    std::atomic<uint32_t> GameThreadFrameNumber;
    std::atomic<uint32_t> RenderThreadFrameNumber;
};
