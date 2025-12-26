#pragma once

#include "../RHI/RHI.h"
#include "../Renderer/Camera.h"
#include "../Core/CoreTypes.h"

// Forward declarations
struct FTransform;

// Base primitive scene proxy - render thread representation
class FPrimitiveSceneProxy {
public:
    FPrimitiveSceneProxy(FRHIBuffer* InVertexBuffer, FRHIBuffer* InIndexBuffer, 
                         FRHIBuffer* InConstantBuffer, FRHIPipelineState* InPSO,
                         uint32 InIndexCount, FCamera* InCamera, const FTransform& InTransform);
    virtual ~FPrimitiveSceneProxy();
    
    // Render this proxy
    virtual void Render(FRHICommandList* RHICmdList);
    
    // Get triangle count
    virtual uint32 GetTriangleCount() const;
    
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
