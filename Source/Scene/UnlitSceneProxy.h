#pragma once

#include "../RHI/RHI.h"
#include "../Renderer/Camera.h"
#include "../Renderer/Renderer.h"
#include "../Core/CoreTypes.h"
#include "ScenePrimitive.h"  // For FTransform

/**
 * FUnlitPrimitiveSceneProxy - Scene proxy for unlit primitives
 * Uses FVertex format (position, color) without lighting calculations
 * Used for debug visualization, gizmos, and other unlit rendering
 */
class FUnlitPrimitiveSceneProxy : public FSceneProxy 
{
public:
    FUnlitPrimitiveSceneProxy(FRHIBuffer* InVertexBuffer, FRHIBuffer* InIndexBuffer, 
                              FRHIBuffer* InConstantBuffer, FRHIPipelineState* InPSO,
                              uint32 InIndexCount, FCamera* InCamera, const FTransform& InTransform);
    virtual ~FUnlitPrimitiveSceneProxy();
    
    virtual void Render(FRHICommandList* RHICmdList) override;
    virtual uint32 GetTriangleCount() const override;
    
    virtual void UpdateTransform(const FTransform& InTransform) override;
    
protected:
    FRHIBuffer* VertexBuffer;
    FRHIBuffer* IndexBuffer;
    FRHIBuffer* ConstantBuffer;
    FRHIPipelineState* PipelineState;
    uint32 IndexCount;
    FCamera* Camera;
    FMatrix4x4 ModelMatrix;
};

/**
 * FScreenSpaceGizmoProxy - Screen-space coordinate axis gizmo
 * Renders in a corner of the screen showing world axis orientation
 * Uses orthographic projection and camera-aligned rotation
 */
class FScreenSpaceGizmoProxy : public FSceneProxy 
{
public:
    FScreenSpaceGizmoProxy(FRHIBuffer* InVertexBuffer, FRHIBuffer* InIndexBuffer, 
                           FRHIBuffer* InConstantBuffer, FRHIPipelineState* InPSO,
                           uint32 InIndexCount, FCamera* InCamera,
                           int InScreenCorner, float InGizmoSize);
    virtual ~FScreenSpaceGizmoProxy();
    
    virtual void Render(FRHICommandList* RHICmdList) override;
    virtual uint32 GetTriangleCount() const override;
    
    void SetScreenCorner(int Corner) { ScreenCorner = Corner; }
    void SetGizmoSize(float Size) { GizmoSize = Size; }
    
protected:
    FRHIBuffer* VertexBuffer;
    FRHIBuffer* IndexBuffer;
    FRHIBuffer* ConstantBuffer;
    FRHIPipelineState* PipelineState;
    uint32 IndexCount;
    FCamera* Camera;
    int ScreenCorner;   // 0=top-left, 1=top-right, 2=bottom-left, 3=bottom-right
    float GizmoSize;    // Size of gizmo in screen pixels
};
