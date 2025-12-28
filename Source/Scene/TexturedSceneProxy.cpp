#include "TexturedSceneProxy.h"

FTexturedSceneProxy::FTexturedSceneProxy(
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
    FRHI* InRHI)
    : VertexBuffer(InVertexBuffer)
    , IndexBuffer(InIndexBuffer)
    , MVPConstantBuffer(InMVPConstantBuffer)
    , LightingConstantBuffer(InLightingConstantBuffer)
    , ShadowConstantBuffer(nullptr)
    , PipelineState(InPSO)
    , ShadowPipelineState(InShadowPSO)
    , IndexCount(InIndexCount)
    , Camera(InCamera)
    , ModelMatrix(InTransform.GetMatrix())
    , LightScene(InLightScene)
    , Material(InMaterial)
    , RHI(InRHI)
    , DiffuseTexture(InDiffuseTexture)
    , ShadowMapTexture(nullptr)
{
    // Create shadow constant buffer
    if (RHI)
    {
        ShadowConstantBuffer = RHI->CreateConstantBuffer(sizeof(FShadowRenderConstants));
    }
    
    FLog::Log(ELogLevel::Info, "FTexturedSceneProxy created - IndexCount: " + std::to_string(IndexCount));
}

FTexturedSceneProxy::~FTexturedSceneProxy()
{
    delete VertexBuffer;
    delete IndexBuffer;
    delete MVPConstantBuffer;
    delete LightingConstantBuffer;
    delete ShadowConstantBuffer;
    delete PipelineState;
    delete ShadowPipelineState;
    // Note: DiffuseTexture is managed by the primitive, not deleted here
    
    FLog::Log(ELogLevel::Info, "FTexturedSceneProxy destroyed");
}

void FTexturedSceneProxy::Render(FRHICommandList* RHICmdList)
{
    if (!Camera)
    {
        FLog::Log(ELogLevel::Error, "FTexturedSceneProxy::Render - Camera is null");
        return;
    }
    
    // Calculate MVP matrix
    FMatrix4x4 viewMatrix = Camera->GetViewMatrix();
    FMatrix4x4 projMatrix = Camera->GetProjectionMatrix();
    FMatrix4x4 mvpMatrix = ModelMatrix * viewMatrix * projMatrix;
    
    // Transpose for HLSL (column-major)
    FMatrix4x4 mvpTransposed = mvpMatrix.Transpose();
    
    // Update constant buffers
    void* mvpData = MVPConstantBuffer->Map();
    memcpy(mvpData, &mvpTransposed.Matrix, sizeof(DirectX::XMMATRIX));
    MVPConstantBuffer->Unmap();
    
    UpdateLightingConstants();
    UpdateShadowConstants();
    
    // Set pipeline state
    RHICmdList->SetPipelineState(PipelineState);
    
    // Set constant buffers
    RHICmdList->SetConstantBuffer(MVPConstantBuffer, 0);
    RHICmdList->SetConstantBuffer(LightingConstantBuffer, 1);
    RHICmdList->SetConstantBuffer(ShadowConstantBuffer, 2);
    
    // Set shadow map texture if available
    if (ShadowMapTexture)
    {
        RHICmdList->SetShadowMapTexture(ShadowMapTexture);
    }
    
    // Set diffuse texture if available
    if (DiffuseTexture)
    {
        RHICmdList->SetDiffuseTexture(DiffuseTexture);
    }
    
    // Set vertex and index buffers
    RHICmdList->SetVertexBuffer(VertexBuffer, 0, sizeof(FTexturedVertex));
    RHICmdList->SetIndexBuffer(IndexBuffer);
    
    // Draw
    RHICmdList->DrawIndexedPrimitive(IndexCount, 0, 0);
}

void FTexturedSceneProxy::RenderShadow(FRHICommandList* RHICmdList, const FMatrix4x4& LightViewProj, FRHIBuffer* ShadowMVPBuffer)
{
    if (!ShadowPipelineState)
    {
        return;
    }
    
    // Calculate light-space MVP (Model * LightViewProj)
    FMatrix4x4 shadowMVP = ModelMatrix * LightViewProj;
    FMatrix4x4 shadowMVPTransposed = shadowMVP.Transpose();
    
    // Set depth-only pipeline state
    RHICmdList->SetPipelineState(ShadowPipelineState);
    
    // Use root constants for shadow pass (avoids buffer sync issues)
    RHICmdList->SetRootConstants(0, 16, &shadowMVPTransposed.Matrix, 0);
    
    // Set vertex and index buffers
    RHICmdList->SetVertexBuffer(VertexBuffer, 0, sizeof(FTexturedVertex));
    RHICmdList->SetIndexBuffer(IndexBuffer);
    
    // Draw
    RHICmdList->DrawIndexedPrimitive(IndexCount, 0, 0);
}

