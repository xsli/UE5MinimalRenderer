#pragma once

#include "../RHI/RHI.h"
#include "../Renderer/Camera.h"
#include "../Renderer/Renderer.h"
#include "../Lighting/Light.h"
#include "../Lighting/LightingConstants.h"
#include "../Core/CoreTypes.h"
#include "../Asset/OBJLoader.h"
#include "ScenePrimitive.h"
#include "LitSceneProxy.h"

/**
 * FTexturedSceneProxy - Scene proxy for textured meshes
 * Uses FTexturedVertex format (position, normal, UV, color) with diffuse texture sampling
 */
class FTexturedSceneProxy : public FSceneProxy
{
public:
    FTexturedSceneProxy(
        FRHIBuffer* InVertexBuffer,
        FRHIBuffer* InIndexBuffer,
        FRHIBuffer* InMVPConstantBuffer,
        FRHIBuffer* InLightingConstantBuffer,
        FRHIPipelineState* InPSO,
        FRHIPipelineState* InShadowPSO,
        uint32 InIndexCount,
        FCamera* InCamera,
        const FTransform& InTransform,
        FLightScene* InLightScene,
        const FMaterial& InMaterial,
        FRHITexture* InDiffuseTexture,
        FRHI* InRHI);
    
    virtual ~FTexturedSceneProxy();
    
    // Render this proxy
    virtual void Render(FRHICommandList* RHICmdList) override;
    
    // Shadow pass rendering
    virtual void RenderShadow(FRHICommandList* RHICmdList, const FMatrix4x4& LightViewProj, FRHIBuffer* ShadowMVPBuffer = nullptr) override;
    
    // Get triangle count
    virtual uint32 GetTriangleCount() const override;
    
    // Update transform
    virtual void UpdateTransform(const FTransform& InTransform) override;
    
    // Get model matrix for shadow calculations
    virtual FMatrix4x4 GetModelMatrix() const override { return ModelMatrix; }
    
    // Update material
    void SetMaterial(const FMaterial& InMaterial) { Material = InMaterial; }
    
    // Update texture
    void SetDiffuseTexture(FRHITexture* InTexture) { DiffuseTexture = InTexture; }
    
    // Shadow settings
    void SetShadowMatrix(const FMatrix4x4& LightViewProj);
    void SetShadowEnabled(bool bEnabled);
    void SetShadowMapTexture(FRHITexture* InShadowMapTexture) { ShadowMapTexture = InShadowMapTexture; }
    
protected:
    void UpdateLightingConstants();
    void UpdateShadowConstants();
    
    FRHIBuffer* VertexBuffer;
    FRHIBuffer* IndexBuffer;
    FRHIBuffer* MVPConstantBuffer;
    FRHIBuffer* LightingConstantBuffer;
    FRHIBuffer* ShadowConstantBuffer;
    FRHIPipelineState* PipelineState;
    FRHIPipelineState* ShadowPipelineState;
    uint32 IndexCount;
    FCamera* Camera;
    FMatrix4x4 ModelMatrix;
    FLightScene* LightScene;
    FMaterial Material;
    FLightingConstants LightingData;
    FShadowRenderConstants ShadowData;
    FRHI* RHI;
    FRHITexture* DiffuseTexture;
    FRHITexture* ShadowMapTexture;
};
