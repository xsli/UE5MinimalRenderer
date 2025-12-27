#include "LitPrimitive.h"
#include "LitSceneProxy.h"
#include "../Game/GameGlobals.h"
#include "../RHI/RHI.h"
#include "../Renderer/Camera.h"
#include <vector>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// FLitPrimitive implementation
FLitPrimitive::FLitPrimitive()
    : Position(0.0f, 0.0f, 0.0f)
    , Rotation(0.0f, 0.0f, 0.0f)
    , Scale(1.0f, 1.0f, 1.0f)
    , Material(FMaterial::Default())
    , bIsDirty(true)
    , bTransformDirty(false)
{
}

void FLitPrimitive::Tick(float DeltaTime)
{
    // Base implementation does nothing
}

FMatrix4x4 FLitPrimitive::GetTransformMatrix() const
{
    FMatrix4x4 scale = FMatrix4x4::Scaling(Scale.X, Scale.Y, Scale.Z);
    FMatrix4x4 rotationX = FMatrix4x4::RotationX(Rotation.X);
    FMatrix4x4 rotationY = FMatrix4x4::RotationY(Rotation.Y);
    FMatrix4x4 rotationZ = FMatrix4x4::RotationZ(Rotation.Z);
    FMatrix4x4 translation = FMatrix4x4::Translation(Position.X, Position.Y, Position.Z);
    
    return scale * rotationX * rotationY * rotationZ * translation;
}

// FLitCubePrimitive implementation
FLitCubePrimitive::FLitCubePrimitive()
    : bAutoRotate(false)
    , RotationSpeed(0.5f)
{
}

void FLitCubePrimitive::Tick(float DeltaTime)
{
    if (bAutoRotate)
    {
        Rotation.Y += DeltaTime * RotationSpeed;
        MarkTransformDirty();
    }
}

FSceneProxy* FLitCubePrimitive::CreateSceneProxy(FRHI* RHI, FLightScene* LightScene)
{
    FLog::Log(ELogLevel::Info, "Creating lit cube primitive proxy...");
    
    // Create cube with per-face normals (24 vertices: 4 per face * 6 faces)
    std::vector<FLitVertex> vertices;
    std::vector<uint32> indices;
    
    // Face normals
    FVector normals[6] = {
        FVector(0.0f, 0.0f, 1.0f),   // Front (+Z)
        FVector(0.0f, 0.0f, -1.0f),  // Back (-Z)
        FVector(0.0f, 1.0f, 0.0f),   // Top (+Y)
        FVector(0.0f, -1.0f, 0.0f),  // Bottom (-Y)
        FVector(1.0f, 0.0f, 0.0f),   // Right (+X)
        FVector(-1.0f, 0.0f, 0.0f),  // Left (-X)
    };
    
    // Vertex color (white, material controls actual color)
    FColor white(1.0f, 1.0f, 1.0f, 1.0f);
    
    // Front face (+Z)
    vertices.push_back({ FVector(-0.5f, -0.5f,  0.5f), normals[0], white });
    vertices.push_back({ FVector( 0.5f, -0.5f,  0.5f), normals[0], white });
    vertices.push_back({ FVector( 0.5f,  0.5f,  0.5f), normals[0], white });
    vertices.push_back({ FVector(-0.5f,  0.5f,  0.5f), normals[0], white });
    
    // Back face (-Z)
    vertices.push_back({ FVector(-0.5f, -0.5f, -0.5f), normals[1], white });
    vertices.push_back({ FVector( 0.5f, -0.5f, -0.5f), normals[1], white });
    vertices.push_back({ FVector( 0.5f,  0.5f, -0.5f), normals[1], white });
    vertices.push_back({ FVector(-0.5f,  0.5f, -0.5f), normals[1], white });
    
    // Top face (+Y)
    vertices.push_back({ FVector(-0.5f,  0.5f,  0.5f), normals[2], white });
    vertices.push_back({ FVector( 0.5f,  0.5f,  0.5f), normals[2], white });
    vertices.push_back({ FVector( 0.5f,  0.5f, -0.5f), normals[2], white });
    vertices.push_back({ FVector(-0.5f,  0.5f, -0.5f), normals[2], white });
    
    // Bottom face (-Y)
    vertices.push_back({ FVector(-0.5f, -0.5f,  0.5f), normals[3], white });
    vertices.push_back({ FVector( 0.5f, -0.5f,  0.5f), normals[3], white });
    vertices.push_back({ FVector( 0.5f, -0.5f, -0.5f), normals[3], white });
    vertices.push_back({ FVector(-0.5f, -0.5f, -0.5f), normals[3], white });
    
    // Right face (+X)
    vertices.push_back({ FVector( 0.5f, -0.5f,  0.5f), normals[4], white });
    vertices.push_back({ FVector( 0.5f, -0.5f, -0.5f), normals[4], white });
    vertices.push_back({ FVector( 0.5f,  0.5f, -0.5f), normals[4], white });
    vertices.push_back({ FVector( 0.5f,  0.5f,  0.5f), normals[4], white });
    
    // Left face (-X)
    vertices.push_back({ FVector(-0.5f, -0.5f,  0.5f), normals[5], white });
    vertices.push_back({ FVector(-0.5f, -0.5f, -0.5f), normals[5], white });
    vertices.push_back({ FVector(-0.5f,  0.5f, -0.5f), normals[5], white });
    vertices.push_back({ FVector(-0.5f,  0.5f,  0.5f), normals[5], white });
    
    // Indices (6 faces * 2 triangles * 3 indices = 36)
    indices = {
        0, 1, 2,    0, 2, 3,     // Front
        5, 4, 7,    5, 7, 6,     // Back
        8, 9, 10,   8, 10, 11,   // Top
        15, 14, 13, 15, 13, 12,  // Bottom
        16, 17, 18, 16, 18, 19,  // Right
        21, 20, 23, 21, 23, 22   // Left
    };
    
    // Create buffers
    FRHIBuffer* vertexBuffer = RHI->CreateVertexBuffer(vertices.size() * sizeof(FLitVertex), vertices.data());
    FRHIBuffer* indexBuffer = RHI->CreateIndexBuffer(indices.size() * sizeof(uint32), indices.data());
    FRHIBuffer* mvpBuffer = RHI->CreateConstantBuffer(sizeof(FMatrix4x4));
    FRHIBuffer* lightingBuffer = RHI->CreateConstantBuffer(sizeof(FLightingConstants));
    
    // Create lit pipeline state
    EPipelineFlags flags = EPipelineFlags::EnableDepth | EPipelineFlags::EnableLighting;
    FRHIPipelineState* pso = RHI->CreateGraphicsPipelineStateEx(flags);
    
    // Create transform for proxy
    FTransform transform;
    transform.Position = Position;
    transform.Rotation = Rotation;
    transform.Scale = Scale;
    
    return new FLitPrimitiveSceneProxy(vertexBuffer, indexBuffer, mvpBuffer, lightingBuffer,
                                        pso, indices.size(), g_Camera, transform, LightScene, Material);
}

