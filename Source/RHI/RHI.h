#pragma once

#include "../Core/CoreTypes.h"

// Forward declarations
class FRHICommandList;
class FRHIBuffer;
class FRHIPipelineState;
class FRHITexture;

// Vertex data structure (basic, used for unlit rendering)
struct FVertex 
{
    FVector Position;
    FColor Color;
};

// Lit vertex data structure (includes normal for lighting calculations)
struct FLitVertex
{
    FVector Position;
    FVector Normal;
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

/**
 * ERTFormat - Render texture format enumeration
 * Maps to underlying graphics API formats
 */
enum class ERTFormat
{
    R8G8B8A8_UNORM,     // Standard color format
    R16G16B16A16_FLOAT, // HDR color format
    R32_FLOAT,          // Single channel float (used for shadow maps)
    D32_FLOAT,          // 32-bit depth format (primary shadow format)
    D16_UNORM,          // 16-bit depth format (compact shadow format)
    D24_UNORM_S8_UINT,  // Depth-stencil format
};

// RHI Texture
class FRHITexture : public FRHIResource 
{
public:
    virtual ~FRHITexture() = default;
    
    // Get texture dimensions
    virtual uint32 GetWidth() const = 0;
    virtual uint32 GetHeight() const = 0;
    virtual uint32 GetArraySize() const = 0;
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
    
    // Draw indexed primitives with line list topology (for wireframe/debug rendering)
    virtual void DrawIndexedLines(uint32 IndexCount, uint32 StartIndex, uint32 BaseVertex) = 0;
    
    // Set primitive topology (for advanced use cases - prefer DrawIndexedLines for line rendering)
    virtual void SetPrimitiveTopology(bool bLineList = false) = 0;
    
    virtual void Present() = 0;
    
    // Flush 3D rendering commands before 2D overlay rendering
    // This ensures all 3D content is rendered before 2D text/UI is drawn on top
    virtual void FlushCommandsFor2D() = 0;
    
    // Text rendering - call FlushCommandsFor2D() before calling this
    virtual void RHIDrawText(const std::string& Text, const FVector2D& Position, float FontSize, const FColor& Color) = 0;
    
    // Debug texture visualization - draws a texture as a quad on screen
    // Position and size are in screen pixels. Call FlushCommandsFor2D() before calling this.
    virtual void DrawDebugTexture(FRHITexture* Texture, float X, float Y, float Width, float Height) = 0;
    
    // Shadow map rendering support
    // Begin rendering to a shadow map texture (sets depth-only render target)
    virtual void BeginShadowPass(FRHITexture* ShadowMap, uint32 FaceIndex = 0) = 0;
    
    // End shadow pass and restore main render target
    virtual void EndShadowPass() = 0;
    
    // Set viewport for shadow map rendering (used for atlas region selection)
    virtual void SetViewport(float X, float Y, float Width, float Height, float MinDepth = 0.0f, float MaxDepth = 1.0f) = 0;
    
    // Clear just the depth buffer (for shadow maps that share atlas)
    virtual void ClearDepthOnly(FRHITexture* DepthTexture, uint32 FaceIndex = 0) = 0;
    
    // GPU event markers for RenderDoc/PIX debugging
    // These create named events that appear in GPU profilers
    virtual void BeginEvent(const std::string& EventName) = 0;
    virtual void EndEvent() = 0;
    
    // Set inline root constants (avoids constant buffer synchronization issues)
    // Data is copied directly into the root signature at record time
    virtual void SetRootConstants(uint32 RootParameterIndex, uint32 Num32BitValues, const void* Data, uint32 DestOffset = 0) = 0;
    
    // Bind shadow map texture for shader sampling
    // Call this before rendering lit geometry that receives shadows
    virtual void SetShadowMapTexture(FRHITexture* ShadowMap) = 0;
};

// Pipeline state creation flags
enum class EPipelineFlags : uint32
{
    None = 0,
    EnableDepth = 1 << 0,
    EnableLighting = 1 << 1,
    WireframeMode = 1 << 2,
    LineTopology = 1 << 3,  // For wireframe line rendering
    EnableShadows = 1 << 4, // Enable shadow mapping (requires shadow map textures bound)
    DepthOnly = 1 << 5,     // Depth-only rendering for shadow pass
};

// Operator overloads for pipeline flags
inline EPipelineFlags operator|(EPipelineFlags a, EPipelineFlags b)
{
    return static_cast<EPipelineFlags>(static_cast<uint32>(a) | static_cast<uint32>(b));
}

inline EPipelineFlags operator&(EPipelineFlags a, EPipelineFlags b)
{
    return static_cast<EPipelineFlags>(static_cast<uint32>(a) & static_cast<uint32>(b));
}

inline bool HasFlag(EPipelineFlags flags, EPipelineFlags flag)
{
    return (static_cast<uint32>(flags) & static_cast<uint32>(flag)) != 0;
}

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
    
    // Texture creation for render targets and shadow maps
    virtual FRHITexture* CreateDepthTexture(uint32 Width, uint32 Height, ERTFormat Format, uint32 ArraySize = 1) = 0;
    
    // Legacy pipeline state creation (unlit)
    virtual FRHIPipelineState* CreateGraphicsPipelineState(bool bEnableDepth = false) = 0;
    
    // Extended pipeline state creation with flags
    virtual FRHIPipelineState* CreateGraphicsPipelineStateEx(EPipelineFlags Flags) = 0;
};

// Factory function to create platform-specific RHI
FRHI* CreateDX12RHI();
