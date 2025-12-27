#include "ScenePrimitive.h"
#include "UnlitSceneProxy.h"
#include "LitSceneProxy.h"
#include "../Game/GameGlobals.h"
#include "../RHI/RHI.h"
#include "../Renderer/Camera.h"
#include <vector>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ============================================================================
// FPrimitive - Base class implementation
// ============================================================================

FPrimitive::FPrimitive()
    : Transform()
    , Material(FMaterial::Default())
    , Color(1.0f, 1.0f, 1.0f, 1.0f)
    , PrimitiveType(EPrimitiveType::Lit)  // Default to lit rendering
    , bIsDirty(true)
    , bTransformDirty(false)
{
}

void FPrimitive::Tick(float DeltaTime)
{
    // Base implementation does nothing
}

// ============================================================================
// LIT PRIMITIVES (Default)
// ============================================================================

// FCubePrimitive implementation (lit)
FCubePrimitive::FCubePrimitive()
    : bAutoRotate(false)
    , RotationSpeed(0.5f)
{
    PrimitiveType = EPrimitiveType::Lit;
}

void FCubePrimitive::Tick(float DeltaTime)
{
    if (bAutoRotate)
    {
        Transform.Rotation.Y += DeltaTime * RotationSpeed;
        MarkTransformDirty();
    }
}

FSceneProxy* FCubePrimitive::CreateSceneProxy(FRHI* RHI, FLightScene* LightScene)
{
    FLog::Log(ELogLevel::Info, "Creating cube primitive proxy...");
    
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
    
    // Indices
    indices = {
        0, 1, 2,    0, 2, 3,     // Front
        5, 4, 7,    5, 7, 6,     // Back
        8, 9, 10,   8, 10, 11,   // Top
        15, 14, 13, 15, 13, 12,  // Bottom
        16, 17, 18, 16, 18, 19,  // Right
        21, 20, 23, 21, 23, 22   // Left
    };
    
    FRHIBuffer* vertexBuffer = RHI->CreateVertexBuffer(vertices.size() * sizeof(FLitVertex), vertices.data());
    FRHIBuffer* indexBuffer = RHI->CreateIndexBuffer(indices.size() * sizeof(uint32), indices.data());
    FRHIBuffer* mvpBuffer = RHI->CreateConstantBuffer(sizeof(FMatrix4x4));
    FRHIBuffer* lightingBuffer = RHI->CreateConstantBuffer(sizeof(FLightingConstants));
    
    EPipelineFlags flags = EPipelineFlags::EnableDepth | EPipelineFlags::EnableLighting;
    FRHIPipelineState* pso = RHI->CreateGraphicsPipelineStateEx(flags);
    
    return new FPrimitiveSceneProxy(vertexBuffer, indexBuffer, mvpBuffer, lightingBuffer,
                                        pso, indices.size(), g_Camera, Transform, LightScene, Material);
}

// FSpherePrimitive implementation (lit)
FSpherePrimitive::FSpherePrimitive(uint32 InSegments, uint32 InRings)
    : Segments(InSegments)
    , Rings(InRings)
    , bAutoRotate(false)
    , RotationSpeed(0.3f)
{
    PrimitiveType = EPrimitiveType::Lit;
}

void FSpherePrimitive::Tick(float DeltaTime)
{
    if (bAutoRotate)
    {
        Transform.Rotation.Y += DeltaTime * RotationSpeed;
        MarkTransformDirty();
    }
}

