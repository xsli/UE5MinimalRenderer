#include "DX12RHI.h"
#include "d3dx12.h"
#include "../Core/ShaderLoader.h"
#include <d3dcompiler.h>
#include <stdexcept>
#include <cstring>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

// Helper function for checking HRESULT
inline void ThrowIfFailed(HRESULT hr)
{
    if (FAILED(hr))
    {
        throw std::runtime_error("DirectX 12 operation failed");
    }
}

// FDX12Buffer implementation
FDX12Buffer::FDX12Buffer(ID3D12Resource* InResource, EBufferType InType)
    : Resource(InResource), BufferType(InType)
{
    
    D3D12_RESOURCE_DESC desc = Resource->GetDesc();
    
    if (BufferType == EBufferType::Vertex)
    {
        VertexBufferView.BufferLocation = Resource->GetGPUVirtualAddress();
        VertexBufferView.SizeInBytes = static_cast<UINT>(desc.Width);
        VertexBufferView.StrideInBytes = sizeof(FVertex);
        
        FLog::Log(ELogLevel::Info, std::string("FDX12Buffer (Vertex) created - GPU Address: 0x") + 
            std::to_string(VertexBufferView.BufferLocation) + 
            ", Size: " + std::to_string(VertexBufferView.SizeInBytes) + 
            ", Stride: " + std::to_string(VertexBufferView.StrideInBytes));
    }
    else if (BufferType == EBufferType::Index)
    {
        IndexBufferView.BufferLocation = Resource->GetGPUVirtualAddress();
        IndexBufferView.SizeInBytes = static_cast<UINT>(desc.Width);
        IndexBufferView.Format = DXGI_FORMAT_R32_UINT;  // 32-bit indices
        
        FLog::Log(ELogLevel::Info, std::string("FDX12Buffer (Index) created - GPU Address: 0x") + 
            std::to_string(IndexBufferView.BufferLocation) + 
            ", Size: " + std::to_string(IndexBufferView.SizeInBytes));
    }
    else if (BufferType == EBufferType::Constant)
    {
        FLog::Log(ELogLevel::Info, std::string("FDX12Buffer (Constant) created - GPU Address: 0x") + 
            std::to_string(Resource->GetGPUVirtualAddress()) + 
            ", Size: " + std::to_string(desc.Width));
    }
}

FDX12Buffer::~FDX12Buffer()
{
}

void* FDX12Buffer::Map()
{
    void* pData = nullptr;
    CD3DX12_RANGE readRange(0, 0);
    ThrowIfFailed(Resource->Map(0, &readRange, &pData));
    return pData;
}

void FDX12Buffer::Unmap()
{
    Resource->Unmap(0, nullptr);
}

// FDX12Texture implementation
FDX12Texture::FDX12Texture(ID3D12Resource* InResource, uint32 InWidth, uint32 InHeight, 
                           uint32 InArraySize, DXGI_FORMAT InFormat,
                           ID3D12DescriptorHeap* InDSVHeap, ID3D12DescriptorHeap* InSRVHeap)
    : Resource(InResource)
    , DSVHeap(InDSVHeap)
    , SRVHeap(InSRVHeap)
    , Width(InWidth)
    , Height(InHeight)
    , ArraySize(InArraySize)
    , Format(InFormat)
    , DSVDescriptorSize(0)
    , SRVDescriptorSize(0)
{
    // Get descriptor sizes from device
    ComPtr<ID3D12Device> device;
    if (Resource)
    {
        Resource->GetDevice(IID_PPV_ARGS(&device));
        if (device)
        {
            DSVDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
            SRVDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        }
    }
    
    FLog::Log(ELogLevel::Info, "FDX12Texture created: " + std::to_string(Width) + "x" + std::to_string(Height) + 
              " ArraySize=" + std::to_string(ArraySize));
}

FDX12Texture::~FDX12Texture()
{
    FLog::Log(ELogLevel::Info, "FDX12Texture destroyed");
}

D3D12_CPU_DESCRIPTOR_HANDLE FDX12Texture::GetDSVHandle(uint32 ArrayIndex) const
{
    if (!DSVHeap)
    {
        return {};
    }
    
    D3D12_CPU_DESCRIPTOR_HANDLE handle = DSVHeap->GetCPUDescriptorHandleForHeapStart();
    if (ArrayIndex > 0 && ArrayIndex < ArraySize)
    {
        handle.ptr += ArrayIndex * DSVDescriptorSize;
    }
    return handle;
}

D3D12_GPU_DESCRIPTOR_HANDLE FDX12Texture::GetSRVGPUHandle() const
{
    if (!SRVHeap)
    {
        return {};
    }
    return SRVHeap->GetGPUDescriptorHandleForHeapStart();
}

// FDX12PipelineState implementation
FDX12PipelineState::FDX12PipelineState(ID3D12PipelineState* InPSO, ID3D12RootSignature* InRootSig)
    : PSO(InPSO), RootSignature(InRootSig)
{
}

FDX12PipelineState::~FDX12PipelineState()
{
}

// FDX12CommandList implementation
FDX12CommandList::FDX12CommandList(ID3D12Device* InDevice, ID3D12CommandQueue* InQueue, IDXGISwapChain3* InSwapChain, uint32 Width, uint32 Height)
    : Device(InDevice), CommandQueue(InQueue), SwapChain(InSwapChain), FrameIndex(0), FenceValue(0)
    , bCommandsFlushedFor2D(false), bInShadowPass(false), CurrentShadowMap(nullptr)
    , SavedViewport{}, SavedScissorRect{}
{
    
    // Create command allocator
    ThrowIfFailed(Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&CommandAllocator)));
    
    // Create command list
    ThrowIfFailed(Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, CommandAllocator.Get(), nullptr, IID_PPV_ARGS(&GraphicsCommandList)));
    ThrowIfFailed(GraphicsCommandList->Close());
    
    // Create RTV descriptor heap
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
    rtvHeapDesc.NumDescriptors = FrameCount;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    ThrowIfFailed(Device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&RTVHeap)));
    
    RTVDescriptorSize = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    
    // Create render target views
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(RTVHeap->GetCPUDescriptorHandleForHeapStart());
    for (uint32 i = 0; i < FrameCount; i++)
    {
        ThrowIfFailed(SwapChain->GetBuffer(i, IID_PPV_ARGS(&RenderTargets[i])));
        Device->CreateRenderTargetView(RenderTargets[i].Get(), nullptr, rtvHandle);
        rtvHandle.Offset(1, RTVDescriptorSize);
    }
    
    // Get swap chain description for viewport/scissor
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc;
    SwapChain->GetDesc1(&swapChainDesc);
    
    // Set up viewport
    Viewport.TopLeftX = 0.0f;
    Viewport.TopLeftY = 0.0f;
    Viewport.Width = static_cast<float>(swapChainDesc.Width);
    Viewport.Height = static_cast<float>(swapChainDesc.Height);
    Viewport.MinDepth = 0.0f;
    Viewport.MaxDepth = 1.0f;
    
    // Set up scissor rect
    ScissorRect.left = 0;
    ScissorRect.top = 0;
    ScissorRect.right = static_cast<LONG>(swapChainDesc.Width);
    ScissorRect.bottom = static_cast<LONG>(swapChainDesc.Height);
    
    // Create synchronization objects
    ThrowIfFailed(Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&Fence)));
    FenceValue = 1;
    
    FenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (FenceEvent == nullptr)
    {
        ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
    }
    
    // Create depth stencil buffer
    CreateDepthStencilBuffer(Width, Height);
    
    // Initialize text rendering
    InitializeTextRendering(InDevice, InSwapChain);
}

