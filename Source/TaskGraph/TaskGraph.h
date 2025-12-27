#pragma once

#include "../Core/CoreTypes.h"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <functional>
#include <memory>
#include <atomic>
#include <vector>

/**
 * Simplified TaskGraph system inspired by UE5's FTaskGraph
 * 
 * Key concepts:
 * - FTask: Base class for executable tasks
 * - FTaskEvent: Synchronization primitive for task completion
 * - FTaskGraph: Manages worker threads and task execution
 * 
 * Usage:
 *   FTaskEvent* Event = TaskGraph->CreateTask([]() { DoWork(); });
 *   Event->Wait();  // Block until task completes
 */

// Forward declarations
class FTask;
class FTaskEvent;
class FTaskGraph;

/**
 * FTaskEvent - Synchronization primitive for task completion
 * Similar to UE5's FGraphEventRef but simplified
 */
class FTaskEvent 
{
public:
    FTaskEvent();
    ~FTaskEvent();
    
    // Signal that the task is complete
    void Signal();
    
    // Wait for the task to complete
    void Wait();
    
    // Check if the task is complete without blocking
    bool IsComplete() const;
    
private:
    std::mutex Mutex;
    std::condition_variable Condition;
    std::atomic<bool> bComplete;
};

/**
 * FTask - Base class for tasks that can be executed on worker threads
 * Similar to UE5's TGraphTask
 */
class FTask 
{
public:
    FTask();
    virtual ~FTask();
    
    // Execute the task
    virtual void Execute() = 0;
    
    // Get the event for this task
    FTaskEvent* GetEvent() { return Event.get(); }
    
protected:
    std::unique_ptr<FTaskEvent> Event;
};

/**
 * FLambdaTask - Task that executes a lambda function
 * Convenience class for quick task creation
 */
class FLambdaTask : public FTask 
{
public:
    explicit FLambdaTask(std::function<void()> InLambda);
    virtual ~FLambdaTask() override;
    
    virtual void Execute() override;
    
private:
    std::function<void()> Lambda;
};

/**
 * FTaskGraph - Manages worker threads and task execution
 * Simplified version of UE5's FTaskGraphImplementation
 */
class FTaskGraph 
{
public:
    FTaskGraph(uint32 NumWorkerThreads = 0);  // 0 = auto-detect
    ~FTaskGraph();
    
    // Initialize the task graph
    void Initialize();
    
    // Shutdown the task graph
    void Shutdown();
    
    // Create and queue a task from a lambda
    // Returns the event that can be used to wait for completion
    FTaskEvent* CreateTask(std::function<void()> Lambda);
    
    // Queue an existing task for execution
    void QueueTask(FTask* Task);
    
    // Get singleton instance
    static FTaskGraph& Get();
    
    // Check if initialized
    bool IsInitialized() const { return bInitialized; }
    
private:
    void WorkerThreadLoop();
    
    std::vector<std::thread> WorkerThreads;
    std::queue<FTask*> TaskQueue;
    std::mutex QueueMutex;
    std::condition_variable QueueCondition;
    std::atomic<bool> bShutdown;
    std::atomic<bool> bInitialized;
    uint32 NumThreads;
    
    // Owned tasks (for lifetime management)
    std::vector<std::unique_ptr<FTask>> OwnedTasks;
    std::mutex OwnedTasksMutex;
    
    // Singleton
    static std::unique_ptr<FTaskGraph> Singleton;
};

/**
 * FNamedThread - Represents a named thread like Game, Render, RHI
 * Similar to UE5's ENamedThreads::Type
 */
enum class ENamedThreads : uint8
{
    GameThread = 0,
    RenderThread = 1,
    RHIThread = 2,
    NumThreads
};

/**
 * FRenderFence - Fence for synchronizing between Game and Render threads
 * Similar to UE5's FRenderCommandFence
 */
class FRenderFence 
{
public:
    FRenderFence();
    ~FRenderFence();
    
    // Begin the fence (called from game thread)
    void BeginFence();
    
    // Wait for the fence to complete (blocks game thread)
    void Wait();
    
    // Check if fence is complete without blocking
    bool IsFenceComplete() const;
    
private:
    std::shared_ptr<FTaskEvent> Event;
    std::atomic<bool> bFenceSet;
};

/**
 * FThreadManager - Manages named threads
 * Simplified version of UE5's FTaskGraphInterface for named threads
 */
class FThreadManager 
{
public:
    FThreadManager();
    ~FThreadManager();
    
    // Initialize named threads
    void Initialize();
    
    // Shutdown named threads
    void Shutdown();
    
    // Check if current thread is a specific named thread
    bool IsCurrentThread(ENamedThreads ThreadType) const;
    
    // Get the thread ID for a named thread
    std::thread::id GetThreadId(ENamedThreads ThreadType) const;
    
    // Set the current thread as a named thread (called during thread initialization)
    void SetCurrentThread(ENamedThreads ThreadType);
    
    // Get singleton instance
    static FThreadManager& Get();
    
private:
    std::thread::id ThreadIds[static_cast<int>(ENamedThreads::NumThreads)];
    mutable std::mutex ThreadIdMutex;
    
    static std::unique_ptr<FThreadManager> Singleton;
};

// Helper macros for thread assertions
#define check_game_thread() do { if (!FThreadManager::Get().IsCurrentThread(ENamedThreads::GameThread)) { FLog::Log(ELogLevel::Error, "Expected to be on Game Thread"); } } while(0)
#define check_render_thread() do { if (!FThreadManager::Get().IsCurrentThread(ENamedThreads::RenderThread)) { FLog::Log(ELogLevel::Error, "Expected to be on Render Thread"); } } while(0)
#define check_rhi_thread() do { if (!FThreadManager::Get().IsCurrentThread(ENamedThreads::RHIThread)) { FLog::Log(ELogLevel::Error, "Expected to be on RHI Thread"); } } while(0)
