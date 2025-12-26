#include "Scene.h"
#include "Primitive.h"
#include <algorithm>

// FRenderScene implementation
FRenderScene::FRenderScene() {
}

FRenderScene::~FRenderScene() {
}

void FRenderScene::AddProxy(FPrimitiveSceneProxy* Proxy) {
    if (Proxy) {
        Proxies.push_back(Proxy);
        FLog::Log(ELogLevel::Info, std::string("FRenderScene::AddProxy - Total proxies: ") + std::to_string(Proxies.size()));
    }
}

void FRenderScene::RemoveProxy(FPrimitiveSceneProxy* Proxy) {
    auto it = std::find(Proxies.begin(), Proxies.end(), Proxy);
    if (it != Proxies.end()) {
        Proxies.erase(it);
        FLog::Log(ELogLevel::Info, std::string("FRenderScene::RemoveProxy - Remaining proxies: ") + std::to_string(Proxies.size()));
    }
}

void FRenderScene::ClearProxies() {
    // Delete all proxies
    for (FPrimitiveSceneProxy* Proxy : Proxies) {
        delete Proxy;
    }
    Proxies.clear();
    FLog::Log(ELogLevel::Info, "FRenderScene::ClearProxies - All proxies cleared");
}

void FRenderScene::Render(FRHICommandList* RHICmdList, FRenderStats& Stats) {
    FLog::Log(ELogLevel::Info, std::string("FRenderScene::Render - Rendering ") + std::to_string(Proxies.size()) + " proxies");
    
    // Render all proxies
    for (FPrimitiveSceneProxy* Proxy : Proxies) {
        Proxy->Render(RHICmdList);
        Stats.AddTriangles(Proxy->GetTriangleCount());
    }
}

// FScene implementation
FScene::FScene(FRHI* InRHI)
    : RHI(InRHI), bNeedsProxyUpdate(false) {
}

FScene::~FScene() {
}

void FScene::AddPrimitive(FPrimitive* Primitive) {
    if (Primitive) {
        Primitives.push_back(Primitive);
        bNeedsProxyUpdate = true;
        FLog::Log(ELogLevel::Info, std::string("FScene::AddPrimitive - Total primitives: ") + std::to_string(Primitives.size()));
    }
}

void FScene::RemovePrimitive(FPrimitive* Primitive) {
    auto it = std::find(Primitives.begin(), Primitives.end(), Primitive);
    if (it != Primitives.end()) {
        Primitives.erase(it);
        bNeedsProxyUpdate = true;
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
    // For this minimal implementation, we recreate all proxies on update
    // In a real engine, this would be more sophisticated with dirty tracking
    if (bNeedsProxyUpdate) {
        FLog::Log(ELogLevel::Info, "FScene::UpdateRenderScene - Updating render scene");
        
        // Clear existing proxies
        RenderScene->ClearProxies();
        
        // Create new proxies for all primitives
        for (FPrimitive* Primitive : Primitives) {
            FPrimitiveSceneProxy* Proxy = Primitive->CreateSceneProxy(RHI);
            if (Proxy) {
                RenderScene->AddProxy(Proxy);
            }
        }
        
        bNeedsProxyUpdate = false;
    }
}

void FScene::Shutdown() {
    FLog::Log(ELogLevel::Info, "FScene::Shutdown - Cleaning up primitives");
    
    // Delete all primitives
    for (FPrimitive* Primitive : Primitives) {
        delete Primitive;
    }
    Primitives.clear();
}
