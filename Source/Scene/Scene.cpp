#include "Scene.h"
#include "ScenePrimitive.h"
#include "../Renderer/Renderer.h"

// FRenderScene implementation
FRenderScene::FRenderScene()
{
}

FRenderScene::~FRenderScene()
{
    ClearProxies();
}

void FRenderScene::AddProxy(FSceneProxy* Proxy)
{
    if (Proxy)
    {
        Proxies.push_back(Proxy);
    }
}

void FRenderScene::RemoveProxy(FSceneProxy* Proxy)
{
    auto it = std::find(Proxies.begin(), Proxies.end(), Proxy);
    if (it != Proxies.end())
    {
        delete *it;
        Proxies.erase(it);
    }
}

void FRenderScene::ClearProxies()
{
    for (FSceneProxy* Proxy : Proxies)
    {
        delete Proxy;
    }
    Proxies.clear();
}

void FRenderScene::Render(FRHICommandList* RHICmdList, FRenderStats& Stats)
{
    uint32 totalTriangles = 0;
    uint32 drawCalls = 0;
    
    for (FSceneProxy* Proxy : Proxies)
    {
        if (Proxy)
        {
            Proxy->Render(RHICmdList);
            totalTriangles += Proxy->GetTriangleCount();
            drawCalls++;
        }
    }
    
    Stats.SetTriangleCount(totalTriangles);
    Stats.SetDrawCallCount(drawCalls);
}

// FScene implementation
FScene::FScene(FRHI* InRHI)
    : RHI(InRHI)
{
}

FScene::~FScene()
{
    Shutdown();
}

void FScene::AddPrimitive(FPrimitive* Primitive)
{
    if (Primitive)
    {
        Primitives.push_back(Primitive);
        Primitive->MarkDirty();  // Mark for proxy creation
    }
}

void FScene::RemovePrimitive(FPrimitive* Primitive)
{
    auto it = std::find(Primitives.begin(), Primitives.end(), Primitive);
    if (it != Primitives.end())
    {
        Primitives.erase(it);
        
        // Remove from proxy map
        auto mapIt = PrimitiveProxyMap.find(Primitive);
        if (mapIt != PrimitiveProxyMap.end())
        {
            PrimitiveProxyMap.erase(mapIt);
        }
    }
}

void FScene::Tick(float DeltaTime)
{
    for (FPrimitive* Primitive : Primitives)
    {
        if (Primitive)
        {
            Primitive->Tick(DeltaTime);
        }
    }
}

void FScene::UpdateRenderScene(FRenderScene* RenderScene)
{
    if (!RenderScene || !RHI) return;
    
    for (FPrimitive* Primitive : Primitives)
    {
        if (!Primitive) continue;
        
        auto it = PrimitiveProxyMap.find(Primitive);
        
        if (Primitive->IsDirty())
        {
            // Need to recreate proxy
            if (it != PrimitiveProxyMap.end() && it->second)
            {
                RenderScene->RemoveProxy(it->second);
            }
            
            // Create new proxy
            FSceneProxy* NewProxy = Primitive->CreateSceneProxy(RHI, &LightScene);
            if (NewProxy)
            {
                RenderScene->AddProxy(NewProxy);
                PrimitiveProxyMap[Primitive] = NewProxy;
            }
            
            Primitive->ClearDirty();
        }
        else if (Primitive->IsTransformDirty())
        {
            // Just update transform
            if (it != PrimitiveProxyMap.end() && it->second)
            {
                it->second->UpdateTransform(Primitive->GetTransform());
            }
            Primitive->ClearDirty();
        }
    }
}

void FScene::Shutdown()
{
    // Clear primitive-proxy mapping
    PrimitiveProxyMap.clear();
    
    // Delete all primitives
    for (FPrimitive* Primitive : Primitives)
    {
        delete Primitive;
    }
    Primitives.clear();
    
    // Clear lights
    LightScene.ClearLights();
}