// FLitSpherePrimitive implementation
FLitSpherePrimitive::FLitSpherePrimitive(uint32 InSegments, uint32 InRings)
    : Segments(InSegments)
    , Rings(InRings)
    , bAutoRotate(false)
    , RotationSpeed(0.3f)
{
}

void FLitSpherePrimitive::Tick(float DeltaTime)
{
    if (bAutoRotate)
    {
        Rotation.Y += DeltaTime * RotationSpeed;
        MarkTransformDirty();
    }
}

FSceneProxy* FLitSpherePrimitive::CreateSceneProxy(FRHI* RHI, FLightScene* LightScene)
{
    FLog::Log(ELogLevel::Info, "Creating lit sphere primitive proxy...");
    
    std::vector<FLitVertex> vertices;
    std::vector<uint32> indices;
    
    FColor white(1.0f, 1.0f, 1.0f, 1.0f);
    const float radius = 0.5f;
    
    // Generate sphere vertices with smooth normals (normal = position for unit sphere)
    for (uint32 ring = 0; ring <= Rings; ++ring)
    {
        float phi = M_PI * float(ring) / float(Rings);  // 0 to PI
        float y = cosf(phi);
        float ringRadius = sinf(phi);
        
        for (uint32 seg = 0; seg <= Segments; ++seg)
        {
            float theta = 2.0f * M_PI * float(seg) / float(Segments);
            float x = ringRadius * cosf(theta);
            float z = ringRadius * sinf(theta);
            
            // Normal is just the normalized position (pointing outward from center)
            FVector normal(x, y, z);
            FVector position(x * radius, y * radius, z * radius);
            
            vertices.push_back({ position, normal, white });
        }
    }
    
    // Generate indices
    for (uint32 ring = 0; ring < Rings; ++ring)
    {
        for (uint32 seg = 0; seg < Segments; ++seg)
        {
            uint32 current = ring * (Segments + 1) + seg;
            uint32 next = current + Segments + 1;
            
            indices.push_back(current);
            indices.push_back(next);
            indices.push_back(current + 1);
            
            indices.push_back(current + 1);
            indices.push_back(next);
            indices.push_back(next + 1);
        }
    }
    
    FLog::Log(ELogLevel::Info, std::string("Lit Sphere: ") + std::to_string(vertices.size()) + 
              " vertices, " + std::to_string(indices.size()) + " indices");
    
    // Create buffers
    FRHIBuffer* vertexBuffer = RHI->CreateVertexBuffer(vertices.size() * sizeof(FLitVertex), vertices.data());
    FRHIBuffer* indexBuffer = RHI->CreateIndexBuffer(indices.size() * sizeof(uint32), indices.data());
    FRHIBuffer* mvpBuffer = RHI->CreateConstantBuffer(sizeof(FMatrix4x4));
    FRHIBuffer* lightingBuffer = RHI->CreateConstantBuffer(sizeof(FLightingConstants));
    
    EPipelineFlags flags = EPipelineFlags::EnableDepth | EPipelineFlags::EnableLighting;
    FRHIPipelineState* pso = RHI->CreateGraphicsPipelineStateEx(flags);
    
    FTransform transform;
    transform.Position = Position;
    transform.Rotation = Rotation;
    transform.Scale = Scale;
    
    return new FLitPrimitiveSceneProxy(vertexBuffer, indexBuffer, mvpBuffer, lightingBuffer,
                                        pso, indices.size(), g_Camera, transform, LightScene, Material);
}