uint32 FTexturedSceneProxy::GetTriangleCount() const
{
    return IndexCount / 3;
}

void FTexturedSceneProxy::UpdateTransform(const FTransform& InTransform)
{
    ModelMatrix = InTransform.GetMatrix();
}

void FTexturedSceneProxy::SetShadowMatrix(const FMatrix4x4& LightViewProj)
{
    ShadowData.DirLightViewProj = LightViewProj.Transpose().Matrix;
}

void FTexturedSceneProxy::SetShadowEnabled(bool bEnabled)
{
    ShadowData.SetEnabled(bEnabled);
}

void FTexturedSceneProxy::UpdateLightingConstants()
{
    if (!Camera || !LightScene)
    {
        return;
    }
    
    // Model matrix (transposed for HLSL)
    FMatrix4x4 modelTransposed = ModelMatrix.Transpose();
    LightingData.ModelMatrix = modelTransposed.Matrix;
    
    // Camera position
    FVector camPos = Camera->GetPosition();
    LightingData.CameraPosition = { camPos.X, camPos.Y, camPos.Z, 1.0f };
    
    // Ambient light
    FColor ambient = LightScene->GetAmbientLight();
    LightingData.AmbientLight = { ambient.R, ambient.G, ambient.B, 1.0f };
    
    // Directional lights
    auto dirLights = LightScene->GetDirectionalLights();
    if (!dirLights.empty())
    {
        FDirectionalLight* dirLight = dirLights[0];
        FVector dir = dirLight->GetDirection();
        LightingData.DirLightDirection = { dir.X, dir.Y, dir.Z, 1.0f };
        FColor color = dirLight->GetColor();
        LightingData.DirLightColor = { color.R, color.G, color.B, dirLight->GetIntensity() };
    }
    else
    {
        LightingData.DirLightDirection = { 0, -1, 0, 0 };
        LightingData.DirLightColor = { 0, 0, 0, 0 };
    }
    
    // Point lights
    auto pointLights = LightScene->GetPointLights();
    for (size_t i = 0; i < 4; ++i)
    {
        DirectX::XMFLOAT4* pos = nullptr;
        DirectX::XMFLOAT4* color = nullptr;
        DirectX::XMFLOAT4* params = nullptr;
        
        switch (i)
        {
            case 0: pos = &LightingData.PointLight0Position; color = &LightingData.PointLight0Color; params = &LightingData.PointLight0Params; break;
            case 1: pos = &LightingData.PointLight1Position; color = &LightingData.PointLight1Color; params = &LightingData.PointLight1Params; break;
            case 2: pos = &LightingData.PointLight2Position; color = &LightingData.PointLight2Color; params = &LightingData.PointLight2Params; break;
            case 3: pos = &LightingData.PointLight3Position; color = &LightingData.PointLight3Color; params = &LightingData.PointLight3Params; break;
        }
        
        if (i < pointLights.size())
        {
            FPointLight* light = pointLights[i];
            FVector p = light->GetPosition();
            *pos = { p.X, p.Y, p.Z, 1.0f };
            FColor c = light->GetColor();
            *color = { c.R, c.G, c.B, light->GetIntensity() };
            *params = { light->GetRadius(), light->GetFalloff(), 0.0f, 0.0f };
        }
        else
        {
            *pos = { 0, 0, 0, 0 };
            *color = { 0, 0, 0, 0 };
            *params = { 1.0f, 1.0f, 0.0f, 0.0f };
        }
    }
    
    // Material properties
    LightingData.MaterialDiffuse = { Material.DiffuseColor.R, Material.DiffuseColor.G, Material.DiffuseColor.B, 1.0f };
    LightingData.MaterialSpecular = { Material.SpecularColor.R, Material.SpecularColor.G, Material.SpecularColor.B, Material.Shininess };
    LightingData.MaterialAmbient = { Material.AmbientColor.R, Material.AmbientColor.G, Material.AmbientColor.B, 1.0f };
    
    // Upload to GPU
    void* data = LightingConstantBuffer->Map();
    memcpy(data, &LightingData, sizeof(FLightingConstants));
    LightingConstantBuffer->Unmap();
}

void FTexturedSceneProxy::UpdateShadowConstants()
{
    if (!ShadowConstantBuffer)
    {
        return;
    }
    
    void* data = ShadowConstantBuffer->Map();
    memcpy(data, &ShadowData, sizeof(FShadowRenderConstants));
    ShadowConstantBuffer->Unmap();
}
