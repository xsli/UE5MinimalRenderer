#include "UnlitSceneProxy.h"
#include <cstring>

FUnlitPrimitiveSceneProxy::FUnlitPrimitiveSceneProxy(FRHIBuffer* InVertexBuffer, FRHIBuffer* InIndexBuffer,
                                                     FRHIBuffer* InConstantBuffer, FRHIPipelineState* InPSO,
                                                     uint32 InIndexCount, FCamera* InCamera, const FTransform& InTransform)
    : VertexBuffer(InVertexBuffer)
    , IndexBuffer(InIndexBuffer)
    , ConstantBuffer(InConstantBuffer)
    , PipelineState(InPSO)
    , IndexCount(InIndexCount)
    , Camera(InCamera)
    , ModelMatrix(InTransform.GetMatrix())
{
}

FUnlitPrimitiveSceneProxy::~FUnlitPrimitiveSceneProxy()
{
    delete VertexBuffer;
    delete IndexBuffer;
    delete ConstantBuffer;
    delete PipelineState;
}

void FUnlitPrimitiveSceneProxy::Render(FRHICommandList* RHICmdList)
{
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

uint32 FUnlitPrimitiveSceneProxy::GetTriangleCount() const
{
    return IndexCount / 3;
}

void FUnlitPrimitiveSceneProxy::UpdateTransform(const FTransform& InTransform)
{
    ModelMatrix = InTransform.GetMatrix();
}

// ============================================================================
// FScreenSpaceGizmoProxy - Screen-space coordinate axis gizmo
// ============================================================================

FScreenSpaceGizmoProxy::FScreenSpaceGizmoProxy(FRHIBuffer* InVertexBuffer, FRHIBuffer* InIndexBuffer,
                                               FRHIBuffer* InConstantBuffer, FRHIPipelineState* InPSO,
                                               uint32 InIndexCount, FCamera* InCamera,
                                               int InScreenCorner, float InGizmoSize)
    : VertexBuffer(InVertexBuffer)
    , IndexBuffer(InIndexBuffer)
    , ConstantBuffer(InConstantBuffer)
    , PipelineState(InPSO)
    , IndexCount(InIndexCount)
    , Camera(InCamera)
    , ScreenCorner(InScreenCorner)
    , GizmoSize(InGizmoSize)
{
}

FScreenSpaceGizmoProxy::~FScreenSpaceGizmoProxy()
{
    delete VertexBuffer;
    delete IndexBuffer;
    delete ConstantBuffer;
    delete PipelineState;
}

void FScreenSpaceGizmoProxy::Render(FRHICommandList* RHICmdList)
{
    // Screen dimensions (assume standard resolution)
    const float screenWidth = 1280.0f;
    const float screenHeight = 720.0f;
    const float margin = 80.0f;  // Distance from screen edge in pixels
    
    // Calculate screen position based on corner
    float screenX, screenY;
    switch (ScreenCorner)
    {
        case 0:  // Top-left
            screenX = margin;
            screenY = margin;
            break;
        case 1:  // Top-right
            screenX = screenWidth - margin;
            screenY = margin;
            break;
        case 2:  // Bottom-left
            screenX = margin;
            screenY = screenHeight - margin;
            break;
        case 3:  // Bottom-right (default)
        default:
            screenX = screenWidth - margin;
            screenY = screenHeight - margin;
            break;
    }
    
    // Convert screen position to NDC (-1 to 1)
    float ndcX = (screenX / screenWidth) * 2.0f - 1.0f;
    float ndcY = 1.0f - (screenY / screenHeight) * 2.0f;  // Y is flipped in NDC
    
    // Get camera view matrix
    // The view matrix transforms world space to view/camera space
    // For a screen-space gizmo, we want to show how world axes appear from the camera's view
    FMatrix4x4 viewMatrix = Camera->GetViewMatrix();
    
    // The view matrix has the form: [R | -R*T] where R is 3x3 rotation
    // We only need the rotation part to show how world axes orient relative to camera
    // In DirectX row-major format, rows 0,1,2 contain the rotation+some translation effect
    // We need to extract just the upper-left 3x3 rotation matrix
    
    // Create a rotation-only matrix from the view matrix
    // For a view matrix built from LookAt, the 3x3 portion is the camera's basis vectors
    FMatrix4x4 viewRotation;
    viewRotation.Matrix.r[0] = DirectX::XMVectorSet(
        DirectX::XMVectorGetX(viewMatrix.Matrix.r[0]),
        DirectX::XMVectorGetY(viewMatrix.Matrix.r[0]),
        DirectX::XMVectorGetZ(viewMatrix.Matrix.r[0]),
        0.0f);
    viewRotation.Matrix.r[1] = DirectX::XMVectorSet(
        DirectX::XMVectorGetX(viewMatrix.Matrix.r[1]),
        DirectX::XMVectorGetY(viewMatrix.Matrix.r[1]),
        DirectX::XMVectorGetZ(viewMatrix.Matrix.r[1]),
        0.0f);
    viewRotation.Matrix.r[2] = DirectX::XMVectorSet(
        DirectX::XMVectorGetX(viewMatrix.Matrix.r[2]),
        DirectX::XMVectorGetY(viewMatrix.Matrix.r[2]),
        DirectX::XMVectorGetZ(viewMatrix.Matrix.r[2]),
        0.0f);
    viewRotation.Matrix.r[3] = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
    
    // Scale to fit in corner
    // Convert pixel size to NDC (screen is -1 to 1, so multiply by 2/width)
    float ndcScale = GizmoSize / screenWidth * 2.0f;
    float aspectRatio = screenWidth / screenHeight;
    
    // Scale matrix - adjust Y for aspect ratio
    FMatrix4x4 scaleMatrix = FMatrix4x4::Scaling(ndcScale, ndcScale * aspectRatio, ndcScale * 0.1f);
    
    // Translation to corner position in NDC space
    // Z is set to 0.0 (front of depth range in NDC after transformation)
    FMatrix4x4 translationMatrix = FMatrix4x4::Translation(ndcX, ndcY, 0.0f);
    
    // Final transformation:
    // 1. viewRotation: Rotate gizmo to show world axes from camera's perspective
    // 2. scaleMatrix: Scale to desired size
    // 3. translationMatrix: Move to screen corner
    // Matrix order (row-major, left to right): vertex * viewRotation * scaleMatrix * translationMatrix
    FMatrix4x4 mvp = viewRotation * scaleMatrix * translationMatrix;
    
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

uint32 FScreenSpaceGizmoProxy::GetTriangleCount() const
{
    return IndexCount / 3;
}
