#pragma once

#include "TaskGraph.h"
#include <functional>
#include <queue>
#include <mutex>
#include <memory>
#include <atomic>
#include <condition_variable>

/**
 * Render Command System
 * 
 * This system allows the game thread to enqueue rendering commands
 * that will be executed on the render thread, similar to UE5's
 * ENQUEUE_RENDER_COMMAND macro and render command system.
 * 
 * Key concepts:
 * - FRenderCommandBase: Base class for render commands
 * - FRenderCommandQueue: Thread-safe queue for render commands
 * - ENQUEUE_RENDER_COMMAND: Macro for enqueuing lambda-based commands
 */

// Forward declarations
class FRenderer;
class FRHI;
class FRHICommandList;

/**
 * FRenderCommandBase - Base class for all render commands
 */
class FRenderCommandBase 
{
public:
    virtual ~FRenderCommandBase() = default;
    virtual void Execute() = 0;
    
    // Debug name for tracking
    const char* GetName() const { return Name; }
    
protected:
    const char* Name = "Unknown";
};

/**
 * FLambdaRenderCommand - Render command that executes a lambda
 */
template<typename LambdaType>
class FLambdaRenderCommand : public FRenderCommandBase 
{
public:
    FLambdaRenderCommand(const char* InName, LambdaType&& InLambda)
        : Lambda(std::forward<LambdaType>(InLambda))
    {
        Name = InName;
    }
    
    virtual void Execute() override
    {
        Lambda();
    }
    
private:
    LambdaType Lambda;
};

/**
 * FRenderCommandQueue - Thread-safe queue for render commands
 * 
 * The game thread enqueues commands, and the render thread dequeues
 * and executes them. This implements a producer-consumer pattern
 * similar to UE5's render command pipe.
 */
class FRenderCommandQueue 
{
public:
    FRenderCommandQueue();
    ~FRenderCommandQueue();
    
    // Enqueue a command (called from game thread)
    void EnqueueCommand(std::unique_ptr<FRenderCommandBase> Command);
    
    // Enqueue a lambda command (called from game thread)
    template<typename LambdaType>
    void EnqueueLambda(const char* Name, LambdaType&& Lambda)
    {
        auto Command = std::make_unique<FLambdaRenderCommand<LambdaType>>(
            Name, std::forward<LambdaType>(Lambda));
        EnqueueCommand(std::move(Command));
    }
    
    // Process all pending commands (called from render thread)
    // Returns the number of commands processed
    uint32 ProcessCommands();
    
    // Process one command if available (non-blocking)
    // Returns true if a command was processed
    bool ProcessOneCommand();
    
    // Wait for a command to be available and process it
    // Returns false if shutdown was signaled
    bool WaitAndProcessCommand();
    
    // Signal the queue to wake up for shutdown
    void SignalShutdown();
    
    // Check if there are pending commands
    bool HasPendingCommands() const;
    
    // Get the number of pending commands
    uint32 GetPendingCommandCount() const;
    
    // Get singleton instance
    static FRenderCommandQueue& Get();
    
private:
    std::queue<std::unique_ptr<FRenderCommandBase>> Commands;
    mutable std::mutex QueueMutex;
    std::condition_variable QueueCondition;
    std::atomic<bool> bShutdown;
    
    static std::unique_ptr<FRenderCommandQueue> Singleton;
};

/**
 * FRenderThread - The render thread that processes render commands
 * 
 * Similar to UE5's FRenderingThread, this class manages the render
 * thread lifecycle and command processing.
 */
class FRenderThread 
{
public:
    FRenderThread();
    ~FRenderThread();
    
    // Start the render thread
    void Start();
    
    // Stop the render thread (waits for completion)
    void Stop();
    
    // Check if the render thread is running
    bool IsRunning() const { return bRunning; }
    
    // Set the renderer (called during initialization)
    void SetRenderer(FRenderer* InRenderer) { Renderer = InRenderer; }
    
    // Set the RHI (called during initialization)
    void SetRHI(FRHI* InRHI) { RHI = InRHI; }
    
    // Signal that a new frame's commands are ready
    void SignalFrameReady();
    
    // Wait for current frame to complete (called from game thread)
    void WaitForFrameComplete();
    
    // Get singleton instance
    static FRenderThread& Get();
    
private:
    void ThreadLoop();
    
    std::thread Thread;
    std::atomic<bool> bRunning;
    std::atomic<bool> bShouldStop;
    
