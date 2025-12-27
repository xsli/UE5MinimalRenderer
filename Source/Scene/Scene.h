#pragma once

#include "../Core/CoreTypes.h"
#include "../Lighting/Light.h"
#include <vector>
#include <unordered_map>

// Forward declarations
class FPrimitive;
class FSceneProxy;
class FRHI;
class FRHICommandList;
class FRenderStats;

/**
 * FRenderScene - Render thread scene representation
 * Contains proxies for actual rendering
 */
class FRenderScene 
{
public:
    FRenderScene();
    ~FRenderScene();
    
    // Proxy management
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

/**
 * FScene - Unified game thread scene
 * Contains all primitives and lights
 */
class FScene 
{
public:
    FScene(FRHI* InRHI);
    ~FScene();
    
    // Primitive management
    void AddPrimitive(FPrimitive* Primitive);
    void RemovePrimitive(FPrimitive* Primitive);
    const std::vector<FPrimitive*>& GetPrimitives() const { return Primitives; }
    
    // Light scene management (now integrated)
    FLightScene* GetLightScene() { return &LightScene; }
    const FLightScene* GetLightScene() const { return &LightScene; }
    
    // Update all primitives
    void Tick(float DeltaTime);
    
    // Synchronize with render scene
    void UpdateRenderScene(FRenderScene* RenderScene);
    
    // Cleanup
    void Shutdown();
    
    // Get RHI
    FRHI* GetRHI() { return RHI; }
    
private:
    FRHI* RHI;
    std::vector<FPrimitive*> Primitives;
    FLightScene LightScene;
    
    // Dirty tracking
    std::unordered_map<FPrimitive*, FSceneProxy*> PrimitiveProxyMap;
};
