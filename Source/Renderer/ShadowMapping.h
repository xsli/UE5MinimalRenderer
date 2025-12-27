#pragma once

#include "../Core/CoreTypes.h"
#include "../RHI/RHI.h"
#include "../Renderer/RTPool.h"
#include "../Lighting/Light.h"
#include <DirectXMath.h>
#include <vector>

// Forward declarations
class FCamera;
class FSceneProxy;
class FRenderScene;

/**
 * Shadow Sampling HLSL Reference (for shader implementation):
 * 
 * PCF 3x3 Kernel for smooth shadows:
 * 
 * float CalcDirectionalShadowPCF(float3 worldPos, float4x4 lightViewProj, 
 *                                 Texture2D shadowMap, SamplerComparisonState shadowSampler,
 *                                 float2 shadowMapSize, float bias)
 * {
 *     // Transform world position to light clip space
 *     float4 lightSpacePos = mul(float4(worldPos, 1.0f), lightViewProj);
 *     lightSpacePos.xyz /= lightSpacePos.w;  // Perspective divide
 *     
 *     // Convert from clip space [-1,1] to texture space [0,1]
 *     float2 shadowUV = lightSpacePos.xy * 0.5f + 0.5f;
 *     shadowUV.y = 1.0f - shadowUV.y;  // Flip Y for DirectX
 *     
 *     // Apply bias
 *     float depth = lightSpacePos.z - bias;
 *     
 *     // 3x3 PCF kernel
 *     float shadow = 0.0f;
 *     float2 texelSize = 1.0f / shadowMapSize;
 *     for (int y = -1; y <= 1; y++)
 *     {
 *         for (int x = -1; x <= 1; x++)
 *         {
 *             float2 offset = float2(x, y) * texelSize;
 *             shadow += shadowMap.SampleCmpLevelZero(shadowSampler, shadowUV + offset, depth);
 *         }
 *     }
 *     return shadow / 9.0f;
 * }
 * 
 * For point light shadows, determine which cubemap face to sample based on direction,
 * then apply similar PCF sampling to the appropriate atlas region.
 */

/**
 * Shadow map constants - GPU constant buffer data for shadow sampling
 * Must match the HLSL ShadowBuffer structure exactly
 */
struct FShadowConstants
{
    // Directional light shadow matrix (world -> light clip space)
    DirectX::XMMATRIX DirLightViewProj;        // 64 bytes
    
    // Point light shadow matrices (6 faces for omnidirectional)
    // Packed into atlas: each face is at different offset
    DirectX::XMMATRIX PointLight0ViewProj[6];  // 384 bytes
    DirectX::XMMATRIX PointLight1ViewProj[6];  // 384 bytes
    
    // Shadow map parameters
    DirectX::XMFLOAT4 ShadowParams;            // 16 bytes
    // x = constant bias, y = slope-scaled bias, z = PCF radius, w = shadow strength
    
    // Directional shadow map info
    DirectX::XMFLOAT4 DirShadowInfo;           // 16 bytes
    // x = enabled, y = map size, z = near, w = far
    
    // Point light shadow info
    DirectX::XMFLOAT4 PointLight0ShadowInfo;   // 16 bytes
    DirectX::XMFLOAT4 PointLight1ShadowInfo;   // 16 bytes
    // x = enabled, y = map size, z = near, w = far
    
    // Point light atlas offsets (for packed shadow atlas)
    DirectX::XMFLOAT4 PointLight0AtlasOffsets[6]; // 96 bytes - UV offset for each face
    DirectX::XMFLOAT4 PointLight1AtlasOffsets[6]; // 96 bytes
    
    // Total: ~1088 bytes, aligned to 256 bytes = 1280 bytes
    
    FShadowConstants()
    {
        Clear();
    }
    
    void Clear()
    {
        DirLightViewProj = DirectX::XMMatrixIdentity();
        for (int i = 0; i < 6; ++i)
        {
            PointLight0ViewProj[i] = DirectX::XMMatrixIdentity();
            PointLight1ViewProj[i] = DirectX::XMMatrixIdentity();
            PointLight0AtlasOffsets[i] = { 0.0f, 0.0f, 0.0f, 0.0f };
            PointLight1AtlasOffsets[i] = { 0.0f, 0.0f, 0.0f, 0.0f };
        }
        ShadowParams = { 0.001f, 0.005f, 1.0f, 1.0f };  // Default bias values
        DirShadowInfo = { 0.0f, 1024.0f, 0.1f, 100.0f };
        PointLight0ShadowInfo = { 0.0f, 512.0f, 0.1f, 50.0f };
        PointLight1ShadowInfo = { 0.0f, 512.0f, 0.1f, 50.0f };
    }
    
    void SetShadowBias(float ConstantBias, float SlopeScaledBias)
    {
        ShadowParams.x = ConstantBias;
        ShadowParams.y = SlopeScaledBias;
    }
    
    void SetPCFRadius(float Radius)
    {
        ShadowParams.z = Radius;
    }
    
