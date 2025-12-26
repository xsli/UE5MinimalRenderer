#include "Renderer.h"
#include "../Game/Scene.h"
#include <algorithm>
#include <string>
#include <cstdio>  // for snprintf

// FTriangleMeshProxy implementation
FTriangleMeshProxy::FTriangleMeshProxy(FRHIBuffer* InVertexBuffer, FRHIPipelineState* InPSO, uint32 InVertexCount)
    : VertexBuffer(InVertexBuffer), PipelineState(InPSO), VertexCount(InVertexCount)
{
}

FTriangleMeshProxy::~FTriangleMeshProxy()
{
    // Note: Raw delete is intentional here. The proxy owns these RHI resources
    // and is responsible for their lifetime. This matches the RHI design pattern
    // where resources are created via factory methods and owned by their users.
    delete VertexBuffer;
    delete PipelineState;
}

void FTriangleMeshProxy::Render(FRHICommandList* RHICmdList)
{
    FLog::Log(ELogLevel::Info, "FTriangleMeshProxy::Render called");
    RHICmdList->SetPipelineState(PipelineState);
    RHICmdList->SetVertexBuffer(VertexBuffer, 0, sizeof(FVertex));
    RHICmdList->DrawPrimitive(VertexCount, 0);
}

uint32 FTriangleMeshProxy::GetTriangleCount() const
{
    return VertexCount / 3;  // Assumes triangle list topology
}

// FCubeMeshProxy implementation
FCubeMeshProxy::FCubeMeshProxy(FRHIBuffer* InVertexBuffer, FRHIBuffer* InIndexBuffer, FRHIBuffer* InConstantBuffer,
                               FRHIPipelineState* InPSO, uint32 InIndexCount, FCamera* InCamera)
    : VertexBuffer(InVertexBuffer), IndexBuffer(InIndexBuffer), ConstantBuffer(InConstantBuffer),
      PipelineState(InPSO), IndexCount(InIndexCount), Camera(InCamera), ModelMatrix(FMatrix4x4::Identity())
{
}

FCubeMeshProxy::~FCubeMeshProxy()
{
    delete VertexBuffer;
    delete IndexBuffer;
    delete ConstantBuffer;
    delete PipelineState;
}

void FCubeMeshProxy::Render(FRHICommandList* RHICmdList)
{
    FLog::Log(ELogLevel::Info, "FCubeMeshProxy::Render called");
    
    // Calculate MVP matrix with current model matrix
    FMatrix4x4 viewProjection = Camera->GetViewProjectionMatrix();
    FMatrix4x4 mvp = ModelMatrix * viewProjection;
    
    // Transpose for HLSL (column-major)
    FMatrix4x4 mvpTransposed = mvp.Transpose();
    
    // Update constant buffer with current MVP
    void* cbData = ConstantBuffer->Map();
    memcpy(cbData, &mvpTransposed.Matrix, sizeof(DirectX::XMMATRIX));
    ConstantBuffer->Unmap();
    
    RHICmdList->SetPipelineState(PipelineState);
    RHICmdList->SetConstantBuffer(ConstantBuffer, 0);
    RHICmdList->SetVertexBuffer(VertexBuffer, 0, sizeof(FVertex));
    RHICmdList->SetIndexBuffer(IndexBuffer);
    RHICmdList->DrawIndexedPrimitive(IndexCount, 0, 0);
}

uint32 FCubeMeshProxy::GetTriangleCount() const
{
    return IndexCount / 3;  // Assumes triangle list topology
}

void FCubeMeshProxy::UpdateModelMatrix(const FMatrix4x4& InModelMatrix)
{
    ModelMatrix = InModelMatrix;
}

// FRenderer implementation
FRenderer::FRenderer(FRHI* InRHI)
    : RHI(InRHI)
{
}

FRenderer::~FRenderer()
{
}

