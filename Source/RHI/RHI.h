#pragma once

#include "../Core/CoreTypes.h"

// Forward declarations
class FRHICommandList;
class FRHIBuffer;
class FRHIPipelineState;
class FRHITexture;

// Vertex data structure
struct FVertex {
    FVector Position;
    FColor Color;
};

// RHI Resource base class
class FRHIResource {
public:
    virtual ~FRHIResource() = default;
};

// RHI Buffer
class FRHIBuffer : public FRHIResource {
public:
    virtual ~FRHIBuffer() = default;
    virtual void* Map() = 0;
    virtual void Unmap() = 0;
};

// RHI Texture
class FRHITexture : public FRHIResource {
public:
    virtual ~FRHITexture() = default;
};

// Pipeline state
class FRHIPipelineState : public FRHIResource {
public:
    virtual ~FRHIPipelineState() = default;
};

// Command list for recording GPU commands
class FRHICommandList {
public:
    virtual ~FRHICommandList() = default;
    
    virtual void BeginFrame() = 0;
    virtual void EndFrame() = 0;
    virtual void ClearRenderTarget(const FColor& Color) = 0;
    virtual void SetPipelineState(FRHIPipelineState* PipelineState) = 0;
    virtual void SetVertexBuffer(FRHIBuffer* VertexBuffer, uint32 Offset, uint32 Stride) = 0;
    virtual void DrawPrimitive(uint32 VertexCount, uint32 StartVertex) = 0;
    virtual void Present() = 0;
};

// RHI Interface - factory for creating RHI resources
class FRHI {
public:
    virtual ~FRHI() = default;
    
    virtual bool Initialize(void* WindowHandle, uint32 Width, uint32 Height) = 0;
    virtual void Shutdown() = 0;
    
    virtual FRHICommandList* GetCommandList() = 0;
    virtual FRHIBuffer* CreateVertexBuffer(uint32 Size, const void* Data) = 0;
    virtual FRHIPipelineState* CreateGraphicsPipelineState() = 0;
    
    virtual void BeginFrame() = 0;
    virtual void EndFrame() = 0;
};

// Factory function to create platform-specific RHI
FRHI* CreateDX12RHI();
