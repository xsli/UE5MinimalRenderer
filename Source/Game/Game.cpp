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
    FLog::Log(ELogLevel::Info, "Creating triangle scene proxy...");
    
    // Define triangle vertices
    FVertex vertices[] = {
        { FVector(0.0f, 0.5f, 0.0f), FColor(1.0f, 0.0f, 0.0f, 1.0f) },   // Top - Red
        { FVector(0.5f, -0.5f, 0.0f), FColor(0.0f, 1.0f, 0.0f, 1.0f) },  // Right - Green
        { FVector(-0.5f, -0.5f, 0.0f), FColor(0.0f, 0.0f, 1.0f, 1.0f) }  // Left - Blue
    };
    
    FLog::Log(ELogLevel::Info, "Triangle vertices:");
    for (int i = 0; i < 3; i++) {
        FLog::Log(ELogLevel::Info, std::string("  Vertex ") + std::to_string(i) + 
            ": Pos(" + std::to_string(vertices[i].Position.X) + ", " + 
            std::to_string(vertices[i].Position.Y) + ", " + 
            std::to_string(vertices[i].Position.Z) + ") Color(" +
            std::to_string(vertices[i].Color.R) + ", " + 
            std::to_string(vertices[i].Color.G) + ", " + 
            std::to_string(vertices[i].Color.B) + ")");
    }
    
    // Create vertex buffer
    FRHIBuffer* vertexBuffer = RHI->CreateVertexBuffer(sizeof(vertices), vertices);
    
    // Create pipeline state
    FRHIPipelineState* pso = RHI->CreateGraphicsPipelineState();
    
    FLog::Log(ELogLevel::Info, "Triangle scene proxy created");
    
    // Create and return scene proxy
    return new FTriangleMeshProxy(vertexBuffer, pso, 3);
}

// FCubeObject implementation
FCubeObject::FCubeObject(FRenderer* InRenderer)
    : Renderer(InRenderer), CubeProxy(nullptr), RotationAngle(0.0f) {
}

FCubeObject::~FCubeObject() {
}

void FCubeObject::Tick(float DeltaTime) {
    // Rotate the cube
    RotationAngle += DeltaTime * 0.5f;  // Rotate at 0.5 radians per second
    
    // Update the cube's model matrix if proxy exists
    if (CubeProxy) {
        // Create rotation matrix (rotate around Y axis)
        FMatrix4x4 rotationY = FMatrix4x4::RotationY(RotationAngle);
        // Also rotate a bit around X for more interesting view
        FMatrix4x4 rotationX = FMatrix4x4::RotationX(RotationAngle * 0.3f);
        FMatrix4x4 modelMatrix = rotationX * rotationY;
        
        CubeProxy->UpdateModelMatrix(modelMatrix);
    }
}