FDX12CommandList::~FDX12CommandList()
{
    WaitForGPU();
    CloseHandle(FenceEvent);
}

void FDX12CommandList::BeginFrame()
{
    FrameIndex = SwapChain->GetCurrentBackBufferIndex();
    
    FLog::Log(ELogLevel::Info, std::string("BeginFrame - Frame Index: ") + std::to_string(FrameIndex));
    
    ThrowIfFailed(CommandAllocator->Reset());
    ThrowIfFailed(GraphicsCommandList->Reset(CommandAllocator.Get(), nullptr));
    
    // Set viewport and scissor rect
    GraphicsCommandList->RSSetViewports(1, &Viewport);
    GraphicsCommandList->RSSetScissorRects(1, &ScissorRect);
    
    FLog::Log(ELogLevel::Info, std::string("Viewport: ") + std::to_string(Viewport.Width) + "x" + std::to_string(Viewport.Height));
    
    // Transition render target to render target state
    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        RenderTargets[FrameIndex].Get(),
        D3D12_RESOURCE_STATE_PRESENT,
        D3D12_RESOURCE_STATE_RENDER_TARGET);
    GraphicsCommandList->ResourceBarrier(1, &barrier);
}

void FDX12CommandList::EndFrame()
{
	// Transition render target to present state
	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		RenderTargets[FrameIndex].Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PRESENT);
	GraphicsCommandList->ResourceBarrier(1, &barrier);

	// Close and execute the command list
	ThrowIfFailed(GraphicsCommandList->Close());
	ID3D12CommandList* ppCommandLists[] = { GraphicsCommandList.Get() };
	CommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
}

void FDX12CommandList::ClearRenderTarget(const FColor& Color)
{
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(RTVHeap->GetCPUDescriptorHandleForHeapStart(), FrameIndex, RTVDescriptorSize);
    
    const float clearColor[] = { Color.R, Color.G, Color.B, Color.A };
    FLog::Log(ELogLevel::Info, std::string("ClearRenderTarget - Color: ") + 
        std::to_string(Color.R) + ", " + std::to_string(Color.G) + ", " + std::to_string(Color.B));
    
    GraphicsCommandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
    
    // Set render target with depth stencil
    if (DSVHeap)
    {
        CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(DSVHeap->GetCPUDescriptorHandleForHeapStart());
        GraphicsCommandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
    }
    else
    {
        GraphicsCommandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);
    }
}

void FDX12CommandList::ClearDepthStencil()
{
    if (DSVHeap)
    {
        CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(DSVHeap->GetCPUDescriptorHandleForHeapStart());
        GraphicsCommandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
        FLog::Log(ELogLevel::Info, "ClearDepthStencil called");
    }
}

void FDX12CommandList::SetPipelineState(FRHIPipelineState* PipelineState)
{
    FLog::Log(ELogLevel::Info, "SetPipelineState called");
    FDX12PipelineState* DX12PSO = static_cast<FDX12PipelineState*>(PipelineState);
    GraphicsCommandList->SetPipelineState(DX12PSO->GetPSO());
    GraphicsCommandList->SetGraphicsRootSignature(DX12PSO->GetRootSignature());
}

void FDX12CommandList::SetVertexBuffer(FRHIBuffer* VertexBuffer, uint32 Offset, uint32 Stride)
{
    FLog::Log(ELogLevel::Info, std::string("SetVertexBuffer - Stride: ") + std::to_string(Stride));
    FDX12Buffer* DX12Buffer = static_cast<FDX12Buffer*>(VertexBuffer);
    
    // Create a vertex buffer view with the specified stride
    // (don't use the stored view which has a fixed stride)
    D3D12_VERTEX_BUFFER_VIEW vbv = {};
    vbv.BufferLocation = DX12Buffer->GetGPUVirtualAddress();
    vbv.SizeInBytes = DX12Buffer->GetVertexBufferView().SizeInBytes;
    vbv.StrideInBytes = Stride;  // Use the caller-specified stride
    
    FLog::Log(ELogLevel::Info, std::string("  VBV - Location: 0x") + 
        std::to_string(vbv.BufferLocation) + 
        ", Size: " + std::to_string(vbv.SizeInBytes) + 
        ", Stride: " + std::to_string(vbv.StrideInBytes));
    GraphicsCommandList->IASetVertexBuffers(0, 1, &vbv);
}

void FDX12CommandList::SetIndexBuffer(FRHIBuffer* IndexBuffer)
{
    FLog::Log(ELogLevel::Info, "SetIndexBuffer called");
    FDX12Buffer* DX12Buffer = static_cast<FDX12Buffer*>(IndexBuffer);
    D3D12_INDEX_BUFFER_VIEW ibv = DX12Buffer->GetIndexBufferView();
    GraphicsCommandList->IASetIndexBuffer(&ibv);
}

void FDX12CommandList::SetConstantBuffer(FRHIBuffer* ConstantBuffer, uint32 RootParameterIndex)
{
    FLog::Log(ELogLevel::Info, std::string("SetConstantBuffer - Root Parameter: ") + std::to_string(RootParameterIndex));
    FDX12Buffer* DX12Buffer = static_cast<FDX12Buffer*>(ConstantBuffer);
    GraphicsCommandList->SetGraphicsRootConstantBufferView(RootParameterIndex, DX12Buffer->GetGPUVirtualAddress());
}

void FDX12CommandList::DrawPrimitive(uint32 VertexCount, uint32 StartVertex)
{
    FLog::Log(ELogLevel::Info, std::string("DrawPrimitive - VertexCount: ") + std::to_string(VertexCount) + 
        ", StartVertex: " + std::to_string(StartVertex));
    
    // Set triangle list topology (required before draw)
    GraphicsCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    GraphicsCommandList->DrawInstanced(VertexCount, 1, StartVertex, 0);
}

void FDX12CommandList::DrawIndexedPrimitive(uint32 IndexCount, uint32 StartIndex, uint32 BaseVertex)
{
    FLog::Log(ELogLevel::Info, std::string("DrawIndexedPrimitive - IndexCount: ") + std::to_string(IndexCount) + 
        ", StartIndex: " + std::to_string(StartIndex) + ", BaseVertex: " + std::to_string(BaseVertex));
    
    // Set triangle list topology (required before draw)
    // Note: If line topology is needed, use DrawIndexedLines instead
    GraphicsCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    GraphicsCommandList->DrawIndexedInstanced(IndexCount, 1, StartIndex, BaseVertex, 0);
}

void FDX12CommandList::DrawIndexedLines(uint32 IndexCount, uint32 StartIndex, uint32 BaseVertex)
{
    FLog::Log(ELogLevel::Info, std::string("DrawIndexedLines - IndexCount: ") + std::to_string(IndexCount) + 
        ", StartIndex: " + std::to_string(StartIndex) + ", BaseVertex: " + std::to_string(BaseVertex));
    
    // Set line list topology for wireframe rendering
    GraphicsCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
    GraphicsCommandList->DrawIndexedInstanced(IndexCount, 1, StartIndex, BaseVertex, 0);
}

void FDX12CommandList::SetPrimitiveTopology(bool bLineList)
{
    if (bLineList)
    {
        GraphicsCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
    }
    else
    {
        GraphicsCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    }
}

