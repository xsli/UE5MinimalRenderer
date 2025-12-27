#include "Primitive.h"
#include "PrimitiveSceneProxy.h"
#include "GameGlobals.h"
#include "../RHI/RHI.h"
#include "../Renderer/Camera.h"
#include <vector>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// FPrimitive implementation
FPrimitive::FPrimitive()
    : Transform()
    , Color(1.0f, 1.0f, 1.0f, 1.0f)
    , bIsDirty(true)  // Start as dirty so proxy is created on first update
    , bTransformDirty(false)
{
}

void FPrimitive::Tick(float DeltaTime)
{
    // Base implementation does nothing
}

// FCubePrimitive implementation
FCubePrimitive::FCubePrimitive()
    : bAutoRotate(true)
    , RotationSpeed(0.5f)
{
}

FCubePrimitive::~FCubePrimitive()
{
}

void FCubePrimitive::Tick(float DeltaTime)
{
    if (bAutoRotate)
    {
        Transform.Rotation.Y += DeltaTime * RotationSpeed;
        Transform.Rotation.X += DeltaTime * RotationSpeed * 0.3f;
        MarkTransformDirty();  // Only transform changed, no need to recreate proxy
    }
}

FPrimitiveSceneProxy* FCubePrimitive::CreateSceneProxy(FRHI* RHI)
{
    FLog::Log(ELogLevel::Info, "Creating cube primitive proxy...");
    
    // Define cube vertices (24 vertices, 4 per face for proper normals)
    std::vector<FVertex> vertices = {
        // Front face (+Z)
        { FVector(-0.5f, -0.5f,  0.5f), Color },
        { FVector( 0.5f, -0.5f,  0.5f), Color },
        { FVector( 0.5f,  0.5f,  0.5f), Color },
        { FVector(-0.5f,  0.5f,  0.5f), Color },
        
        // Back face (-Z)
        { FVector(-0.5f, -0.5f, -0.5f), Color },
        { FVector( 0.5f, -0.5f, -0.5f), Color },
        { FVector( 0.5f,  0.5f, -0.5f), Color },
        { FVector(-0.5f,  0.5f, -0.5f), Color },
        
        // Top face (+Y)
        { FVector(-0.5f,  0.5f,  0.5f), Color },
        { FVector( 0.5f,  0.5f,  0.5f), Color },
        { FVector( 0.5f,  0.5f, -0.5f), Color },
        { FVector(-0.5f,  0.5f, -0.5f), Color },
        
        // Bottom face (-Y)
        { FVector(-0.5f, -0.5f,  0.5f), Color },
        { FVector( 0.5f, -0.5f,  0.5f), Color },
        { FVector( 0.5f, -0.5f, -0.5f), Color },
        { FVector(-0.5f, -0.5f, -0.5f), Color },
        
        // Right face (+X)
        { FVector( 0.5f, -0.5f,  0.5f), Color },
        { FVector( 0.5f, -0.5f, -0.5f), Color },
        { FVector( 0.5f,  0.5f, -0.5f), Color },
        { FVector( 0.5f,  0.5f,  0.5f), Color },
        
        // Left face (-X)
        { FVector(-0.5f, -0.5f,  0.5f), Color },
        { FVector(-0.5f, -0.5f, -0.5f), Color },
        { FVector(-0.5f,  0.5f, -0.5f), Color },
        { FVector(-0.5f,  0.5f,  0.5f), Color },
    };
    
    // Define indices (6 faces * 2 triangles * 3 indices = 36)
    std::vector<uint32> indices = {
        // Front
        0, 1, 2,    0, 2, 3,
        // Back
        5, 4, 7,    5, 7, 6,
        // Top
        8, 9, 10,   8, 10, 11,
        // Bottom
        15, 14, 13, 15, 13, 12,
        // Right
        16, 17, 18, 16, 18, 19,
        // Left
        21, 20, 23, 21, 23, 22
    };
    
    // Create buffers
    FRHIBuffer* vertexBuffer = RHI->CreateVertexBuffer(vertices.size() * sizeof(FVertex), vertices.data());
    FRHIBuffer* indexBuffer = RHI->CreateIndexBuffer(indices.size() * sizeof(uint32), indices.data());
    FRHIBuffer* constantBuffer = RHI->CreateConstantBuffer(sizeof(FMatrix4x4));
    FRHIPipelineState* pso = RHI->CreateGraphicsPipelineState(true);
    
    return new FPrimitiveSceneProxy(vertexBuffer, indexBuffer, constantBuffer, pso, 
                                    indices.size(), g_Camera, Transform);
}

