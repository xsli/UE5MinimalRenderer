#pragma once

#include "../Renderer/Renderer.h"
#include <vector>

// Forward declarations
class FPrimitive;
class FRHI;

// FRenderScene - Render thread scene representation
// Contains proxies for actual rendering
class FRenderScene {
public:
    FRenderScene();
    ~FRenderScene();
    
    // Proxy management (called from render thread or during sync)
    void AddProxy(FSceneProxy* Proxy);
    void RemoveProxy(FSceneProxy* Proxy);
    void ClearProxies();
    
    // Rendering
    void Render(FRHICommandList* RHICmdList, FRenderStats& Stats);
    
    // Get proxy list
    const std::vector<FSceneProxy*>& GetProxies() const { return Proxies; }
    
private:
    std::vector<FSceneProxy*> Proxies;
};

// FScene - Game thread scene representation
// Contains primitive objects
class FScene {
public:
    FScene(FRHI* InRHI);
    ~FScene();
    
    // Primitive management (called from game thread)
    void AddPrimitive(FPrimitive* Primitive);
    void RemovePrimitive(FPrimitive* Primitive);
    
    // Update game thread primitives
    void Tick(float DeltaTime);
    
    // Synchronize with render scene (creates/updates proxies)
    void UpdateRenderScene(FRenderScene* RenderScene);
    
    // Cleanup
    void Shutdown();
    
private:
    FRHI* RHI;
    std::vector<FPrimitive*> Primitives;
    bool bNeedsProxyUpdate;
};
