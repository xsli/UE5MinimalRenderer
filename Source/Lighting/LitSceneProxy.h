#pragma once

#include "../RHI/RHI.h"
#include "../Renderer/Camera.h"
#include "../Renderer/Renderer.h"
#include "../Lighting/Light.h"
#include "../Lighting/LightingConstants.h"
#include "../Core/CoreTypes.h"

// Forward declaration - FTransform is defined in Scene/ScenePrimitive.h
struct FTransform;

/**
 * FLitPrimitiveSceneProxy - Scene proxy for lit primitives with Phong shading
 * Uses FLitVertex format (position, normal, color) and lighting constant buffer
 */
class FLitPrimitiveSceneProxy : public FSceneProxy 
{
public:
    FLitPrimitiveSceneProxy(
        FRHIBuffer* InVertexBuffer, 
        FRHIBuffer* InIndexBuffer, 
        FRHIBuffer* InMVPConstantBuffer,
        FRHIBuffer* InLightingConstantBuffer,
        FRHIPipelineState* InPSO,
        uint32 InIndexCount, 
        FCamera* InCamera, 
        const FTransform& InTransform,
        FLightScene* InLightScene,
        const FMaterial& InMaterial);
    
    virtual ~FLitPrimitiveSceneProxy();
    
    // Render this proxy (override from FSceneProxy)
    virtual void Render(FRHICommandList* RHICmdList) override;
    
    // Get triangle count (override from FSceneProxy)
    virtual uint32 GetTriangleCount() const override;
    
    // Update transform
    void UpdateTransform(const FTransform& InTransform);
    
    // Update material
    void SetMaterial(const FMaterial& InMaterial) { Material = InMaterial; }
    
protected:
    void UpdateLightingConstants();
    
    FRHIBuffer* VertexBuffer;
    FRHIBuffer* IndexBuffer;
    FRHIBuffer* MVPConstantBuffer;
    FRHIBuffer* LightingConstantBuffer;
    FRHIPipelineState* PipelineState;
    uint32 IndexCount;
    FCamera* Camera;
    FMatrix4x4 ModelMatrix;
    FLightScene* LightScene;
    FMaterial Material;
    FLightingConstants LightingData;
};

/**
 * FLightVisualizationProxy - Scene proxy for visualizing light sources
 * Renders wireframe representations of lights (arrows for directional, spheres for point)
 */
class FLightVisualizationProxy : public FSceneProxy
{
public:
    FLightVisualizationProxy(
        FRHIBuffer* InVertexBuffer,
        FRHIBuffer* InIndexBuffer,
        FRHIBuffer* InConstantBuffer,
        FRHIPipelineState* InPSO,
        uint32 InIndexCount,
        FCamera* InCamera,
        const FVector& InPosition,
        bool bIsLineList = true);
    
    virtual ~FLightVisualizationProxy();
    
    virtual void Render(FRHICommandList* RHICmdList) override;
    virtual uint32 GetTriangleCount() const override;
    
    void UpdatePosition(const FVector& InPosition);
    
protected:
    FRHIBuffer* VertexBuffer;
    FRHIBuffer* IndexBuffer;
    FRHIBuffer* ConstantBuffer;
    FRHIPipelineState* PipelineState;
    uint32 IndexCount;
    FCamera* Camera;
    FVector Position;
    bool bLineList;
};
