#include "Scene.h"
#include "Primitive.h"
#include "PrimitiveSceneProxy.h"
#include <algorithm>

// FRenderScene implementation
FRenderScene::FRenderScene() {
}

FRenderScene::~FRenderScene() {
}

void FRenderScene::AddProxy(FSceneProxy* Proxy) {
    if (Proxy) {
        Proxies.push_back(Proxy);
        FLog::Log(ELogLevel::Info, std::string("FRenderScene::AddProxy - Total proxies: ") + std::to_string(Proxies.size()));
    }
}

void FRenderScene::RemoveProxy(FSceneProxy* Proxy) {
    auto it = std::find(Proxies.begin(), Proxies.end(), Proxy);
    if (it != Proxies.end()) {
        Proxies.erase(it);
        FLog::Log(ELogLevel::Info, std::string("FRenderScene::RemoveProxy - Remaining proxies: ") + std::to_string(Proxies.size()));
    }
}

void FRenderScene::ClearProxies() {
    // Delete all proxies
    for (FSceneProxy* Proxy : Proxies) {
        delete Proxy;
    }
    Proxies.clear();
    FLog::Log(ELogLevel::Info, "FRenderScene::ClearProxies - All proxies cleared");
}

void FRenderScene::Render(FRHICommandList* RHICmdList, FRenderStats& Stats) {
    FLog::Log(ELogLevel::Info, std::string("FRenderScene::Render - Rendering ") + std::to_string(Proxies.size()) + " proxies");
    
    // Render all proxies
    for (FSceneProxy* Proxy : Proxies) {
        Proxy->Render(RHICmdList);
        Stats.AddTriangles(Proxy->GetTriangleCount());
    }
}

// FScene implementation
FScene::FScene(FRHI* InRHI)
    : RHI(InRHI) {
}

FScene::~FScene() {
}

void FScene::AddPrimitive(FPrimitive* Primitive) {
    if (Primitive) {
        Primitives.push_back(Primitive);
        Primitive->MarkDirty();  // Ensure new primitives get proxies created
        FLog::Log(ELogLevel::Info, std::string("FScene::AddPrimitive - Total primitives: ") + std::to_string(Primitives.size()));
    }
}

void FScene::RemovePrimitive(FPrimitive* Primitive) {
    auto it = std::find(Primitives.begin(), Primitives.end(), Primitive);
    if (it != Primitives.end()) {
        Primitives.erase(it);
        // Note: Proxy cleanup happens automatically in UpdateRenderScene
        FLog::Log(ELogLevel::Info, std::string("FScene::RemovePrimitive - Remaining primitives: ") + std::to_string(Primitives.size()));
    }
}

void FScene::Tick(float DeltaTime) {
    // Tick all primitives on game thread
    for (FPrimitive* Primitive : Primitives) {
        Primitive->Tick(DeltaTime);
    }
}

void FScene::UpdateRenderScene(FRenderScene* RenderScene) {
    // Dirty tracking implementation: only recreate proxies for changed primitives
    // Transform-only changes just update the proxy's transform without recreation
    
    // Track which proxies are still valid
    std::unordered_map<FPrimitive*, FSceneProxy*> newProxyMap;
    
    for (FPrimitive* Primitive : Primitives) {
        auto existingProxyIt = PrimitiveProxyMap.find(Primitive);
        
        if (existingProxyIt != PrimitiveProxyMap.end()) {
            // Proxy exists - check what kind of update is needed
            if (!Primitive->IsDirty() && !Primitive->IsTransformDirty()) {
                // No changes - reuse existing proxy as-is
                newProxyMap[Primitive] = existingProxyIt->second;
            } else if (Primitive->IsTransformDirty() && !Primitive->IsDirty()) {
                // Only transform changed - update proxy transform without recreating
                FPrimitiveSceneProxy* proxy = static_cast<FPrimitiveSceneProxy*>(existingProxyIt->second);
                proxy->UpdateTransform(Primitive->GetTransform());
                newProxyMap[Primitive] = existingProxyIt->second;
                Primitive->ClearDirty();
            } else {
                // Full dirty (color changed or other property) - recreate proxy
                RenderScene->RemoveProxy(existingProxyIt->second);
                delete existingProxyIt->second;
                
                FPrimitiveSceneProxy* newProxy = Primitive->CreateSceneProxy(RHI);
                if (newProxy) {
                    RenderScene->AddProxy(newProxy);
                    newProxyMap[Primitive] = newProxy;
                    Primitive->ClearDirty();
                }
            }
        } else {
            // No existing proxy - create new one
            FPrimitiveSceneProxy* newProxy = Primitive->CreateSceneProxy(RHI);
            if (newProxy) {
                RenderScene->AddProxy(newProxy);
                newProxyMap[Primitive] = newProxy;
                Primitive->ClearDirty();
            }
        }
    }
    
    // Remove proxies for primitives that no longer exist
    for (auto& pair : PrimitiveProxyMap) {
        if (newProxyMap.find(pair.first) == newProxyMap.end()) {
            RenderScene->RemoveProxy(pair.second);
            delete pair.second;
        }
    }
    
    // Update the proxy map
    PrimitiveProxyMap = std::move(newProxyMap);
}

void FScene::Shutdown() {
    FLog::Log(ELogLevel::Info, "FScene::Shutdown - Cleaning up primitives");
    
    // Delete all primitives
    for (FPrimitive* Primitive : Primitives) {
        delete Primitive;
    }
    Primitives.clear();
    
    // Clear the proxy map (proxies are owned by RenderScene, will be cleaned up there)
    PrimitiveProxyMap.clear();
}
