#pragma once

#include "../RHI/RHI.h"
#include "../Renderer/Camera.h"
#include "../Renderer/Renderer.h"
#include "../Core/CoreTypes.h"

// Forward declarations
struct FTransform;

// Base primitive scene proxy - render thread representation
// Inherits from FSceneProxy to maintain compatibility with the renderer
class FPrimitiveSceneProxy : public FSceneProxy {
public:
    FPrimitiveSceneProxy(FRHIBuffer* InVertexBuffer, FRHIBuffer* InIndexBuffer, 
                         FRHIBuffer* InConstantBuffer, FRHIPipelineState* InPSO,
                         uint32 InIndexCount, FCamera* InCamera, const FTransform& InTransform);
    virtual ~FPrimitiveSceneProxy();
    
    // Render this proxy (override from FSceneProxy)
    virtual void Render(FRHICommandList* RHICmdList) override;
    
    // Get triangle count (override from FSceneProxy)
    virtual uint32 GetTriangleCount() const override;
    
    // Update transform
    void UpdateTransform(const FTransform& InTransform);
    
protected:
    FRHIBuffer* VertexBuffer;
    FRHIBuffer* IndexBuffer;
    FRHIBuffer* ConstantBuffer;
    FRHIPipelineState* PipelineState;
    uint32 IndexCount;
    FCamera* Camera;
    FMatrix4x4 ModelMatrix;
};
