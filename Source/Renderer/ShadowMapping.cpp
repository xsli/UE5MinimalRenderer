#include "ShadowMapping.h"
#include "../Scene/Scene.h"
#include "../Renderer/Renderer.h"
#include "../Renderer/RTPool.h"
#include <cstring>
#include <cmath>

// ============================================================================
// FShadowMapPass Implementation
// ============================================================================

FShadowMapPass::FShadowMapPass()
    : RHI(nullptr)
    , PooledShadowTexture(nullptr)
    , ShadowPSO(nullptr)
    , ShadowConstantBuffer(nullptr)
    , MapSize(0)
    , bInitialized(false)
    , bIsDirectional(true)
    , ConstantBias(0.001f)
    , SlopeScaledBias(0.005f)
    , NearPlane(0.1f)
    , FarPlane(100.0f)
{
    for (int i = 0; i < 6; ++i)
    {
        ViewProjectionMatrices[i] = DirectX::XMMatrixIdentity();
    }
}

FShadowMapPass::~FShadowMapPass()
{
    Shutdown();
}

void FShadowMapPass::Shutdown()
{
    // Release pooled shadow texture back to the pool
    if (PooledShadowTexture)
    {
        FRTPool* pool = FRTPool::Get();
        if (pool)
        {
            pool->Release(PooledShadowTexture);
        }
        PooledShadowTexture = nullptr;
    }
    if (ShadowPSO)
    {
        delete ShadowPSO;
        ShadowPSO = nullptr;
    }
    if (ShadowConstantBuffer)
    {
        delete ShadowConstantBuffer;
        ShadowConstantBuffer = nullptr;
    }
    bInitialized = false;
}

FRHITexture* FShadowMapPass::GetShadowTexture() const
{
    return PooledShadowTexture ? PooledShadowTexture->Texture : nullptr;
}

void FShadowMapPass::InitializeDirectional(FRHI* InRHI, uint32 InMapSize)
{
    if (!InRHI) return;
    
    RHI = InRHI;
    MapSize = InMapSize;
    bIsDirectional = true;
    
    // Fetch depth texture for shadow map from RT Pool
    FRTPool* pool = FRTPool::Get();
    if (pool)
    {
        FRTDescriptor desc(MapSize, MapSize, ERTFormat::D32_FLOAT, 1, 1, 1);
        PooledShadowTexture = pool->Fetch(desc);
    }
    
    // Create constant buffer for shadow pass MVP matrix
    ShadowConstantBuffer = RHI->CreateConstantBuffer(sizeof(DirectX::XMMATRIX));
    
    // Create shadow pass pipeline state (depth-only rendering)
    // Use DepthOnly flag for simple depth pass without color output
    ShadowPSO = RHI->CreateGraphicsPipelineStateEx(EPipelineFlags::DepthOnly);
    
    bInitialized = (PooledShadowTexture != nullptr && PooledShadowTexture->Texture != nullptr && ShadowPSO != nullptr);
    
    FLog::Log(ELogLevel::Info, "FShadowMapPass: Initialized directional shadow map " + 
              std::to_string(MapSize) + "x" + std::to_string(MapSize) + " (from RT Pool)");
}

void FShadowMapPass::InitializePointLight(FRHI* InRHI, uint32 FaceSize)
{
    if (!InRHI) return;
    
    RHI = InRHI;
    MapSize = FaceSize;
    bIsDirectional = false;
    
    // Create depth texture array for 6 cubemap faces
    // We pack them into an atlas: 3x2 grid
    uint32 atlasWidth = FaceSize * ATLAS_COLS;
    uint32 atlasHeight = FaceSize * ATLAS_ROWS;
    
    // Fetch depth texture for shadow map from RT Pool
    FRTPool* pool = FRTPool::Get();
    if (pool)
    {
        FRTDescriptor desc(atlasWidth, atlasHeight, ERTFormat::D32_FLOAT, 1, 1, 1);
        PooledShadowTexture = pool->Fetch(desc);
    }
    
    // Create constant buffer
    ShadowConstantBuffer = RHI->CreateConstantBuffer(sizeof(DirectX::XMMATRIX));
    
    // Create shadow pass pipeline state (depth-only rendering)
    // Use DepthOnly flag for simple depth pass without color output
    ShadowPSO = RHI->CreateGraphicsPipelineStateEx(EPipelineFlags::DepthOnly);
    
    bInitialized = (PooledShadowTexture != nullptr && PooledShadowTexture->Texture != nullptr && ShadowPSO != nullptr);
    
    FLog::Log(ELogLevel::Info, "FShadowMapPass: Initialized point light shadow atlas " + 
              std::to_string(atlasWidth) + "x" + std::to_string(atlasHeight) + " (from RT Pool)");
}