void FDX12CommandList::Present()
{
    FLog::Log(ELogLevel::Info, "Presenting frame...");
    // disable vsync
    ThrowIfFailed(SwapChain->Present(0, 0));
    WaitForGPU();
    FLog::Log(ELogLevel::Info, "Frame presented");
}

void FDX12CommandList::FlushCommandsFor2D()
{
	try
	{
		// Close and execute D3D12 command list
		ThrowIfFailed(GraphicsCommandList->Close());
		ID3D12CommandList* ppCommandLists[] = { GraphicsCommandList.Get() };
		CommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

		// Wait for D3D12 to finish rendering
		const UINT64 fenceValueForD2D = FenceValue++;
		ThrowIfFailed(CommandQueue->Signal(Fence.Get(), fenceValueForD2D));
		if (Fence->GetCompletedValue() < fenceValueForD2D)
		{
			ThrowIfFailed(Fence->SetEventOnCompletion(fenceValueForD2D, FenceEvent));
			WaitForSingleObjectEx(FenceEvent, INFINITE, FALSE);
		}

		// Reset command list for potential additional commands
		ThrowIfFailed(CommandAllocator->Reset());
		ThrowIfFailed(GraphicsCommandList->Reset(CommandAllocator.Get(), nullptr));

		// Reset viewport and scissor since we reset the command list
		GraphicsCommandList->RSSetViewports(1, &Viewport);
		GraphicsCommandList->RSSetScissorRects(1, &ScissorRect);
	}
	catch (const std::exception& e)
	{
		FLog::Log(ELogLevel::Error, std::string("FlushCommandsFor2D error: ") + e.what());
		throw;
	}
}

void FDX12CommandList::CreateDepthStencilBuffer(uint32 Width, uint32 Height)
{
    FLog::Log(ELogLevel::Info, "Creating depth stencil buffer...");
    
    // Create depth stencil descriptor heap
    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
    dsvHeapDesc.NumDescriptors = 1;
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    ThrowIfFailed(Device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&DSVHeap)));
    
    // Create depth stencil texture
    CD3DX12_HEAP_PROPERTIES depthHeapProps(D3D12_HEAP_TYPE_DEFAULT);
    CD3DX12_RESOURCE_DESC depthResourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(
        DXGI_FORMAT_D32_FLOAT,
        Width,
        Height,
        1, // Array size
        1  // Mip levels
    );
    depthResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
    
    D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
    depthOptimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
    depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
    depthOptimizedClearValue.DepthStencil.Stencil = 0;
    
    ThrowIfFailed(Device->CreateCommittedResource(
        &depthHeapProps,
        D3D12_HEAP_FLAG_NONE,
        &depthResourceDesc,
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &depthOptimizedClearValue,
        IID_PPV_ARGS(&DepthStencilBuffer)
    ));
    
    // Create depth stencil view
    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
    
    Device->CreateDepthStencilView(DepthStencilBuffer.Get(), &dsvDesc, DSVHeap->GetCPUDescriptorHandleForHeapStart());
    
    FLog::Log(ELogLevel::Info, "Depth stencil buffer created successfully");
}

void FDX12CommandList::BeginShadowPass(FRHITexture* ShadowMap, uint32 FaceIndex)
{
    FLog::Log(ELogLevel::Info, "BeginShadowPass - FaceIndex: " + std::to_string(FaceIndex));
    
    if (!ShadowMap)
    {
        FLog::Log(ELogLevel::Error, "BeginShadowPass: ShadowMap is null");
        return;
    }
    
    FDX12Texture* DX12ShadowMap = static_cast<FDX12Texture*>(ShadowMap);
    
    // Save current state
    bInShadowPass = true;
    CurrentShadowMap = DX12ShadowMap;
    SavedViewport = Viewport;
    SavedScissorRect = ScissorRect;
    
    // Transition shadow map to depth write state if needed
    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        DX12ShadowMap->GetResource(),
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        D3D12_RESOURCE_STATE_DEPTH_WRITE);
    GraphicsCommandList->ResourceBarrier(1, &barrier);
    
    // Get DSV handle for the specified face
    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = DX12ShadowMap->GetDSVHandle(FaceIndex);
    
    // Set depth-only render target (no color target)
    GraphicsCommandList->OMSetRenderTargets(0, nullptr, FALSE, &dsvHandle);
    
    // Clear the depth buffer
    GraphicsCommandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
    
    FLog::Log(ELogLevel::Info, "BeginShadowPass: Render target set to shadow map");
}

void FDX12CommandList::EndShadowPass()
{
    FLog::Log(ELogLevel::Info, "EndShadowPass");
    
    if (!bInShadowPass || !CurrentShadowMap)
    {
        FLog::Log(ELogLevel::Warning, "EndShadowPass: Not in shadow pass");
        return;
    }
    
    // Transition shadow map back to shader resource state
    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        CurrentShadowMap->GetResource(),
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    GraphicsCommandList->ResourceBarrier(1, &barrier);
    
    // Restore viewport and scissor
    Viewport = SavedViewport;
    ScissorRect = SavedScissorRect;
    GraphicsCommandList->RSSetViewports(1, &Viewport);
    GraphicsCommandList->RSSetScissorRects(1, &ScissorRect);
    
    // Restore main render target
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(RTVHeap->GetCPUDescriptorHandleForHeapStart(), FrameIndex, RTVDescriptorSize);
    if (DSVHeap)
    {
        CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(DSVHeap->GetCPUDescriptorHandleForHeapStart());
        GraphicsCommandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
    }
    else
    {
        GraphicsCommandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);
    }
    
    bInShadowPass = false;
    CurrentShadowMap = nullptr;
    
    FLog::Log(ELogLevel::Info, "EndShadowPass: Main render target restored");
}

void FDX12CommandList::SetViewport(float X, float Y, float Width, float Height, float MinDepth, float MaxDepth)
{
    D3D12_VIEWPORT vp = {};
    vp.TopLeftX = X;
    vp.TopLeftY = Y;
    vp.Width = Width;
    vp.Height = Height;
    vp.MinDepth = MinDepth;
    vp.MaxDepth = MaxDepth;
    
    GraphicsCommandList->RSSetViewports(1, &vp);
    
    D3D12_RECT scissor = {};
    scissor.left = static_cast<LONG>(X);
    scissor.top = static_cast<LONG>(Y);
    scissor.right = static_cast<LONG>(X + Width);
    scissor.bottom = static_cast<LONG>(Y + Height);
    
    GraphicsCommandList->RSSetScissorRects(1, &scissor);
}

void FDX12CommandList::ClearDepthOnly(FRHITexture* DepthTexture, uint32 FaceIndex)
{
    if (!DepthTexture)
    {
        FLog::Log(ELogLevel::Error, "ClearDepthOnly: DepthTexture is null");
        return;
    }
    
    FDX12Texture* DX12Texture = static_cast<FDX12Texture*>(DepthTexture);
    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = DX12Texture->GetDSVHandle(FaceIndex);
    
    GraphicsCommandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
}

void FDX12CommandList::BeginEvent(const std::string& EventName)
{
    // PIXBeginEvent is preferred but requires PIX headers
    // For RenderDoc compatibility, use SetMarker with ID3D12GraphicsCommandList
    // Using a simple approach: SetMarker followed by the marker being visible in RenderDoc
    
    // Convert to wide string for D3D12 marker API
    std::wstring wideEventName(EventName.begin(), EventName.end());
    
    // Use PIXSetMarker via command list (requires D3D12_MESSAGE_ID_PIX_API)
    // For broader compatibility, we use the standard D3D12 debug layer marker
#ifdef _DEBUG
    // In debug mode, this marker will be visible in RenderDoc and GPU profilers
    GraphicsCommandList->BeginEvent(0, wideEventName.c_str(), static_cast<UINT>((wideEventName.length() + 1) * sizeof(wchar_t)));
#else
    (void)wideEventName;  // Avoid unused variable warning in release
#endif
}

