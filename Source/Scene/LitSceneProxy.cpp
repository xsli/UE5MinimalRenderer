#include "LitSceneProxy.h"
#include "ScenePrimitive.h"
#include <cstring>
#include <cmath>

// FPrimitiveSceneProxy implementation (lit rendering with Phong shading)
FPrimitiveSceneProxy::FPrimitiveSceneProxy(
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
    FRHI* InRHI)
    : VertexBuffer(InVertexBuffer)
    , IndexBuffer(InIndexBuffer)
    , MVPConstantBuffer(InMVPConstantBuffer)
    , LightingConstantBuffer(InLightingConstantBuffer)
    , ShadowConstantBuffer(nullptr)
    , PipelineState(InPSO)
    , IndexCount(InIndexCount)
    , Camera(InCamera)
    , ModelMatrix(InTransform.GetMatrix())
    , LightScene(InLightScene)
    , Material(InMaterial)
    , RHI(InRHI)
{
    // Create shadow constant buffer if RHI is available
    if (RHI)
    {
        ShadowConstantBuffer = RHI->CreateConstantBuffer(256);  // 256-byte aligned
        // Enable shadows by default for directional light
        ShadowData.SetEnabled(true);
        ShadowData.SetStrength(0.5f);  // 50% shadow strength for visible effect
    }
}

FPrimitiveSceneProxy::~FPrimitiveSceneProxy()
{
    delete VertexBuffer;
    delete IndexBuffer;
    delete MVPConstantBuffer;
    delete LightingConstantBuffer;
    if (ShadowConstantBuffer)
    {
        delete ShadowConstantBuffer;
    }
    delete PipelineState;
}

void FPrimitiveSceneProxy::UpdateLightingConstants()
{
    // Set model matrix
    LightingData.SetModelMatrix(ModelMatrix);
    
    // Set camera position
    LightingData.SetCameraPosition(Camera->GetPosition());
    
    // Set material
    LightingData.SetMaterial(Material);
    
    // Set lights from light scene
    if (LightScene)
    {
        LightingData.SetAmbientLight(LightScene->GetAmbientLight());
        
        // Get directional lights (use first one)
        auto dirLights = LightScene->GetDirectionalLights();
        if (!dirLights.empty())
        {
            LightingData.SetDirectionalLight(dirLights[0]);
        }
        else
        {
            LightingData.SetDirectionalLight(nullptr);
        }
        
        // Get point lights (up to 4)
        auto pointLights = LightScene->GetPointLights();
        for (int i = 0; i < 4; ++i)
        {
            if (i < static_cast<int>(pointLights.size()))
            {
                LightingData.SetPointLight(i, pointLights[i]);
            }
            else
            {
                LightingData.SetPointLight(i, nullptr);
            }
        }
    }
}

void FPrimitiveSceneProxy::UpdateShadowConstants()
{
    // Update shadow matrix from light scene
    if (LightScene)
    {
        auto dirLights = LightScene->GetDirectionalLights();
        if (!dirLights.empty() && dirLights[0]->IsEnabled())
        {
            // Calculate a simple directional light view-projection matrix
            FVector lightDir = dirLights[0]->GetDirection();
            
            // Normalize and create light view
            DirectX::XMVECTOR lightDirVec = DirectX::XMVector3Normalize(
                DirectX::XMVectorSet(lightDir.X, lightDir.Y, lightDir.Z, 0.0f));
            
            // Light position far behind scene
            DirectX::XMVECTOR lightPos = DirectX::XMVectorScale(
                DirectX::XMVectorNegate(lightDirVec), 50.0f);
            
            // Up vector
            DirectX::XMVECTOR upVec = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
            float dotUp = std::abs(DirectX::XMVectorGetX(DirectX::XMVector3Dot(lightDirVec, upVec)));
            if (dotUp > 0.99f)
            {
                upVec = DirectX::XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
            }
            
            // View and projection
            DirectX::XMMATRIX viewMatrix = DirectX::XMMatrixLookAtLH(
                lightPos, 
                DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f), 
                upVec);
            DirectX::XMMATRIX projMatrix = DirectX::XMMatrixOrthographicLH(40.0f, 40.0f, 0.1f, 100.0f);
            
            // Transpose for HLSL
            ShadowData.DirLightViewProj = DirectX::XMMatrixTranspose(
                DirectX::XMMatrixMultiply(viewMatrix, projMatrix));
            ShadowData.SetEnabled(true);
        }
        else
        {
            ShadowData.SetEnabled(false);
        }
    }
}

