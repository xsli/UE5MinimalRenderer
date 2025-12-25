#pragma once

#include "../RHI/RHI.h"
#include <memory>

// Render commands that can be enqueued from game thread
class FRenderCommand {
public:
    virtual ~FRenderCommand() = default;
    virtual void Execute(FRHICommandList* RHICmdList) = 0;
};

// Scene proxy - represents renderable object
class FSceneProxy {
public:
    virtual ~FSceneProxy() = default;
    virtual void Render(FRHICommandList* RHICmdList) = 0;
};

// Triangle mesh scene proxy
class FTriangleMeshProxy : public FSceneProxy {
public:
    FTriangleMeshProxy(FRHIBuffer* InVertexBuffer, FRHIPipelineState* InPSO, uint32 InVertexCount);
    virtual ~FTriangleMeshProxy() override;
    
    virtual void Render(FRHICommandList* RHICmdList) override;
    
private:
    FRHIBuffer* VertexBuffer;
    FRHIPipelineState* PipelineState;
    uint32 VertexCount;
};

// Renderer - manages render thread and scene rendering
class FRenderer {
public:
    FRenderer(FRHI* InRHI);
    ~FRenderer();
    
    void Initialize();
    void Shutdown();
    
    // Called from game thread to render a frame
    void RenderFrame();
    
    // Scene management
    void AddSceneProxy(FSceneProxy* Proxy);
    void RemoveSceneProxy(FSceneProxy* Proxy);
    
private:
    void RenderScene(FRHICommandList* RHICmdList);
    
    FRHI* RHI;
    std::vector<FSceneProxy*> SceneProxies;
};