void FDX12CommandList::EndEvent()
{
#ifdef _DEBUG
    GraphicsCommandList->EndEvent();
#endif
}

void FDX12CommandList::WaitForGPU()
{
    const UINT64 currentFenceValue = FenceValue;
    ThrowIfFailed(CommandQueue->Signal(Fence.Get(), currentFenceValue));
    FenceValue++;
    
    if (Fence->GetCompletedValue() < currentFenceValue)
    {
        ThrowIfFailed(Fence->SetEventOnCompletion(currentFenceValue, FenceEvent));
        WaitForSingleObjectEx(FenceEvent, INFINITE, FALSE);
    }
}

void FDX12CommandList::SetRootConstants(uint32 RootParameterIndex, uint32 Num32BitValues, const void* Data, uint32 DestOffset)
{
    // Set inline root constants directly in the root signature
    // This copies data at record time, avoiding constant buffer sync issues
    GraphicsCommandList->SetGraphicsRoot32BitConstants(RootParameterIndex, Num32BitValues, Data, DestOffset);
}

void FDX12CommandList::SetShadowMapTexture(FRHITexture* ShadowMap)
{
    if (!ShadowMap) return;
    
    FDX12Texture* dx12Texture = static_cast<FDX12Texture*>(ShadowMap);
    ID3D12DescriptorHeap* srvHeap = dx12Texture->GetSRVHeap();
    
    if (!srvHeap)
    {
        FLog::Log(ELogLevel::Warning, "SetShadowMapTexture: Shadow map has no SRV heap");
        return;
    }
    
    // Set the descriptor heap for the shadow map SRV
    ID3D12DescriptorHeap* heaps[] = { srvHeap };
    GraphicsCommandList->SetDescriptorHeaps(1, heaps);
    
    // Set the descriptor table for the shadow map (root parameter index 3 for lit shader)
    GraphicsCommandList->SetGraphicsRootDescriptorTable(3, dx12Texture->GetSRVGPUHandle());
}

void FDX12CommandList::InitializeTextRendering(ID3D12Device* InDevice, IDXGISwapChain3* InSwapChain)
{
    FLog::Log(ELogLevel::Info, "Initializing text rendering (D2D/DWrite)...");
    
    try
    {
        // Create D3D11 device
        ComPtr<ID3D11Device> d3d11Device;
        D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_0 };
        UINT deviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
        
        ThrowIfFailed(D3D11CreateDevice(
            nullptr,
            D3D_DRIVER_TYPE_HARDWARE,
            nullptr,
            deviceFlags,
            featureLevels,
            _countof(featureLevels),
            D3D11_SDK_VERSION,
            &d3d11Device,
            nullptr,
            &D3D11DeviceContext
        ));
        
        ThrowIfFailed(d3d11Device.As(&D3D11Device));
        
        // Create D3D11On12 device
        ComPtr<IUnknown> d3d12CommandQueue;
        ThrowIfFailed(CommandQueue.As(&d3d12CommandQueue));
        
        IUnknown* ppQueues[] = { d3d12CommandQueue.Get() };
        ThrowIfFailed(D3D11On12CreateDevice(
            InDevice,
            deviceFlags,
            featureLevels,
            _countof(featureLevels),
            ppQueues,
            _countof(ppQueues),
            0,
            &d3d11Device,
            &D3D11DeviceContext,
            nullptr
        ));
        
        ThrowIfFailed(d3d11Device.As(&D3D11On12Device));
        
        // Create D2D factory
        D2D1_FACTORY_OPTIONS factoryOptions = {};
        ThrowIfFailed(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, __uuidof(ID2D1Factory3), &factoryOptions, &D2DFactory));
        
        // Create D2D device
        ComPtr<IDXGIDevice> dxgiDevice;
        ThrowIfFailed(D3D11On12Device.As(&dxgiDevice));
        ThrowIfFailed(D2DFactory->CreateDevice(dxgiDevice.Get(), &D2DDevice));
        
        // Create D2D device context
        ThrowIfFailed(D2DDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &D2DDeviceContext));
        
        // Create DWrite factory
        ThrowIfFailed(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), &DWriteFactory));
        
        // Wrap D3D12 render targets for D3D11
        D3D11_RESOURCE_FLAGS d3d11Flags = { D3D11_BIND_RENDER_TARGET };
        for (uint32 i = 0; i < FrameCount; i++)
        {
            ThrowIfFailed(D3D11On12Device->CreateWrappedResource(
                RenderTargets[i].Get(),
                &d3d11Flags,
                D3D12_RESOURCE_STATE_RENDER_TARGET,
                D3D12_RESOURCE_STATE_RENDER_TARGET,
                IID_PPV_ARGS(&WrappedBackBuffers[i])
            ));
            
            // Create D2D bitmap render target
            ComPtr<IDXGISurface> dxgiSurface;
            ThrowIfFailed(WrappedBackBuffers[i].As(&dxgiSurface));
            
            D2D1_BITMAP_PROPERTIES1 bitmapProperties = D2D1::BitmapProperties1(
                D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
                D2D1::PixelFormat(DXGI_FORMAT_R8G8B8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)
            );
            
            ThrowIfFailed(D2DDeviceContext->CreateBitmapFromDxgiSurface(
                dxgiSurface.Get(),
                &bitmapProperties,
                &D2DRenderTargets[i]
            ));
        }
        
        FLog::Log(ELogLevel::Info, "Text rendering initialized successfully");
    }
	catch (const std::exception& e)
	{
		FLog::Log(ELogLevel::Error, std::string("Failed to initialize text rendering: ") + e.what());
	}
}

void FDX12CommandList::RHIDrawText(const std::string& Text, const FVector2D& Position, float FontSize, const FColor& Color)
{
	if (!D2DDeviceContext || !DWriteFactory)
	{
		FLog::Log(ELogLevel::Warning, "D2D/DWrite not initialized");
		return;
	}

	try
	{
		FLog::Log(ELogLevel::Info, std::string("RHIDrawText: '") + Text + "'");

		// Now it's safe to use D2D
		D3D11On12Device->AcquireWrappedResources(WrappedBackBuffers[FrameIndex].GetAddressOf(), 1);

		D2DDeviceContext->SetTarget(D2DRenderTargets[FrameIndex].Get());
		D2DDeviceContext->BeginDraw();

		// Create text format
		ComPtr<IDWriteTextFormat> textFormat;
		ThrowIfFailed(DWriteFactory->CreateTextFormat(
			L"Arial", nullptr,
			DWRITE_FONT_WEIGHT_BOLD,
			DWRITE_FONT_STYLE_NORMAL,
			DWRITE_FONT_STRETCH_NORMAL,
			FontSize, L"en-us", &textFormat));

		// Create brush
		ComPtr<ID2D1SolidColorBrush> brush;
		ThrowIfFailed(D2DDeviceContext->CreateSolidColorBrush(
			D2D1::ColorF(Color.R, Color.G, Color.B, Color.A), &brush));

		// Draw text
		std::wstring wText(Text.begin(), Text.end());
		D2D1_RECT_F layoutRect = D2D1::RectF(Position.X, Position.Y, Position.X + 1000.0f, Position.Y + FontSize * 2.0f);
		D2DDeviceContext->DrawText(wText.c_str(), static_cast<UINT32>(wText.length()), textFormat.Get(), &layoutRect, brush.Get());

		ThrowIfFailed(D2DDeviceContext->EndDraw());

		D3D11On12Device->ReleaseWrappedResources(WrappedBackBuffers[FrameIndex].GetAddressOf(), 1);
		D3D11DeviceContext->Flush();

		// NOTE: No need to reset D3D12 command list here
	    // - FlushCommandsFor2D() already reset it before text rendering
	    // - D2D uses D3D11On12, which has its own command submission path
	    // - D3D12 command list stays open for EndFrame's resource barrier

		FLog::Log(ELogLevel::Info, "Text rendered successfully");
	}
	catch (const std::exception& e)
	{
		FLog::Log(ELogLevel::Error, std::string("DrawText error: ") + e.what());
	}
}