void FShadowMapPass::UpdateDirectionalLight(const FDirectionalLight* Light, const FVector& SceneCenter, float SceneRadius)
{
    if (!Light || !bIsDirectional) return;
    
    FVector lightDir = Light->GetDirection();
    CalculateDirectionalMatrices(lightDir, SceneCenter, SceneRadius);
}

void FShadowMapPass::UpdatePointLight(const FPointLight* Light)
{
    if (!Light || bIsDirectional) return;
    
    FVector lightPos = Light->GetPosition();
    float radius = Light->GetRadius();
    CalculatePointLightMatrices(lightPos, radius);
}

void FShadowMapPass::CalculateDirectionalMatrices(const FVector& LightDir, const FVector& SceneCenter, float SceneRadius)
{
    // Normalize light direction
    DirectX::XMVECTOR lightDirVec = DirectX::XMVector3Normalize(
        DirectX::XMVectorSet(LightDir.X, LightDir.Y, LightDir.Z, 0.0f));
    
    // Calculate light position (behind the scene, opposite to light direction)
    DirectX::XMVECTOR sceneCenterVec = DirectX::XMVectorSet(SceneCenter.X, SceneCenter.Y, SceneCenter.Z, 1.0f);
    DirectX::XMVECTOR lightPosVec = DirectX::XMVectorSubtract(sceneCenterVec, 
        DirectX::XMVectorScale(lightDirVec, SceneRadius * 2.0f));
    
    // Calculate up vector (avoid parallel to light direction)
    DirectX::XMVECTOR upVec = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    float dotUp = DirectX::XMVectorGetX(DirectX::XMVector3Dot(lightDirVec, upVec));
    if (std::abs(dotUp) > 0.99f)
    {
        upVec = DirectX::XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
    }
    
    // Create view matrix (from light's perspective looking at scene center)
    DirectX::XMMATRIX viewMatrix = DirectX::XMMatrixLookAtLH(lightPosVec, sceneCenterVec, upVec);
    
    // Create orthographic projection matrix for directional light
    float orthoSize = SceneRadius * 2.0f;
    DirectX::XMMATRIX projMatrix = DirectX::XMMatrixOrthographicLH(
        orthoSize, orthoSize, NearPlane, SceneRadius * 4.0f);
    
    // Combine view and projection
    ViewProjectionMatrices[0] = DirectX::XMMatrixMultiply(viewMatrix, projMatrix);
}

void FShadowMapPass::CalculatePointLightMatrices(const FVector& LightPos, float Radius)
{
    DirectX::XMVECTOR lightPosVec = DirectX::XMVectorSet(LightPos.X, LightPos.Y, LightPos.Z, 1.0f);
    
    // Cube map face directions and up vectors (in left-handed coordinate system)
    // Order: +X, -X, +Y, -Y, +Z, -Z
    struct CubeFace
    {
        DirectX::XMVECTOR Direction;
        DirectX::XMVECTOR Up;
    };
    
    CubeFace faces[6] = {
        { DirectX::XMVectorSet( 1.0f,  0.0f,  0.0f, 0.0f), DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f) },  // +X
        { DirectX::XMVectorSet(-1.0f,  0.0f,  0.0f, 0.0f), DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f) },  // -X
        { DirectX::XMVectorSet( 0.0f,  1.0f,  0.0f, 0.0f), DirectX::XMVectorSet(0.0f, 0.0f, -1.0f, 0.0f) }, // +Y
        { DirectX::XMVectorSet( 0.0f, -1.0f,  0.0f, 0.0f), DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f) },  // -Y
        { DirectX::XMVectorSet( 0.0f,  0.0f,  1.0f, 0.0f), DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f) },  // +Z
        { DirectX::XMVectorSet( 0.0f,  0.0f, -1.0f, 0.0f), DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f) },  // -Z
    };
    
    // 90 degree FOV perspective projection
    float fov = DirectX::XM_PIDIV2;  // 90 degrees
    DirectX::XMMATRIX projMatrix = DirectX::XMMatrixPerspectiveFovLH(fov, 1.0f, NearPlane, Radius);
    
    // Calculate view-projection for each face
    for (int i = 0; i < 6; ++i)
    {
        DirectX::XMVECTOR targetVec = DirectX::XMVectorAdd(lightPosVec, faces[i].Direction);
        DirectX::XMMATRIX viewMatrix = DirectX::XMMatrixLookAtLH(lightPosVec, targetVec, faces[i].Up);
        ViewProjectionMatrices[i] = DirectX::XMMatrixMultiply(viewMatrix, projMatrix);
    }
}

