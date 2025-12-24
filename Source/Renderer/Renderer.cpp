#include "Renderer.h"

// FTriangleMeshProxy implementation
FTriangleMeshProxy::FTriangleMeshProxy(FRHIBuffer* InVertexBuffer, FRHIPipelineState* InPSO, uint32 InVertexCount)
    : VertexBuffer(InVertexBuffer), PipelineState(InPSO), VertexCount(InVertexCount) {
}

FTriangleMeshProxy::~FTriangleMeshProxy() {
    delete VertexBuffer;
    delete PipelineState;
}

void FTriangleMeshProxy::Render(FRHICommandList* RHICmdList) {
    RHICmdList->SetPipelineState(PipelineState);
    RHICmdList->SetVertexBuffer(VertexBuffer, 0, sizeof(FVertex));
    RHICmdList->DrawPrimitive(VertexCount, 0);
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
    // This simulates UE5's parallel rendering architecture
    // In real UE5, this would kick off a render thread task
    
    FRHICommandList* RHICmdList = RHI->GetCommandList();
    
    // Begin rendering
    RHI->BeginFrame();
    RHICmdList->BeginFrame();
    
    // Clear screen
    RHICmdList->ClearRenderTarget(FColor(0.2f, 0.3f, 0.4f, 1.0f));
    
    // Render scene
    RenderScene(RHICmdList);
    
    // End rendering
    RHICmdList->EndFrame();
    RHI->EndFrame();
    
    // Present
    RHICmdList->Present();
}

void FRenderer::AddSceneProxy(FSceneProxy* Proxy) {
    SceneProxies.push_back(Proxy);
}

void FRenderer::RemoveSceneProxy(FSceneProxy* Proxy) {
    auto it = std::find(SceneProxies.begin(), SceneProxies.end(), Proxy);
    if (it != SceneProxies.end()) {
        SceneProxies.erase(it);
    }
}

void FRenderer::RenderScene(FRHICommandList* RHICmdList) {
    // Render all scene proxies
    for (FSceneProxy* Proxy : SceneProxies) {
        Proxy->Render(RHICmdList);
    }
}