void FDX12CommandList::DrawDebugTexture(FRHITexture* Texture, float X, float Y, float Width, float Height)
{
    if (!D2DDeviceContext || !Texture)
    {
        return;
    }
    
    FDX12Texture* dx12Texture = static_cast<FDX12Texture*>(Texture);
    
    try
    {
        // Draw debug texture visualization as an opaque rectangle with texture info
        D3D11On12Device->AcquireWrappedResources(WrappedBackBuffers[FrameIndex].GetAddressOf(), 1);
        
        D2DDeviceContext->SetTarget(D2DRenderTargets[FrameIndex].Get());
        D2DDeviceContext->BeginDraw();
        
        // Draw an opaque black background for the debug texture area
        ComPtr<ID2D1SolidColorBrush> bgBrush;
        D2DDeviceContext->CreateSolidColorBrush(D2D1::ColorF(0.0f, 0.0f, 0.0f, 1.0f), &bgBrush);
        
        D2D1_RECT_F rect = D2D1::RectF(X, Y, X + Width, Y + Height);
        D2DDeviceContext->FillRectangle(rect, bgBrush.Get());
        
        // Draw yellow border
        ComPtr<ID2D1SolidColorBrush> borderBrush;
        D2DDeviceContext->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 0.0f, 1.0f), &borderBrush);
        D2DDeviceContext->DrawRectangle(rect, borderBrush.Get(), 2.0f);
        
        // Draw "Shadow Map" header text
        ComPtr<IDWriteTextFormat> textFormat;
        DWriteFactory->CreateTextFormat(
            L"Arial", nullptr,
            DWRITE_FONT_WEIGHT_BOLD,
            DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL,
            14.0f, L"en-us", &textFormat);
        
        ComPtr<ID2D1SolidColorBrush> textBrush;
        D2DDeviceContext->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 1.0f), &textBrush);
        
        std::wstring label = L"Shadow Map (Directional)";
        D2D1_RECT_F textRect = D2D1::RectF(X + 5, Y + 5, X + Width - 5, Y + 22);
        D2DDeviceContext->DrawText(label.c_str(), static_cast<UINT32>(label.length()), textFormat.Get(), &textRect, textBrush.Get());
        
        // Draw texture dimensions
        wchar_t dimText[64];
        swprintf_s(dimText, L"Size: %ux%u D32_FLOAT", dx12Texture->GetWidth(), dx12Texture->GetHeight());
        
        ComPtr<IDWriteTextFormat> smallTextFormat;
        DWriteFactory->CreateTextFormat(
            L"Arial", nullptr,
            DWRITE_FONT_WEIGHT_NORMAL,
            DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL,
            11.0f, L"en-us", &smallTextFormat);
            
        textRect = D2D1::RectF(X + 5, Y + Height - 35, X + Width - 5, Y + Height - 20);
        D2DDeviceContext->DrawText(dimText, static_cast<UINT32>(wcslen(dimText)), smallTextFormat.Get(), &textRect, textBrush.Get());
        
        // Draw usage hint
        std::wstring hint = L"Use RenderDoc for capture";
        textRect = D2D1::RectF(X + 5, Y + Height - 18, X + Width - 5, Y + Height - 5);
        D2DDeviceContext->DrawText(hint.c_str(), static_cast<UINT32>(hint.length()), smallTextFormat.Get(), &textRect, textBrush.Get());
        
        D2DDeviceContext->EndDraw();
        
        D3D11On12Device->ReleaseWrappedResources(WrappedBackBuffers[FrameIndex].GetAddressOf(), 1);
        D3D11DeviceContext->Flush();
        
        FLog::Log(ELogLevel::Info, "Debug texture drawn at (" + std::to_string(X) + ", " + std::to_string(Y) + ")");
    }
    catch (const std::exception& e)
    {
        FLog::Log(ELogLevel::Error, std::string("DrawDebugTexture error: ") + e.what());
    }
}


// FDX12RHI implementation
FDX12RHI::FDX12RHI() : Width(0), Height(0)
{
}

FDX12RHI::~FDX12RHI()
{
    Shutdown();
}

bool FDX12RHI::Initialize(void* WindowHandle, uint32 InWidth, uint32 InHeight)
{
    Width = InWidth;
    Height = InHeight;
    
    try
    {
        UINT dxgiFactoryFlags = 0;
        
#ifdef _DEBUG
        // Enable debug layer
        ComPtr<ID3D12Debug> debugController;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
        {
            debugController->EnableDebugLayer();
            dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
        }
#endif
        
        // Create DXGI factory
        ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&Factory)));
        
        // Create device
        ComPtr<IDXGIAdapter1> hardwareAdapter;
        for (UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != Factory->EnumAdapters1(adapterIndex, &hardwareAdapter); ++adapterIndex)
        {
            DXGI_ADAPTER_DESC1 desc;
            hardwareAdapter->GetDesc1(&desc);
            
            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            {
                continue;
            }
            
            if (SUCCEEDED(D3D12CreateDevice(hardwareAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&Device))))
            {
                break;
            }
        }
        
        if (!Device)
        {
            FLog::Log(ELogLevel::Error, "Failed to create D3D12 device");
            return false;
        }
        
        // Create command queue
        D3D12_COMMAND_QUEUE_DESC queueDesc = {};
        queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        ThrowIfFailed(Device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&CommandQueue)));
        
        // Create swap chain
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
        swapChainDesc.BufferCount = 2;
        swapChainDesc.Width = Width;
        swapChainDesc.Height = Height;
        swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapChainDesc.SampleDesc.Count = 1;
        
        ComPtr<IDXGISwapChain1> swapChain1;
        ThrowIfFailed(Factory->CreateSwapChainForHwnd(
            CommandQueue.Get(),
            static_cast<HWND>(WindowHandle),
            &swapChainDesc,
            nullptr,
            nullptr,
            &swapChain1
        ));
        
        ThrowIfFailed(swapChain1.As(&SwapChain));
        ThrowIfFailed(Factory->MakeWindowAssociation(static_cast<HWND>(WindowHandle), DXGI_MWA_NO_ALT_ENTER));
        
        // Create command list
        CommandList = std::make_unique<FDX12CommandList>(Device.Get(), CommandQueue.Get(), SwapChain.Get(), Width, Height);
        
        FLog::Log(ELogLevel::Info, "DX12 RHI initialized successfully");
        return true;
        
    }
	catch (const std::exception& e)
	{
		FLog::Log(ELogLevel::Error, std::string("Failed to initialize DX12: ") + e.what());
		return false;
	}
}

