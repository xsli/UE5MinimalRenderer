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
    const float margin = 100.0f;  // Distance from screen edge in pixels
    
    // Calculate screen position based on corner (in NDC space)
    float ndcX, ndcY;
    switch (ScreenCorner)
    {
        case 0:  // Top-left
            ndcX = -1.0f + (margin / screenWidth) * 2.0f;
            ndcY = 1.0f - (margin / screenHeight) * 2.0f;
            break;
        case 1:  // Top-right
            ndcX = 1.0f - (margin / screenWidth) * 2.0f;
            ndcY = 1.0f - (margin / screenHeight) * 2.0f;
            break;
        case 2:  // Bottom-left (default)
            ndcX = -1.0f + (margin / screenWidth) * 2.0f;
            ndcY = -1.0f + (margin / screenHeight) * 2.0f;
            break;
        case 3:  // Bottom-right
        default:
            ndcX = 1.0f - (margin / screenWidth) * 2.0f;
            ndcY = -1.0f + (margin / screenHeight) * 2.0f;
            break;
    }
    
    // Step 1: Get the camera view matrix and zero out translation
    // This gives us just the rotation part - how the camera is oriented
    FMatrix4x4 viewMatrix = Camera->GetViewMatrix();
    
    // Zero out translation (row 3 in row-major format contains translation)
    // View matrix format: [R11 R12 R13 0]  row 0
    //                     [R21 R22 R23 0]  row 1
    //                     [R31 R32 R33 0]  row 2
    //                     [Tx  Ty  Tz  1]  row 3 <- translation
    viewMatrix.Matrix.r[3] = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
    
    // Step 2: Create orthographic projection for screen-space rendering
    // The gizmo size of 1.0 unit maps to GizmoSize pixels
    float aspectRatio = screenWidth / screenHeight;
    float orthoSize = 1.5f;  // The gizmo geometry goes from 0 to ~1 unit
    
    // Create orthographic projection matrix
    // Left-handed DirectX convention: XMMatrixOrthographicLH
    FMatrix4x4 projMatrix;
    projMatrix.Matrix = DirectX::XMMatrixOrthographicLH(
        orthoSize * 2.0f,                    // Width in world units
        orthoSize * 2.0f / aspectRatio,      // Height (adjusted for aspect ratio)
        0.1f,                                // Near plane
        100.0f                               // Far plane
    );
    
    // Step 3: Create model matrix with scale and translation to corner
    // Scale to fit desired pixel size
    float scale = GizmoSize / screenWidth * orthoSize * 2.0f;
    FMatrix4x4 modelMatrix = FMatrix4x4::Scaling(scale, scale, scale);
    
    // Step 4: Build final MVP matrix
    // MVP = Model * View * Projection (row-major order)
    FMatrix4x4 mv = modelMatrix * viewMatrix;
    FMatrix4x4 mvp = mv * projMatrix;
    
    // Step 5: Apply translation in NDC space (post-projection)
    // After projection, we translate to the corner
    // This is done by adding to the last row of the MVP matrix
    // In row-major: row 3 contains translation
    float w = 1.0f;  // Homogeneous coordinate
    mvp.Matrix.r[3] = DirectX::XMVectorAdd(
        mvp.Matrix.r[3],
        DirectX::XMVectorSet(ndcX * w, ndcY * w, 0.0f, 0.0f)
    );
    
    // Transpose for HLSL (column-major)
    FMatrix4x4 mvpTransposed = mvp.Transpose();
    
    // Update constant buffer
    void* cbData = ConstantBuffer->Map();
    memcpy(cbData, &mvpTransposed.Matrix, sizeof(DirectX::XMMATRIX));
    ConstantBuffer->Unmap();
    
    // Set render state and draw
    // Note: Depth testing should be disabled in the pipeline state for proper overlay
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
