#pragma once

#include "../Core/CoreTypes.h"
#include <chrono>

// Render statistics tracker
class FRenderStats {
public:
    FRenderStats();
    
    // Update stats each frame
    void BeginFrame();
    void EndFrame();
    
    // Add triangle count for the current frame
    void AddTriangles(uint32 Count);
    
    // Get statistics
    uint64 GetFrameCount() const
{ return FrameCount; }
    float GetFPS() const
{ return FPS; }
    float GetFrameTime() const
{ return FrameTimeMs; }
    uint32 GetTriangleCount() const
{ return TriangleCount; }
    
private:
    uint64 FrameCount;
    float FPS;
    float FrameTimeMs;
    uint32 TriangleCount;
    
    std::chrono::high_resolution_clock::time_point FrameStartTime;
    std::chrono::high_resolution_clock::time_point LastFPSUpdateTime;
    uint32 FramesSinceLastFPSUpdate;
};
