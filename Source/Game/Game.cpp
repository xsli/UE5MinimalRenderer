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