// FLitPlanePrimitive implementation
FLitPlanePrimitive::FLitPlanePrimitive(uint32 InSubdivisions)
    : Subdivisions(InSubdivisions)
{
}

void FLitPlanePrimitive::Tick(float DeltaTime)
{
    // Planes don't animate
}

FSceneProxy* FLitPlanePrimitive::CreateSceneProxy(FRHI* RHI, FLightScene* LightScene)
{
    FLog::Log(ELogLevel::Info, "Creating lit plane primitive proxy...");
    
    std::vector<FLitVertex> vertices;
    std::vector<uint32> indices;
    
    FColor white(1.0f, 1.0f, 1.0f, 1.0f);
    FVector upNormal(0.0f, 1.0f, 0.0f);  // Plane faces up
    
    const float size = 1.0f;
    const uint32 divs = Subdivisions + 1;
    
    // Generate grid vertices
    for (uint32 z = 0; z <= Subdivisions; ++z)
    {
        for (uint32 x = 0; x <= Subdivisions; ++x)
        {
            float px = (float(x) / float(Subdivisions) - 0.5f) * size;
            float pz = (float(z) / float(Subdivisions) - 0.5f) * size;
            
            vertices.push_back({ FVector(px, 0.0f, pz), upNormal, white });
        }
    }
    
    // Generate indices
    for (uint32 z = 0; z < Subdivisions; ++z)
    {
        for (uint32 x = 0; x < Subdivisions; ++x)
        {
            uint32 topLeft = z * divs + x;
            uint32 topRight = topLeft + 1;
            uint32 bottomLeft = (z + 1) * divs + x;
            uint32 bottomRight = bottomLeft + 1;
            
            indices.push_back(topLeft);
            indices.push_back(bottomLeft);
            indices.push_back(topRight);
            
            indices.push_back(topRight);
            indices.push_back(bottomLeft);
            indices.push_back(bottomRight);
        }
    }
    
    FLog::Log(ELogLevel::Info, std::string("Lit Plane: ") + std::to_string(vertices.size()) + 
              " vertices, " + std::to_string(indices.size()) + " indices");
    
    // Create buffers
    FRHIBuffer* vertexBuffer = RHI->CreateVertexBuffer(vertices.size() * sizeof(FLitVertex), vertices.data());
    FRHIBuffer* indexBuffer = RHI->CreateIndexBuffer(indices.size() * sizeof(uint32), indices.data());
    FRHIBuffer* mvpBuffer = RHI->CreateConstantBuffer(sizeof(FMatrix4x4));
    FRHIBuffer* lightingBuffer = RHI->CreateConstantBuffer(sizeof(FLightingConstants));
    
    EPipelineFlags flags = EPipelineFlags::EnableDepth | EPipelineFlags::EnableLighting;
    FRHIPipelineState* pso = RHI->CreateGraphicsPipelineStateEx(flags);
    
    FTransform transform;
    transform.Position = Position;
    transform.Rotation = Rotation;
    transform.Scale = Scale;
    
    return new FLitPrimitiveSceneProxy(vertexBuffer, indexBuffer, mvpBuffer, lightingBuffer,
                                        pso, indices.size(), g_Camera, transform, LightScene, Material);
}

// FLitCylinderPrimitive implementation
FLitCylinderPrimitive::FLitCylinderPrimitive(uint32 InSegments)
    : Segments(InSegments)
    , bAutoRotate(false)
    , RotationSpeed(0.4f)
{
}

void FLitCylinderPrimitive::Tick(float DeltaTime)
{
    if (bAutoRotate)
    {
        Rotation.Y += DeltaTime * RotationSpeed;
        MarkTransformDirty();
    }
}

