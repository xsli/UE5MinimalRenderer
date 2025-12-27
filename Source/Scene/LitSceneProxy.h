#pragma once

#include "../RHI/RHI.h"
#include "../Renderer/Camera.h"
#include "../Renderer/Renderer.h"
#include "../Lighting/Light.h"
#include "../Lighting/LightingConstants.h"
#include "../Core/CoreTypes.h"
#include "ScenePrimitive.h"

// FTransform is defined in ScenePrimitive.h (included above)

/**
 * FPrimitiveSceneProxy - Default scene proxy for lit primitives with Phong shading
 * Uses FLitVertex format (position, normal, color) and lighting constant buffer
 * This is the primary proxy type for scene rendering with lighting support
 */
class FPrimitiveSceneProxy : public FSceneProxy 
{
public:
    FPrimitiveSceneProxy(
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
    
    virtual ~FPrimitiveSceneProxy();
    
    // Render this proxy (override from FSceneProxy)
    virtual void Render(FRHICommandList* RHICmdList) override;
    
    // Shadow pass rendering - renders depth-only with light's view-projection
    virtual void RenderShadow(FRHICommandList* RHICmdList, const FMatrix4x4& LightViewProj) override;
    
    // Get triangle count (override from FSceneProxy)
    virtual uint32 GetTriangleCount() const override;
    
    // Update transform
    virtual void UpdateTransform(const FTransform& InTransform) override;
    
    // Get model matrix for shadow calculations
    virtual FMatrix4x4 GetModelMatrix() const override { return ModelMatrix; }
    
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
