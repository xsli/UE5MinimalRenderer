#include "Game.h"
#include "GameGlobals.h"
#include "../RHI_DX12/DX12RHI.h"
#include "../Lighting/LightVisualization.h"
#include "../Scene/LitSceneProxy.h"
#include "../Scene/OBJPrimitive.h"
#include "../Asset/TextureLoader.h"
#include "../Shaders/ShaderCompiler.h"
#include <filesystem>
#include <algorithm>
#include <Windows.h>

// Helper function to get the executable directory
static std::string GetExecutableDirectory()
{
    char buffer[MAX_PATH];
    DWORD result = GetModuleFileNameA(NULL, buffer, MAX_PATH);
    
    if (result == 0 || result == MAX_PATH)
    {
        FLog::Log(ELogLevel::Warning, "GetModuleFileNameA failed, using current directory");
        return ".";
    }
    
    std::string path(buffer);
    size_t lastSlash = path.find_last_of("\\/");
    if (lastSlash != std::string::npos)
    {
        path = path.substr(0, lastSlash);
    }
    
    std::replace(path.begin(), path.end(), '\\', '/');
    return path;
}

// Helper function to resolve content path relative to executable
static std::string ResolveContentPath(const std::string& RelativePath)
{
    std::string exeDir = GetExecutableDirectory();
    
    // If the path is absolute, use it directly
    if (RelativePath.length() > 1 && (RelativePath[1] == ':' || RelativePath[0] == '/'))
    {
        return RelativePath;
    }
    
    // Try going up directories from the executable to find Content/
    // Build structure: build/Source/Runtime/Release/UE5MinimalRenderer.exe
    // We need to go up to find Content/
    std::string currentDir = exeDir;
    for (int i = 0; i < 5; ++i)
    {
        std::string testPath = currentDir + "/" + RelativePath;
        if (std::filesystem::exists(testPath))
        {
            FLog::Log(ELogLevel::Info, "Found content path: " + testPath);
            return testPath;
        }
        
        size_t lastSlash = currentDir.find_last_of("/");
        if (lastSlash == std::string::npos || lastSlash == 0)
        {
            break;
        }
        currentDir = currentDir.substr(0, lastSlash);
    }
    
    // Fall back to original path
    FLog::Log(ELogLevel::Warning, "Could not find content path, using fallback: " + RelativePath);
    return RelativePath;
}

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
    FLog::Log(ELogLevel::Info, "Initializing game with unified scene system...");
    
    // Register main thread as the game thread
    FThreadManager::Get().SetCurrentThread(ENamedThreads::GameThread);
    
    // Initialize shader manager with shader directory
    // Note: In a real engine, this path would be relative to the executable or configurable
    FShaderManager::Get().Initialize("../Source/Shaders");
    FLog::Log(ELogLevel::Info, "Shader manager initialized");
    
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
    
    // Create unified scene
    Scene = std::make_unique<FScene>(RHI.get());
    
    // Set global light scene pointer
    g_LightScene = Scene->GetLightScene();
    
    // Setup the demo scene
    SetupScene();
    
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
    
    FLog::Log(ELogLevel::Info, "Game initialized successfully with unified scene system");
    return true;
}

