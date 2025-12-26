#pragma once

#include "../Renderer/Renderer.h"
#include "Scene.h"

// Main game class
class FGame {
public:
    FGame();
    ~FGame();
    
    bool Initialize(void* WindowHandle, uint32 Width, uint32 Height);
    void Shutdown();
    void Tick(float DeltaTime);
    
    // Get camera for input handling
    FCamera* GetCamera();
    
private:
    std::unique_ptr<FRHI> RHI;
    std::unique_ptr<FRenderer> Renderer;
    std::unique_ptr<FScene> Scene;
};
