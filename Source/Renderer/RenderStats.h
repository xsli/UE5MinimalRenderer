#pragma once

#include "../Core/CoreTypes.h"
#include <chrono>
#include <atomic>

// Render statistics tracker
class FRenderStats 
{
public:
    FRenderStats();
    
    // Update stats each frame
    void BeginFrame();
    void EndFrame();
    
    // Add triangle count for the current frame
    void AddTriangles(uint32 Count);
    
    // Thread timing - called from respective threads
    void BeginGameThreadTiming();
    void EndGameThreadTiming();
    void BeginRenderThreadTiming();
    void EndRenderThreadTiming();
    void BeginRHIThreadTiming();
    void EndRHIThreadTiming();
    
    // Get statistics
    uint64 GetFrameCount() const { return FrameCount; }
    float GetFPS() const { return FPS; }
    float GetFrameTime() const { return FrameTimeMs; }
    uint32 GetTriangleCount() const { return TriangleCount; }
    
    // Thread times in milliseconds
    float GetGameThreadTime() const { return GameThreadTimeMs; }
    float GetRenderThreadTime() const { return RenderThreadTimeMs; }
    float GetRHIThreadTime() const { return RHIThreadTimeMs; }
    
private:
    uint64 FrameCount;
    float FPS;
    float FrameTimeMs;
    uint32 TriangleCount;
    
    std::chrono::high_resolution_clock::time_point FrameStartTime;
    std::chrono::high_resolution_clock::time_point LastFPSUpdateTime;
    uint32 FramesSinceLastFPSUpdate;
    
    // Thread timing
    std::chrono::high_resolution_clock::time_point GameThreadStartTime;
    std::chrono::high_resolution_clock::time_point RenderThreadStartTime;
    std::chrono::high_resolution_clock::time_point RHIThreadStartTime;
    
    std::atomic<float> GameThreadTimeMs;
    std::atomic<float> RenderThreadTimeMs;
    std::atomic<float> RHIThreadTimeMs;
};
