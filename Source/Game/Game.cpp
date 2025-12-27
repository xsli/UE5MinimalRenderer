#include "Game.h"
#include "Primitive.h"
#include "GameGlobals.h"
#include "../RHI_DX12/DX12RHI.h"

// Define the global camera pointer (declared in GameGlobals.h)
FCamera* g_Camera = nullptr;

// FGame implementation
FGame::FGame()
    : bMultiThreaded(true)  // Enable multi-threading by default
    , GameFrameNumber(0)
{
}

FGame::~FGame()
{
}

bool FGame::Initialize(void* WindowHandle, uint32 Width, uint32 Height)
{
    FLog::Log(ELogLevel::Info, "Initializing game...");
    
    // Register main thread as the game thread
    FThreadManager::Get().SetCurrentThread(ENamedThreads::GameThread);
    
    // Create RHI
    RHI.reset(CreateDX12RHI());
    if (!RHI->Initialize(WindowHandle, Width, Height))
    {
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
    
    // ==========================================
    // Create demo scene showcasing matrix transformations
    // ==========================================
    
    // --- Gizmo at scene origin (UE-style coordinate axes) ---
    FGizmoPrimitive* gizmo = new FGizmoPrimitive(1.5f);
    gizmo->GetTransform().Position = FVector(0.0f, 0.0f, 0.0f);
    Scene->AddPrimitive(gizmo);
    
    // --- Ground plane ---
    FPlanePrimitive* plane = new FPlanePrimitive(4);
    plane->SetColor(FColor(0.5f, 0.5f, 0.5f, 1.0f));  // Gray
    plane->GetTransform().Position = FVector(0.0f, -1.5f, 0.0f);
    plane->GetTransform().Scale = FVector(15.0f, 1.0f, 15.0f);
    Scene->AddPrimitive(plane);
    
    // ==========================================
    // Group 1: Rotation demos (Pink/Rose macaron colors)
    // Positioned in a row at Z = -3
    // ==========================================
    
    // Rotate around X axis - Pale Pink
    FDemoCubePrimitive* rotateX = new FDemoCubePrimitive();
    rotateX->SetColor(FColor(0.98f, 0.76f, 0.80f, 1.0f));  // Pale Pink macaron
    rotateX->SetBasePosition(FVector(-3.0f, 0.5f, -3.0f));
    rotateX->GetTransform().Position = FVector(-3.0f, 0.5f, -3.0f);
    rotateX->GetTransform().Scale = FVector(0.6f, 0.6f, 0.6f);
    rotateX->SetAnimationType(EAnimationType::RotateX);
    rotateX->SetAnimationSpeed(1.0f);
    Scene->AddPrimitive(rotateX);
    
    // Rotate around Y axis - Rose Pink
    FDemoCubePrimitive* rotateY = new FDemoCubePrimitive();
    rotateY->SetColor(FColor(1.0f, 0.71f, 0.76f, 1.0f));  // Rose Pink macaron
    rotateY->SetBasePosition(FVector(0.0f, 0.5f, -3.0f));
    rotateY->GetTransform().Position = FVector(0.0f, 0.5f, -3.0f);
    rotateY->GetTransform().Scale = FVector(0.6f, 0.6f, 0.6f);
    rotateY->SetAnimationType(EAnimationType::RotateY);
    rotateY->SetAnimationSpeed(1.0f);
    Scene->AddPrimitive(rotateY);
    
    // Rotate around Z axis - Coral Pink
    FDemoCubePrimitive* rotateZ = new FDemoCubePrimitive();
    rotateZ->SetColor(FColor(1.0f, 0.60f, 0.65f, 1.0f));  // Coral Pink macaron
    rotateZ->SetBasePosition(FVector(3.0f, 0.5f, -3.0f));
    rotateZ->GetTransform().Position = FVector(3.0f, 0.5f, -3.0f);
    rotateZ->GetTransform().Scale = FVector(0.6f, 0.6f, 0.6f);
    rotateZ->SetAnimationType(EAnimationType::RotateZ);
    rotateZ->SetAnimationSpeed(1.0f);
    Scene->AddPrimitive(rotateZ);
    
    // ==========================================
    // Group 2: Translation demos (Mint/Green macaron colors)
    // Positioned in a row at Z = 0
    // ==========================================
    
    // Translate along X axis - Mint Green
    FDemoCubePrimitive* translateX = new FDemoCubePrimitive();
    translateX->SetColor(FColor(0.68f, 0.93f, 0.82f, 1.0f));  // Mint Green macaron
    translateX->SetBasePosition(FVector(-4.0f, 0.5f, 0.0f));
    translateX->GetTransform().Position = FVector(-4.0f, 0.5f, 0.0f);
    translateX->GetTransform().Scale = FVector(0.5f, 0.5f, 0.5f);
    translateX->SetAnimationType(EAnimationType::TranslateX);
    translateX->SetAnimationSpeed(2.0f);
    Scene->AddPrimitive(translateX);
    
    // Translate along Y axis - Seafoam
    FDemoCubePrimitive* translateY = new FDemoCubePrimitive();
    translateY->SetColor(FColor(0.56f, 0.88f, 0.74f, 1.0f));  // Seafoam macaron
    translateY->SetBasePosition(FVector(-2.0f, 0.5f, 0.0f));
    translateY->GetTransform().Position = FVector(-2.0f, 0.5f, 0.0f);
    translateY->GetTransform().Scale = FVector(0.5f, 0.5f, 0.5f);
    translateY->SetAnimationType(EAnimationType::TranslateY);
    translateY->SetAnimationSpeed(2.0f);
    Scene->AddPrimitive(translateY);
    
    // Translate along Z axis - Pistachio
    FDemoCubePrimitive* translateZ = new FDemoCubePrimitive();
    translateZ->SetColor(FColor(0.73f, 0.89f, 0.67f, 1.0f));  // Pistachio macaron
    translateZ->SetBasePosition(FVector(2.0f, 0.5f, 0.0f));
    translateZ->GetTransform().Position = FVector(2.0f, 0.5f, 0.0f);
    translateZ->GetTransform().Scale = FVector(0.5f, 0.5f, 0.5f);
    translateZ->SetAnimationType(EAnimationType::TranslateZ);
    translateZ->SetAnimationSpeed(2.0f);
    Scene->AddPrimitive(translateZ);
    
    // Translate diagonally - Lime Green
    FDemoCubePrimitive* translateDiag = new FDemoCubePrimitive();
    translateDiag->SetColor(FColor(0.82f, 0.95f, 0.55f, 1.0f));  // Lime Green macaron
    translateDiag->SetBasePosition(FVector(4.0f, 0.5f, 0.0f));
    translateDiag->GetTransform().Position = FVector(4.0f, 0.5f, 0.0f);
    translateDiag->GetTransform().Scale = FVector(0.5f, 0.5f, 0.5f);
    translateDiag->SetAnimationType(EAnimationType::TranslateDiagonal);
    translateDiag->SetAnimationSpeed(2.0f);
    Scene->AddPrimitive(translateDiag);
    
    // ==========================================
    // Group 3: Scale demos (Lavender/Purple macaron colors)
    // Positioned in a row at Z = 3
    // ==========================================
    
    // Scale uniformly - Lavender
    FDemoCubePrimitive* scale1 = new FDemoCubePrimitive();
    scale1->SetColor(FColor(0.79f, 0.69f, 0.89f, 1.0f));  // Lavender macaron
    scale1->SetBasePosition(FVector(-2.0f, 0.5f, 3.0f));
    scale1->SetBaseScale(FVector(0.6f, 0.6f, 0.6f));
    scale1->GetTransform().Position = FVector(-2.0f, 0.5f, 3.0f);
    scale1->GetTransform().Scale = FVector(0.6f, 0.6f, 0.6f);
    scale1->SetAnimationType(EAnimationType::Scale);
    scale1->SetAnimationSpeed(3.0f);
    Scene->AddPrimitive(scale1);
    
    // Scale uniformly (different phase) - Violet
    FDemoCubePrimitive* scale2 = new FDemoCubePrimitive();
    scale2->SetColor(FColor(0.85f, 0.68f, 0.95f, 1.0f));  // Violet macaron
    scale2->SetBasePosition(FVector(0.0f, 0.5f, 3.0f));
    scale2->SetBaseScale(FVector(0.6f, 0.6f, 0.6f));
    scale2->GetTransform().Position = FVector(0.0f, 0.5f, 3.0f);
    scale2->GetTransform().Scale = FVector(0.6f, 0.6f, 0.6f);
    scale2->SetAnimationType(EAnimationType::Scale);
    scale2->SetAnimationSpeed(2.5f);
    Scene->AddPrimitive(scale2);
    
    // Scale uniformly (different phase) - Grape
    FDemoCubePrimitive* scale3 = new FDemoCubePrimitive();
    scale3->SetColor(FColor(0.70f, 0.58f, 0.85f, 1.0f));  // Grape macaron
    scale3->SetBasePosition(FVector(2.0f, 0.5f, 3.0f));
    scale3->SetBaseScale(FVector(0.6f, 0.6f, 0.6f));
    scale3->GetTransform().Position = FVector(2.0f, 0.5f, 3.0f);
    scale3->GetTransform().Scale = FVector(0.6f, 0.6f, 0.6f);
    scale3->SetAnimationType(EAnimationType::Scale);
    scale3->SetAnimationSpeed(2.0f);
    Scene->AddPrimitive(scale3);
    
    // ==========================================
    // Additional demo: Sky Blue sphere at center for reference
    // ==========================================
    FSpherePrimitive* centerSphere = new FSpherePrimitive(24, 16);
    centerSphere->SetColor(FColor(0.68f, 0.85f, 0.95f, 1.0f));  // Sky Blue macaron
    centerSphere->GetTransform().Position = FVector(0.0f, 0.5f, 0.0f);
    centerSphere->GetTransform().Scale = FVector(0.4f, 0.4f, 0.4f);
    centerSphere->SetAutoRotate(false);
    Scene->AddPrimitive(centerSphere);
    
    // Update render scene with all primitives
    Renderer->UpdateFromScene(Scene.get());
    
    // Initialize multi-threading if enabled
    if (bMultiThreaded)
    {
        FLog::Log(ELogLevel::Info, "Initializing multi-threaded rendering...");
        
        // Initialize task graph
        FTaskGraph::Get();
        
        // Initialize frame sync manager
        FFrameSyncManager::Get();
        
        // Setup and start render thread
        FRenderThread::Get().SetRenderer(Renderer.get());
        FRenderThread::Get().SetRHI(RHI.get());
        FRenderThread::Get().Start();
        
        // Setup and start RHI thread
        FRHIThread::Get().SetRHI(RHI.get());
        FRHIThread::Get().Start();
        
        FLog::Log(ELogLevel::Info, "Multi-threaded rendering initialized");
    }
    
    FLog::Log(ELogLevel::Info, "Game initialized successfully with multiple primitives");
    return true;
}

void FGame::Shutdown()
{
    FLog::Log(ELogLevel::Info, "Shutting down game...");
    
    // Stop multi-threading first
    if (bMultiThreaded)
    {
        FLog::Log(ELogLevel::Info, "Stopping multi-threaded systems...");
        
        // Wait for pending frame to complete
        FRenderThread::Get().WaitForFrameComplete();
        FRHIThread::Get().WaitForFrameComplete();
        
        // Stop threads
        FRenderThread::Get().Stop();
        FRHIThread::Get().Stop();
    }
    
    if (Scene)
    {
        Scene->Shutdown();
        Scene.reset();
    }
    
    if (Renderer)
    {
        Renderer->Shutdown();
        Renderer.reset();
    }
    
    if (RHI)
    {
        RHI->Shutdown();
        RHI.reset();
    }
    
    g_Camera = nullptr;
    
    FLog::Log(ELogLevel::Info, "Game shutdown complete");
}

void FGame::Tick(float DeltaTime)
{
    if (bMultiThreaded)
    {
        TickMultiThreaded(DeltaTime);
    }
    else
    {
        TickSingleThreaded(DeltaTime);
    }
}

void FGame::TickSingleThreaded(float DeltaTime)
{
    static int tickCount = 0;
    tickCount++;
    
    if (tickCount <= 3)
    {
        FLog::Log(ELogLevel::Info, std::string("FGame::Tick (SingleThreaded) ") + std::to_string(tickCount));
    }
    
    // Track game thread time (actual work, no idle waiting in single-threaded mode)
    Renderer->GetStats().BeginGameThreadTiming();
    
    // Game thread tick - update primitives
    if (Scene)
    {
        Scene->Tick(DeltaTime);
        
        // Update render scene (in real UE5, this would be a sync point)
        Renderer->UpdateFromScene(Scene.get());
    }
    
    Renderer->GetStats().EndGameThreadTiming();
    
    // Track render thread time (in single-threaded mode, this runs on same thread)
    // Note: RHI time is tracked inside RenderFrame(), so we only track the outer render time here
    Renderer->GetStats().BeginRenderThreadTiming();
    
    // Render thread work (in UE5 this would be on a separate thread)
    // RenderFrame() internally tracks RHI time
    if (Renderer)
    {
        Renderer->RenderFrame();
    }
    
    Renderer->GetStats().EndRenderThreadTiming();
}

void FGame::TickMultiThreaded(float DeltaTime)
{
    static int tickCount = 0;
    tickCount++;
    
    if (tickCount <= 3)
    {
        FLog::Log(ELogLevel::Info, std::string("FGame::Tick (MultiThreaded) ") + std::to_string(tickCount));
    }
    
    // Begin frame synchronization - may wait if game is too far ahead (idle time, not measured)
    FFrameSyncManager::Get().GameThread_BeginFrame();
    
    ++GameFrameNumber;
    
    // Track game thread time - starts AFTER sync wait, excludes idle time
    Renderer->GetStats().BeginGameThreadTiming();
    
    // Game thread tick - update primitives
    if (Scene)
    {
        Scene->Tick(DeltaTime);
        
        // Update render scene on game thread (prepare data for render thread)
        Renderer->UpdateFromScene(Scene.get());
    }
    
    Renderer->GetStats().EndGameThreadTiming();
    
    // Capture renderer and RHI pointers for lambda
    FRenderer* RendererPtr = Renderer.get();
    
    // Enqueue render commands for this frame
    // The render thread will execute these after waking from its wait (idle time excluded)
    ENQUEUE_RENDER_COMMAND(RenderFrame)[RendererPtr]() 
    {
        // Track render thread time - starts when command executes, excludes idle wait time
        RendererPtr->GetStats().BeginRenderThreadTiming();
        
        // Begin render frame on render thread
        FFrameSyncManager::Get().RenderThread_BeginFrame();
        
        if (RendererPtr)
        {
            // RenderFrame() internally tracks RHI time
            RendererPtr->RenderFrame();
        }
        
        // End render frame
        FFrameSyncManager::Get().RenderThread_EndFrame();
        
        RendererPtr->GetStats().EndRenderThreadTiming();
    });
    
    // End frame - signal render thread that commands are ready
    FFrameSyncManager::Get().GameThread_EndFrame();
}

FCamera* FGame::GetCamera()
{
    if (Renderer)
    {
        return Renderer->GetCamera();
    }
    return nullptr;
}