// FSpherePrimitive implementation
FSpherePrimitive::FSpherePrimitive(uint32 InSegments, uint32 InRings)
    : Segments(InSegments)
    , Rings(InRings)
    , bAutoRotate(true)
    , RotationSpeed(0.3f)
{
}

FSpherePrimitive::~FSpherePrimitive()
{
}

void FSpherePrimitive::Tick(float DeltaTime)
{
    if (bAutoRotate)
    {
        Transform.Rotation.Y += DeltaTime * RotationSpeed;
        MarkTransformDirty();  // Only transform changed, no need to recreate proxy
    }
}

FPrimitiveSceneProxy* FSpherePrimitive::CreateSceneProxy(FRHI* RHI)
{
    FLog::Log(ELogLevel::Info, "Creating sphere primitive proxy...");
    
    std::vector<FVertex> vertices;
    std::vector<uint32> indices;
    
    // Generate sphere vertices using UV sphere algorithm
    for (uint32 ring = 0; ring <= Rings; ++ring)
    {
        float phi = M_PI * float(ring) / float(Rings);  // 0 to PI
        float y = cosf(phi);
        float ringRadius = sinf(phi);
        
        for (uint32 seg = 0; seg <= Segments; ++seg)
        {
            float theta = 2.0f * M_PI * float(seg) / float(Segments);  // 0 to 2PI
            float x = ringRadius * cosf(theta);
            float z = ringRadius * sinf(theta);
            
            vertices.push_back({ FVector(x * 0.5f, y * 0.5f, z * 0.5f), Color });
        }
    }
    
    // Generate indices
    for (uint32 ring = 0; ring < Rings; ++ring)
    {
        for (uint32 seg = 0; seg < Segments; ++seg)
        {
            uint32 current = ring * (Segments + 1) + seg;
            uint32 next = current + Segments + 1;
            
            // Two triangles per quad
            indices.push_back(current);
            indices.push_back(next);
            indices.push_back(current + 1);
            
            indices.push_back(current + 1);
            indices.push_back(next);
            indices.push_back(next + 1);
        }
    }
    
    FLog::Log(ELogLevel::Info, std::string("Sphere: ") + std::to_string(vertices.size()) + 
              " vertices, " + std::to_string(indices.size()) + " indices");
    
    // Create buffers
    FRHIBuffer* vertexBuffer = RHI->CreateVertexBuffer(vertices.size() * sizeof(FVertex), vertices.data());
    FRHIBuffer* indexBuffer = RHI->CreateIndexBuffer(indices.size() * sizeof(uint32), indices.data());
    FRHIBuffer* constantBuffer = RHI->CreateConstantBuffer(sizeof(FMatrix4x4));
    FRHIPipelineState* pso = RHI->CreateGraphicsPipelineState(true);
    
    return new FPrimitiveSceneProxy(vertexBuffer, indexBuffer, constantBuffer, pso,
                                    indices.size(), g_Camera, Transform);
}

// FCylinderPrimitive implementation
FCylinderPrimitive::FCylinderPrimitive(uint32 InSegments)
    : Segments(InSegments)
    , bAutoRotate(true)
    , RotationSpeed(0.4f)
{
}

FCylinderPrimitive::~FCylinderPrimitive()
{
}

void FCylinderPrimitive::Tick(float DeltaTime)
{
    if (bAutoRotate)
    {
        Transform.Rotation.Y += DeltaTime * RotationSpeed;
        MarkTransformDirty();  // Only transform changed, no need to recreate proxy
    }
}