FMatrix4x4 FShadowMapPass::GetViewProjectionMatrix(uint32 FaceIndex) const
{
    if (FaceIndex >= 6) FaceIndex = 0;
    return FMatrix4x4(ViewProjectionMatrices[FaceIndex]);
}

FVector4 FShadowMapPass::GetAtlasOffset(uint32 FaceIndex) const
{
    if (bIsDirectional || FaceIndex >= 6)
    {
        return FVector4(0.0f, 0.0f, 1.0f, 1.0f);  // Full texture
    }
    
    // Calculate UV offset in atlas (3x2 grid)
    uint32 col = FaceIndex % ATLAS_COLS;
    uint32 row = FaceIndex / ATLAS_COLS;
    
    float uOffset = static_cast<float>(col) / static_cast<float>(ATLAS_COLS);
    float vOffset = static_cast<float>(row) / static_cast<float>(ATLAS_ROWS);
    float uScale = 1.0f / static_cast<float>(ATLAS_COLS);
    float vScale = 1.0f / static_cast<float>(ATLAS_ROWS);
    
    return FVector4(uOffset, vOffset, uScale, vScale);
}

// ============================================================================
// FShadowSystem Implementation
// ============================================================================

FShadowSystem::FShadowSystem()
    : RHI(nullptr)
    , bInitialized(false)
    , CurrentDirLight(nullptr)
    , DirectionalMapSize(1024)
    , PointLightMapSize(512)
    , GlobalConstantBias(0.001f)
    , GlobalSlopeScaledBias(0.005f)
    , ShadowDrawCallCount(0)
{
    CurrentPointLights[0] = nullptr;
    CurrentPointLights[1] = nullptr;
}

FShadowSystem::~FShadowSystem()
{
    Shutdown();
}

void FShadowSystem::Initialize(FRHI* InRHI)
{
    if (!InRHI) return;
    
    RHI = InRHI;
    
    // Initialize directional shadow pass
    DirectionalShadowPass.InitializeDirectional(RHI, DirectionalMapSize);
    DirectionalShadowPass.SetConstantBias(GlobalConstantBias);
    DirectionalShadowPass.SetSlopeScaledBias(GlobalSlopeScaledBias);
    
    // Initialize point light shadow passes
    for (int i = 0; i < 2; ++i)
    {
        PointLightShadowPasses[i].InitializePointLight(RHI, PointLightMapSize);
        PointLightShadowPasses[i].SetConstantBias(GlobalConstantBias);
        PointLightShadowPasses[i].SetSlopeScaledBias(GlobalSlopeScaledBias);
    }
    
    bInitialized = true;
    
    FLog::Log(ELogLevel::Info, "FShadowSystem: Initialized with dir map " + 
              std::to_string(DirectionalMapSize) + ", point light map " + std::to_string(PointLightMapSize));
}

void FShadowSystem::Shutdown()
{
    // Explicitly shutdown shadow passes to release pooled RTs
    DirectionalShadowPass.Shutdown();
    for (int i = 0; i < 2; ++i)
    {
        PointLightShadowPasses[i].Shutdown();
    }
    
    bInitialized = false;
    RHI = nullptr;
    CurrentDirLight = nullptr;
    CurrentPointLights[0] = nullptr;
    CurrentPointLights[1] = nullptr;
}

void FShadowSystem::Update(FLightScene* LightScene, const FVector& SceneCenter, float SceneRadius)
{
    if (!LightScene) return;
    
    // Get directional light
    auto dirLights = LightScene->GetDirectionalLights();
    if (!dirLights.empty() && dirLights[0]->IsEnabled())
    {
        CurrentDirLight = dirLights[0];
        DirectionalShadowPass.UpdateDirectionalLight(CurrentDirLight, SceneCenter, SceneRadius);
    }
    else
    {
        CurrentDirLight = nullptr;
    }
    
    // Get point lights (up to 2 for shadows)
    auto pointLights = LightScene->GetPointLights();
    for (int i = 0; i < 2; ++i)
    {
        if (i < static_cast<int>(pointLights.size()) && pointLights[i]->IsEnabled())
        {
            CurrentPointLights[i] = pointLights[i];
            PointLightShadowPasses[i].UpdatePointLight(CurrentPointLights[i]);
        }
        else
        {
            CurrentPointLights[i] = nullptr;
        }
    }
}

