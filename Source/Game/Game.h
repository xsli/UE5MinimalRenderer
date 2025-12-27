#pragma once

#include "../Renderer/Renderer.h"
#include "../TaskGraph/TaskGraph.h"
#include "../TaskGraph/RenderCommands.h"
#include "../Scene/Scene.h"
#include "../Scene/ScenePrimitive.h"

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
    
    // Get scene
    FScene* GetScene() { return Scene.get(); }
    
private:
    // Single-threaded tick (legacy)
    void TickSingleThreaded(float DeltaTime);
    
    // Multi-threaded tick
    void TickMultiThreaded(float DeltaTime);
    
    // Setup the demo scene with lighting
    void SetupScene();
    
    std::unique_ptr<FRHI> RHI;
    std::unique_ptr<FRenderer> Renderer;
    std::unique_ptr<FScene> Scene;
    
    // Screen-space gizmo (rendered separately in corner)
    FGizmoPrimitive* ScreenGizmo;
    FSceneProxy* ScreenGizmoProxy;
    
    // Multi-threading flag
    bool bMultiThreaded;
    
    // Frame counter for multi-threaded mode
    uint64 GameFrameNumber;
};
