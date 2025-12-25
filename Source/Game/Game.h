#pragma once

#include "../Renderer/Renderer.h"

// Game object base class
class FGameObject {
public:
    virtual ~FGameObject() = default;
    virtual void Tick(float DeltaTime) = 0;
    virtual FSceneProxy* CreateSceneProxy(FRHI* RHI) = 0;
};

// Triangle game object
class FTriangleObject : public FGameObject {
public:
    FTriangleObject();
    virtual ~FTriangleObject() override;
    
    virtual void Tick(float DeltaTime) override;
    virtual FSceneProxy* CreateSceneProxy(FRHI* RHI) override;
};

// Cube game object (3D)
class FCubeObject : public FGameObject {
public:
    FCubeObject(FRenderer* InRenderer);
    virtual ~FCubeObject() override;
    
    virtual void Tick(float DeltaTime) override;
    virtual FSceneProxy* CreateSceneProxy(FRHI* RHI) override;
    
private:
    FRenderer* Renderer;
    FCubeMeshProxy* CubeProxy;
    float RotationAngle;
};

// Game world/level
class FGameWorld {
public:
    FGameWorld(FRHI* InRHI, FRenderer* InRenderer);
    ~FGameWorld();
    
    void Initialize();
    void Shutdown();
    void Tick(float DeltaTime);
    
    void AddGameObject(FGameObject* Object);
    
private:
    FRHI* RHI;
    FRenderer* Renderer;
    std::vector<FGameObject*> GameObjects;
};

// Main game class
class FGame {
public:
    FGame();
    ~FGame();
    
    bool Initialize(void* WindowHandle, uint32 Width, uint32 Height);
    void Shutdown();
    void Tick(float DeltaTime);
    
private:
    std::unique_ptr<FRHI> RHI;
    std::unique_ptr<FRenderer> Renderer;
    std::unique_ptr<FGameWorld> World;
};
