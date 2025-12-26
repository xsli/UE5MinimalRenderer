#pragma once

#include "../Renderer/Renderer.h"
#include "../TaskGraph/TaskGraph.h"
#include "../TaskGraph/RenderCommands.h"
#include "Scene.h"

// Main game class
class FGame 
{
public:
    FGame();
    ~FGame();
    
    bool Initialize(void* WindowHandle, uint32 Width, uint32 Height);
    void Shutdown();
    void Tick(float DeltaTime);
    
    // Get camera for input handling
    FCamera* GetCamera();
    
    // Multi-threading control
    bool IsMultiThreaded() const { return bMultiThreaded; }
    void SetMultiThreaded(bool bEnable) { bMultiThreaded = bEnable; }
    
private:
    // Single-threaded tick (legacy)
    void TickSingleThreaded(float DeltaTime);
    
    // Multi-threaded tick
    void TickMultiThreaded(float DeltaTime);
    
    std::unique_ptr<FRHI> RHI;
    std::unique_ptr<FRenderer> Renderer;
    std::unique_ptr<FScene> Scene;
    
    // Multi-threading flag
    bool bMultiThreaded;
    
    // Frame counter for multi-threaded mode
    uint64 GameFrameNumber;
};
