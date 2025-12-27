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
 * FShadowRenderConstants - Shadow constant buffer data
 * Used for passing shadow matrices and parameters to shader
 * Must match the HLSL ShadowBuffer structure
 */
struct FShadowRenderConstants
{
    DirectX::XMMATRIX DirLightViewProj;        // 64 bytes - Directional light view-projection
    DirectX::XMFLOAT4 ShadowParams;            // 16 bytes - x=constant bias, y=enabled, z=strength, w=slope bias
    DirectX::XMMATRIX PointLight0ViewProj[6];  // 384 bytes - 6 cubemap face matrices for point light 0
    DirectX::XMMATRIX PointLight1ViewProj[6];  // 384 bytes - 6 cubemap face matrices for point light 1
    DirectX::XMFLOAT4 PointShadowParams;       // 16 bytes - x=enabled0, y=enabled1, z=point shadow strength, w=unused
    // Total: 864 bytes, padded to 1024 for constant buffer alignment
    
    FShadowRenderConstants()
    {
        DirLightViewProj = DirectX::XMMatrixIdentity();
        ShadowParams = { 0.002f, 0.0f, 1.0f, 0.005f };  // Default: constant bias, disabled, full strength, slope bias
        for (int i = 0; i < 6; ++i)
        {
            PointLight0ViewProj[i] = DirectX::XMMatrixIdentity();
            PointLight1ViewProj[i] = DirectX::XMMatrixIdentity();
        }
        PointShadowParams = { 0.0f, 0.0f, 1.0f, 0.0f };  // Disabled by default
    }
    
    void SetEnabled(bool bEnabled)
    {
        ShadowParams.y = bEnabled ? 1.0f : 0.0f;
    }
    
    void SetBias(float Bias)
    {
        ShadowParams.x = Bias;
    }
    
    void SetSlopeBias(float SlopeBias)
    {
        ShadowParams.w = SlopeBias;
    }
    
    void SetStrength(float Strength)
    {
        ShadowParams.z = Strength;
    }
    
    void SetPointLight0Enabled(bool bEnabled)
    {
        PointShadowParams.x = bEnabled ? 1.0f : 0.0f;
    }
    
    void SetPointLight1Enabled(bool bEnabled)
    {
        PointShadowParams.y = bEnabled ? 1.0f : 0.0f;
    }
    
    void SetPointShadowStrength(float Strength)
    {
        PointShadowParams.z = Strength;
    }
};

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
        const FMaterial& InMaterial,
        FRHI* InRHI = nullptr);
    
    virtual ~FPrimitiveSceneProxy();
    
    // Render this proxy (override from FSceneProxy)
    virtual void Render(FRHICommandList* RHICmdList) override;
    
    // Shadow pass rendering - renders depth-only with light's view-projection
    // @param ShadowMVPBuffer - Separate buffer for shadow MVP to avoid GPU race condition
    virtual void RenderShadow(FRHICommandList* RHICmdList, const FMatrix4x4& LightViewProj, FRHIBuffer* ShadowMVPBuffer = nullptr) override;
    
    // Get triangle count (override from FSceneProxy)
    virtual uint32 GetTriangleCount() const override;
    
    // Update transform
    virtual void UpdateTransform(const FTransform& InTransform) override;
    
    // Get model matrix for shadow calculations
    virtual FMatrix4x4 GetModelMatrix() const override { return ModelMatrix; }
    
    // Update material
    void SetMaterial(const FMaterial& InMaterial) { Material = InMaterial; }
    
    // Update shadow constants from shadow system
    void SetShadowMatrix(const FMatrix4x4& LightViewProj);
    void SetShadowEnabled(bool bEnabled);
    void SetShadowBias(float Bias);
    void SetShadowStrength(float Strength);
    
    // Set shadow map texture for shader sampling (called before rendering)
    void SetShadowMapTexture(FRHITexture* InShadowMapTexture) { ShadowMapTexture = InShadowMapTexture; }
    
protected:
    void UpdateLightingConstants();
    void UpdateShadowConstants();
    
    FRHIBuffer* VertexBuffer;
    FRHIBuffer* IndexBuffer;
    FRHIBuffer* MVPConstantBuffer;
    FRHIBuffer* LightingConstantBuffer;
    FRHIBuffer* ShadowConstantBuffer;  // NEW: Shadow constant buffer
    FRHIPipelineState* PipelineState;
    uint32 IndexCount;
    FCamera* Camera;
    FMatrix4x4 ModelMatrix;
    FLightScene* LightScene;
    FMaterial Material;
    FLightingConstants LightingData;
    FShadowRenderConstants ShadowData;  // NEW: Shadow data
    FRHI* RHI;  // NEW: RHI reference for creating shadow buffer
    FRHITexture* ShadowMapTexture;  // Shadow map texture for shader sampling
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
