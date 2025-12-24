#pragma once

#include "../RHI/RHI.h"
#include "d3dx12.h"
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <vector>

using Microsoft::WRL::ComPtr;

class FDX12Buffer : public FRHIBuffer {
public:
    FDX12Buffer(ID3D12Resource* InResource);
    virtual ~FDX12Buffer() override;
    
    virtual void* Map() override;
    virtual void Unmap() override;
    
    ID3D12Resource* GetResource() const { return Resource.Get(); }
    D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView() const { return VertexBufferView; }
    
private:
    ComPtr<ID3D12Resource> Resource;
    D3D12_VERTEX_BUFFER_VIEW VertexBufferView;
};

class FDX12PipelineState : public FRHIPipelineState {
public:
    FDX12PipelineState(ID3D12PipelineState* InPSO, ID3D12RootSignature* InRootSig);
    virtual ~FDX12PipelineState() override;
    
    ID3D12PipelineState* GetPSO() const { return PSO.Get(); }
    ID3D12RootSignature* GetRootSignature() const { return RootSignature.Get(); }
    
private:
    ComPtr<ID3D12PipelineState> PSO;
    ComPtr<ID3D12RootSignature> RootSignature;
};

class FDX12CommandList : public FRHICommandList {
public:
    FDX12CommandList(ID3D12Device* Device, ID3D12CommandQueue* Queue, IDXGISwapChain3* SwapChain);
    virtual ~FDX12CommandList() override;
    
    virtual void BeginFrame() override;
    virtual void EndFrame() override;
    virtual void ClearRenderTarget(const FColor& Color) override;
    virtual void SetPipelineState(FRHIPipelineState* PipelineState) override;
    virtual void SetVertexBuffer(FRHIBuffer* VertexBuffer, uint32 Offset, uint32 Stride) override;
    virtual void DrawPrimitive(uint32 VertexCount, uint32 StartVertex) override;
    virtual void Present() override;
    
private:
    void WaitForGPU();
    
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
    
    ComPtr<ID3D12Fence> Fence;
    uint64 FenceValue;
    void* FenceEvent;
    
    D3D12_VIEWPORT Viewport;
    D3D12_RECT ScissorRect;
};

class FDX12RHI : public FRHI {
public:
    FDX12RHI();
    virtual ~FDX12RHI() override;
    
    virtual bool Initialize(void* WindowHandle, uint32 Width, uint32 Height) override;
    virtual void Shutdown() override;
    
    virtual FRHICommandList* GetCommandList() override;
    virtual FRHIBuffer* CreateVertexBuffer(uint32 Size, const void* Data) override;
    virtual FRHIPipelineState* CreateGraphicsPipelineState() override;
    
    virtual void BeginFrame() override;
    virtual void EndFrame() override;
    
private:
    ComPtr<IDXGIFactory4> Factory;
    ComPtr<ID3D12Device> Device;
    ComPtr<ID3D12CommandQueue> CommandQueue;
    ComPtr<IDXGISwapChain3> SwapChain;
    
    std::unique_ptr<FDX12CommandList> CommandList;
    uint32 Width, Height;
};