FSceneProxy* FLitCylinderPrimitive::CreateSceneProxy(FRHI* RHI, FLightScene* LightScene)
{
    FLog::Log(ELogLevel::Info, "Creating lit cylinder primitive proxy...");
    
    std::vector<FLitVertex> vertices;
    std::vector<uint32> indices;
    
    FColor white(1.0f, 1.0f, 1.0f, 1.0f);
    const float height = 1.0f;
    const float radius = 0.5f;
    
    // Generate side vertices with outward-facing normals
    for (uint32 seg = 0; seg <= Segments; ++seg)
    {
        float theta = 2.0f * M_PI * float(seg) / float(Segments);
        float x = cosf(theta);
        float z = sinf(theta);
        
        FVector sideNormal(x, 0.0f, z);  // Horizontal normal pointing outward
        
        // Top and bottom vertices for side
        vertices.push_back({ FVector(x * radius, height * 0.5f, z * radius), sideNormal, white });
        vertices.push_back({ FVector(x * radius, -height * 0.5f, z * radius), sideNormal, white });
    }
    
    // Generate side indices
    for (uint32 seg = 0; seg < Segments; ++seg)
    {
        uint32 top1 = seg * 2;
        uint32 bottom1 = seg * 2 + 1;
        uint32 top2 = (seg + 1) * 2;
        uint32 bottom2 = (seg + 1) * 2 + 1;
        
        indices.push_back(top1);
        indices.push_back(bottom1);
        indices.push_back(top2);
        
        indices.push_back(top2);
        indices.push_back(bottom1);
        indices.push_back(bottom2);
    }
    
    // Add top cap
    FVector topNormal(0.0f, 1.0f, 0.0f);
    uint32 topCenterIdx = vertices.size();
    vertices.push_back({ FVector(0.0f, height * 0.5f, 0.0f), topNormal, white });
    
    for (uint32 seg = 0; seg < Segments; ++seg)
    {
        float theta1 = 2.0f * M_PI * float(seg) / float(Segments);
        float theta2 = 2.0f * M_PI * float(seg + 1) / float(Segments);
        
        uint32 idx1 = vertices.size();
        uint32 idx2 = vertices.size() + 1;
        
        vertices.push_back({ FVector(radius * cosf(theta1), height * 0.5f, radius * sinf(theta1)), topNormal, white });
        vertices.push_back({ FVector(radius * cosf(theta2), height * 0.5f, radius * sinf(theta2)), topNormal, white });
        
        indices.push_back(topCenterIdx);
        indices.push_back(idx1);
        indices.push_back(idx2);
    }
    
    // Add bottom cap
    FVector bottomNormal(0.0f, -1.0f, 0.0f);
    uint32 bottomCenterIdx = vertices.size();
    vertices.push_back({ FVector(0.0f, -height * 0.5f, 0.0f), bottomNormal, white });
    
    for (uint32 seg = 0; seg < Segments; ++seg)
    {
        float theta1 = 2.0f * M_PI * float(seg) / float(Segments);
        float theta2 = 2.0f * M_PI * float(seg + 1) / float(Segments);
        
        uint32 idx1 = vertices.size();
        uint32 idx2 = vertices.size() + 1;
        
        vertices.push_back({ FVector(radius * cosf(theta1), -height * 0.5f, radius * sinf(theta1)), bottomNormal, white });
        vertices.push_back({ FVector(radius * cosf(theta2), -height * 0.5f, radius * sinf(theta2)), bottomNormal, white });
        
        indices.push_back(bottomCenterIdx);
        indices.push_back(idx2);
        indices.push_back(idx1);
    }
    
    FLog::Log(ELogLevel::Info, std::string("Lit Cylinder: ") + std::to_string(vertices.size()) + 
              " vertices, " + std::to_string(indices.size()) + " indices");
    
    // Create buffers
    FRHIBuffer* vertexBuffer = RHI->CreateVertexBuffer(vertices.size() * sizeof(FLitVertex), vertices.data());
    FRHIBuffer* indexBuffer = RHI->CreateIndexBuffer(indices.size() * sizeof(uint32), indices.data());
    FRHIBuffer* mvpBuffer = RHI->CreateConstantBuffer(sizeof(FMatrix4x4));
    FRHIBuffer* lightingBuffer = RHI->CreateConstantBuffer(sizeof(FLightingConstants));
    
    EPipelineFlags flags = EPipelineFlags::EnableDepth | EPipelineFlags::EnableLighting;
    FRHIPipelineState* pso = RHI->CreateGraphicsPipelineStateEx(flags);
    
    FTransform transform;
    transform.Position = Position;
    transform.Rotation = Rotation;
    transform.Scale = Scale;
    
    return new FLitPrimitiveSceneProxy(vertexBuffer, indexBuffer, mvpBuffer, lightingBuffer,
                                        pso, indices.size(), g_Camera, transform, LightScene, Material);
}
