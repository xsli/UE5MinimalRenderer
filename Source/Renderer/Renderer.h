#pragma once

#include "../RHI/RHI.h"
#include "RenderStats.h"
#include "Camera.h"
#include "RTPool.h"
#include "ShadowMapping.h"
#include <memory>

// Render commands that can be enqueued from game thread
class FRenderCommand 
{
public:
    virtual ~FRenderCommand() = default;
    virtual void Execute(FRHICommandList* RHICmdList) = 0;
};

// Forward declaration - FTransform is defined in Scene/ScenePrimitive.h
struct FTransform;

// Scene proxy - represents renderable object
class FSceneProxy 
{
public:
    virtual ~FSceneProxy() = default;
    virtual void Render(FRHICommandList* RHICmdList) = 0;
    virtual uint32 GetTriangleCount() const = 0;
    
    // Shadow pass rendering - renders with light's view-projection matrix
    // Default implementation does nothing - override for shadow-casting objects
    virtual void RenderShadow(FRHICommandList* RHICmdList, const FMatrix4x4& LightViewProj) {}
    
    // Update transform - default implementation does nothing
    // Derived classes should override this to handle transform updates
    virtual void UpdateTransform(const FTransform& InTransform) {}
    
    // Get model matrix for shadow calculations
    virtual FMatrix4x4 GetModelMatrix() const { return FMatrix4x4::Identity(); }
};

// Triangle mesh scene proxy
class FTriangleMeshProxy : public FSceneProxy 
{
public:
    FTriangleMeshProxy(FRHIBuffer* InVertexBuffer, FRHIPipelineState* InPSO, uint32 InVertexCount);
    virtual ~FTriangleMeshProxy() override;
    
    virtual void Render(FRHICommandList* RHICmdList) override;
    virtual uint32 GetTriangleCount() const override;
    
private:
    FRHIBuffer* VertexBuffer;
    FRHIPipelineState* PipelineState;
    uint32 VertexCount;
};

// Cube mesh scene proxy (indexed rendering with MVP matrix)
class FCubeMeshProxy : public FSceneProxy 
{
public:
    FCubeMeshProxy(FRHIBuffer* InVertexBuffer, FRHIBuffer* InIndexBuffer, FRHIBuffer* InConstantBuffer,
                   FRHIPipelineState* InPSO, uint32 InIndexCount, FCamera* InCamera);
    virtual ~FCubeMeshProxy() override;
    
    virtual void Render(FRHICommandList* RHICmdList) override;
    virtual uint32 GetTriangleCount() const override;
    
    void UpdateModelMatrix(const FMatrix4x4& InModelMatrix);
    
private:
    FRHIBuffer* VertexBuffer;
    FRHIBuffer* IndexBuffer;
    FRHIBuffer* ConstantBuffer;
    FRHIPipelineState* PipelineState;
    uint32 IndexCount;
    FCamera* Camera;
    FMatrix4x4 ModelMatrix;
};

// Forward declarations
class FScene;
class FRenderScene;

// Renderer - manages render thread and scene rendering
class FRenderer 
{
public:
    FRenderer(FRHI* InRHI);
    ~FRenderer();
    
    void Initialize();
    void Shutdown();
    
    // Called from game thread to render a frame
    void RenderFrame();
    
    // Update render scene from game scene (sync point)
    void UpdateFromScene(FScene* GameScene);
    
    // Scene management (legacy, kept for compatibility)
    void AddSceneProxy(FSceneProxy* Proxy);
    void RemoveSceneProxy(FSceneProxy* Proxy);
    
    // Get statistics
    const FRenderStats& GetStats() const { return Stats; }
    FRenderStats& GetStats() { return Stats; }
    
    // Get camera
    FCamera* GetCamera() { return Camera.get(); }
    
    // Get shadow system
    FShadowSystem* GetShadowSystem() { return ShadowSystem.get(); }
    
    // Get RT pool statistics
    const FRTPoolStats* GetRTPoolStats() const;
    uint32 GetDrawCallCount() const { return DrawCallCount; }
    
private:
    void RenderStats(FRHICommandList* RHICmdList);
    void RenderShadowPasses(FRHICommandList* RHICmdList);
    
    FRHI* RHI;
    std::unique_ptr<FRenderScene> RenderScene;
    FRenderStats Stats;
    std::unique_ptr<FCamera> Camera;
    std::unique_ptr<FRTPool> RTPool;
    std::unique_ptr<FShadowSystem> ShadowSystem;
    
    // Per-frame tracking
    uint32 DrawCallCount;
    
    // Scene reference for shadow pass updates
    FScene* CurrentScene;
};
