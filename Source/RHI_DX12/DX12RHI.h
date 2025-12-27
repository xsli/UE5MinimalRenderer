#pragma once

#include "../RHI/RHI.h"
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3d11on12.h>
#include <d2d1_3.h>
#include <dwrite.h>
#include <wrl.h>
#include <vector>
#include "d3dx12.h"

using Microsoft::WRL::ComPtr;

class FDX12Buffer : public FRHIBuffer 
{
public:
    enum class EBufferType 
    {
        Vertex,
        Index,
        Constant
    };
    
    FDX12Buffer(ID3D12Resource* InResource, EBufferType InType);
    virtual ~FDX12Buffer() override;
    
    virtual void* Map() override;
    virtual void Unmap() override;
    
    ID3D12Resource* GetResource() const { return Resource.Get(); }
    D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView() const { return VertexBufferView; }
    D3D12_INDEX_BUFFER_VIEW GetIndexBufferView() const { return IndexBufferView; }
    D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() const { return Resource->GetGPUVirtualAddress(); }
    
private:
    ComPtr<ID3D12Resource> Resource;
    EBufferType BufferType;
    D3D12_VERTEX_BUFFER_VIEW VertexBufferView;
    D3D12_INDEX_BUFFER_VIEW IndexBufferView;
};

/**
 * FDX12Texture - DirectX 12 texture implementation
 * Used for depth textures, render targets, and shadow maps
 */
class FDX12Texture : public FRHITexture
{
public:
    FDX12Texture(ID3D12Resource* InResource, uint32 InWidth, uint32 InHeight, 
                 uint32 InArraySize, DXGI_FORMAT InFormat,
                 ID3D12DescriptorHeap* InDSVHeap = nullptr, ID3D12DescriptorHeap* InSRVHeap = nullptr);
    virtual ~FDX12Texture() override;
    
    virtual uint32 GetWidth() const override { return Width; }
    virtual uint32 GetHeight() const override { return Height; }
    virtual uint32 GetArraySize() const override { return ArraySize; }
    
    ID3D12Resource* GetResource() const { return Resource.Get(); }
    DXGI_FORMAT GetFormat() const { return Format; }
    
    // Get descriptor heap for depth stencil views
    ID3D12DescriptorHeap* GetDSVHeap() const { return DSVHeap.Get(); }
    
    // Get descriptor heap for shader resource views
    ID3D12DescriptorHeap* GetSRVHeap() const { return SRVHeap.Get(); }
    
    // Get CPU descriptor handle for DSV
    D3D12_CPU_DESCRIPTOR_HANDLE GetDSVHandle(uint32 ArrayIndex = 0) const;
    
    // Get GPU descriptor handle for SRV (for shader sampling)
    D3D12_GPU_DESCRIPTOR_HANDLE GetSRVGPUHandle() const;
    
private:
    ComPtr<ID3D12Resource> Resource;
    ComPtr<ID3D12DescriptorHeap> DSVHeap;  // For rendering to depth
    ComPtr<ID3D12DescriptorHeap> SRVHeap;  // For sampling in shaders
    uint32 Width;
    uint32 Height;
    uint32 ArraySize;
    DXGI_FORMAT Format;
    uint32 DSVDescriptorSize;
    uint32 SRVDescriptorSize;
};

class FDX12PipelineState : public FRHIPipelineState 
{
public:
    FDX12PipelineState(ID3D12PipelineState* InPSO, ID3D12RootSignature* InRootSig);
    virtual ~FDX12PipelineState() override;
    
    ID3D12PipelineState* GetPSO() const { return PSO.Get(); }
    ID3D12RootSignature* GetRootSignature() const { return RootSignature.Get(); }
    
private:
    ComPtr<ID3D12PipelineState> PSO;
    ComPtr<ID3D12RootSignature> RootSignature;
};

class FDX12CommandList : public FRHICommandList 
{
public:
    FDX12CommandList(ID3D12Device* Device, ID3D12CommandQueue* Queue, IDXGISwapChain3* SwapChain, uint32 Width, uint32 Height);
    virtual ~FDX12CommandList() override;
    
    virtual void BeginFrame() override;
    virtual void EndFrame() override;
    virtual void ClearRenderTarget(const FColor& Color) override;
    virtual void ClearDepthStencil() override;
    virtual void SetPipelineState(FRHIPipelineState* PipelineState) override;
    virtual void SetVertexBuffer(FRHIBuffer* VertexBuffer, uint32 Offset, uint32 Stride) override;
    virtual void SetIndexBuffer(FRHIBuffer* IndexBuffer) override;
    virtual void SetConstantBuffer(FRHIBuffer* ConstantBuffer, uint32 RootParameterIndex) override;
    virtual void DrawPrimitive(uint32 VertexCount, uint32 StartVertex) override;
    virtual void DrawIndexedPrimitive(uint32 IndexCount, uint32 StartIndex, uint32 BaseVertex) override;
    virtual void DrawIndexedLines(uint32 IndexCount, uint32 StartIndex, uint32 BaseVertex) override;
    virtual void SetPrimitiveTopology(bool bLineList = false) override;
    virtual void Present() override;
    virtual void FlushCommandsFor2D() override;
    virtual void RHIDrawText(const std::string& Text, const FVector2D& Position, float FontSize, const FColor& Color) override;
    
