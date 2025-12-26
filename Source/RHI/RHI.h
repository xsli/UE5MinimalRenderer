#pragma once

#include "../Core/CoreTypes.h"

// Forward declarations
class FRHICommandList;
class FRHIBuffer;
class FRHIPipelineState;
class FRHITexture;

// Vertex data structure
struct FVertex 
{
    FVector Position;
    FColor Color;
};

// RHI Resource base class
class FRHIResource 
{
public:
    virtual ~FRHIResource() = default;
};

// RHI Buffer
class FRHIBuffer : public FRHIResource 
{
public:
    virtual ~FRHIBuffer() = default;
    virtual void* Map() = 0;
    virtual void Unmap() = 0;
};

// RHI Texture
class FRHITexture : public FRHIResource 
{
public:
    virtual ~FRHITexture() = default;
};

// Pipeline state
class FRHIPipelineState : public FRHIResource 
{
public:
    virtual ~FRHIPipelineState() = default;
};

// Command list for recording GPU commands
class FRHICommandList 
{
public:
    virtual ~FRHICommandList() = default;
    
    virtual void BeginFrame() = 0;
    virtual void EndFrame() = 0;
    virtual void ClearRenderTarget(const FColor& Color) = 0;
    virtual void ClearDepthStencil() = 0;
    virtual void SetPipelineState(FRHIPipelineState* PipelineState) = 0;
    virtual void SetVertexBuffer(FRHIBuffer* VertexBuffer, uint32 Offset, uint32 Stride) = 0;
    virtual void SetIndexBuffer(FRHIBuffer* IndexBuffer) = 0;
    virtual void SetConstantBuffer(FRHIBuffer* ConstantBuffer, uint32 RootParameterIndex) = 0;
    virtual void DrawPrimitive(uint32 VertexCount, uint32 StartVertex) = 0;
    virtual void DrawIndexedPrimitive(uint32 IndexCount, uint32 StartIndex, uint32 BaseVertex) = 0;
    virtual void Present() = 0;
    
    // Flush 3D rendering commands before 2D overlay rendering
    // This ensures all 3D content is rendered before 2D text/UI is drawn on top
    virtual void FlushCommandsFor2D() = 0;
    
    // Text rendering - call FlushCommandsFor2D() before calling this
    virtual void RHIDrawText(const std::string& Text, const FVector2D& Position, float FontSize, const FColor& Color) = 0;
};

// RHI Interface - factory for creating RHI resources
// Note: Resource ownership model - resources created by FRHI are owned by the caller
// and must be deleted when no longer needed. This matches traditional graphics API patterns.
class FRHI 
{
public:
    virtual ~FRHI() = default;
    
    virtual bool Initialize(void* WindowHandle, uint32 Width, uint32 Height) = 0;
    virtual void Shutdown() = 0;
    
    virtual FRHICommandList* GetCommandList() = 0;
    
    // Resource creation - caller takes ownership and is responsible for deletion
    virtual FRHIBuffer* CreateVertexBuffer(uint32 Size, const void* Data) = 0;
    virtual FRHIBuffer* CreateIndexBuffer(uint32 Size, const void* Data) = 0;
    virtual FRHIBuffer* CreateConstantBuffer(uint32 Size) = 0;
    virtual FRHIPipelineState* CreateGraphicsPipelineState(bool bEnableDepth = false) = 0;
};

// Factory function to create platform-specific RHI
FRHI* CreateDX12RHI();