void FShadowSystem::RenderShadowPasses(FRHICommandList* RHICmdList, FRenderScene* Scene)
{
    if (!bInitialized || !RHICmdList || !Scene) return;
    
    ShadowDrawCallCount = 0;
    
    // Render directional light shadow pass
    if (CurrentDirLight && DirectionalShadowPass.IsInitialized())
    {
        RenderDirectionalShadowPass(RHICmdList, Scene);
    }
    
    // Render point light shadow passes
    for (int i = 0; i < 2; ++i)
    {
        if (CurrentPointLights[i] && PointLightShadowPasses[i].IsInitialized())
        {
            RenderPointLightShadowPass(RHICmdList, Scene, i);
        }
    }
}

void FShadowSystem::GetShadowConstants(FShadowConstants& OutConstants) const
{
    OutConstants.Clear();
    
    // Global shadow parameters
    OutConstants.SetShadowBias(GlobalConstantBias, GlobalSlopeScaledBias);
    
    // Directional light shadow
    if (CurrentDirLight && DirectionalShadowPass.IsInitialized())
    {
        OutConstants.DirLightViewProj = DirectX::XMMatrixTranspose(
            DirectionalShadowPass.GetViewProjectionMatrix().Matrix);
        OutConstants.DirShadowInfo.x = 1.0f;  // Enabled
        OutConstants.DirShadowInfo.y = static_cast<float>(DirectionalShadowPass.GetMapSize());
    }
    
    // Point light 0 shadow
    if (CurrentPointLights[0] && PointLightShadowPasses[0].IsInitialized())
    {
        for (int face = 0; face < 6; ++face)
        {
            OutConstants.PointLight0ViewProj[face] = DirectX::XMMatrixTranspose(
                PointLightShadowPasses[0].GetViewProjectionMatrix(face).Matrix);
            FVector4 offset = PointLightShadowPasses[0].GetAtlasOffset(face);
            OutConstants.PointLight0AtlasOffsets[face] = { offset.X, offset.Y, offset.Z, offset.W };
        }
        OutConstants.PointLight0ShadowInfo.x = 1.0f;  // Enabled
        OutConstants.PointLight0ShadowInfo.y = static_cast<float>(PointLightShadowPasses[0].GetMapSize());
    }
    
    // Point light 1 shadow
    if (CurrentPointLights[1] && PointLightShadowPasses[1].IsInitialized())
    {
        for (int face = 0; face < 6; ++face)
        {
            OutConstants.PointLight1ViewProj[face] = DirectX::XMMatrixTranspose(
                PointLightShadowPasses[1].GetViewProjectionMatrix(face).Matrix);
            FVector4 offset = PointLightShadowPasses[1].GetAtlasOffset(face);
            OutConstants.PointLight1AtlasOffsets[face] = { offset.X, offset.Y, offset.Z, offset.W };
        }
        OutConstants.PointLight1ShadowInfo.x = 1.0f;  // Enabled
        OutConstants.PointLight1ShadowInfo.y = static_cast<float>(PointLightShadowPasses[1].GetMapSize());
    }
}

FRHITexture* FShadowSystem::GetDirectionalShadowMap() const
{
    if (CurrentDirLight && DirectionalShadowPass.IsInitialized())
    {
        return DirectionalShadowPass.GetShadowTexture();
    }
    return nullptr;
}

FRHITexture* FShadowSystem::GetPointLightShadowAtlas(uint32 LightIndex) const
{
    if (LightIndex < 2 && CurrentPointLights[LightIndex] && PointLightShadowPasses[LightIndex].IsInitialized())
    {
        return PointLightShadowPasses[LightIndex].GetShadowTexture();
    }
    return nullptr;
}

void FShadowSystem::SetDirectionalMapSize(uint32 Size)
{
    DirectionalMapSize = Size;
    // Re-initialization would be needed if already initialized
}

void FShadowSystem::SetPointLightMapSize(uint32 Size)
{
    PointLightMapSize = Size;
}

void FShadowSystem::SetConstantBias(float Bias)
{
    GlobalConstantBias = Bias;
    DirectionalShadowPass.SetConstantBias(Bias);
    for (int i = 0; i < 2; ++i)
    {
        PointLightShadowPasses[i].SetConstantBias(Bias);
    }
}

void FShadowSystem::SetSlopeScaledBias(float Bias)
{
    GlobalSlopeScaledBias = Bias;
    DirectionalShadowPass.SetSlopeScaledBias(Bias);
    for (int i = 0; i < 2; ++i)
    {
        PointLightShadowPasses[i].SetSlopeScaledBias(Bias);
    }
}