FSceneProxy* FSpherePrimitive::CreateSceneProxy(FRHI* RHI, FLightScene* LightScene)
{
    FLog::Log(ELogLevel::Info, "Creating sphere primitive proxy...");
    
    std::vector<FLitVertex> vertices;
    std::vector<uint32> indices;
    
    FColor white(1.0f, 1.0f, 1.0f, 1.0f);
    const float radius = 0.5f;
    
    for (uint32 ring = 0; ring <= Rings; ++ring)
    {
        float phi = M_PI * float(ring) / float(Rings);
        float y = cosf(phi);
        float ringRadius = sinf(phi);
        
        for (uint32 seg = 0; seg <= Segments; ++seg)
        {
            float theta = 2.0f * M_PI * float(seg) / float(Segments);
            float x = ringRadius * cosf(theta);
            float z = ringRadius * sinf(theta);
            
            FVector normal(x, y, z);
            FVector position(x * radius, y * radius, z * radius);
            
            vertices.push_back({ position, normal, white });
        }
    }
    
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
    
    FRHIBuffer* vertexBuffer = RHI->CreateVertexBuffer(vertices.size() * sizeof(FLitVertex), vertices.data());
    FRHIBuffer* indexBuffer = RHI->CreateIndexBuffer(indices.size() * sizeof(uint32), indices.data());
    FRHIBuffer* mvpBuffer = RHI->CreateConstantBuffer(sizeof(FMatrix4x4));
    FRHIBuffer* lightingBuffer = RHI->CreateConstantBuffer(sizeof(FLightingConstants));
    
    EPipelineFlags flags = EPipelineFlags::EnableDepth | EPipelineFlags::EnableLighting;
    FRHIPipelineState* pso = RHI->CreateGraphicsPipelineStateEx(flags);
    
    return new FPrimitiveSceneProxy(vertexBuffer, indexBuffer, mvpBuffer, lightingBuffer,
                                        pso, indices.size(), g_Camera, Transform, LightScene, Material);
}

// FPlanePrimitive implementation (lit)
FPlanePrimitive::FPlanePrimitive(uint32 InSubdivisions)
    : Subdivisions(InSubdivisions)
{
    PrimitiveType = EPrimitiveType::Lit;
}

void FPlanePrimitive::Tick(float DeltaTime)
{
    // Planes don't animate
}

FSceneProxy* FPlanePrimitive::CreateSceneProxy(FRHI* RHI, FLightScene* LightScene)
{
    FLog::Log(ELogLevel::Info, "Creating plane primitive proxy...");
    
    std::vector<FLitVertex> vertices;
    std::vector<uint32> indices;
    
    FColor white(1.0f, 1.0f, 1.0f, 1.0f);
    FVector upNormal(0.0f, 1.0f, 0.0f);
    
    const float size = 1.0f;
    const uint32 divs = Subdivisions + 1;
    
    for (uint32 z = 0; z <= Subdivisions; ++z)
    {
        for (uint32 x = 0; x <= Subdivisions; ++x)
        {
            float px = (float(x) / float(Subdivisions) - 0.5f) * size;
            float pz = (float(z) / float(Subdivisions) - 0.5f) * size;
            
            vertices.push_back({ FVector(px, 0.0f, pz), upNormal, white });
        }
    }
    
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
    
    FRHIBuffer* vertexBuffer = RHI->CreateVertexBuffer(vertices.size() * sizeof(FLitVertex), vertices.data());
    FRHIBuffer* indexBuffer = RHI->CreateIndexBuffer(indices.size() * sizeof(uint32), indices.data());
    FRHIBuffer* mvpBuffer = RHI->CreateConstantBuffer(sizeof(FMatrix4x4));
    FRHIBuffer* lightingBuffer = RHI->CreateConstantBuffer(sizeof(FLightingConstants));
    
    EPipelineFlags flags = EPipelineFlags::EnableDepth | EPipelineFlags::EnableLighting;
    FRHIPipelineState* pso = RHI->CreateGraphicsPipelineStateEx(flags);
    
    return new FPrimitiveSceneProxy(vertexBuffer, indexBuffer, mvpBuffer, lightingBuffer,
                                        pso, indices.size(), g_Camera, Transform, LightScene, Material);
}

// FCylinderPrimitive implementation (lit)
FCylinderPrimitive::FCylinderPrimitive(uint32 InSegments)
    : Segments(InSegments)
    , bAutoRotate(false)
    , RotationSpeed(0.4f)
{
    PrimitiveType = EPrimitiveType::Lit;
}

void FCylinderPrimitive::Tick(float DeltaTime)
{
    if (bAutoRotate)
    {
        Transform.Rotation.Y += DeltaTime * RotationSpeed;
        MarkTransformDirty();
    }
}