void FGame::SetupScene()
{
    FLightScene* LightScene = Scene->GetLightScene();
    
    // ==========================================
    // LIGHTING SETUP - Daylight Scene (Reduced Intensity)
    // ==========================================
    
    // Set softer ambient light for balanced scene
    LightScene->SetAmbientLight(FColor(0.15f, 0.18f, 0.22f, 1.0f));
    
    // Main directional light (Sun) - warm daylight from above-right (reduced intensity)
    FDirectionalLight* sunLight = new FDirectionalLight();
    sunLight->SetDirection(FVector(0.5f, -0.8f, 0.3f));
    sunLight->SetColor(FColor(1.0f, 0.95f, 0.85f, 1.0f));
    sunLight->SetIntensity(0.7f);  // Reduced from 1.2
    LightScene->AddLight(sunLight);
    
    // Fill light (weaker directional from opposite side)
    FDirectionalLight* fillLight = new FDirectionalLight();
    fillLight->SetDirection(FVector(-0.3f, -0.5f, -0.4f));
    fillLight->SetColor(FColor(0.6f, 0.7f, 0.9f, 1.0f));
    fillLight->SetIntensity(0.15f);  // Reduced from 0.3
    LightScene->AddLight(fillLight);
    
    // Point light 1 - Warm accent light (reduced intensity)
    FPointLight* pointLight1 = new FPointLight();
    pointLight1->SetPosition(FVector(-3.0f, 2.0f, -2.0f));
    pointLight1->SetColor(FColor(1.0f, 0.8f, 0.4f, 1.0f));
    pointLight1->SetIntensity(0.8f);  // Reduced from 2.0
    pointLight1->SetRadius(8.0f);
    LightScene->AddLight(pointLight1);
    
    // Point light 2 - Cool accent light (reduced intensity)
    FPointLight* pointLight2 = new FPointLight();
    pointLight2->SetPosition(FVector(3.0f, 2.0f, 2.0f));
    pointLight2->SetColor(FColor(0.4f, 0.6f, 1.0f, 1.0f));
    pointLight2->SetIntensity(0.6f);  // Reduced from 1.5
    pointLight2->SetRadius(8.0f);
    LightScene->AddLight(pointLight2);
    
    // ==========================================
    // SCENE OBJECTS - Lit Primitives with Macaron Colors
    // ==========================================
    
    // --- Ground Plane (soft lavender) ---
    FPlanePrimitive* groundPlane = new FPlanePrimitive(8);
    groundPlane->SetPosition(FVector(0.0f, -1.0f, 0.0f));
    groundPlane->SetScale(FVector(20.0f, 1.0f, 20.0f));
    FMaterial groundMat = FMaterial::Diffuse(FColor(0.85f, 0.82f, 0.9f, 1.0f));  // Soft lavender
    groundMat.Shininess = 8.0f;
    groundPlane->SetMaterial(groundMat);
    Scene->AddPrimitive(groundPlane);
    
    // --- Central sphere (glossy white with pink tint) ---
    FSpherePrimitive* centerSphere = new FSpherePrimitive(32, 24);
    centerSphere->SetPosition(FVector(0.0f, 0.5f, 0.0f));
    centerSphere->SetScale(FVector(1.5f, 1.5f, 1.5f));
    centerSphere->SetMaterial(FMaterial::Glossy(FColor(1.0f, 0.95f, 0.97f, 1.0f), 128.0f));  // Pearl white
    Scene->AddPrimitive(centerSphere);
    
    // --- Row of cubes with Macaron colors ---
    
    // Macaron Pink cube (soft strawberry)
    FCubePrimitive* pinkCube = new FCubePrimitive();
    pinkCube->SetPosition(FVector(-4.0f, 0.0f, -3.0f));
    pinkCube->SetScale(FVector(1.2f, 1.2f, 1.2f));
    pinkCube->SetMaterial(FMaterial::Diffuse(FColor(1.0f, 0.71f, 0.76f, 1.0f)));  // Macaron pink
    pinkCube->SetAutoRotate(true);
    Scene->AddPrimitive(pinkCube);
    
    // Macaron Mint cube (soft green)
    FCubePrimitive* mintCube = new FCubePrimitive();
    mintCube->SetPosition(FVector(-1.5f, 0.0f, -3.0f));
    mintCube->SetScale(FVector(1.2f, 1.2f, 1.2f));
    mintCube->SetMaterial(FMaterial::Glossy(FColor(0.6f, 0.95f, 0.78f, 1.0f), 64.0f));  // Macaron mint
    mintCube->SetAutoRotate(true);
    Scene->AddPrimitive(mintCube);
    
    // Macaron Blue cube (soft sky blue)
    FCubePrimitive* blueCube = new FCubePrimitive();
    blueCube->SetPosition(FVector(1.5f, 0.0f, -3.0f));
    blueCube->SetScale(FVector(1.2f, 1.2f, 1.2f));
    blueCube->SetMaterial(FMaterial::Metal(FColor(0.68f, 0.85f, 0.95f, 1.0f), 96.0f));  // Macaron sky blue
    blueCube->SetAutoRotate(true);
    Scene->AddPrimitive(blueCube);
    
    // Macaron Lemon cube (soft yellow)
    FCubePrimitive* lemonCube = new FCubePrimitive();
    lemonCube->SetPosition(FVector(4.0f, 0.0f, -3.0f));
    lemonCube->SetScale(FVector(1.2f, 1.2f, 1.2f));
    lemonCube->SetMaterial(FMaterial::Metal(FColor(1.0f, 0.97f, 0.7f, 1.0f), 128.0f));  // Macaron lemon
    lemonCube->SetAutoRotate(true);
    Scene->AddPrimitive(lemonCube);
    
    // --- Spheres with Macaron colors ---
    
    // Macaron Peach sphere (soft orange-pink)
    FSpherePrimitive* peachSphere = new FSpherePrimitive(24, 16);
    peachSphere->SetPosition(FVector(-3.0f, 0.5f, 2.0f));
    peachSphere->SetScale(FVector(1.0f, 1.0f, 1.0f));
    peachSphere->SetMaterial(FMaterial::Diffuse(FColor(1.0f, 0.8f, 0.7f, 1.0f)));  // Macaron peach
    Scene->AddPrimitive(peachSphere);
    
    // Macaron Lavender sphere (soft purple)
    FSpherePrimitive* lavenderSphere = new FSpherePrimitive(24, 16);
    lavenderSphere->SetPosition(FVector(0.0f, 0.5f, 3.0f));
    lavenderSphere->SetScale(FVector(1.0f, 1.0f, 1.0f));
    lavenderSphere->SetMaterial(FMaterial::Glossy(FColor(0.8f, 0.7f, 0.95f, 1.0f), 48.0f));  // Macaron lavender
    Scene->AddPrimitive(lavenderSphere);
    
    // Macaron Berry sphere (soft raspberry)
    FSpherePrimitive* berrySphere = new FSpherePrimitive(24, 16);
    berrySphere->SetPosition(FVector(3.0f, 0.5f, 2.0f));
    berrySphere->SetScale(FVector(1.0f, 1.0f, 1.0f));
    berrySphere->SetMaterial(FMaterial::Metal(FColor(0.9f, 0.55f, 0.7f, 1.0f), 80.0f));  // Macaron raspberry
    Scene->AddPrimitive(berrySphere);
    
    // --- Cylinders with Macaron colors ---
    
    // Macaron Cream cylinder (soft vanilla)
    FCylinderPrimitive* creamCylinder = new FCylinderPrimitive(24);
    creamCylinder->SetPosition(FVector(-5.0f, 0.5f, 0.0f));
    creamCylinder->SetScale(FVector(0.5f, 2.0f, 0.5f));
    creamCylinder->SetMaterial(FMaterial::Glossy(FColor(1.0f, 0.98f, 0.9f, 1.0f), 32.0f));  // Macaron vanilla
    Scene->AddPrimitive(creamCylinder);
    
    // Macaron Rose cylinder (soft rose)
    FCylinderPrimitive* roseCylinder = new FCylinderPrimitive(24);
    roseCylinder->SetPosition(FVector(5.0f, 0.5f, 0.0f));
    roseCylinder->SetScale(FVector(0.5f, 2.0f, 0.5f));
    roseCylinder->SetMaterial(FMaterial::Metal(FColor(0.95f, 0.75f, 0.8f, 1.0f), 64.0f));  // Macaron rose
    Scene->AddPrimitive(roseCylinder);
    
    // ==========================================
    // TEXTURED OBJ MODEL DEMO
    // ==========================================
    
    // Load Stanford Bunny (classic 3D test model)
    std::string bunnyObjPath = ResolveContentPath("Content/Models/bunny.obj");
    FOBJPrimitive* bunny = new FOBJPrimitive(bunnyObjPath, RHI.get());
    if (bunny->IsValid())
    {
        bunny->SetPosition(FVector(-3.0f, 0.0f, 0.0f));
        bunny->SetScale(FVector(15.0f, 15.0f, 15.0f));  // Bunny is small, scale up
        bunny->SetAutoRotate(true);
        bunny->SetRotationSpeed(0.5f);
        Scene->AddPrimitive(bunny);
        FLog::Log(ELogLevel::Info, "Added Stanford Bunny to scene");
    }
    else
    {
        FLog::Log(ELogLevel::Warning, "Failed to load Stanford Bunny, skipping");
        delete bunny;
    }
    
    // Load Utah Teapot (another classic 3D test model)
    std::string teapotObjPath = ResolveContentPath("Content/Models/teapot.obj");
    FOBJPrimitive* teapot = new FOBJPrimitive(teapotObjPath, RHI.get());
    if (teapot->IsValid())
    {
        teapot->SetPosition(FVector(3.0f, 0.5f, 0.0f));
        teapot->SetScale(FVector(0.5f, 0.5f, 0.5f));
        teapot->SetAutoRotate(true);
        teapot->SetRotationSpeed(0.6f);
        Scene->AddPrimitive(teapot);
        FLog::Log(ELogLevel::Info, "Added Utah Teapot to scene");
    }
    else
    {
        FLog::Log(ELogLevel::Warning, "Failed to load Utah Teapot, skipping");
        delete teapot;
    }
    
    // Load Cornell Box (classic rendering test scene)
    std::string cornellObjPath = ResolveContentPath("Content/Models/cornell_box.obj");
    FOBJPrimitive* cornellBox = new FOBJPrimitive(cornellObjPath, RHI.get());
    if (cornellBox->IsValid())
    {
        cornellBox->SetPosition(FVector(0.0f, 0.0f, 5.0f));
        cornellBox->SetScale(FVector(0.8f, 0.8f, 0.8f));
        Scene->AddPrimitive(cornellBox);
        FLog::Log(ELogLevel::Info, "Added Cornell Box to scene");
        
        // Add point light inside Cornell Box
        FPointLight* cornellLight = new FPointLight();
        // Position light at center-top of Cornell Box (box is at 0,0,5 with scale 0.8)
        // Cornell box internal coords are 0-5 in all axes, so center is (2.5, 4.5, 2.5) * 0.8 + position
        cornellLight->SetPosition(FVector(0.0f * 0.8f + 0.0f, 4.0f * 0.8f + 0.0f, 2.5f * 0.8f + 5.0f));  // Near ceiling
        cornellLight->SetColor(FColor(1.0f, 0.98f, 0.95f, 1.0f));  // Warm white
        cornellLight->SetIntensity(1.5f);
        cornellLight->SetRadius(5.0f);
        LightScene->AddLight(cornellLight);
        FLog::Log(ELogLevel::Info, "Added point light inside Cornell Box");
    }
    else
    {
        FLog::Log(ELogLevel::Warning, "Failed to load Cornell Box, skipping");
        delete cornellBox;
    }
    
    // Load African Head (textured model demo)
    std::string headObjPath = ResolveContentPath("Content/Models/african_head.obj");
    FOBJPrimitive* africanHead = new FOBJPrimitive(headObjPath, RHI.get());
    if (africanHead->IsValid())
    {
        africanHead->SetPosition(FVector(0.0f, 1.5f, -5.0f));
        africanHead->SetScale(FVector(2.0f, 2.0f, 2.0f));
        africanHead->SetAutoRotate(true);
        africanHead->SetRotationSpeed(0.4f);
        Scene->AddPrimitive(africanHead);
        FLog::Log(ELogLevel::Info, "Added African Head (textured) to scene");
    }
    else
    {
        FLog::Log(ELogLevel::Warning, "Failed to load African Head, skipping");
        delete africanHead;
    }
    
    // ==========================================
    // LIGHT VISUALIZATION (Wireframe)
    // ==========================================
    
    auto pointLights = LightScene->GetPointLights();
    for (size_t i = 0; i < pointLights.size(); ++i)
    {
        FPointLight* light = pointLights[i];
        
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
    
    auto dirLights = LightScene->GetDirectionalLights();
    for (size_t i = 0; i < dirLights.size() && i < 1; ++i)
    {
        FDirectionalLight* light = dirLights[i];
        
        std::vector<FVertex> vertices;
        std::vector<uint32> indices;
        FLightVisualization::GenerateDirectionalLightGeometry(
            light->GetDirection(),
            FColor(1.0f, 1.0f, 0.0f, 1.0f),
            2.0f,
            vertices, indices);
        
        if (!vertices.empty())
        {
            FRHIBuffer* vb = RHI->CreateVertexBuffer(vertices.size() * sizeof(FVertex), vertices.data());
            FRHIBuffer* ib = RHI->CreateIndexBuffer(indices.size() * sizeof(uint32), indices.data());
            FRHIBuffer* cb = RHI->CreateConstantBuffer(sizeof(FMatrix4x4));
            
            EPipelineFlags flags = EPipelineFlags::EnableDepth | EPipelineFlags::LineTopology;
            FRHIPipelineState* pso = RHI->CreateGraphicsPipelineStateEx(flags);
            
            FVector sunIconPos(5.0f, 8.0f, 5.0f);
            FLightVisualizationProxy* sunVizProxy = new FLightVisualizationProxy(
                vb, ib, cb, pso, indices.size(), g_Camera, sunIconPos, true);
            Renderer->AddSceneProxy(sunVizProxy);
        }
    }
    
    FLog::Log(ELogLevel::Info, std::string("Scene setup complete: ") + 
              std::to_string(Scene->GetPrimitives().size()) + " primitives, " +
              std::to_string(LightScene->GetLights().size()) + " lights");
}

void FGame::Shutdown()
{
    FLog::Log(ELogLevel::Info, "Shutting down game...");
    
    // Stop multi-threading first
    if (bMultiThreaded)
    {
        FLog::Log(ELogLevel::Info, "Stopping multi-threaded systems...");
        
        FRenderThread::Get().WaitForFrameComplete();
        FRHIThread::Get().WaitForFrameComplete();
        
        FRenderThread::Get().Stop();
        FRHIThread::Get().Stop();
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
    
    // Shutdown shader manager
    FShaderManager::Get().Shutdown();
    
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
    
    Renderer->GetStats().BeginGameThreadTiming();
    
    if (Scene)
    {
        Scene->Tick(DeltaTime);
        Renderer->UpdateFromScene(Scene.get());
    }
    
    Renderer->GetStats().EndGameThreadTiming();
    
    Renderer->GetStats().BeginRenderThreadTiming();
    
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
    
    FFrameSyncManager::Get().GameThread_BeginFrame();
    
    ++GameFrameNumber;
    
    Renderer->GetStats().BeginGameThreadTiming();
    
    if (Scene)
    {
        Scene->Tick(DeltaTime);
        Renderer->UpdateFromScene(Scene.get());
    }
    
    Renderer->GetStats().EndGameThreadTiming();
    
    FRenderer* RendererPtr = Renderer.get();
    
    ENQUEUE_RENDER_COMMAND(RenderFrame)[RendererPtr]() 
    {
        RendererPtr->GetStats().BeginRenderThreadTiming();
        
        FFrameSyncManager::Get().RenderThread_BeginFrame();
        
        if (RendererPtr)
        {
            RendererPtr->RenderFrame();
        }
        
        FFrameSyncManager::Get().RenderThread_EndFrame();
        
        RendererPtr->GetStats().EndRenderThreadTiming();
    });
    
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