void FRenderer::Initialize()
{
    // Create camera
    Camera = std::make_unique<FCamera>();
    Camera->SetPosition(FVector(0.0f, 2.0f, -8.0f));
    Camera->SetLookAt(FVector(0.0f, 0.0f, 0.0f));
    Camera->SetPerspective(DirectX::XM_PIDIV4, 16.0f / 9.0f, 0.1f, 100.0f);
    
    // Create render scene
    RenderScene = std::make_unique<FRenderScene>();
    
    FLog::Log(ELogLevel::Info, "Renderer initialized");
}

void FRenderer::Shutdown()
{
    if (RenderScene)
    {
        RenderScene->ClearProxies();
        RenderScene.reset();
    }
    
    FLog::Log(ELogLevel::Info, "Renderer shutdown");
}

void FRenderer::RenderFrame()
{
    static int renderFrameCount = 0;
    renderFrameCount++;
    
    if (renderFrameCount <= 3)
    {
        FLog::Log(ELogLevel::Info, std::string("=== RenderFrame ") + std::to_string(renderFrameCount) + " ===");
    }
    
    // Begin stats tracking for this frame
    Stats.BeginFrame();
    
    // This simulates UE5's parallel rendering architecture
    // In real UE5, this would kick off a render thread task
    
    FRHICommandList* RHICmdList = RHI->GetCommandList();
    
    // Begin rendering
    RHICmdList->BeginFrame();
    
    // Clear screen
    RHICmdList->ClearRenderTarget(FColor(0.2f, 0.3f, 0.4f, 1.0f));
    RHICmdList->ClearDepthStencil();
    
    // Render scene using render scene
    if (RenderScene)
    {
        RenderScene->Render(RHICmdList, Stats);
    }

	// === CRITICAL: Flush 3D commands before 2D rendering ===
	RHICmdList->FlushCommandsFor2D();
    
    // Render statistics overlay
    RenderStats(RHICmdList);
    
    // End rendering
    RHICmdList->EndFrame();
    
    // Present
    RHICmdList->Present();
    
    // End stats tracking
    Stats.EndFrame();
}

void FRenderer::UpdateFromScene(FScene* GameScene)
{
    // This is the sync point between game and render thread
    // In real UE5, this would be more sophisticated with command queues
    if (GameScene && RenderScene)
    {
        GameScene->UpdateRenderScene(RenderScene.get());
    }
}

void FRenderer::AddSceneProxy(FSceneProxy* Proxy)
{
    // Legacy method for compatibility
    if (RenderScene)
    {
        RenderScene->AddProxy(Proxy);
    }
}

void FRenderer::RemoveSceneProxy(FSceneProxy* Proxy)
{
    // Legacy method for compatibility
    if (RenderScene)
    {
        RenderScene->RemoveProxy(Proxy);
    }
}

void FRenderer::RenderStats(FRHICommandList* RHICmdList)
{
    // Draw statistics overlay
    float yPos = 10.0f;
    const float fontSize = 18.0f;
    const float lineHeight = 25.0f;
    const FColor textColor(1.0f, 1.0f, 0.0f, 1.0f);  // Yellow
    
    // Frame count
    std::string frameCountText = "Frame: " + std::to_string(Stats.GetFrameCount());
    RHICmdList->RHIDrawText(frameCountText, FVector2D(10.0f, yPos), fontSize, textColor);
    yPos += lineHeight;
    
    // FPS
    char fpsBuffer[64];
    snprintf(fpsBuffer, sizeof(fpsBuffer), "FPS: %.1f", Stats.GetFPS());
    RHICmdList->RHIDrawText(std::string(fpsBuffer), FVector2D(10.0f, yPos), fontSize, textColor);
    yPos += lineHeight;
    
    // Frame time
    char frameTimeBuffer[64];
    snprintf(frameTimeBuffer, sizeof(frameTimeBuffer), "Frame Time: %.2f ms", Stats.GetFrameTime());
    RHICmdList->RHIDrawText(std::string(frameTimeBuffer), FVector2D(10.0f, yPos), fontSize, textColor);
    yPos += lineHeight;
    
    // Triangle count
    std::string triangleCountText = "Triangles: " + std::to_string(Stats.GetTriangleCount());
    RHICmdList->RHIDrawText(triangleCountText, FVector2D(10.0f, yPos), fontSize, textColor);
}