FPrimitiveSceneProxy* FCylinderPrimitive::CreateSceneProxy(FRHI* RHI)
{
    FLog::Log(ELogLevel::Info, "Creating cylinder primitive proxy...");
    
    std::vector<FVertex> vertices;
    std::vector<uint32> indices;
    
    const float height = 1.0f;
    const float radius = 0.5f;
    
    // Generate side vertices
    for (uint32 seg = 0; seg <= Segments; ++seg)
    {
        float theta = 2.0f * M_PI * float(seg) / float(Segments);
        float x = radius * cosf(theta);
        float z = radius * sinf(theta);
        
        // Top and bottom vertices for side
        vertices.push_back({ FVector(x, height * 0.5f, z), Color });
        vertices.push_back({ FVector(x, -height * 0.5f, z), Color });
    }
    
    // Generate side indices
    for (uint32 seg = 0; seg < Segments; ++seg)
    {
        uint32 top1 = seg * 2;
        uint32 bottom1 = seg * 2 + 1;
        uint32 top2 = (seg + 1) * 2;
        uint32 bottom2 = (seg + 1) * 2 + 1;
        
        // Two triangles per side quad
        indices.push_back(top1);
        indices.push_back(bottom1);
        indices.push_back(top2);
        
        indices.push_back(top2);
        indices.push_back(bottom1);
        indices.push_back(bottom2);
    }
    
    // Add top cap
    uint32 topCenterIdx = vertices.size();
    vertices.push_back({ FVector(0.0f, height * 0.5f, 0.0f), Color });
    for (uint32 seg = 0; seg < Segments; ++seg)
    {
        float theta1 = 2.0f * M_PI * float(seg) / float(Segments);
        float theta2 = 2.0f * M_PI * float(seg + 1) / float(Segments);
        
        uint32 idx1 = vertices.size();
        uint32 idx2 = vertices.size() + 1;
        
        vertices.push_back({ FVector(radius * cosf(theta1), height * 0.5f, radius * sinf(theta1)), Color });
        vertices.push_back({ FVector(radius * cosf(theta2), height * 0.5f, radius * sinf(theta2)), Color });
        
        indices.push_back(topCenterIdx);
        indices.push_back(idx1);
        indices.push_back(idx2);
    }
    
    // Add bottom cap
    uint32 bottomCenterIdx = vertices.size();
    vertices.push_back({ FVector(0.0f, -height * 0.5f, 0.0f), Color });
    for (uint32 seg = 0; seg < Segments; ++seg)
    {
        float theta1 = 2.0f * M_PI * float(seg) / float(Segments);
        float theta2 = 2.0f * M_PI * float(seg + 1) / float(Segments);
        
        uint32 idx1 = vertices.size();
        uint32 idx2 = vertices.size() + 1;
        
        vertices.push_back({ FVector(radius * cosf(theta1), -height * 0.5f, radius * sinf(theta1)), Color });
        vertices.push_back({ FVector(radius * cosf(theta2), -height * 0.5f, radius * sinf(theta2)), Color });
        
        indices.push_back(bottomCenterIdx);
        indices.push_back(idx2);
        indices.push_back(idx1);
    }
    
    FLog::Log(ELogLevel::Info, std::string("Cylinder: ") + std::to_string(vertices.size()) + 
              " vertices, " + std::to_string(indices.size()) + " indices");
    
    // Create buffers
    FRHIBuffer* vertexBuffer = RHI->CreateVertexBuffer(vertices.size() * sizeof(FVertex), vertices.data());
    FRHIBuffer* indexBuffer = RHI->CreateIndexBuffer(indices.size() * sizeof(uint32), indices.data());
    FRHIBuffer* constantBuffer = RHI->CreateConstantBuffer(sizeof(FMatrix4x4));
    FRHIPipelineState* pso = RHI->CreateGraphicsPipelineState(true);
    
    return new FPrimitiveSceneProxy(vertexBuffer, indexBuffer, constantBuffer, pso,
                                    indices.size(), g_Camera, Transform);
}

