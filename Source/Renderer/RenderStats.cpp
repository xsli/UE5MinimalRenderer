#include "RenderStats.h"

FRenderStats::FRenderStats()
    : FrameCount(0)
    , FPS(0.0f)
    , FrameTimeMs(0.0f)
    , TriangleCount(0)
    , FramesSinceLastFPSUpdate(0)
    , GameThreadTimeMs(0.0f)
    , RenderThreadTimeMs(0.0f)
    , RHIThreadTimeMs(0.0f)
{
    LastFPSUpdateTime = std::chrono::high_resolution_clock::now();
}

void FRenderStats::BeginFrame()
{
    FrameStartTime = std::chrono::high_resolution_clock::now();
    TriangleCount = 0;  // Reset triangle count for this frame
}

void FRenderStats::EndFrame()
{
    FrameCount++;
    FramesSinceLastFPSUpdate++;
    
    // Calculate frame time
    auto frameEndTime = std::chrono::high_resolution_clock::now();
    auto frameDuration = std::chrono::duration<float, std::milli>(frameEndTime - FrameStartTime);
    FrameTimeMs = frameDuration.count();
    
    // Update FPS every 0.5 seconds
    auto timeSinceLastUpdate = std::chrono::duration<float>(frameEndTime - LastFPSUpdateTime);
    if (timeSinceLastUpdate.count() >= 0.5f)
    {
        FPS = FramesSinceLastFPSUpdate / timeSinceLastUpdate.count();
        LastFPSUpdateTime = frameEndTime;
        FramesSinceLastFPSUpdate = 0;
    }
}

void FRenderStats::AddTriangles(uint32 Count)
{
    TriangleCount += Count;
}

void FRenderStats::BeginGameThreadTiming()
{
    GameThreadStartTime = std::chrono::high_resolution_clock::now();
}

void FRenderStats::EndGameThreadTiming()
{
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration<float, std::milli>(endTime - GameThreadStartTime);
    GameThreadTimeMs = duration.count();
}

void FRenderStats::BeginRenderThreadTiming()
{
    RenderThreadStartTime = std::chrono::high_resolution_clock::now();
}

void FRenderStats::EndRenderThreadTiming()
{
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration<float, std::milli>(endTime - RenderThreadStartTime);
    RenderThreadTimeMs = duration.count();
}

void FRenderStats::BeginRHIThreadTiming()
{
    RHIThreadStartTime = std::chrono::high_resolution_clock::now();
}

void FRenderStats::EndRHIThreadTiming()
{
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration<float, std::milli>(endTime - RHIThreadStartTime);
    RHIThreadTimeMs = duration.count();
}