    // Debug texture visualization
    virtual void DrawDebugTexture(FRHITexture* Texture, float X, float Y, float Width, float Height) override;
    
    // Shadow map support
    virtual void BeginShadowPass(FRHITexture* ShadowMap, uint32 FaceIndex = 0) override;
    virtual void EndShadowPass() override;
    virtual void SetViewport(float X, float Y, float Width, float Height, float MinDepth = 0.0f, float MaxDepth = 1.0f) override;
    virtual void ClearDepthOnly(FRHITexture* DepthTexture, uint32 FaceIndex = 0) override;
    
    // GPU event markers for RenderDoc/PIX debugging
    virtual void BeginEvent(const std::string& EventName) override;
    virtual void EndEvent() override;
    
    // Set inline root constants
    virtual void SetRootConstants(uint32 RootParameterIndex, uint32 Num32BitValues, const void* Data, uint32 DestOffset = 0) override;
    
    void InitializeTextRendering(ID3D12Device* Device, IDXGISwapChain3* SwapChain);
    
private:
    void WaitForGPU();
    void CreateDepthStencilBuffer(uint32 Width, uint32 Height);
    
    ComPtr<ID3D12Device> Device;
    ComPtr<ID3D12CommandQueue> CommandQueue;
    ComPtr<ID3D12CommandAllocator> CommandAllocator;
    ComPtr<ID3D12GraphicsCommandList> GraphicsCommandList;
    ComPtr<IDXGISwapChain3> SwapChain;
    
    static const uint32 FrameCount = 2;
    ComPtr<ID3D12Resource> RenderTargets[FrameCount];
    ComPtr<ID3D12DescriptorHeap> RTVHeap;
    uint32 RTVDescriptorSize;
    uint32 FrameIndex;
    
    // Depth buffer
    ComPtr<ID3D12Resource> DepthStencilBuffer;
    ComPtr<ID3D12DescriptorHeap> DSVHeap;
    
    ComPtr<ID3D12Fence> Fence;
    uint64 FenceValue;
    void* FenceEvent;
    
    D3D12_VIEWPORT Viewport;
    D3D12_RECT ScissorRect;
    
    // D2D/DWrite resources for text rendering
    ComPtr<ID3D11Device> D3D11Device;
    ComPtr<ID3D11DeviceContext> D3D11DeviceContext;
    ComPtr<ID3D11On12Device> D3D11On12Device;
    ComPtr<ID2D1Factory3> D2DFactory;
    ComPtr<ID2D1Device2> D2DDevice;
    ComPtr<ID2D1DeviceContext2> D2DDeviceContext;
    ComPtr<IDWriteFactory> DWriteFactory;
    ComPtr<ID3D11Resource> WrappedBackBuffers[FrameCount];
    ComPtr<ID2D1Bitmap1> D2DRenderTargets[FrameCount];
    
    // Track whether 3D commands have been flushed for 2D rendering
    bool bCommandsFlushedFor2D;
    
    // Shadow pass state
    bool bInShadowPass;
    FDX12Texture* CurrentShadowMap;
    D3D12_VIEWPORT SavedViewport;
    D3D12_RECT SavedScissorRect;
};

class FDX12RHI : public FRHI 
{
public:
    FDX12RHI();
    virtual ~FDX12RHI() override;
    
    virtual bool Initialize(void* WindowHandle, uint32 Width, uint32 Height) override;
    virtual void Shutdown() override;
    
    virtual FRHICommandList* GetCommandList() override;
    virtual FRHIBuffer* CreateVertexBuffer(uint32 Size, const void* Data) override;
    virtual FRHIBuffer* CreateIndexBuffer(uint32 Size, const void* Data) override;
    virtual FRHIBuffer* CreateConstantBuffer(uint32 Size) override;
    virtual FRHITexture* CreateDepthTexture(uint32 Width, uint32 Height, ERTFormat Format, uint32 ArraySize = 1) override;
    virtual FRHIPipelineState* CreateGraphicsPipelineState(bool bEnableDepth = false) override;
    virtual FRHIPipelineState* CreateGraphicsPipelineStateEx(EPipelineFlags Flags) override;
    
private:
    ComPtr<IDXGIFactory4> Factory;
    ComPtr<ID3D12Device> Device;
    ComPtr<ID3D12CommandQueue> CommandQueue;
    ComPtr<IDXGISwapChain3> SwapChain;
    
    std::unique_ptr<FDX12CommandList> CommandList;
    uint32 Width, Height;
};