// FPlanePrimitive implementation
FPlanePrimitive::FPlanePrimitive(uint32 InSubdivisions)
    : Subdivisions(InSubdivisions)
{
}

FPlanePrimitive::~FPlanePrimitive()
{
}

void FPlanePrimitive::Tick(float DeltaTime)
{
    // Planes typically don't auto-rotate
}

FPrimitiveSceneProxy* FPlanePrimitive::CreateSceneProxy(FRHI* RHI)
{
    FLog::Log(ELogLevel::Info, "Creating plane primitive proxy...");
    
    std::vector<FVertex> vertices;
    std::vector<uint32> indices;
    
    const float size = 1.0f;
    const uint32 divs = Subdivisions + 1;
    
    // Generate grid vertices
    for (uint32 z = 0; z <= Subdivisions; ++z)
    {
        for (uint32 x = 0; x <= Subdivisions; ++x)
        {
            float px = (float(x) / float(Subdivisions) - 0.5f) * size;
            float pz = (float(z) / float(Subdivisions) - 0.5f) * size;
            
            vertices.push_back({ FVector(px, 0.0f, pz), Color });
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
            
            // Two triangles per quad
            indices.push_back(topLeft);
            indices.push_back(bottomLeft);
            indices.push_back(topRight);
            
            indices.push_back(topRight);
            indices.push_back(bottomLeft);
            indices.push_back(bottomRight);
        }
    }
    
    FLog::Log(ELogLevel::Info, std::string("Plane: ") + std::to_string(vertices.size()) + 
              " vertices, " + std::to_string(indices.size()) + " indices");
    
    // Create buffers
    FRHIBuffer* vertexBuffer = RHI->CreateVertexBuffer(vertices.size() * sizeof(FVertex), vertices.data());
    FRHIBuffer* indexBuffer = RHI->CreateIndexBuffer(indices.size() * sizeof(uint32), indices.data());
    FRHIBuffer* constantBuffer = RHI->CreateConstantBuffer(sizeof(FMatrix4x4));
    FRHIPipelineState* pso = RHI->CreateGraphicsPipelineState(true);
    
    return new FPrimitiveSceneProxy(vertexBuffer, indexBuffer, constantBuffer, pso,
                                    indices.size(), g_Camera, Transform);
}

// FGizmoPrimitive implementation - UE-style coordinate axis
FGizmoPrimitive::FGizmoPrimitive(float InAxisLength)
    : AxisLength(InAxisLength)
{
    // Gizmo stays at origin by default
}

FGizmoPrimitive::~FGizmoPrimitive()
{
}

void FGizmoPrimitive::Tick(float DeltaTime)
{
    // Gizmo doesn't animate
}

