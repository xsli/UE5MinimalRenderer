#include "LitSceneProxy.h"
#include "../Scene/ScenePrimitive.h"  // For FTransform implementation
#include <cstring>

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
    const FMaterial& InMaterial)
    : VertexBuffer(InVertexBuffer)
    , IndexBuffer(InIndexBuffer)
    , MVPConstantBuffer(InMVPConstantBuffer)
    , LightingConstantBuffer(InLightingConstantBuffer)
    , PipelineState(InPSO)
    , IndexCount(InIndexCount)
    , Camera(InCamera)
    , ModelMatrix(InTransform.GetMatrix())
    , LightScene(InLightScene)
    , Material(InMaterial)
{
}

FPrimitiveSceneProxy::~FPrimitiveSceneProxy()
{
    delete VertexBuffer;
    delete IndexBuffer;
    delete MVPConstantBuffer;
    delete LightingConstantBuffer;
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
    
    // Set render state and draw
    RHICmdList->SetPipelineState(PipelineState);
    RHICmdList->SetConstantBuffer(MVPConstantBuffer, 0);      // b0 = MVP
    RHICmdList->SetConstantBuffer(LightingConstantBuffer, 1); // b1 = Lighting
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