    FRenderer* Renderer;
    FRHI* RHI;
    
    // Frame synchronization
    std::mutex FrameMutex;
    std::condition_variable FrameReadyCondition;
    std::condition_variable FrameCompleteCondition;
    std::atomic<bool> bFrameReady;
    std::atomic<bool> bFrameComplete;
    
    static std::unique_ptr<FRenderThread> Singleton;
};

/**
 * FRHIThread - The RHI thread that translates and executes RHI commands
 * 
 * In UE5, the RHI thread is responsible for translating platform-agnostic
 * RHI commands into actual GPU commands. This simplified version just
 * executes the RHI commands.
 */
class FRHIThread 
{
public:
    FRHIThread();
    ~FRHIThread();
    
    // Start the RHI thread
    void Start();
    
    // Stop the RHI thread
    void Stop();
    
    // Check if running
    bool IsRunning() const { return bRunning; }
    
    // Set RHI
    void SetRHI(FRHI* InRHI) { RHI = InRHI; }
    
    // Enqueue RHI work
    void EnqueueWork(std::function<void()> Work);
    
    // Signal frame ready for RHI processing
    void SignalFrameReady();
    
    // Wait for RHI frame complete
    void WaitForFrameComplete();
    
    // Get singleton
    static FRHIThread& Get();
    
private:
    void ThreadLoop();
    
    std::thread Thread;
    std::atomic<bool> bRunning;
    std::atomic<bool> bShouldStop;
    
    FRHI* RHI;
    
    // Work queue
    std::queue<std::function<void()>> WorkQueue;
    std::mutex WorkMutex;
    std::condition_variable WorkCondition;
    
    // Frame synchronization
    std::mutex FrameMutex;
    std::condition_variable FrameReadyCondition;
    std::condition_variable FrameCompleteCondition;
    std::atomic<bool> bFrameReady;
    std::atomic<bool> bFrameComplete;
    
    static std::unique_ptr<FRHIThread> Singleton;
};

/**
 * Frame Synchronization Manager
 * 
 * Manages the synchronization between Game, Render, and RHI threads
 * following UE5's pattern where Game can lead Render by 1 frame.
 */
class FFrameSyncManager 
{
public:
    FFrameSyncManager();
    ~FFrameSyncManager();
    
    // Initialize synchronization
    void Initialize();
    
    // Shutdown synchronization
    void Shutdown();
    
    // Game thread: Start a new frame
    void GameThread_BeginFrame();
    
    // Game thread: End frame (may wait if 1 frame ahead)
    void GameThread_EndFrame();
    
    // Render thread: Begin processing a frame
    void RenderThread_BeginFrame();
    
    // Render thread: End frame
    void RenderThread_EndFrame();
    
    // RHI thread: Begin processing a frame
    void RHIThread_BeginFrame();
    
    // RHI thread: End frame
    void RHIThread_EndFrame();
    
    // Get current game frame number
    uint64 GetGameFrameNumber() const { return GameFrameNumber; }
    
    // Get current render frame number
    uint64 GetRenderFrameNumber() const { return RenderFrameNumber; }
    
    // Get singleton
    static FFrameSyncManager& Get();
    
private:
    std::atomic<uint64> GameFrameNumber;
    std::atomic<uint64> RenderFrameNumber;
    std::atomic<uint64> RHIFrameNumber;
    
    // Fence for game-render synchronization
    std::mutex SyncMutex;
    std::condition_variable GameCanProceedCondition;
    
    // Maximum frame lead (game can be at most this many frames ahead of render)
    static constexpr uint64 MaxFrameLead = 1;
    
    static std::unique_ptr<FFrameSyncManager> Singleton;
};

/**
 * Macro to enqueue a render command
 * Usage: ENQUEUE_RENDER_COMMAND(CommandName)([](){ ... code ... });
 */
#define ENQUEUE_RENDER_COMMAND(CommandName) \
    FRenderCommandQueue::Get().EnqueueLambda(#CommandName, 

#define ENQUEUE_RENDER_COMMAND_END() \
    )

/**
 * Helper macro for cleaner syntax
 * Usage: ENQUEUE_RENDER_COMMAND_LAMBDA(CommandName, [=](){ ... code ... });
 */
#define ENQUEUE_RENDER_COMMAND_LAMBDA(CommandName, Lambda) \
    FRenderCommandQueue::Get().EnqueueLambda(#CommandName, Lambda)