FPrimitiveSceneProxy* FGizmoPrimitive::CreateSceneProxy(FRHI* RHI)
{
    FLog::Log(ELogLevel::Info, "Creating gizmo primitive proxy...");
    
    std::vector<FVertex> vertices;
    std::vector<uint32> indices;
    
    const float arrowHeadLength = AxisLength * 0.15f;
    const float arrowHeadRadius = 0.08f;
    const float shaftRadius = 0.02f;
    const uint32 segments = 12;
    
    // UE5 coordinate system colors:
    // X axis = Red, Y axis = Green, Z axis = Blue
    FColor xColor(1.0f, 0.2f, 0.2f, 1.0f);  // Red for X
    FColor yColor(0.2f, 1.0f, 0.2f, 1.0f);  // Green for Y
    FColor zColor(0.3f, 0.5f, 1.0f, 1.0f);  // Blue for Z
    
    constexpr float EPSILON = 0.0001f;
    
    // Helper lambda to add an axis (shaft + arrow head)
    auto AddAxis = [&](const FVector& direction, const FColor& color) {
        uint32 baseIdx = vertices.size();
        
        // Normalize direction (guard against zero-length)
        float length = std::sqrt(direction.X * direction.X + direction.Y * direction.Y + direction.Z * direction.Z);
        if (length < EPSILON) return;  // Skip invalid direction
        FVector dir(direction.X / length, direction.Y / length, direction.Z / length);
        
        // Find perpendicular vectors for the cylinder cross-section
        FVector perp1, perp2;
        if (std::abs(dir.Y) < 0.9f) {
            // Cross with Y axis
            perp1 = FVector(-dir.Z, 0.0f, dir.X);
        } else {
            // Cross with X axis
            perp1 = FVector(0.0f, dir.Z, -dir.Y);
        }
        // Normalize perp1 (guard against zero-length)
        float p1len = std::sqrt(perp1.X * perp1.X + perp1.Y * perp1.Y + perp1.Z * perp1.Z);
        if (p1len < EPSILON) return;  // Skip if perpendicular calculation failed
        perp1 = FVector(perp1.X / p1len, perp1.Y / p1len, perp1.Z / p1len);
        
        // perp2 = dir cross perp1
        perp2 = FVector(
            dir.Y * perp1.Z - dir.Z * perp1.Y,
            dir.Z * perp1.X - dir.X * perp1.Z,
            dir.X * perp1.Y - dir.Y * perp1.X
        );
        
        float shaftLength = AxisLength - arrowHeadLength;
        
        // Generate shaft cylinder
        for (uint32 i = 0; i <= segments; ++i) {
            float angle = 2.0f * M_PI * float(i) / float(segments);
            float c = cosf(angle);
            float s = sinf(angle);
            
            // Point on circle at base (origin)
            FVector base(
                shaftRadius * (c * perp1.X + s * perp2.X),
                shaftRadius * (c * perp1.Y + s * perp2.Y),
                shaftRadius * (c * perp1.Z + s * perp2.Z)
            );
            
            // Point on circle at shaft end
            FVector top(
                base.X + dir.X * shaftLength,
                base.Y + dir.Y * shaftLength,
                base.Z + dir.Z * shaftLength
            );
            
            vertices.push_back({ base, color });
            vertices.push_back({ top, color });
        }
        
        // Generate shaft indices
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
        
        // Arrow head (cone)
        uint32 coneBaseIdx = vertices.size();
        FVector coneBase(dir.X * shaftLength, dir.Y * shaftLength, dir.Z * shaftLength);
        FVector coneTip(dir.X * AxisLength, dir.Y * AxisLength, dir.Z * AxisLength);
        
        // Cone tip vertex
        uint32 tipIdx = vertices.size();
        vertices.push_back({ coneTip, color });
        
        // Cone base vertices
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
        
        // Cone side indices
        for (uint32 i = 0; i < segments; ++i) {
            indices.push_back(tipIdx);
            indices.push_back(tipIdx + 1 + i);
            indices.push_back(tipIdx + 1 + i + 1);
        }
        
        // Cone base cap
        uint32 baseCenterIdx = vertices.size();
        vertices.push_back({ coneBase, color });
        for (uint32 i = 0; i < segments; ++i) {
            indices.push_back(baseCenterIdx);
            indices.push_back(tipIdx + 1 + i + 1);
            indices.push_back(tipIdx + 1 + i);
        }
    };
    
    // Add the three axes
    AddAxis(FVector(1.0f, 0.0f, 0.0f), xColor);  // X axis - Red
    AddAxis(FVector(0.0f, 1.0f, 0.0f), yColor);  // Y axis - Green
    AddAxis(FVector(0.0f, 0.0f, 1.0f), zColor);  // Z axis - Blue
    
    FLog::Log(ELogLevel::Info, std::string("Gizmo: ") + std::to_string(vertices.size()) + 
              " vertices, " + std::to_string(indices.size()) + " indices");
    
    // Create buffers
    FRHIBuffer* vertexBuffer = RHI->CreateVertexBuffer(vertices.size() * sizeof(FVertex), vertices.data());
    FRHIBuffer* indexBuffer = RHI->CreateIndexBuffer(indices.size() * sizeof(uint32), indices.data());
    FRHIBuffer* constantBuffer = RHI->CreateConstantBuffer(sizeof(FMatrix4x4));
    FRHIPipelineState* pso = RHI->CreateGraphicsPipelineState(true);
    
    return new FPrimitiveSceneProxy(vertexBuffer, indexBuffer, constantBuffer, pso,
                                    indices.size(), g_Camera, Transform);
}