    void SetShadowStrength(float Strength)
    {
        ShadowParams.w = Strength;
    }
};

/**
 * FShadowMapPass - Manages shadow map generation for a single light
 */
class FShadowMapPass
{
public:
    FShadowMapPass();
    ~FShadowMapPass();
    
    // Initialize for directional light (orthographic projection)
    void InitializeDirectional(FRHI* RHI, uint32 MapSize = 1024);
    
    // Initialize for point light (6-face cubemap as atlas)
    void InitializePointLight(FRHI* RHI, uint32 FaceSize = 512);
    
    // Update light matrices
    void UpdateDirectionalLight(const FDirectionalLight* Light, const FVector& SceneCenter, float SceneRadius);
    void UpdatePointLight(const FPointLight* Light);
    
    // Get view-projection matrix for rendering
    FMatrix4x4 GetViewProjectionMatrix(uint32 FaceIndex = 0) const;
    
    // Get shadow texture
    FRHITexture* GetShadowTexture() const { return ShadowTexture; }
    
    // Shadow bias configuration
    void SetConstantBias(float Bias) { ConstantBias = Bias; }
    void SetSlopeScaledBias(float Bias) { SlopeScaledBias = Bias; }
    float GetConstantBias() const { return ConstantBias; }
    float GetSlopeScaledBias() const { return SlopeScaledBias; }
    
    // Get atlas offset for point light face
    FVector4 GetAtlasOffset(uint32 FaceIndex) const;
    
    bool IsInitialized() const { return bInitialized; }
    bool IsDirectional() const { return bIsDirectional; }
    uint32 GetMapSize() const { return MapSize; }

private:
    void CalculateDirectionalMatrices(const FVector& LightDir, const FVector& SceneCenter, float SceneRadius);
    void CalculatePointLightMatrices(const FVector& LightPos, float Radius);
    
    FRHI* RHI;
    FRHITexture* ShadowTexture;         // Depth texture for shadow map
    FRHIPipelineState* ShadowPSO;       // Shadow pass pipeline state
    FRHIBuffer* ShadowConstantBuffer;   // MVP for shadow pass
    
    uint32 MapSize;
    bool bInitialized;
    bool bIsDirectional;
    
    // Bias values
    float ConstantBias;
    float SlopeScaledBias;
    
    // Near/far planes
    float NearPlane;
    float FarPlane;
    
    // View-projection matrices (1 for directional, 6 for point light)
    DirectX::XMMATRIX ViewProjectionMatrices[6];
    
    // Atlas layout for point light (6 faces packed into single texture)
    // Layout: 3x2 grid of 512x512 faces = 1536x1024 texture
    static constexpr uint32 ATLAS_COLS = 3;
    static constexpr uint32 ATLAS_ROWS = 2;
};

/**
 * FShadowSystem - Main shadow mapping system
 * Manages all shadow maps and coordinates shadow pass rendering
 */
class FShadowSystem
{
public:
    FShadowSystem();
    ~FShadowSystem();
    
    // Initialize shadow system
    void Initialize(FRHI* RHI);
    void Shutdown();
    
    // Update shadow maps for current frame
    void Update(FLightScene* LightScene, const FVector& SceneCenter, float SceneRadius);
    
    // Render shadow passes (call before main scene rendering)
    void RenderShadowPasses(FRHICommandList* RHICmdList, FRenderScene* Scene);
    
    // Get shadow constant buffer data (for binding to main shader)
    void GetShadowConstants(FShadowConstants& OutConstants) const;
    
    // Get shadow textures for binding
    FRHITexture* GetDirectionalShadowMap() const;
    FRHITexture* GetPointLightShadowAtlas(uint32 LightIndex) const;
    
    // Shadow quality settings
    void SetDirectionalMapSize(uint32 Size);
    void SetPointLightMapSize(uint32 Size);
    
    // Global shadow bias
    void SetConstantBias(float Bias);
    void SetSlopeScaledBias(float Bias);
    
    // Statistics
    uint32 GetShadowDrawCallCount() const { return ShadowDrawCallCount; }
    
private:
    void RenderDirectionalShadowPass(FRHICommandList* RHICmdList, FRenderScene* Scene);
    void RenderPointLightShadowPass(FRHICommandList* RHICmdList, FRenderScene* Scene, uint32 LightIndex);
    
    FRHI* RHI;
    bool bInitialized;
    
    // Shadow passes
    FShadowMapPass DirectionalShadowPass;
    FShadowMapPass PointLightShadowPasses[2];  // Support up to 2 shadowed point lights
    
    // Current light references
    FDirectionalLight* CurrentDirLight;
    FPointLight* CurrentPointLights[2];
    
    // Settings
    uint32 DirectionalMapSize;
    uint32 PointLightMapSize;
    float GlobalConstantBias;
    float GlobalSlopeScaledBias;
    
    // Statistics
    uint32 ShadowDrawCallCount;
};