void FDX12RHI::Shutdown()
{
    CommandList.reset();
    SwapChain.Reset();
    CommandQueue.Reset();
    Device.Reset();
    Factory.Reset();
}

FRHICommandList* FDX12RHI::GetCommandList()
{
    return CommandList.get();
}

FRHIBuffer* FDX12RHI::CreateVertexBuffer(uint32 Size, const void* Data)
{
    FLog::Log(ELogLevel::Info, std::string("Creating vertex buffer - Size: ") + std::to_string(Size) + " bytes");
    
    // Create upload heap
    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(Size);
    
    ComPtr<ID3D12Resource> vertexBuffer;
    ThrowIfFailed(Device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&vertexBuffer)));
    
    // Copy data
    if (Data)
    {
        void* pVertexDataBegin;
        CD3DX12_RANGE readRange(0, 0);
        ThrowIfFailed(vertexBuffer->Map(0, &readRange, &pVertexDataBegin));
        memcpy(pVertexDataBegin, Data, Size);
        vertexBuffer->Unmap(0, nullptr);
        FLog::Log(ELogLevel::Info, "Vertex data copied to buffer");
    }
    
    FLog::Log(ELogLevel::Info, "Vertex buffer created successfully");
    return new FDX12Buffer(vertexBuffer.Detach(), FDX12Buffer::EBufferType::Vertex);
}

FRHIBuffer* FDX12RHI::CreateIndexBuffer(uint32 Size, const void* Data)
{
    FLog::Log(ELogLevel::Info, std::string("Creating index buffer - Size: ") + std::to_string(Size) + " bytes");
    
    // Create upload heap
    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(Size);
    
    ComPtr<ID3D12Resource> indexBuffer;
    ThrowIfFailed(Device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&indexBuffer)));
    
    // Copy data
    if (Data)
    {
        void* pIndexDataBegin;
        CD3DX12_RANGE readRange(0, 0);
        ThrowIfFailed(indexBuffer->Map(0, &readRange, &pIndexDataBegin));
        memcpy(pIndexDataBegin, Data, Size);
        indexBuffer->Unmap(0, nullptr);
        FLog::Log(ELogLevel::Info, "Index data copied to buffer");
    }
    
    FLog::Log(ELogLevel::Info, "Index buffer created successfully");
    return new FDX12Buffer(indexBuffer.Detach(), FDX12Buffer::EBufferType::Index);
}

FRHIBuffer* FDX12RHI::CreateConstantBuffer(uint32 Size)
{
    FLog::Log(ELogLevel::Info, std::string("Creating constant buffer - Size: ") + std::to_string(Size) + " bytes");
    
    // Constant buffers must be 256-byte aligned (D3D12 requirement)
    static constexpr uint32 CONSTANT_BUFFER_ALIGNMENT = 256;
    uint32 alignedSize = (Size + CONSTANT_BUFFER_ALIGNMENT - 1) & ~(CONSTANT_BUFFER_ALIGNMENT - 1);
    
    // Create upload heap
    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(alignedSize);
    
    ComPtr<ID3D12Resource> constantBuffer;
    ThrowIfFailed(Device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&constantBuffer)));
    
    FLog::Log(ELogLevel::Info, "Constant buffer created successfully");
    return new FDX12Buffer(constantBuffer.Detach(), FDX12Buffer::EBufferType::Constant);
}

FRHITexture* FDX12RHI::CreateDepthTexture(uint32 InWidth, uint32 InHeight, ERTFormat Format, uint32 ArraySize)
{
    FLog::Log(ELogLevel::Info, "Creating depth texture: " + std::to_string(InWidth) + "x" + 
              std::to_string(InHeight) + " ArraySize=" + std::to_string(ArraySize));
    
    // Map ERTFormat to DXGI_FORMAT
    DXGI_FORMAT resourceFormat;
    DXGI_FORMAT dsvFormat;
    DXGI_FORMAT srvFormat;
    
    switch (Format)
    {
        case ERTFormat::D32_FLOAT:
            resourceFormat = DXGI_FORMAT_R32_TYPELESS;  // Typeless for multi-use
            dsvFormat = DXGI_FORMAT_D32_FLOAT;
            srvFormat = DXGI_FORMAT_R32_FLOAT;
            break;
        case ERTFormat::D16_UNORM:
            resourceFormat = DXGI_FORMAT_R16_TYPELESS;
            dsvFormat = DXGI_FORMAT_D16_UNORM;
            srvFormat = DXGI_FORMAT_R16_UNORM;
            break;
        case ERTFormat::D24_UNORM_S8_UINT:
            resourceFormat = DXGI_FORMAT_R24G8_TYPELESS;
            dsvFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
            srvFormat = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
            break;
        case ERTFormat::R32_FLOAT:
            resourceFormat = DXGI_FORMAT_R32_FLOAT;
            dsvFormat = DXGI_FORMAT_D32_FLOAT;
            srvFormat = DXGI_FORMAT_R32_FLOAT;
            break;
        default:
            FLog::Log(ELogLevel::Error, "Unsupported depth format");
            return nullptr;
    }
    
    // Create texture resource
    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
    
    D3D12_RESOURCE_DESC texDesc = {};
    texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    texDesc.Alignment = 0;
    texDesc.Width = InWidth;
    texDesc.Height = InHeight;
    texDesc.DepthOrArraySize = static_cast<UINT16>(ArraySize);
    texDesc.MipLevels = 1;
    texDesc.Format = resourceFormat;
    texDesc.SampleDesc.Count = 1;
    texDesc.SampleDesc.Quality = 0;
    texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
    
    D3D12_CLEAR_VALUE clearValue = {};
    clearValue.Format = dsvFormat;
    clearValue.DepthStencil.Depth = 1.0f;
    clearValue.DepthStencil.Stencil = 0;
    
    ComPtr<ID3D12Resource> texture;
    ThrowIfFailed(Device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &texDesc,
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &clearValue,
        IID_PPV_ARGS(&texture)));
    
    // Create DSV descriptor heap (one DSV per array slice)
    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
    dsvHeapDesc.NumDescriptors = ArraySize;
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    
    ComPtr<ID3D12DescriptorHeap> dsvHeap;
    ThrowIfFailed(Device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&dsvHeap)));
    
    uint32 dsvDescriptorSize = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    
    // Create DSVs for each array slice
    CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(dsvHeap->GetCPUDescriptorHandleForHeapStart());
    for (uint32 i = 0; i < ArraySize; ++i)
    {
        D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
        dsvDesc.Format = dsvFormat;
        
        if (ArraySize > 1)
        {
            dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
            dsvDesc.Texture2DArray.MipSlice = 0;
            dsvDesc.Texture2DArray.FirstArraySlice = i;
            dsvDesc.Texture2DArray.ArraySize = 1;
        }
        else
        {
            dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
            dsvDesc.Texture2D.MipSlice = 0;
        }
        dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
        
        Device->CreateDepthStencilView(texture.Get(), &dsvDesc, dsvHandle);
        dsvHandle.Offset(1, dsvDescriptorSize);
    }
    
    // Create SRV descriptor heap (for sampling in shaders)
    D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
    srvHeapDesc.NumDescriptors = 1;  // Single SRV for entire texture/array
    srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    
    ComPtr<ID3D12DescriptorHeap> srvHeap;
    ThrowIfFailed(Device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&srvHeap)));
    
    // Create SRV
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = srvFormat;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    
    if (ArraySize > 1)
    {
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
        srvDesc.Texture2DArray.MostDetailedMip = 0;
        srvDesc.Texture2DArray.MipLevels = 1;
        srvDesc.Texture2DArray.FirstArraySlice = 0;
        srvDesc.Texture2DArray.ArraySize = ArraySize;
        srvDesc.Texture2DArray.PlaneSlice = 0;
        srvDesc.Texture2DArray.ResourceMinLODClamp = 0.0f;
    }
    else
    {
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MostDetailedMip = 0;
        srvDesc.Texture2D.MipLevels = 1;
        srvDesc.Texture2D.PlaneSlice = 0;
        srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
    }
    
    Device->CreateShaderResourceView(texture.Get(), &srvDesc, srvHeap->GetCPUDescriptorHandleForHeapStart());
    
    FLog::Log(ELogLevel::Info, "Depth texture created successfully");
    
    return new FDX12Texture(texture.Detach(), InWidth, InHeight, ArraySize, dsvFormat, 
                            dsvHeap.Detach(), srvHeap.Detach());
}