// FDemoCubePrimitive implementation - cube with configurable animation
FDemoCubePrimitive::FDemoCubePrimitive()
    : AnimationType(EAnimationType::None)
    , AnimationSpeed(1.0f)
    , AnimationTime(0.0f)
    , BasePosition(0.0f, 0.0f, 0.0f)
    , BaseScale(1.0f, 1.0f, 1.0f)
{
}

FDemoCubePrimitive::~FDemoCubePrimitive()
{
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

FPrimitiveSceneProxy* FDemoCubePrimitive::CreateSceneProxy(FRHI* RHI)
{
    FLog::Log(ELogLevel::Info, "Creating demo cube primitive proxy...");
    
    // Define cube vertices (24 vertices, 4 per face for proper normals)
    std::vector<FVertex> vertices = {
        // Front face (+Z)
        { FVector(-0.5f, -0.5f,  0.5f), Color },
        { FVector( 0.5f, -0.5f,  0.5f), Color },
        { FVector( 0.5f,  0.5f,  0.5f), Color },
        { FVector(-0.5f,  0.5f,  0.5f), Color },
        
        // Back face (-Z)
        { FVector(-0.5f, -0.5f, -0.5f), Color },
        { FVector( 0.5f, -0.5f, -0.5f), Color },
        { FVector( 0.5f,  0.5f, -0.5f), Color },
        { FVector(-0.5f,  0.5f, -0.5f), Color },
        
        // Top face (+Y)
        { FVector(-0.5f,  0.5f,  0.5f), Color },
        { FVector( 0.5f,  0.5f,  0.5f), Color },
        { FVector( 0.5f,  0.5f, -0.5f), Color },
        { FVector(-0.5f,  0.5f, -0.5f), Color },
        
        // Bottom face (-Y)
        { FVector(-0.5f, -0.5f,  0.5f), Color },
        { FVector( 0.5f, -0.5f,  0.5f), Color },
        { FVector( 0.5f, -0.5f, -0.5f), Color },
        { FVector(-0.5f, -0.5f, -0.5f), Color },
        
        // Right face (+X)
        { FVector( 0.5f, -0.5f,  0.5f), Color },
        { FVector( 0.5f, -0.5f, -0.5f), Color },
        { FVector( 0.5f,  0.5f, -0.5f), Color },
        { FVector( 0.5f,  0.5f,  0.5f), Color },
        
        // Left face (-X)
        { FVector(-0.5f, -0.5f,  0.5f), Color },
        { FVector(-0.5f, -0.5f, -0.5f), Color },
        { FVector(-0.5f,  0.5f, -0.5f), Color },
        { FVector(-0.5f,  0.5f,  0.5f), Color },
    };
    
    // Define indices (6 faces * 2 triangles * 3 indices = 36)
    std::vector<uint32> indices = {
        // Front
        0, 1, 2,    0, 2, 3,
        // Back
        5, 4, 7,    5, 7, 6,
        // Top
        8, 9, 10,   8, 10, 11,
        // Bottom
        15, 14, 13, 15, 13, 12,
        // Right
        16, 17, 18, 16, 18, 19,
        // Left
        21, 20, 23, 21, 23, 22
    };
    
    // Create buffers
    FRHIBuffer* vertexBuffer = RHI->CreateVertexBuffer(vertices.size() * sizeof(FVertex), vertices.data());
    FRHIBuffer* indexBuffer = RHI->CreateIndexBuffer(indices.size() * sizeof(uint32), indices.data());
    FRHIBuffer* constantBuffer = RHI->CreateConstantBuffer(sizeof(FMatrix4x4));
    FRHIPipelineState* pso = RHI->CreateGraphicsPipelineState(true);
    
    return new FPrimitiveSceneProxy(vertexBuffer, indexBuffer, constantBuffer, pso, 
                                    indices.size(), g_Camera, Transform);
}
