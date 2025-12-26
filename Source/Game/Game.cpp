#include "Game.h"
#include "Primitive.h"
#include "GameGlobals.h"
#include "../RHI_DX12/DX12RHI.h"

// Define the global camera pointer (declared in GameGlobals.h)
FCamera* g_Camera = nullptr;

// FGame implementation
FGame::FGame()
{
}

FGame::~FGame()
{
}

bool FGame::Initialize(void* WindowHandle, uint32 Width, uint32 Height)
{
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
    
    // Set global camera reference
    g_Camera = Renderer->GetCamera();
    
    // Create scene
    Scene = std::make_unique<FScene>(RHI.get());
    
    // Create and add multiple primitives to demonstrate the system
    
    // Cube at center-left, rotating
    FCubePrimitive* cube = new FCubePrimitive();
    cube->SetColor(FColor(1.0f, 0.3f, 0.3f, 1.0f));  // Reddish
    cube->GetTransform().Position = FVector(-2.0f, 0.5f, 0.0f);
    cube->GetTransform().Scale = FVector(0.8f, 0.8f, 0.8f);
    cube->SetAutoRotate(true);
    Scene->AddPrimitive(cube);
    
    // Sphere at center-right, rotating
    FSpherePrimitive* sphere = new FSpherePrimitive(24, 16);
    sphere->SetColor(FColor(0.3f, 0.7f, 1.0f, 1.0f));  // Blue
    sphere->GetTransform().Position = FVector(2.0f, 0.5f, 0.0f);
    sphere->GetTransform().Scale = FVector(0.8f, 0.8f, 0.8f);
    sphere->SetAutoRotate(true);
    Scene->AddPrimitive(sphere);
    
    // Cylinder in front, rotating
    FCylinderPrimitive* cylinder = new FCylinderPrimitive(20);
    cylinder->SetColor(FColor(0.3f, 1.0f, 0.3f, 1.0f));  // Green
    cylinder->GetTransform().Position = FVector(0.0f, 0.0f, 2.0f);
    cylinder->GetTransform().Scale = FVector(0.6f, 1.0f, 0.6f);
    cylinder->SetAutoRotate(true);
    Scene->AddPrimitive(cylinder);
    
    // Ground plane
    FPlanePrimitive* plane = new FPlanePrimitive(4);
    plane->SetColor(FColor(0.6f, 0.6f, 0.6f, 1.0f));  // Gray
    plane->GetTransform().Position = FVector(0.0f, -1.0f, 0.0f);
    plane->GetTransform().Scale = FVector(10.0f, 1.0f, 10.0f);
    Scene->AddPrimitive(plane);
    
    // Small cube on the back-left
    FCubePrimitive* smallCube = new FCubePrimitive();
    smallCube->SetColor(FColor(1.0f, 1.0f, 0.3f, 1.0f));  // Yellow
    smallCube->GetTransform().Position = FVector(-1.5f, 0.2f, -2.0f);
    smallCube->GetTransform().Scale = FVector(0.4f, 0.4f, 0.4f);
    smallCube->SetAutoRotate(true);
    Scene->AddPrimitive(smallCube);
    
    // Small sphere on the back-right
    FSpherePrimitive* smallSphere = new FSpherePrimitive(16, 12);
    smallSphere->SetColor(FColor(1.0f, 0.3f, 1.0f, 1.0f));  // Magenta
    smallSphere->GetTransform().Position = FVector(1.5f, 0.2f, -2.0f);
    smallSphere->GetTransform().Scale = FVector(0.4f, 0.4f, 0.4f);
    smallSphere->SetAutoRotate(false);
    Scene->AddPrimitive(smallSphere);
    
    // Update render scene with all primitives
    Renderer->UpdateFromScene(Scene.get());
    
    FLog::Log(ELogLevel::Info, "Game initialized successfully with multiple primitives");
    return true;
}

void FGame::Shutdown()
{
    FLog::Log(ELogLevel::Info, "Shutting down game...");
    
    if (Scene) {
        Scene->Shutdown();
        Scene.reset();
    }
    
    if (Renderer) {
        Renderer->Shutdown();
        Renderer.reset();
    }
    
    if (RHI) {
        RHI->Shutdown();
        RHI.reset();
    }
    
    g_Camera = nullptr;
    
    FLog::Log(ELogLevel::Info, "Game shutdown complete");
}

void FGame::Tick(float DeltaTime)
{
    static int tickCount = 0;
    tickCount++;
    
    if (tickCount <= 3) {
        FLog::Log(ELogLevel::Info, std::string("FGame::Tick ") + std::to_string(tickCount));
    }
    
    // Game thread tick - update primitives
    if (Scene) {
        Scene->Tick(DeltaTime);
        
        // Update render scene (in real UE5, this would be a sync point)
        Renderer->UpdateFromScene(Scene.get());
    }
    
    // Render thread work (in UE5 this would be on a separate thread)
    if (Renderer) {
        Renderer->RenderFrame();
    }
}

FCamera* FGame::GetCamera()
{
    if (Renderer) {
        return Renderer->GetCamera();
    }
    return nullptr;
}
