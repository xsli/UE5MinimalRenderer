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
    
    // Begin RHI timing (tracks GPU command submission time)
    Stats.BeginRHIThreadTiming();
    
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
    
    // End RHI timing
    Stats.EndRHIThreadTiming();
    
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
    // UE5-style stat display: green text, top-right corner, monospace-style font
    const float fontSize = 14.0f;
    const float lineHeight = 18.0f;
    const FColor statColor(0.0f, 1.0f, 0.0f, 1.0f);  // Green (UE5 stat style)
    const FColor headerColor(0.3f, 1.0f, 0.3f, 1.0f);  // Lighter green for headers
    
    // Position from top-right (assuming 1280 width, leaving margin)
    const float rightMargin = 10.0f;
    const float startX = 1280.0f - 120.0f - rightMargin;  // Right-aligned with 200px width
    float yPos = 100.0f;
    
    // Header
//     RHICmdList->RHIDrawText("stat unit", FVector2D(startX, yPos), fontSize, headerColor);
//     yPos += lineHeight;
    
    // Frame number
    char buffer[128];
    snprintf(buffer, sizeof(buffer), "Frame: %llu", Stats.GetFrameCount());
    RHICmdList->RHIDrawText(buffer, FVector2D(startX, yPos), fontSize, statColor);
    yPos += lineHeight;
    
    // FPS
    snprintf(buffer, sizeof(buffer), "FPS: %.1f", Stats.GetFPS());
    RHICmdList->RHIDrawText(buffer, FVector2D(startX, yPos), fontSize, statColor);
    yPos += lineHeight;
    
    // Frame time
    snprintf(buffer, sizeof(buffer), "Frame: %.2f ms", Stats.GetFrameTime());
    RHICmdList->RHIDrawText(buffer, FVector2D(startX, yPos), fontSize, statColor);
    yPos += lineHeight;
    
    // Thread times - UE5 style "Thread: X.XX ms"
    snprintf(buffer, sizeof(buffer), "Game: %.2f ms", Stats.GetGameThreadTime());
    RHICmdList->RHIDrawText(buffer, FVector2D(startX, yPos), fontSize, statColor);
    yPos += lineHeight;
    
    snprintf(buffer, sizeof(buffer), "Draw: %.2f ms", Stats.GetRenderThreadTime());
    RHICmdList->RHIDrawText(buffer, FVector2D(startX, yPos), fontSize, statColor);
    yPos += lineHeight;
    
    snprintf(buffer, sizeof(buffer), "RHI: %.2f ms", Stats.GetRHIThreadTime());
    RHICmdList->RHIDrawText(buffer, FVector2D(startX, yPos), fontSize, statColor);
    yPos += lineHeight;
    
    // Triangle count
    snprintf(buffer, sizeof(buffer), "Primitives: %u", Stats.GetTriangleCount());
    RHICmdList->RHIDrawText(buffer, FVector2D(startX, yPos), fontSize, statColor);
}
