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
    , ShadowMapTexture(nullptr)
{
    // Create shadow constant buffer if RHI is available
    if (RHI)
    {
        // Larger buffer for extended shadow data (point light matrices, etc.)
        ShadowConstantBuffer = RHI->CreateConstantBuffer(1024);  // 1024-byte aligned for ~864 bytes of data
        // Enable shadows by default for directional light
        ShadowData.SetEnabled(true);
        ShadowData.SetStrength(0.8f);  // 80% shadow strength
        ShadowData.SetBias(0.0005f);   // Constant bias
        ShadowData.SetSlopeBias(0.002f);  // Slope-scaled bias
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
    // Bind shadow map texture AFTER pipeline state is set (root signature must be active)
    if (ShadowMapTexture)
    {
        RHICmdList->SetShadowMapTexture(ShadowMapTexture);
    }
    RHICmdList->SetVertexBuffer(VertexBuffer, 0, sizeof(FLitVertex));
    RHICmdList->SetIndexBuffer(IndexBuffer);
    RHICmdList->DrawIndexedPrimitive(IndexCount, 0, 0);
}

void FPrimitiveSceneProxy::RenderShadow(FRHICommandList* RHICmdList, const FMatrix4x4& LightViewProj, FRHIBuffer* ShadowMVPBuffer)
{
    // IMPORTANT: Use SetRootConstants for shadow pass MVP matrix
    // This copies the matrix data at command record time, avoiding the constant buffer
    // synchronization issue where all draws would see the same (last) matrix value.
    (void)ShadowMVPBuffer;  // Unused - using root constants instead
    
    // Calculate shadow MVP matrix: Model * LightViewProj
    FMatrix4x4 shadowMVP = ModelMatrix * LightViewProj;
    
    // Transpose for HLSL (column-major)
    FMatrix4x4 shadowMVPTransposed = shadowMVP.Transpose();
    
    // Set MVP using root constants - this copies data at record time!
    // 16 floats = 16 DWORDs = sizeof(XMMATRIX) / sizeof(float)
    RHICmdList->SetRootConstants(0, 16, &shadowMVPTransposed.Matrix, 0);
    
    // Set vertex and index buffers, then draw
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
