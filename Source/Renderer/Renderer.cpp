#include "Renderer.h"
#include <algorithm>
#include <string>
#include <cstdio>  // for snprintf

// FTriangleMeshProxy implementation
FTriangleMeshProxy::FTriangleMeshProxy(FRHIBuffer* InVertexBuffer, FRHIPipelineState* InPSO, uint32 InVertexCount)
    : VertexBuffer(InVertexBuffer), PipelineState(InPSO), VertexCount(InVertexCount) {
}

FTriangleMeshProxy::~FTriangleMeshProxy() {
    // Note: Raw delete is intentional here. The proxy owns these RHI resources
    // and is responsible for their lifetime. This matches the RHI design pattern
    // where resources are created via factory methods and owned by their users.
    delete VertexBuffer;
    delete PipelineState;
}

void FTriangleMeshProxy::Render(FRHICommandList* RHICmdList) {
    FLog::Log(ELogLevel::Info, "FTriangleMeshProxy::Render called");
    RHICmdList->SetPipelineState(PipelineState);
    RHICmdList->SetVertexBuffer(VertexBuffer, 0, sizeof(FVertex));
    RHICmdList->DrawPrimitive(VertexCount, 0);
}

uint32 FTriangleMeshProxy::GetTriangleCount() const {
    return VertexCount / 3;  // Assumes triangle list topology
}

// FRenderer implementation
FRenderer::FRenderer(FRHI* InRHI)
    : RHI(InRHI) {
}

FRenderer::~FRenderer() {
}

void FRenderer::Initialize() {
    FLog::Log(ELogLevel::Info, "Renderer initialized");
}

void FRenderer::Shutdown() {
    for (FSceneProxy* Proxy : SceneProxies) {
        delete Proxy;
    }
    SceneProxies.clear();
    
    FLog::Log(ELogLevel::Info, "Renderer shutdown");
}

void FRenderer::RenderFrame() {
    static int renderFrameCount = 0;
    renderFrameCount++;
    
    if (renderFrameCount <= 3) {
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
    
    // Render scene
    RenderScene(RHICmdList);

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

void FRenderer::AddSceneProxy(FSceneProxy* Proxy) {
    SceneProxies.push_back(Proxy);
    FLog::Log(ELogLevel::Info, std::string("AddSceneProxy - Total proxies: ") + std::to_string(SceneProxies.size()));
}

void FRenderer::RemoveSceneProxy(FSceneProxy* Proxy) {
    auto it = std::find(SceneProxies.begin(), SceneProxies.end(), Proxy);
    if (it != SceneProxies.end()) {
        SceneProxies.erase(it);
    }
}

void FRenderer::RenderScene(FRHICommandList* RHICmdList) {
    FLog::Log(ELogLevel::Info, std::string("RenderScene - Rendering ") + std::to_string(SceneProxies.size()) + " proxies");
    
    // Render all scene proxies
    for (FSceneProxy* Proxy : SceneProxies) {
        Proxy->Render(RHICmdList);
        Stats.AddTriangles(Proxy->GetTriangleCount());
    }
}

void FRenderer::RenderStats(FRHICommandList* RHICmdList) {
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