void FPrimitiveSceneProxy::Render(FRHICommandList* RHICmdList)
{
    // Calculate MVP matrix
    FMatrix4x4 viewProjection = Camera->GetViewProjectionMatrix();
    FMatrix4x4 mvp = ModelMatrix * viewProjection;
    
    // Transpose for HLSL (column-major)
    FMatrix4x4 mvpTransposed = mvp.Transpose();
    
    // Update MVP constant buffer
    void* mvpData = MVPConstantBuffer->Map();
    memcpy(mvpData, &mvpTransposed.Matrix, sizeof(DirectX::XMMATRIX));
    MVPConstantBuffer->Unmap();
    
    // Update lighting constant buffer
    UpdateLightingConstants();
    void* lightData = LightingConstantBuffer->Map();
    memcpy(lightData, &LightingData, sizeof(FLightingConstants));
    LightingConstantBuffer->Unmap();
    
    // Update shadow constant buffer
    if (ShadowConstantBuffer)
    {
        UpdateShadowConstants();
        void* shadowData = ShadowConstantBuffer->Map();
        memcpy(shadowData, &ShadowData, sizeof(FShadowRenderConstants));
        ShadowConstantBuffer->Unmap();
    }
    
    // Set render state and draw
    RHICmdList->SetPipelineState(PipelineState);
    RHICmdList->SetConstantBuffer(MVPConstantBuffer, 0);      // b0 = MVP
    RHICmdList->SetConstantBuffer(LightingConstantBuffer, 1); // b1 = Lighting
    if (ShadowConstantBuffer)
    {
        RHICmdList->SetConstantBuffer(ShadowConstantBuffer, 2);  // b2 = Shadow
    }
    RHICmdList->SetVertexBuffer(VertexBuffer, 0, sizeof(FLitVertex));
    RHICmdList->SetIndexBuffer(IndexBuffer);
    RHICmdList->DrawIndexedPrimitive(IndexCount, 0, 0);
}

void FPrimitiveSceneProxy::RenderShadow(FRHICommandList* RHICmdList, const FMatrix4x4& LightViewProj, FRHIBuffer* ShadowMVPBuffer)
{
    // IMPORTANT: Always use the proxy's own MVPConstantBuffer for shadow rendering.
    // Using a shared buffer would cause all proxies to overwrite each other's data,
    // resulting in only the last proxy's matrix being visible to the GPU.
    // The caller is responsible for flushing commands after shadow pass so that
    // GPU reads this data before main pass overwrites the buffer.
    (void)ShadowMVPBuffer;  // Unused - kept for API compatibility
    
    // Calculate shadow MVP matrix: Model * LightViewProj
    FMatrix4x4 shadowMVP = ModelMatrix * LightViewProj;
    
    // Transpose for HLSL (column-major)
    FMatrix4x4 shadowMVPTransposed = shadowMVP.Transpose();
    
    // Update MVP constant buffer with shadow matrix
    void* mvpData = MVPConstantBuffer->Map();
    memcpy(mvpData, &shadowMVPTransposed.Matrix, sizeof(DirectX::XMMATRIX));
    MVPConstantBuffer->Unmap();
    
    // Set render state and draw (PSO should be shadow/depth-only PSO)
    // Note: Pipeline state is already set by the shadow pass
    RHICmdList->SetConstantBuffer(MVPConstantBuffer, 0);      // b0 = MVP
    RHICmdList->SetVertexBuffer(VertexBuffer, 0, sizeof(FLitVertex));
    RHICmdList->SetIndexBuffer(IndexBuffer);
    RHICmdList->DrawIndexedPrimitive(IndexCount, 0, 0);
}