void FShadowSystem::RenderDirectionalShadowPass(FRHICommandList* RHICmdList, FRenderScene* Scene)
{
    FRHITexture* shadowTexture = DirectionalShadowPass.GetShadowTexture();
    FRHIPipelineState* shadowPSO = DirectionalShadowPass.GetShadowPSO();
    FRHIBuffer* shadowMVPBuffer = DirectionalShadowPass.GetShadowConstantBuffer();
    
    if (!shadowTexture || !shadowPSO || !Scene)
    {
        return;
    }
    
    const auto& proxies = Scene->GetProxies();
    
    // GPU Event: Directional Shadow Pass
    RHICmdList->BeginEvent("Shadow: Directional Light");
    
    // Begin shadow pass - sets depth-only render target and clears depth buffer
    RHICmdList->BeginShadowPass(shadowTexture, 0);
    
    // Set viewport to full shadow map size
    uint32 mapSize = DirectionalShadowPass.GetMapSize();
    RHICmdList->SetViewport(0.0f, 0.0f, static_cast<float>(mapSize), static_cast<float>(mapSize));
    
    // Set shadow PSO (depth-only rendering)
    RHICmdList->SetPipelineState(shadowPSO);
    
    // Get light view-projection matrix (calculated from light direction)
    FMatrix4x4 lightVP = DirectionalShadowPass.GetViewProjectionMatrix();
    
    // Render each proxy with shadow pass (only if it casts shadows)
    // Note: Each proxy uses its own MVPConstantBuffer. Caller must flush after
    // shadow pass to ensure GPU reads shadow data before main pass overwrites it.
    for (FSceneProxy* proxy : proxies)
    {
        if (proxy && proxy->GetCastShadow())
        {
            proxy->RenderShadow(RHICmdList, lightVP, shadowMVPBuffer);
            ShadowDrawCallCount++;
        }
    }
    
    // End shadow pass - restores main render target
    RHICmdList->EndShadowPass();
    
    RHICmdList->EndEvent();  // End "Shadow: Directional Light"
}

void FShadowSystem::RenderPointLightShadowPass(FRHICommandList* RHICmdList, FRenderScene* Scene, uint32 LightIndex)
{
    if (LightIndex >= 2) return;
    
    FShadowMapPass& shadowPass = PointLightShadowPasses[LightIndex];
    FRHITexture* shadowTexture = shadowPass.GetShadowTexture();
    FRHIPipelineState* shadowPSO = shadowPass.GetShadowPSO();
    FRHIBuffer* shadowMVPBuffer = shadowPass.GetShadowConstantBuffer();
    if (!shadowTexture || !shadowPSO) return;
    
    uint32 faceSize = shadowPass.GetMapSize();
    const auto& proxies = Scene->GetProxies();
    
    // Atlas layout: 3x2 grid
    static const uint32 ATLAS_COLS = 3;
    static const uint32 ATLAS_ROWS = 2;
    
    // GPU Event: Point Light Shadow Pass
    RHICmdList->BeginEvent("Shadow: Point Light " + std::to_string(LightIndex));
    
    // Begin shadow pass for the atlas texture
    // NOTE: BeginShadowPass clears the entire depth buffer once (this is correct)
    RHICmdList->BeginShadowPass(shadowTexture, 0);
    
    // Set shadow PSO once for all faces
    RHICmdList->SetPipelineState(shadowPSO);
    
    // Face names for debugging
    static const char* faceNames[] = { "+X", "-X", "+Y", "-Y", "+Z", "-Z" };
    
    // Render each face
    for (uint32 face = 0; face < 6; ++face)
    {
        // GPU Event: Per-face rendering
        RHICmdList->BeginEvent("Face " + std::to_string(face) + " (" + faceNames[face] + ")");
        
        // Calculate face position in atlas
        uint32 col = face % ATLAS_COLS;
        uint32 row = face / ATLAS_COLS;
        float x = static_cast<float>(col * faceSize);
        float y = static_cast<float>(row * faceSize);
        
        // Set viewport to face region
        RHICmdList->SetViewport(x, y, static_cast<float>(faceSize), static_cast<float>(faceSize));
        
        // Get face view-projection matrix
        FMatrix4x4 faceVP = shadowPass.GetViewProjectionMatrix(face);
        
        // Render each proxy (only if it casts shadows)
        for (FSceneProxy* proxy : proxies)
        {
            if (proxy && proxy->GetCastShadow())
            {
                proxy->RenderShadow(RHICmdList, faceVP, shadowMVPBuffer);
                ShadowDrawCallCount++;
            }
        }
        
        RHICmdList->EndEvent();  // End face event
    }
    
    // End shadow pass
    RHICmdList->EndShadowPass();
    
    RHICmdList->EndEvent();  // End "Shadow: Point Light X"
}
