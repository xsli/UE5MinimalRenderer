#include "Game.h"
#include "Primitive.h"
#include "GameGlobals.h"
#include "../RHI_DX12/DX12RHI.h"
#include "../Lighting/LitPrimitive.h"
#include "../Lighting/LightVisualization.h"
#include "../Lighting/LitSceneProxy.h"

// Define the global camera pointer (declared in GameGlobals.h)
FCamera* g_Camera = nullptr;

// Global light scene pointer for lit primitives to access
FLightScene* g_LightScene = nullptr;

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
    FLog::Log(ELogLevel::Info, "Initializing game with lighting system...");
    
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
    
    // Create scene (for unlit objects like gizmo)
    Scene = std::make_unique<FScene>(RHI.get());
    
    // Create light scene
    LightScene = std::make_unique<FLightScene>();
    g_LightScene = LightScene.get();
    
    // Setup the lit demo scene
    SetupLitScene();
    
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
    
    FLog::Log(ELogLevel::Info, "Game initialized successfully with lighting system");
    return true;
}

void FGame::SetupLitScene()
{
    // ==========================================
    // LIGHTING SETUP - Daylight Scene
    // ==========================================
    
    // Set bright ambient light for daytime
    LightScene->SetAmbientLight(FColor(0.3f, 0.35f, 0.4f, 1.0f));
    
    // Main directional light (Sun) - warm daylight from above-right
    FDirectionalLight* sunLight = new FDirectionalLight();
    sunLight->SetDirection(FVector(0.5f, -0.8f, 0.3f));  // Coming from upper-right
    sunLight->SetColor(FColor(1.0f, 0.95f, 0.85f, 1.0f)); // Warm white
    sunLight->SetIntensity(1.2f);
    LightScene->AddLight(sunLight);
    
    // Fill light (weaker directional from opposite side)
    FDirectionalLight* fillLight = new FDirectionalLight();
    fillLight->SetDirection(FVector(-0.3f, -0.5f, -0.4f));
    fillLight->SetColor(FColor(0.6f, 0.7f, 0.9f, 1.0f)); // Cool blue tint
    fillLight->SetIntensity(0.3f);
    LightScene->AddLight(fillLight);
    
    // Point light 1 - Warm accent light
    FPointLight* pointLight1 = new FPointLight();
    pointLight1->SetPosition(FVector(-3.0f, 2.0f, -2.0f));
    pointLight1->SetColor(FColor(1.0f, 0.8f, 0.4f, 1.0f)); // Warm orange
    pointLight1->SetIntensity(2.0f);
    pointLight1->SetRadius(8.0f);
    LightScene->AddLight(pointLight1);
    
    // Point light 2 - Cool accent light
    FPointLight* pointLight2 = new FPointLight();
    pointLight2->SetPosition(FVector(3.0f, 2.0f, 2.0f));
    pointLight2->SetColor(FColor(0.4f, 0.6f, 1.0f, 1.0f)); // Cool blue
    pointLight2->SetIntensity(1.5f);
    pointLight2->SetRadius(8.0f);
    LightScene->AddLight(pointLight2);
    
    // ==========================================
    // SCENE OBJECTS - Lit Primitives
    // ==========================================
    
    // --- Gizmo at scene origin (UE-style coordinate axes, unlit) ---
    FGizmoPrimitive* gizmo = new FGizmoPrimitive(1.5f);
    gizmo->GetTransform().Position = FVector(0.0f, 0.0f, 0.0f);
    Scene->AddPrimitive(gizmo);
    
    // --- Lit Ground Plane ---
    FLitPlanePrimitive* groundPlane = new FLitPlanePrimitive(8);
    groundPlane->SetPosition(FVector(0.0f, -1.0f, 0.0f));
    groundPlane->SetScale(FVector(20.0f, 1.0f, 20.0f));
    FMaterial groundMat = FMaterial::Diffuse(FColor(0.4f, 0.45f, 0.4f, 1.0f));
    groundMat.Shininess = 8.0f;  // Low shininess for ground
    groundPlane->SetMaterial(groundMat);
    LitPrimitives.push_back(groundPlane);
    
    // --- Central sphere (glossy, like a metallic ball) ---
    FLitSpherePrimitive* centerSphere = new FLitSpherePrimitive(32, 24);
    centerSphere->SetPosition(FVector(0.0f, 0.5f, 0.0f));
    centerSphere->SetScale(FVector(1.5f, 1.5f, 1.5f));
    centerSphere->SetMaterial(FMaterial::Glossy(FColor(0.9f, 0.9f, 0.9f, 1.0f), 128.0f));
    LitPrimitives.push_back(centerSphere);
    
    // --- Row of cubes with different materials ---
    
    // Red matte cube
    FLitCubePrimitive* redCube = new FLitCubePrimitive();
    redCube->SetPosition(FVector(-4.0f, 0.0f, -3.0f));
    redCube->SetScale(FVector(1.2f, 1.2f, 1.2f));
    redCube->SetMaterial(FMaterial::Diffuse(FColor(0.9f, 0.2f, 0.2f, 1.0f)));
    redCube->SetAutoRotate(true);
    LitPrimitives.push_back(redCube);
    
    // Green glossy cube
    FLitCubePrimitive* greenCube = new FLitCubePrimitive();
    greenCube->SetPosition(FVector(-1.5f, 0.0f, -3.0f));
    greenCube->SetScale(FVector(1.2f, 1.2f, 1.2f));
    greenCube->SetMaterial(FMaterial::Glossy(FColor(0.2f, 0.8f, 0.3f, 1.0f), 64.0f));
    greenCube->SetAutoRotate(true);
    LitPrimitives.push_back(greenCube);
    
    // Blue metallic cube
    FLitCubePrimitive* blueCube = new FLitCubePrimitive();
    blueCube->SetPosition(FVector(1.5f, 0.0f, -3.0f));
    blueCube->SetScale(FVector(1.2f, 1.2f, 1.2f));
    blueCube->SetMaterial(FMaterial::Metal(FColor(0.3f, 0.4f, 0.9f, 1.0f), 96.0f));
    blueCube->SetAutoRotate(true);
    LitPrimitives.push_back(blueCube);
    
    // Gold metallic cube
    FLitCubePrimitive* goldCube = new FLitCubePrimitive();
    goldCube->SetPosition(FVector(4.0f, 0.0f, -3.0f));
    goldCube->SetScale(FVector(1.2f, 1.2f, 1.2f));
    goldCube->SetMaterial(FMaterial::Metal(FColor(1.0f, 0.84f, 0.0f, 1.0f), 128.0f));
    goldCube->SetAutoRotate(true);
    LitPrimitives.push_back(goldCube);
    
    // --- Spheres with various materials ---
    
    // Pink diffuse sphere
    FLitSpherePrimitive* pinkSphere = new FLitSpherePrimitive(24, 16);
    pinkSphere->SetPosition(FVector(-3.0f, 0.5f, 2.0f));
    pinkSphere->SetScale(FVector(1.0f, 1.0f, 1.0f));
    pinkSphere->SetMaterial(FMaterial::Diffuse(FColor(1.0f, 0.6f, 0.7f, 1.0f)));
    LitPrimitives.push_back(pinkSphere);
    
    // Cyan glossy sphere
    FLitSpherePrimitive* cyanSphere = new FLitSpherePrimitive(24, 16);
    cyanSphere->SetPosition(FVector(0.0f, 0.5f, 3.0f));
    cyanSphere->SetScale(FVector(1.0f, 1.0f, 1.0f));
    cyanSphere->SetMaterial(FMaterial::Glossy(FColor(0.2f, 0.9f, 0.9f, 1.0f), 48.0f));
    LitPrimitives.push_back(cyanSphere);
    
    // Purple metallic sphere
    FLitSpherePrimitive* purpleSphere = new FLitSpherePrimitive(24, 16);
    purpleSphere->SetPosition(FVector(3.0f, 0.5f, 2.0f));
    purpleSphere->SetScale(FVector(1.0f, 1.0f, 1.0f));
    purpleSphere->SetMaterial(FMaterial::Metal(FColor(0.7f, 0.3f, 0.9f, 1.0f), 80.0f));
    LitPrimitives.push_back(purpleSphere);
    
    // --- Cylinders ---
    
    // White cylinder (pillar)
    FLitCylinderPrimitive* whiteCylinder = new FLitCylinderPrimitive(24);
    whiteCylinder->SetPosition(FVector(-5.0f, 0.5f, 0.0f));
    whiteCylinder->SetScale(FVector(0.5f, 2.0f, 0.5f));
    whiteCylinder->SetMaterial(FMaterial::Glossy(FColor(0.95f, 0.95f, 0.95f, 1.0f), 32.0f));
    LitPrimitives.push_back(whiteCylinder);
    
    // Bronze cylinder (pillar)
    FLitCylinderPrimitive* bronzeCylinder = new FLitCylinderPrimitive(24);
    bronzeCylinder->SetPosition(FVector(5.0f, 0.5f, 0.0f));
    bronzeCylinder->SetScale(FVector(0.5f, 2.0f, 0.5f));
    bronzeCylinder->SetMaterial(FMaterial::Metal(FColor(0.8f, 0.5f, 0.2f, 1.0f), 64.0f));
    LitPrimitives.push_back(bronzeCylinder);
    
    // Create scene proxies for all lit primitives
    for (FLitPrimitive* litPrim : LitPrimitives)
    {
        FSceneProxy* proxy = litPrim->CreateSceneProxy(RHI.get(), LightScene.get());
        if (proxy)
        {
            Renderer->AddSceneProxy(proxy);
        }
    }
    
    // ==========================================
    // LIGHT VISUALIZATION (Wireframe)
    // ==========================================
    
    // Create wireframe visualization for point lights
    auto pointLights = LightScene->GetPointLights();
    for (size_t i = 0; i < pointLights.size(); ++i)
    {
        FPointLight* light = pointLights[i];
        
        // Generate sphere wireframe for light range
        std::vector<FVertex> vertices;
        std::vector<uint32> indices;
        FLightVisualization::GeneratePointLightGeometry(
            light->GetRadius(), 
            light->GetColor(),
            24,
            vertices, indices);
        
        if (!vertices.empty())
        {
            FRHIBuffer* vb = RHI->CreateVertexBuffer(vertices.size() * sizeof(FVertex), vertices.data());
            FRHIBuffer* ib = RHI->CreateIndexBuffer(indices.size() * sizeof(uint32), indices.data());
            FRHIBuffer* cb = RHI->CreateConstantBuffer(sizeof(FMatrix4x4));
            
            EPipelineFlags flags = EPipelineFlags::EnableDepth | EPipelineFlags::LineTopology;
            FRHIPipelineState* pso = RHI->CreateGraphicsPipelineStateEx(flags);
            
            FLightVisualizationProxy* lightVizProxy = new FLightVisualizationProxy(
                vb, ib, cb, pso, indices.size(), g_Camera, light->GetPosition(), true);
            Renderer->AddSceneProxy(lightVizProxy);
        }
        
        // Add small marker at light center
        std::vector<FVertex> markerVerts;
        std::vector<uint32> markerIndices;
        FLightVisualization::GenerateLightMarker(light->GetColor(), 0.2f, markerVerts, markerIndices);
        
        if (!markerVerts.empty())
        {
            FRHIBuffer* mvb = RHI->CreateVertexBuffer(markerVerts.size() * sizeof(FVertex), markerVerts.data());
            FRHIBuffer* mib = RHI->CreateIndexBuffer(markerIndices.size() * sizeof(uint32), markerIndices.data());
            FRHIBuffer* mcb = RHI->CreateConstantBuffer(sizeof(FMatrix4x4));
            
            EPipelineFlags flags = EPipelineFlags::EnableDepth | EPipelineFlags::LineTopology;
            FRHIPipelineState* mpso = RHI->CreateGraphicsPipelineStateEx(flags);
            
            FLightVisualizationProxy* markerProxy = new FLightVisualizationProxy(
                mvb, mib, mcb, mpso, markerIndices.size(), g_Camera, light->GetPosition(), true);
            Renderer->AddSceneProxy(markerProxy);
        }
    }
    
    // Create visualization for directional light
    auto dirLights = LightScene->GetDirectionalLights();
    for (size_t i = 0; i < dirLights.size() && i < 1; ++i)  // Only first directional light
    {
        FDirectionalLight* light = dirLights[i];
        
        std::vector<FVertex> vertices;
        std::vector<uint32> indices;
        FLightVisualization::GenerateDirectionalLightGeometry(
            light->GetDirection(),
            FColor(1.0f, 1.0f, 0.0f, 1.0f),  // Yellow for sun
            2.0f,
            vertices, indices);
        
        if (!vertices.empty())
        {
            FRHIBuffer* vb = RHI->CreateVertexBuffer(vertices.size() * sizeof(FVertex), vertices.data());
            FRHIBuffer* ib = RHI->CreateIndexBuffer(indices.size() * sizeof(uint32), indices.data());
            FRHIBuffer* cb = RHI->CreateConstantBuffer(sizeof(FMatrix4x4));
            
            EPipelineFlags flags = EPipelineFlags::EnableDepth | EPipelineFlags::LineTopology;
            FRHIPipelineState* pso = RHI->CreateGraphicsPipelineStateEx(flags);
            
            // Position the sun icon in the sky
            FVector sunIconPos(5.0f, 8.0f, 5.0f);
            FLightVisualizationProxy* sunVizProxy = new FLightVisualizationProxy(
                vb, ib, cb, pso, indices.size(), g_Camera, sunIconPos, true);
            Renderer->AddSceneProxy(sunVizProxy);
        }
    }
    
    FLog::Log(ELogLevel::Info, std::string("Lit scene setup complete: ") + 
              std::to_string(LitPrimitives.size()) + " lit primitives, " +
              std::to_string(LightScene->GetLights().size()) + " lights");
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
    
    // Cleanup lit primitives
    for (FLitPrimitive* litPrim : LitPrimitives)
    {
        delete litPrim;
    }
    LitPrimitives.clear();
    
    // Cleanup light scene
    if (LightScene)
    {
        LightScene->ClearLights();
        LightScene.reset();
    }
    g_LightScene = nullptr;
    
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
    
    // Tick lit primitives
    for (FLitPrimitive* litPrim : LitPrimitives)
    {
        litPrim->Tick(DeltaTime);
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
    
    // Tick lit primitives
    for (FLitPrimitive* litPrim : LitPrimitives)
    {
        litPrim->Tick(DeltaTime);
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