uint32 FPrimitiveSceneProxy::GetTriangleCount() const
{
    return IndexCount / 3;
}

void FPrimitiveSceneProxy::UpdateTransform(const FTransform& InTransform)
{
    ModelMatrix = InTransform.GetMatrix();
}

void FPrimitiveSceneProxy::SetShadowMatrix(const FMatrix4x4& LightViewProj)
{
    ShadowData.DirLightViewProj = DirectX::XMMatrixTranspose(LightViewProj.Matrix);
}

void FPrimitiveSceneProxy::SetShadowEnabled(bool bEnabled)
{
    ShadowData.SetEnabled(bEnabled);
}

void FPrimitiveSceneProxy::SetShadowBias(float Bias)
{
    ShadowData.SetBias(Bias);
}

void FPrimitiveSceneProxy::SetShadowStrength(float Strength)
{
    ShadowData.SetStrength(Strength);
}

// FLightVisualizationProxy implementation
FLightVisualizationProxy::FLightVisualizationProxy(
    FRHIBuffer* InVertexBuffer,
    FRHIBuffer* InIndexBuffer,
    FRHIBuffer* InConstantBuffer,
    FRHIPipelineState* InPSO,
    uint32 InIndexCount,
    FCamera* InCamera,
    const FVector& InPosition,
    bool bIsLineList)
    : VertexBuffer(InVertexBuffer)
    , IndexBuffer(InIndexBuffer)
    , ConstantBuffer(InConstantBuffer)
    , PipelineState(InPSO)
    , IndexCount(InIndexCount)
    , Camera(InCamera)
    , Position(InPosition)
    , bLineList(bIsLineList)
{
}

FLightVisualizationProxy::~FLightVisualizationProxy()
{
    delete VertexBuffer;
    delete IndexBuffer;
    delete ConstantBuffer;
    delete PipelineState;
}

void FLightVisualizationProxy::Render(FRHICommandList* RHICmdList)
{
    // Calculate MVP matrix with position as translation
    FMatrix4x4 modelMatrix = FMatrix4x4::Translation(Position.X, Position.Y, Position.Z);
    FMatrix4x4 viewProjection = Camera->GetViewProjectionMatrix();
    FMatrix4x4 mvp = modelMatrix * viewProjection;
    
    // Transpose for HLSL
    FMatrix4x4 mvpTransposed = mvp.Transpose();
    
    // Update constant buffer
    void* cbData = ConstantBuffer->Map();
    memcpy(cbData, &mvpTransposed.Matrix, sizeof(DirectX::XMMATRIX));
    ConstantBuffer->Unmap();
    
    // Set render state and draw
    RHICmdList->SetPipelineState(PipelineState);
    RHICmdList->SetConstantBuffer(ConstantBuffer, 0);
    RHICmdList->SetVertexBuffer(VertexBuffer, 0, sizeof(FVertex));
    RHICmdList->SetIndexBuffer(IndexBuffer);
    
    if (bLineList)
    {
        RHICmdList->DrawIndexedLines(IndexCount, 0, 0);
    }
    else
    {
        RHICmdList->DrawIndexedPrimitive(IndexCount, 0, 0);
    }
}

uint32 FLightVisualizationProxy::GetTriangleCount() const
{
    // Line lists don't contribute to triangle count since they're debug visualization
    // For triangle mode, calculate normally
    return bLineList ? 0 : IndexCount / 3;
}

void FLightVisualizationProxy::UpdatePosition(const FVector& InPosition)
{
    Position = InPosition;
}