FSceneProxy* FCubeObject::CreateSceneProxy(FRHI* RHI) {
    FLog::Log(ELogLevel::Info, "Creating cube scene proxy...");
    
    // Define cube vertices (8 unique vertices)
    // Position each vertex at the corners of a 1x1x1 cube centered at origin
    FVertex vertices[] = {
        // Front face (Z = 0.5) - Red
        { FVector(-0.5f, -0.5f,  0.5f), FColor(1.0f, 0.0f, 0.0f, 1.0f) },  // 0: Bottom-left-front
        { FVector( 0.5f, -0.5f,  0.5f), FColor(1.0f, 0.0f, 0.0f, 1.0f) },  // 1: Bottom-right-front
        { FVector( 0.5f,  0.5f,  0.5f), FColor(1.0f, 0.0f, 0.0f, 1.0f) },  // 2: Top-right-front
        { FVector(-0.5f,  0.5f,  0.5f), FColor(1.0f, 0.0f, 0.0f, 1.0f) },  // 3: Top-left-front
        
        // Back face (Z = -0.5) - Green
        { FVector(-0.5f, -0.5f, -0.5f), FColor(0.0f, 1.0f, 0.0f, 1.0f) },  // 4: Bottom-left-back
        { FVector( 0.5f, -0.5f, -0.5f), FColor(0.0f, 1.0f, 0.0f, 1.0f) },  // 5: Bottom-right-back
        { FVector( 0.5f,  0.5f, -0.5f), FColor(0.0f, 1.0f, 0.0f, 1.0f) },  // 6: Top-right-back
        { FVector(-0.5f,  0.5f, -0.5f), FColor(0.0f, 1.0f, 0.0f, 1.0f) },  // 7: Top-left-back
        
        // Top face (Y = 0.5) - Blue
        { FVector(-0.5f,  0.5f,  0.5f), FColor(0.0f, 0.0f, 1.0f, 1.0f) },  // 8: Top-left-front
        { FVector( 0.5f,  0.5f,  0.5f), FColor(0.0f, 0.0f, 1.0f, 1.0f) },  // 9: Top-right-front
        { FVector( 0.5f,  0.5f, -0.5f), FColor(0.0f, 0.0f, 1.0f, 1.0f) },  // 10: Top-right-back
        { FVector(-0.5f,  0.5f, -0.5f), FColor(0.0f, 0.0f, 1.0f, 1.0f) },  // 11: Top-left-back
        
        // Bottom face (Y = -0.5) - Yellow
        { FVector(-0.5f, -0.5f,  0.5f), FColor(1.0f, 1.0f, 0.0f, 1.0f) },  // 12: Bottom-left-front
        { FVector( 0.5f, -0.5f,  0.5f), FColor(1.0f, 1.0f, 0.0f, 1.0f) },  // 13: Bottom-right-front
        { FVector( 0.5f, -0.5f, -0.5f), FColor(1.0f, 1.0f, 0.0f, 1.0f) },  // 14: Bottom-right-back
        { FVector(-0.5f, -0.5f, -0.5f), FColor(1.0f, 1.0f, 0.0f, 1.0f) },  // 15: Bottom-left-back
        
        // Right face (X = 0.5) - Magenta
        { FVector( 0.5f, -0.5f,  0.5f), FColor(1.0f, 0.0f, 1.0f, 1.0f) },  // 16: Bottom-right-front
        { FVector( 0.5f, -0.5f, -0.5f), FColor(1.0f, 0.0f, 1.0f, 1.0f) },  // 17: Bottom-right-back
        { FVector( 0.5f,  0.5f, -0.5f), FColor(1.0f, 0.0f, 1.0f, 1.0f) },  // 18: Top-right-back
        { FVector( 0.5f,  0.5f,  0.5f), FColor(1.0f, 0.0f, 1.0f, 1.0f) },  // 19: Top-right-front
        
        // Left face (X = -0.5) - Cyan
        { FVector(-0.5f, -0.5f,  0.5f), FColor(0.0f, 1.0f, 1.0f, 1.0f) },  // 20: Bottom-left-front
        { FVector(-0.5f, -0.5f, -0.5f), FColor(0.0f, 1.0f, 1.0f, 1.0f) },  // 21: Bottom-left-back
        { FVector(-0.5f,  0.5f, -0.5f), FColor(0.0f, 1.0f, 1.0f, 1.0f) },  // 22: Top-left-back
        { FVector(-0.5f,  0.5f,  0.5f), FColor(0.0f, 1.0f, 1.0f, 1.0f) },  // 23: Top-left-front
    };
    
    // Define indices for 6 faces (2 triangles per face = 36 indices)
    uint32 indices[] = {
        // Front face (Red)
        0, 1, 2,    0, 2, 3,
        // Back face (Green)
        5, 4, 7,    5, 7, 6,
        // Top face (Blue)
        8, 9, 10,   8, 10, 11,
        // Bottom face (Yellow)
        15, 14, 13, 15, 13, 12,
        // Right face (Magenta)
        16, 17, 18, 16, 18, 19,
        // Left face (Cyan)
        21, 20, 23, 21, 23, 22
    };
    
    FLog::Log(ELogLevel::Info, std::string("Cube has ") + std::to_string(sizeof(vertices)/sizeof(FVertex)) + 
        " vertices and " + std::to_string(sizeof(indices)/sizeof(uint32)) + " indices");
    
    // Create vertex buffer
    FRHIBuffer* vertexBuffer = RHI->CreateVertexBuffer(sizeof(vertices), vertices);
    
    // Create index buffer
    FRHIBuffer* indexBuffer = RHI->CreateIndexBuffer(sizeof(indices), indices);
    
    // Create constant buffer for MVP matrix
    FRHIBuffer* constantBuffer = RHI->CreateConstantBuffer(sizeof(FMatrix4x4));
    
    // Create pipeline state with depth testing enabled
    FRHIPipelineState* pso = RHI->CreateGraphicsPipelineState(true);
    
    FLog::Log(ELogLevel::Info, "Cube scene proxy created");
    
    // Create and store scene proxy reference
    CubeProxy = new FCubeMeshProxy(vertexBuffer, indexBuffer, constantBuffer, pso, 36, Renderer->GetCamera());
    
    // Return the proxy
    return CubeProxy;
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
    
    // Add a cube to the world (replace triangle with cube for 3D rendering)
    FCubeObject* cube = new FCubeObject(Renderer.get());
    World->AddGameObject(cube);
    
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
    static int tickCount = 0;
    tickCount++;
    
    if (tickCount <= 3) {
        FLog::Log(ELogLevel::Info, std::string("FGame::Tick ") + std::to_string(tickCount));
    }
    
    // Game thread tick
    if (World) {
        World->Tick(DeltaTime);
    }
    
    // Render thread work (in UE5 this would be on a separate thread)
    if (Renderer) {
        Renderer->RenderFrame();
    }
}

FCamera* FGame::GetCamera() {
    if (Renderer) {
        return Renderer->GetCamera();
    }
    return nullptr;
}
