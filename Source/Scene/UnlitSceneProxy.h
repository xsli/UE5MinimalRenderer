#pragma once

#include "../RHI/RHI.h"
#include "../Renderer/Camera.h"
#include "../Renderer/Renderer.h"
#include "../Core/CoreTypes.h"
#include "ScenePrimitive.h"  // For FTransform

/**
 * FPrimitiveSceneProxy - Unlit primitive scene proxy
 * Render thread representation for unlit primitives using FVertex format
 */
class FPrimitiveSceneProxy : public FSceneProxy 
{
public:
    FPrimitiveSceneProxy(FRHIBuffer* InVertexBuffer, FRHIBuffer* InIndexBuffer, 
                         FRHIBuffer* InConstantBuffer, FRHIPipelineState* InPSO,
                         uint32 InIndexCount, FCamera* InCamera, const FTransform& InTransform);
    virtual ~FPrimitiveSceneProxy();
    
    virtual void Render(FRHICommandList* RHICmdList) override;
    virtual uint32 GetTriangleCount() const override;
    
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