FSceneProxy* FCylinderPrimitive::CreateSceneProxy(FRHI* RHI, FLightScene* LightScene)
{
    FLog::Log(ELogLevel::Info, "Creating cylinder primitive proxy...");
    
    std::vector<FLitVertex> vertices;
    std::vector<uint32> indices;
    
    FColor white(1.0f, 1.0f, 1.0f, 1.0f);
    const float height = 1.0f;
    const float radius = 0.5f;
    
    // Generate side vertices
    for (uint32 seg = 0; seg <= Segments; ++seg)
    {
        float theta = 2.0f * M_PI * float(seg) / float(Segments);
        float x = cosf(theta);
        float z = sinf(theta);
        
        FVector sideNormal(x, 0.0f, z);
        
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
    
    // Top cap
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
    
    // Bottom cap
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
    
    FRHIBuffer* vertexBuffer = RHI->CreateVertexBuffer(vertices.size() * sizeof(FLitVertex), vertices.data());
    FRHIBuffer* indexBuffer = RHI->CreateIndexBuffer(indices.size() * sizeof(uint32), indices.data());
    FRHIBuffer* mvpBuffer = RHI->CreateConstantBuffer(sizeof(FMatrix4x4));
    FRHIBuffer* lightingBuffer = RHI->CreateConstantBuffer(sizeof(FLightingConstants));
    
    EPipelineFlags flags = EPipelineFlags::EnableDepth | EPipelineFlags::EnableLighting;
    FRHIPipelineState* pso = RHI->CreateGraphicsPipelineStateEx(flags);
    
    return new FPrimitiveSceneProxy(vertexBuffer, indexBuffer, mvpBuffer, lightingBuffer,
                                        pso, indices.size(), g_Camera, Transform, LightScene, Material);
}

// ============================================================================
// UNLIT PRIMITIVES
// ============================================================================

FUnlitCubePrimitive::FUnlitCubePrimitive()
    : bAutoRotate(true)
    , RotationSpeed(0.5f)
{
    PrimitiveType = EPrimitiveType::Unlit;
}

void FUnlitCubePrimitive::Tick(float DeltaTime)
{
    if (bAutoRotate)
    {
        Transform.Rotation.Y += DeltaTime * RotationSpeed;
        Transform.Rotation.X += DeltaTime * RotationSpeed * 0.3f;
        MarkTransformDirty();
    }
}

FSceneProxy* FUnlitCubePrimitive::CreateSceneProxy(FRHI* RHI, FLightScene* /*LightScene*/)
{
    FLog::Log(ELogLevel::Info, "Creating unlit cube primitive proxy...");
    
    std::vector<FVertex> vertices = {
        { FVector(-0.5f, -0.5f,  0.5f), Color },
        { FVector( 0.5f, -0.5f,  0.5f), Color },
        { FVector( 0.5f,  0.5f,  0.5f), Color },
        { FVector(-0.5f,  0.5f,  0.5f), Color },
        { FVector(-0.5f, -0.5f, -0.5f), Color },
        { FVector( 0.5f, -0.5f, -0.5f), Color },
        { FVector( 0.5f,  0.5f, -0.5f), Color },
        { FVector(-0.5f,  0.5f, -0.5f), Color },
        { FVector(-0.5f,  0.5f,  0.5f), Color },
        { FVector( 0.5f,  0.5f,  0.5f), Color },
        { FVector( 0.5f,  0.5f, -0.5f), Color },
        { FVector(-0.5f,  0.5f, -0.5f), Color },
        { FVector(-0.5f, -0.5f,  0.5f), Color },
        { FVector( 0.5f, -0.5f,  0.5f), Color },
        { FVector( 0.5f, -0.5f, -0.5f), Color },
        { FVector(-0.5f, -0.5f, -0.5f), Color },
        { FVector( 0.5f, -0.5f,  0.5f), Color },
        { FVector( 0.5f, -0.5f, -0.5f), Color },
        { FVector( 0.5f,  0.5f, -0.5f), Color },
        { FVector( 0.5f,  0.5f,  0.5f), Color },
        { FVector(-0.5f, -0.5f,  0.5f), Color },
        { FVector(-0.5f, -0.5f, -0.5f), Color },
        { FVector(-0.5f,  0.5f, -0.5f), Color },
        { FVector(-0.5f,  0.5f,  0.5f), Color },
    };
    
    std::vector<uint32> indices = {
        0, 1, 2,    0, 2, 3,
        5, 4, 7,    5, 7, 6,
        8, 9, 10,   8, 10, 11,
        15, 14, 13, 15, 13, 12,
        16, 17, 18, 16, 18, 19,
        21, 20, 23, 21, 23, 22
    };
    
    FRHIBuffer* vertexBuffer = RHI->CreateVertexBuffer(vertices.size() * sizeof(FVertex), vertices.data());
    FRHIBuffer* indexBuffer = RHI->CreateIndexBuffer(indices.size() * sizeof(uint32), indices.data());
    FRHIBuffer* constantBuffer = RHI->CreateConstantBuffer(sizeof(FMatrix4x4));
    FRHIPipelineState* pso = RHI->CreateGraphicsPipelineState(true);
    
    return new FUnlitPrimitiveSceneProxy(vertexBuffer, indexBuffer, constantBuffer, pso, 
                                    indices.size(), g_Camera, Transform);
}

FUnlitSpherePrimitive::FUnlitSpherePrimitive(uint32 InSegments, uint32 InRings)
    : Segments(InSegments)
    , Rings(InRings)
    , bAutoRotate(true)
    , RotationSpeed(0.3f)
{
    PrimitiveType = EPrimitiveType::Unlit;
}

void FUnlitSpherePrimitive::Tick(float DeltaTime)
{
    if (bAutoRotate)
    {
        Transform.Rotation.Y += DeltaTime * RotationSpeed;
        MarkTransformDirty();
    }
}

FSceneProxy* FUnlitSpherePrimitive::CreateSceneProxy(FRHI* RHI, FLightScene* /*LightScene*/)
{
    FLog::Log(ELogLevel::Info, "Creating unlit sphere primitive proxy...");
    
    std::vector<FVertex> vertices;
    std::vector<uint32> indices;
    
    for (uint32 ring = 0; ring <= Rings; ++ring)
    {
        float phi = M_PI * float(ring) / float(Rings);
        float y = cosf(phi);
        float ringRadius = sinf(phi);
        
        for (uint32 seg = 0; seg <= Segments; ++seg)
        {
            float theta = 2.0f * M_PI * float(seg) / float(Segments);
            float x = ringRadius * cosf(theta);
            float z = ringRadius * sinf(theta);
            
            vertices.push_back({ FVector(x * 0.5f, y * 0.5f, z * 0.5f), Color });
        }
    }
    
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
    
    FRHIBuffer* vertexBuffer = RHI->CreateVertexBuffer(vertices.size() * sizeof(FVertex), vertices.data());
    FRHIBuffer* indexBuffer = RHI->CreateIndexBuffer(indices.size() * sizeof(uint32), indices.data());
    FRHIBuffer* constantBuffer = RHI->CreateConstantBuffer(sizeof(FMatrix4x4));
    FRHIPipelineState* pso = RHI->CreateGraphicsPipelineState(true);
    
    return new FUnlitPrimitiveSceneProxy(vertexBuffer, indexBuffer, constantBuffer, pso,
                                    indices.size(), g_Camera, Transform);
}

// ============================================================================
// SPECIAL PRIMITIVES
// ============================================================================

// FGizmoPrimitive implementation
FGizmoPrimitive::FGizmoPrimitive(float InAxisLength)
    : AxisLength(InAxisLength)
    , bScreenSpace(false)  // World-space by default
    , ScreenCorner(2)  // Bottom-left by default (for screen-space mode)
    , GizmoSize(40.0f)  // 40 pixels default size (for screen-space mode)
{
    PrimitiveType = EPrimitiveType::Unlit;  // Gizmo is always unlit
}

void FGizmoPrimitive::Tick(float DeltaTime)
{
    // Gizmo doesn't animate
}

FSceneProxy* FGizmoPrimitive::CreateSceneProxy(FRHI* RHI, FLightScene* /*LightScene*/)
{
    FLog::Log(ELogLevel::Info, "Creating gizmo primitive proxy...");
    
    std::vector<FVertex> vertices;
    std::vector<uint32> indices;
    
    const float arrowHeadLength = AxisLength * 0.15f;
    const float arrowHeadRadius = 0.08f;
    const float shaftRadius = 0.02f;
    const uint32 segments = 12;
    
    FColor xColor(1.0f, 0.2f, 0.2f, 1.0f);
    FColor yColor(0.2f, 1.0f, 0.2f, 1.0f);
    FColor zColor(0.3f, 0.5f, 1.0f, 1.0f);
    
    constexpr float EPSILON = 0.0001f;
    
    auto AddAxis = [&](const FVector& direction, const FColor& color) {
        uint32 baseIdx = vertices.size();
        
        float length = std::sqrt(direction.X * direction.X + direction.Y * direction.Y + direction.Z * direction.Z);
        if (length < EPSILON) return;
        FVector dir(direction.X / length, direction.Y / length, direction.Z / length);
        
        FVector perp1, perp2;
        if (std::abs(dir.Y) < 0.9f) {
            perp1 = FVector(-dir.Z, 0.0f, dir.X);
        } else {
            perp1 = FVector(0.0f, dir.Z, -dir.Y);
        }
        float p1len = std::sqrt(perp1.X * perp1.X + perp1.Y * perp1.Y + perp1.Z * perp1.Z);
        if (p1len < EPSILON) return;
        perp1 = FVector(perp1.X / p1len, perp1.Y / p1len, perp1.Z / p1len);
        
        perp2 = FVector(
            dir.Y * perp1.Z - dir.Z * perp1.Y,
            dir.Z * perp1.X - dir.X * perp1.Z,
            dir.X * perp1.Y - dir.Y * perp1.X
        );
        
        float shaftLength = AxisLength - arrowHeadLength;
        
        for (uint32 i = 0; i <= segments; ++i) {
            float angle = 2.0f * M_PI * float(i) / float(segments);
            float c = cosf(angle);
            float s = sinf(angle);
            
            FVector base(
                shaftRadius * (c * perp1.X + s * perp2.X),
                shaftRadius * (c * perp1.Y + s * perp2.Y),
                shaftRadius * (c * perp1.Z + s * perp2.Z)
            );
            
            FVector top(
                base.X + dir.X * shaftLength,
                base.Y + dir.Y * shaftLength,
                base.Z + dir.Z * shaftLength
            );
            
            vertices.push_back({ base, color });
            vertices.push_back({ top, color });
        }
        
        for (uint32 i = 0; i < segments; ++i) {
            uint32 curr = baseIdx + i * 2;
            uint32 next = baseIdx + (i + 1) * 2;
            
            indices.push_back(curr);
            indices.push_back(curr + 1);
            indices.push_back(next);
            
            indices.push_back(next);
            indices.push_back(curr + 1);
            indices.push_back(next + 1);
        }
        
        uint32 coneBaseIdx = vertices.size();
        FVector coneBase(dir.X * shaftLength, dir.Y * shaftLength, dir.Z * shaftLength);
        FVector coneTip(dir.X * AxisLength, dir.Y * AxisLength, dir.Z * AxisLength);
        
        uint32 tipIdx = vertices.size();
        vertices.push_back({ coneTip, color });
        
        for (uint32 i = 0; i <= segments; ++i) {
            float angle = 2.0f * M_PI * float(i) / float(segments);
            float c = cosf(angle);
            float s = sinf(angle);
            
            FVector pt(
                coneBase.X + arrowHeadRadius * (c * perp1.X + s * perp2.X),
                coneBase.Y + arrowHeadRadius * (c * perp1.Y + s * perp2.Y),
                coneBase.Z + arrowHeadRadius * (c * perp1.Z + s * perp2.Z)
            );
            vertices.push_back({ pt, color });
        }
        
        for (uint32 i = 0; i < segments; ++i) {
            indices.push_back(tipIdx);
            indices.push_back(tipIdx + 1 + i);
            indices.push_back(tipIdx + 1 + i + 1);
        }
        
        uint32 baseCenterIdx = vertices.size();
        vertices.push_back({ coneBase, color });
        for (uint32 i = 0; i < segments; ++i) {
            indices.push_back(baseCenterIdx);
            indices.push_back(tipIdx + 1 + i + 1);
            indices.push_back(tipIdx + 1 + i);
        }
    };
    
    AddAxis(FVector(1.0f, 0.0f, 0.0f), xColor);
    AddAxis(FVector(0.0f, 1.0f, 0.0f), yColor);
    AddAxis(FVector(0.0f, 0.0f, 1.0f), zColor);
    
    FRHIBuffer* vertexBuffer = RHI->CreateVertexBuffer(vertices.size() * sizeof(FVertex), vertices.data());
    FRHIBuffer* indexBuffer = RHI->CreateIndexBuffer(indices.size() * sizeof(uint32), indices.data());
    FRHIBuffer* constantBuffer = RHI->CreateConstantBuffer(sizeof(FMatrix4x4));
    
    if (bScreenSpace)
    {
        // Screen-space gizmo: Disable depth testing so it always renders on top
        FRHIPipelineState* pso = RHI->CreateGraphicsPipelineState(false);  // Depth disabled
        // Screen-space gizmo renders in corner showing world axis orientation
        return new FScreenSpaceGizmoProxy(vertexBuffer, indexBuffer, constantBuffer, pso,
                                          indices.size(), g_Camera, ScreenCorner, GizmoSize);
    }
    else
    {
        // World-space gizmo: Enable depth testing for proper occlusion
        FRHIPipelineState* pso = RHI->CreateGraphicsPipelineState(true);  // Depth enabled
        // World-space gizmo renders at transform position
        return new FUnlitPrimitiveSceneProxy(vertexBuffer, indexBuffer, constantBuffer, pso,
                                             indices.size(), g_Camera, Transform);
    }
}

// FDemoCubePrimitive implementation
FDemoCubePrimitive::FDemoCubePrimitive()
    : AnimationType(EAnimationType::None)
    , AnimationSpeed(1.0f)
    , AnimationTime(0.0f)
    , BasePosition(0.0f, 0.0f, 0.0f)
    , BaseScale(1.0f, 1.0f, 1.0f)
{
    PrimitiveType = EPrimitiveType::Lit;
}

void FDemoCubePrimitive::Tick(float DeltaTime)
{
    AnimationTime += DeltaTime * AnimationSpeed;
    
    switch (AnimationType)
    {
        case EAnimationType::RotateX:
            Transform.Rotation.X = AnimationTime;
            MarkTransformDirty();
            break;
        case EAnimationType::RotateY:
            Transform.Rotation.Y = AnimationTime;
            MarkTransformDirty();
            break;
        case EAnimationType::RotateZ:
            Transform.Rotation.Z = AnimationTime;
            MarkTransformDirty();
            break;
        case EAnimationType::TranslateX:
            Transform.Position.X = BasePosition.X + sinf(AnimationTime) * 1.0f;
            MarkTransformDirty();
            break;
        case EAnimationType::TranslateY:
            Transform.Position.Y = BasePosition.Y + sinf(AnimationTime) * 1.0f;
            MarkTransformDirty();
            break;
        case EAnimationType::TranslateZ:
            Transform.Position.Z = BasePosition.Z + sinf(AnimationTime) * 1.0f;
            MarkTransformDirty();
            break;
        case EAnimationType::TranslateDiagonal:
            {
                float offset = sinf(AnimationTime) * 0.8f;
                Transform.Position.X = BasePosition.X + offset;
                Transform.Position.Y = BasePosition.Y + offset;
                Transform.Position.Z = BasePosition.Z + offset;
                MarkTransformDirty();
            }
            break;
        case EAnimationType::Scale:
            {
                float scaleFactor = 1.0f + 0.3f * sinf(AnimationTime);
                Transform.Scale.X = BaseScale.X * scaleFactor;
                Transform.Scale.Y = BaseScale.Y * scaleFactor;
                Transform.Scale.Z = BaseScale.Z * scaleFactor;
                MarkTransformDirty();
            }
            break;
        case EAnimationType::None:
        default:
            break;
    }
}

FSceneProxy* FDemoCubePrimitive::CreateSceneProxy(FRHI* RHI, FLightScene* LightScene)
{
    FLog::Log(ELogLevel::Info, "Creating demo cube primitive proxy...");
    
    std::vector<FLitVertex> vertices;
    std::vector<uint32> indices;
    
    FVector normals[6] = {
        FVector(0.0f, 0.0f, 1.0f),
        FVector(0.0f, 0.0f, -1.0f),
        FVector(0.0f, 1.0f, 0.0f),
        FVector(0.0f, -1.0f, 0.0f),
        FVector(1.0f, 0.0f, 0.0f),
        FVector(-1.0f, 0.0f, 0.0f),
    };
    
    FColor white(1.0f, 1.0f, 1.0f, 1.0f);
    
    // Front
    vertices.push_back({ FVector(-0.5f, -0.5f,  0.5f), normals[0], white });
    vertices.push_back({ FVector( 0.5f, -0.5f,  0.5f), normals[0], white });
    vertices.push_back({ FVector( 0.5f,  0.5f,  0.5f), normals[0], white });
    vertices.push_back({ FVector(-0.5f,  0.5f,  0.5f), normals[0], white });
    // Back
    vertices.push_back({ FVector(-0.5f, -0.5f, -0.5f), normals[1], white });
    vertices.push_back({ FVector( 0.5f, -0.5f, -0.5f), normals[1], white });
    vertices.push_back({ FVector( 0.5f,  0.5f, -0.5f), normals[1], white });
    vertices.push_back({ FVector(-0.5f,  0.5f, -0.5f), normals[1], white });
    // Top
    vertices.push_back({ FVector(-0.5f,  0.5f,  0.5f), normals[2], white });
    vertices.push_back({ FVector( 0.5f,  0.5f,  0.5f), normals[2], white });
    vertices.push_back({ FVector( 0.5f,  0.5f, -0.5f), normals[2], white });
    vertices.push_back({ FVector(-0.5f,  0.5f, -0.5f), normals[2], white });
    // Bottom
    vertices.push_back({ FVector(-0.5f, -0.5f,  0.5f), normals[3], white });
    vertices.push_back({ FVector( 0.5f, -0.5f,  0.5f), normals[3], white });
    vertices.push_back({ FVector( 0.5f, -0.5f, -0.5f), normals[3], white });
    vertices.push_back({ FVector(-0.5f, -0.5f, -0.5f), normals[3], white });
    // Right
    vertices.push_back({ FVector( 0.5f, -0.5f,  0.5f), normals[4], white });
    vertices.push_back({ FVector( 0.5f, -0.5f, -0.5f), normals[4], white });
    vertices.push_back({ FVector( 0.5f,  0.5f, -0.5f), normals[4], white });
    vertices.push_back({ FVector( 0.5f,  0.5f,  0.5f), normals[4], white });
    // Left
    vertices.push_back({ FVector(-0.5f, -0.5f,  0.5f), normals[5], white });
    vertices.push_back({ FVector(-0.5f, -0.5f, -0.5f), normals[5], white });
    vertices.push_back({ FVector(-0.5f,  0.5f, -0.5f), normals[5], white });
    vertices.push_back({ FVector(-0.5f,  0.5f,  0.5f), normals[5], white });
    
    indices = {
        0, 1, 2,    0, 2, 3,
        5, 4, 7,    5, 7, 6,
        8, 9, 10,   8, 10, 11,
        15, 14, 13, 15, 13, 12,
        16, 17, 18, 16, 18, 19,
        21, 20, 23, 21, 23, 22
    };
    
    FRHIBuffer* vertexBuffer = RHI->CreateVertexBuffer(vertices.size() * sizeof(FLitVertex), vertices.data());
    FRHIBuffer* indexBuffer = RHI->CreateIndexBuffer(indices.size() * sizeof(uint32), indices.data());
    FRHIBuffer* mvpBuffer = RHI->CreateConstantBuffer(sizeof(FMatrix4x4));
    FRHIBuffer* lightingBuffer = RHI->CreateConstantBuffer(sizeof(FLightingConstants));
    
    EPipelineFlags flags = EPipelineFlags::EnableDepth | EPipelineFlags::EnableLighting;
    FRHIPipelineState* pso = RHI->CreateGraphicsPipelineStateEx(flags);
    
    return new FPrimitiveSceneProxy(vertexBuffer, indexBuffer, mvpBuffer, lightingBuffer,
                                        pso, indices.size(), g_Camera, Transform, LightScene, Material);
}