FRHIPipelineState* FDX12RHI::CreateGraphicsPipelineState(bool bEnableDepth)
{
    FLog::Log(ELogLevel::Info, std::string("Creating graphics pipeline state (depth: ") + 
        (bEnableDepth ? "enabled" : "disabled") + ")...");
    
    // Shader code with optional MVP matrix
    const char* shaderCode = bEnableDepth ? R"(
        cbuffer MVPBuffer : register(b0)
{
            float4x4 MVP;
        };
        
        struct VSInput {
            float3 position : POSITION;
            float4 color : COLOR;
        };
        
        struct PSInput {
            float4 position : SV_POSITION;
            float4 color : COLOR;
        };
        
        PSInput VSMain(VSInput input)
{
            PSInput result;
            result.position = mul(float4(input.position, 1.0f), MVP);
            result.color = input.color;
            return result;
        }
        
        float4 PSMain(PSInput input) : SV_TARGET {
            return input.color;
        }
    )" : R"(
        struct VSInput {
            float3 position : POSITION;
            float4 color : COLOR;
        };
        
        struct PSInput {
            float4 position : SV_POSITION;
            float4 color : COLOR;
        };
        
        PSInput VSMain(VSInput input)
{
            PSInput result;
            result.position = float4(input.position, 1.0f);
            result.color = input.color;
            return result;
        }
        
        float4 PSMain(PSInput input) : SV_TARGET {
            return input.color;
        }
    )";
    
    ComPtr<ID3DBlob> vertexShader;
    ComPtr<ID3DBlob> pixelShader;
    ComPtr<ID3DBlob> error;
    
    // Shader compile flags - disable optimization in debug mode for RenderDoc debugging
    UINT shaderCompileFlags = 0;
#ifdef _DEBUG
    shaderCompileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
    
    // Compile vertex shader
    if (FAILED(D3DCompile(shaderCode, strlen(shaderCode), "BasicShader", nullptr, nullptr,
                         "VSMain", "vs_5_0", shaderCompileFlags, 0, &vertexShader, &error)))
{
        if (error)
        {
            FLog::Log(ELogLevel::Error, std::string("Vertex shader compile error: ") + static_cast<char*>(error->GetBufferPointer()));
        }
        return nullptr;
    }
    FLog::Log(ELogLevel::Info, "Vertex shader compiled successfully");
    
    // Compile pixel shader
    if (FAILED(D3DCompile(shaderCode, strlen(shaderCode), "BasicShader", nullptr, nullptr,
                         "PSMain", "ps_5_0", shaderCompileFlags, 0, &pixelShader, &error)))
{
        if (error)
        {
            FLog::Log(ELogLevel::Error, std::string("Pixel shader compile error: ") + static_cast<char*>(error->GetBufferPointer()));
        }
        return nullptr;
    }
    FLog::Log(ELogLevel::Info, "Pixel shader compiled successfully");
    
    // Create root signature
    // NOTE: rootParameters must be declared outside the if block to remain valid
    // when D3D12SerializeRootSignature is called (it stores a pointer, not a copy)
    CD3DX12_ROOT_PARAMETER rootParameters[1];
    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    
    if (bEnableDepth)
    {
        // Root parameter for MVP constant buffer
        rootParameters[0].InitAsConstantBufferView(0); // b0 register
        
        rootSignatureDesc.Init(1, rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
    }
    else
    {
        rootSignatureDesc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
    }
    
    ComPtr<ID3DBlob> signature;
    ComPtr<ID3D12RootSignature> rootSignature;
    ThrowIfFailed(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
    ThrowIfFailed(Device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&rootSignature)));
    
    FLog::Log(ELogLevel::Info, "Root signature created");
    
    // Define input layout
    D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = 
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };
    
    // Create PSO
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
    psoDesc.pRootSignature = rootSignature.Get();
    psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());
    psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get());
    
    // Rasterizer state - DISABLE BACKFACE CULLING to ensure triangle is visible
    CD3DX12_RASTERIZER_DESC rasterizerDesc(D3D12_DEFAULT);
    rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;  // Don't cull any faces
    psoDesc.RasterizerState = rasterizerDesc;
    
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    
    if (bEnableDepth)
    {
        psoDesc.DepthStencilState.DepthEnable = TRUE;
        psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
        psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
        psoDesc.DepthStencilState.StencilEnable = FALSE;
        psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    }
    else
    {
        psoDesc.DepthStencilState.DepthEnable = FALSE;
        psoDesc.DepthStencilState.StencilEnable = FALSE;
    }
    
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.SampleDesc.Count = 1;
    
    ComPtr<ID3D12PipelineState> pipelineState;
    ThrowIfFailed(Device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState)));
    
    FLog::Log(ELogLevel::Info, "Graphics pipeline state created successfully");
    
    return new FDX12PipelineState(pipelineState.Detach(), rootSignature.Detach());
}

