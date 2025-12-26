#include "PrimitiveSceneProxy.h"
#include "Primitive.h"
#include <cstring>

FPrimitiveSceneProxy::FPrimitiveSceneProxy(FRHIBuffer* InVertexBuffer, FRHIBuffer* InIndexBuffer,
                                           FRHIBuffer* InConstantBuffer, FRHIPipelineState* InPSO,
                                           uint32 InIndexCount, FCamera* InCamera, const FTransform& InTransform)
    : VertexBuffer(InVertexBuffer)
    , IndexBuffer(InIndexBuffer)
    , ConstantBuffer(InConstantBuffer)
    , PipelineState(InPSO)
    , IndexCount(InIndexCount)
    , Camera(InCamera)
    , ModelMatrix(InTransform.GetMatrix()) {
}

FPrimitiveSceneProxy::~FPrimitiveSceneProxy() {
    delete VertexBuffer;
    delete IndexBuffer;
    delete ConstantBuffer;
    delete PipelineState;
}

void FPrimitiveSceneProxy::Render(FRHICommandList* RHICmdList) {
    // Calculate MVP matrix
    FMatrix4x4 viewProjection = Camera->GetViewProjectionMatrix();
    FMatrix4x4 mvp = ModelMatrix * viewProjection;
    
    // Transpose for HLSL (column-major)
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
    RHICmdList->DrawIndexedPrimitive(IndexCount, 0, 0);
}

uint32 FPrimitiveSceneProxy::GetTriangleCount() const {
    return IndexCount / 3;
}

void FPrimitiveSceneProxy::UpdateTransform(const FTransform& InTransform) {
    ModelMatrix = InTransform.GetMatrix();
}
