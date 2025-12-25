#include "Game.h"
#include "../RHI_DX12/DX12RHI.h"

// FTriangleObject implementation
FTriangleObject::FTriangleObject() {
}

FTriangleObject::~FTriangleObject() {
}

void FTriangleObject::Tick(float DeltaTime) {
    // Could add rotation or animation here
}

FSceneProxy* FTriangleObject::CreateSceneProxy(FRHI* RHI) {
    // Define triangle vertices
    FVertex vertices[] = {
        { FVector(0.0f, 0.5f, 0.0f), FColor(1.0f, 0.0f, 0.0f, 1.0f) },   // Top - Red
        { FVector(0.5f, -0.5f, 0.0f), FColor(0.0f, 1.0f, 0.0f, 1.0f) },  // Right - Green
        { FVector(-0.5f, -0.5f, 0.0f), FColor(0.0f, 0.0f, 1.0f, 1.0f) }  // Left - Blue
    };
    
    // Create vertex buffer
    FRHIBuffer* vertexBuffer = RHI->CreateVertexBuffer(sizeof(vertices), vertices);
    
    // Create pipeline state
    FRHIPipelineState* pso = RHI->CreateGraphicsPipelineState();
    
    // Create and return scene proxy
    return new FTriangleMeshProxy(vertexBuffer, pso, 3);
}

// FGameWorld implementation
FGameWorld::FGameWorld(FRHI* InRHI, FRenderer* InRenderer)
    : RHI(InRHI), Renderer(InRenderer) {
}

FGameWorld::~FGameWorld() {
}

void FGameWorld::Initialize() {
    FLog::Log(ELogLevel::Info, "Game world initialized");
}

void FGameWorld::Shutdown() {
    for (FGameObject* Object : GameObjects) {
        delete Object;
    }
    GameObjects.clear();
    
    FLog::Log(ELogLevel::Info, "Game world shutdown");
}

void FGameWorld::Tick(float DeltaTime) {
    // Tick all game objects
    for (FGameObject* Object : GameObjects) {
        Object->Tick(DeltaTime);
    }
}

void FGameWorld::AddGameObject(FGameObject* Object) {
    GameObjects.push_back(Object);
    
    // Create scene proxy and add to renderer
    FSceneProxy* Proxy = Object->CreateSceneProxy(RHI);
    Renderer->AddSceneProxy(Proxy);
}

// FGame implementation
FGame::FGame() {
}

FGame::~FGame() {
}

bool FGame::Initialize(void* WindowHandle, uint32 Width, uint32 Height) {
    FLog::Log(ELogLevel::Info, "Initializing game...");
    
    // Create RHI
    RHI.reset(CreateDX12RHI());
    if (!RHI->Initialize(WindowHandle, Width, Height)) {
        FLog::Log(ELogLevel::Error, "Failed to initialize RHI");
        return false;
    }
    
    // Create renderer
    Renderer = std::make_unique<FRenderer>(RHI.get());
    Renderer->Initialize();
    
    // Create game world
    World = std::make_unique<FGameWorld>(RHI.get(), Renderer.get());
    World->Initialize();
    
    // Add a triangle to the world
    FTriangleObject* triangle = new FTriangleObject();
    World->AddGameObject(triangle);
    
    FLog::Log(ELogLevel::Info, "Game initialized successfully");
    return true;
}

void FGame::Shutdown() {
    FLog::Log(ELogLevel::Info, "Shutting down game...");
    
    if (World) {
        World->Shutdown();
        World.reset();
    }
    
    if (Renderer) {
        Renderer->Shutdown();
        Renderer.reset();
    }
    
    if (RHI) {
        RHI->Shutdown();
        RHI.reset();
    }
    
    FLog::Log(ELogLevel::Info, "Game shutdown complete");
}

void FGame::Tick(float DeltaTime) {
    // Game thread tick
    if (World) {
        World->Tick(DeltaTime);
    }
    
    // Render thread work (in UE5 this would be on a separate thread)
    if (Renderer) {
        Renderer->RenderFrame();
    }
}