FRHIPipelineState* FDX12RHI::CreateGraphicsPipelineStateEx(EPipelineFlags Flags)
{
    bool bEnableDepth = HasFlag(Flags, EPipelineFlags::EnableDepth);
    bool bEnableLighting = HasFlag(Flags, EPipelineFlags::EnableLighting);
    bool bWireframe = HasFlag(Flags, EPipelineFlags::WireframeMode);
    bool bLineTopology = HasFlag(Flags, EPipelineFlags::LineTopology);
    bool bEnableShadows = HasFlag(Flags, EPipelineFlags::EnableShadows);
    bool bDepthOnly = HasFlag(Flags, EPipelineFlags::DepthOnly);
    
    FLog::Log(ELogLevel::Info, std::string("Creating graphics pipeline state Ex (depth: ") + 
        (bEnableDepth ? "on" : "off") + ", lighting: " + (bEnableLighting ? "on" : "off") + 
        ", wireframe: " + (bWireframe ? "on" : "off") + ", lines: " + (bLineTopology ? "on" : "off") + 
        ", shadows: " + (bEnableShadows ? "on" : "off") + ", depth-only: " + (bDepthOnly ? "on" : "off") + ")...");
    
    // Load shader from file based on pipeline type
    std::string shaderSource;
    std::string shaderName;
    
    if (bDepthOnly)
    {
        shaderName = "DepthOnly";
    }
    else if (bEnableLighting)
    {
        shaderName = "Lit";
    }
    else
    {
        shaderName = "Unlit";
    }
    
    shaderSource = FShaderLoader::LoadShaderFromFile(shaderName);
    if (shaderSource.empty())
    {
        FLog::Log(ELogLevel::Error, "Failed to load shader: " + shaderName);
        return nullptr;
    }
    
    ComPtr<ID3DBlob> vertexShader;
    ComPtr<ID3DBlob> pixelShader;
    ComPtr<ID3DBlob> error;
    
    // Shader compile flags - disable optimization in debug mode for RenderDoc debugging
    UINT shaderCompileFlags = 0;
#ifdef _DEBUG
    // D3DCOMPILE_DEBUG: Include debug info for shader debugging
    // D3DCOMPILE_SKIP_OPTIMIZATION: Disable optimization for source-level debugging
    shaderCompileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
    FLog::Log(ELogLevel::Info, "Compiling shaders with DEBUG flags (no optimization, debug info enabled)");
#endif
    
    // Compile vertex shader
    if (FAILED(D3DCompile(shaderSource.c_str(), shaderSource.size(), shaderName.c_str(), nullptr, nullptr,
                         "VSMain", "vs_5_0", shaderCompileFlags, 0, &vertexShader, &error)))
    {
        if (error)
        {
            FLog::Log(ELogLevel::Error, std::string("Vertex shader compile error: ") + static_cast<char*>(error->GetBufferPointer()));
        }
        return nullptr;
    }
    FLog::Log(ELogLevel::Info, "Vertex shader compiled successfully");
    
    // Compile pixel shader (skip for depth-only since we use null pixel shader)
    if (!bDepthOnly)
    {
        if (FAILED(D3DCompile(shaderSource.c_str(), shaderSource.size(), shaderName.c_str(), nullptr, nullptr,
                             "PSMain", "ps_5_0", shaderCompileFlags, 0, &pixelShader, &error)))
        {
            if (error)
            {
                FLog::Log(ELogLevel::Error, std::string("Pixel shader compile error: ") + static_cast<char*>(error->GetBufferPointer()));
            }
            return nullptr;
        }
        FLog::Log(ELogLevel::Info, "Pixel shader compiled successfully");
    }
    else
    {
        FLog::Log(ELogLevel::Info, "Skipping pixel shader compilation for depth-only PSO (null pixel shader)");
    }
    
    // Create root signature with appropriate number of constant buffers
    CD3DX12_ROOT_PARAMETER rootParameters[4];
    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    
    // Declare these outside the if block to ensure they survive until root signature is serialized
    CD3DX12_DESCRIPTOR_RANGE srvRange;
    D3D12_STATIC_SAMPLER_DESC shadowSampler = {};
    
    if (bEnableLighting)
    {
        // Four root parameters:
        // 0: CBV for MVP (b0)
        // 1: CBV for Lighting (b1)
        // 2: CBV for Shadow (b2)
        // 3: Descriptor table for shadow map texture (t0)
        rootParameters[0].InitAsConstantBufferView(0);
        rootParameters[1].InitAsConstantBufferView(1);
        rootParameters[2].InitAsConstantBufferView(2);
        
        // Descriptor range for shadow map texture (t0)
        srvRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);  // 1 SRV at t0
        rootParameters[3].InitAsDescriptorTable(1, &srvRange);
        
        // Static sampler for shadow map comparison sampling
        shadowSampler.Filter = D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
        shadowSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
        shadowSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
        shadowSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
        shadowSampler.MipLODBias = 0;
        shadowSampler.MaxAnisotropy = 1;
        shadowSampler.ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
        shadowSampler.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;  // White border = no shadow outside
        shadowSampler.MinLOD = 0.0f;
        shadowSampler.MaxLOD = D3D12_FLOAT32_MAX;
        shadowSampler.ShaderRegister = 0;  // s0
        shadowSampler.RegisterSpace = 0;
        shadowSampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
        
        rootSignatureDesc.Init(4, rootParameters, 1, &shadowSampler, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
        FLog::Log(ELogLevel::Info, "Creating lit PSO with shadow map sampling support");
    }
    else if (bDepthOnly)
    {
        // For depth-only (shadow pass), use 32-bit root constants instead of CBV
        // This allows per-draw matrix data without constant buffer sync issues
        // MVP matrix = 16 floats = 16 DWORDs
        rootParameters[0].InitAsConstants(16, 0);  // 16 constants at b0
        rootSignatureDesc.Init(1, rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
        FLog::Log(ELogLevel::Info, "Using root constants for depth-only PSO (shadow pass)");
    }
    else if (bEnableDepth)
    {
        // One CBV: MVP (b0) - for depth-enabled non-lit rendering
        rootParameters[0].InitAsConstantBufferView(0);
        rootSignatureDesc.Init(1, rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
    }
    else
    {
        rootSignatureDesc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
    }
    
    ComPtr<ID3DBlob> signature;
    ComPtr<ID3D12RootSignature> rootSignature;
    ThrowIfFailed(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
    ThrowIfFailed(Device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&rootSignature)));
    
    FLog::Log(ELogLevel::Info, "Root signature created");
    
    // Define input layout based on vertex type
    D3D12_INPUT_ELEMENT_DESC litInputElementDescs[] = 
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };
    
    D3D12_INPUT_ELEMENT_DESC unlitInputElementDescs[] = 
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };
    
    // Create PSO
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    if (bEnableLighting || bDepthOnly)
    {
        // Depth-only shader uses lit vertex format (position, normal, color)
        psoDesc.InputLayout = { litInputElementDescs, _countof(litInputElementDescs) };
    }
    else
    {
        psoDesc.InputLayout = { unlitInputElementDescs, _countof(unlitInputElementDescs) };
    }
    psoDesc.pRootSignature = rootSignature.Get();
    psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());
    
    // For depth-only rendering, we can skip the pixel shader entirely
    // D3D12 allows null pixel shader when NumRenderTargets = 0
    if (!bDepthOnly)
    {
        psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get());
    }
    // else psoDesc.PS remains default/empty (null pixel shader)
    
    // Rasterizer state
    CD3DX12_RASTERIZER_DESC rasterizerDesc(D3D12_DEFAULT);
    rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;  // Don't cull any faces for now
    if (bWireframe)
    {
        rasterizerDesc.FillMode = D3D12_FILL_MODE_WIREFRAME;
    }
    psoDesc.RasterizerState = rasterizerDesc;
    
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    
    if (bDepthOnly)
    {
        // Depth-only rendering (shadow pass)
        psoDesc.DepthStencilState.DepthEnable = TRUE;
        psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
        psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
        psoDesc.DepthStencilState.StencilEnable = FALSE;
        psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
        psoDesc.NumRenderTargets = 0;  // No color output
        // RTVFormats remain unset (nullptr/UNKNOWN)
    }
    else if (bEnableDepth)
    {
        psoDesc.DepthStencilState.DepthEnable = TRUE;
        psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
        psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
        psoDesc.DepthStencilState.StencilEnable = FALSE;
        psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    }
    else
    {
        psoDesc.DepthStencilState.DepthEnable = FALSE;
        psoDesc.DepthStencilState.StencilEnable = FALSE;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    }
    
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = bLineTopology ? D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE : D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.SampleDesc.Count = 1;
    
    ComPtr<ID3D12PipelineState> pipelineState;
    ThrowIfFailed(Device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState)));
    
    FLog::Log(ELogLevel::Info, "Graphics pipeline state Ex created successfully");
    
    return new FDX12PipelineState(pipelineState.Detach(), rootSignature.Detach());
}

// Factory function
FRHI* CreateDX12RHI()
{
    return new FDX12RHI();
}
