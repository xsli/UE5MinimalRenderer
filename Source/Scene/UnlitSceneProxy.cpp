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
    const float margin = 60.0f;  // Distance from screen edge
    
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
    // The view matrix transforms world space to camera/view space
    // For a screen-space gizmo showing world axis orientation, we need to:
    // 1. Extract only the rotation part of the view matrix (3x3 upper-left)
    // 2. This rotation transforms world axes to appear as they would from camera's viewpoint
    FMatrix4x4 viewMatrix = Camera->GetViewMatrix();
    
    // Extract the 3x3 rotation part from the view matrix
    // View matrix is typically: [R | T] where R is 3x3 rotation, T is translation
    // We only want R to rotate the gizmo axes to match camera orientation
    FMatrix4x4 viewRotation;
    // Copy rotation part (rows 0,1,2, columns 0,1,2)
    viewRotation.Matrix.r[0] = DirectX::XMVectorSetW(viewMatrix.Matrix.r[0], 0.0f);
    viewRotation.Matrix.r[1] = DirectX::XMVectorSetW(viewMatrix.Matrix.r[1], 0.0f);
    viewRotation.Matrix.r[2] = DirectX::XMVectorSetW(viewMatrix.Matrix.r[2], 0.0f);
    viewRotation.Matrix.r[3] = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
    
    // Scale to fit in corner (normalize gizmo size to NDC)
    float ndcScale = GizmoSize / screenWidth * 2.0f;  // Convert pixel size to NDC
    float aspectRatio = screenWidth / screenHeight;
    
    // Create orthographic-style transformation:
    // We scale uniformly but adjust for aspect ratio in Y
    FMatrix4x4 scaleMatrix = FMatrix4x4::Scaling(ndcScale, ndcScale * aspectRatio, ndcScale);
    
    // Translation to corner position in NDC space
    // Z is set to 0.5 to be in the middle of depth range
    FMatrix4x4 translationMatrix = FMatrix4x4::Translation(ndcX, ndcY, 0.5f);
    
    // Final MVP transformation:
    // 1. viewRotation: Rotate world axes to camera view orientation
    // 2. scaleMatrix: Scale to desired size in NDC
    // 3. translationMatrix: Position in screen corner
    // 
    // Matrix multiplication order (DirectX row-major, left to right):
    // vertex * viewRotation * scaleMatrix * translationMatrix
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
