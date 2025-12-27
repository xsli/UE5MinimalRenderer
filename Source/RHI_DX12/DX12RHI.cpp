#include "DX12RHI.h"
#include "d3dx12.h"
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
	FLog::Log(ELogLevel::Info, "FlushCommandsFor2D: Submitting 3D rendering commands");

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

		FLog::Log(ELogLevel::Info, "FlushCommandsFor2D: 3D commands submitted, ready for 2D rendering");
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
    
    // Compile vertex shader
    if (FAILED(D3DCompile(shaderCode, strlen(shaderCode), nullptr, nullptr, nullptr,
                         "VSMain", "vs_5_0", 0, 0, &vertexShader, &error)))
{
        if (error)
        {
            FLog::Log(ELogLevel::Error, std::string("Vertex shader compile error: ") + static_cast<char*>(error->GetBufferPointer()));
        }
        return nullptr;
    }
    FLog::Log(ELogLevel::Info, "Vertex shader compiled successfully");
    
    // Compile pixel shader
    if (FAILED(D3DCompile(shaderCode, strlen(shaderCode), nullptr, nullptr, nullptr,
                         "PSMain", "ps_5_0", 0, 0, &pixelShader, &error)))
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
    
    FLog::Log(ELogLevel::Info, std::string("Creating graphics pipeline state Ex (depth: ") + 
        (bEnableDepth ? "on" : "off") + ", lighting: " + (bEnableLighting ? "on" : "off") + 
        ", wireframe: " + (bWireframe ? "on" : "off") + ", lines: " + (bLineTopology ? "on" : "off") + ")...");
    
    // Phong lighting shader - uses FLitVertex format (position, normal, color)
    // Constant buffer layout:
    //   b0: MVP matrix (model-view-projection)
    //   b1: Lighting data (model matrix, camera pos, lights, material)
    const char* litShaderCode = R"(
        cbuffer MVPBuffer : register(b0)
        {
            float4x4 MVP;
        };
        
        cbuffer LightingBuffer : register(b1)
        {
            float4x4 ModelMatrix;
            float4 CameraPosition;      // xyz = camera pos, w = unused
            float4 AmbientLight;        // xyz = ambient color, w = intensity
            
            // Directional light
            float4 DirLightDirection;   // xyz = direction (normalized), w = enabled
            float4 DirLightColor;       // xyz = color, w = intensity
            
            // Point light (up to 4)
            float4 PointLight0Position; // xyz = position, w = enabled
            float4 PointLight0Color;    // xyz = color, w = intensity
            float4 PointLight0Params;   // x = radius, y = falloff, zw = unused
            
            float4 PointLight1Position;
            float4 PointLight1Color;
            float4 PointLight1Params;
            
            float4 PointLight2Position;
            float4 PointLight2Color;
            float4 PointLight2Params;
            
            float4 PointLight3Position;
            float4 PointLight3Color;
            float4 PointLight3Params;
            
            // Material properties
            float4 MaterialDiffuse;     // xyz = diffuse color, w = unused
            float4 MaterialSpecular;    // xyz = specular color, w = shininess
            float4 MaterialAmbient;     // xyz = ambient color, w = unused
        };
        
        struct VSInput {
            float3 position : POSITION;
            float3 normal : NORMAL;
            float4 color : COLOR;
        };
        
        struct PSInput {
            float4 position : SV_POSITION;
            float3 worldPos : WORLDPOS;
            float3 normal : NORMAL;
            float4 color : COLOR;
        };
        
        PSInput VSMain(VSInput input)
        {
            PSInput result;
            result.position = mul(float4(input.position, 1.0f), MVP);
            
            // Transform position to world space for lighting
            result.worldPos = mul(float4(input.position, 1.0f), ModelMatrix).xyz;
            
            // Transform normal to world space (using upper 3x3 of model matrix)
            // NOTE: For proper normal transformation with non-uniform scaling,
            // we should use the inverse transpose of the model matrix.
            // This simplified approach works correctly for:
            //   - Uniform scaling (same scale in X, Y, Z)
            //   - Rotation and translation only
            // Limitation: Non-uniform scaling will produce incorrect normals.
            float3x3 normalMatrix = (float3x3)ModelMatrix;
            result.normal = normalize(mul(input.normal, normalMatrix));
            
            result.color = input.color;
            return result;
        }
        
        // Calculate point light attenuation
        float CalcAttenuation(float distance, float radius, float falloff)
        {
            if (distance >= radius) return 0.0f;
            float normalizedDist = distance / radius;
            float attenuation = 1.0f / (1.0f + pow(normalizedDist, falloff));
            float smooth = 1.0f - pow(normalizedDist, 4.0f);
            return attenuation * saturate(smooth);
        }
        
        // Apply point light contribution
        float3 CalcPointLight(float3 worldPos, float3 N, float3 V, 
                             float4 lightPos, float4 lightColor, float4 lightParams,
                             float3 diffuseColor, float3 specularColor, float shininess)
        {
            if (lightPos.w < 0.5f) return float3(0, 0, 0); // Light disabled
            
            float3 lightPosition = lightPos.xyz;
            float3 L = lightPosition - worldPos;
            float distance = length(L);
            L = normalize(L);
            
            float attenuation = CalcAttenuation(distance, lightParams.x, lightParams.y);
            if (attenuation < 0.001f) return float3(0, 0, 0);
            
            float3 lightColorRGB = lightColor.xyz * lightColor.w;
            
            // Diffuse
            float NdotL = max(dot(N, L), 0.0f);
            float3 diffuse = diffuseColor * lightColorRGB * NdotL;
            
            // Specular (Blinn-Phong)
            float3 H = normalize(L + V);
            float NdotH = max(dot(N, H), 0.0f);
            float3 specular = specularColor * lightColorRGB * pow(NdotH, shininess);
            
            return (diffuse + specular) * attenuation;
        }
        
        float4 PSMain(PSInput input) : SV_TARGET 
        {
            float3 N = normalize(input.normal);
            float3 V = normalize(CameraPosition.xyz - input.worldPos);
            
            // Base material colors (multiply with vertex color)
            float3 diffuseColor = MaterialDiffuse.xyz * input.color.xyz;
            float3 specularColor = MaterialSpecular.xyz;
            float3 ambientColor = MaterialAmbient.xyz * input.color.xyz;
            float shininess = MaterialSpecular.w;
            
            // Ambient contribution
            float3 ambient = ambientColor * AmbientLight.xyz * AmbientLight.w;
            
            // Directional light contribution
            float3 directional = float3(0, 0, 0);
            if (DirLightDirection.w > 0.5f) // Enabled
            {
                float3 L = normalize(-DirLightDirection.xyz); // Direction toward light
                float NdotL = max(dot(N, L), 0.0f);
                
                float3 lightColorRGB = DirLightColor.xyz * DirLightColor.w;
                
                // Diffuse
                float3 diffuse = diffuseColor * lightColorRGB * NdotL;
                
                // Specular (Blinn-Phong)
                float3 H = normalize(L + V);
                float NdotH = max(dot(N, H), 0.0f);
                float3 specular = specularColor * lightColorRGB * pow(NdotH, shininess);
                
                directional = diffuse + specular;
            }
            
            // Point light contributions
            float3 pointLights = float3(0, 0, 0);
            pointLights += CalcPointLight(input.worldPos, N, V, PointLight0Position, PointLight0Color, PointLight0Params, diffuseColor, specularColor, shininess);
            pointLights += CalcPointLight(input.worldPos, N, V, PointLight1Position, PointLight1Color, PointLight1Params, diffuseColor, specularColor, shininess);
            pointLights += CalcPointLight(input.worldPos, N, V, PointLight2Position, PointLight2Color, PointLight2Params, diffuseColor, specularColor, shininess);
            pointLights += CalcPointLight(input.worldPos, N, V, PointLight3Position, PointLight3Color, PointLight3Params, diffuseColor, specularColor, shininess);
            
            // Final color
            float3 finalColor = ambient + directional + pointLights;
            
            return float4(finalColor, input.color.a);
        }
    )";
    
    // Unlit shader with MVP (for wireframes, lines, etc.)
    const char* unlitShaderCode = R"(
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
    )";
    
    const char* shaderCode = bEnableLighting ? litShaderCode : unlitShaderCode;
    
    ComPtr<ID3DBlob> vertexShader;
    ComPtr<ID3DBlob> pixelShader;
    ComPtr<ID3DBlob> error;
    
    // Compile vertex shader
    if (FAILED(D3DCompile(shaderCode, strlen(shaderCode), nullptr, nullptr, nullptr,
                         "VSMain", "vs_5_0", 0, 0, &vertexShader, &error)))
    {
        if (error)
        {
            FLog::Log(ELogLevel::Error, std::string("Vertex shader compile error: ") + static_cast<char*>(error->GetBufferPointer()));
        }
        return nullptr;
    }
    FLog::Log(ELogLevel::Info, "Vertex shader compiled successfully");
    
    // Compile pixel shader
    if (FAILED(D3DCompile(shaderCode, strlen(shaderCode), nullptr, nullptr, nullptr,
                         "PSMain", "ps_5_0", 0, 0, &pixelShader, &error)))
    {
        if (error)
        {
            FLog::Log(ELogLevel::Error, std::string("Pixel shader compile error: ") + static_cast<char*>(error->GetBufferPointer()));
        }
        return nullptr;
    }
    FLog::Log(ELogLevel::Info, "Pixel shader compiled successfully");
    
    // Create root signature with appropriate number of constant buffers
    CD3DX12_ROOT_PARAMETER rootParameters[2];
    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    
    if (bEnableLighting)
    {
        // Two CBVs: MVP (b0) and Lighting (b1)
        rootParameters[0].InitAsConstantBufferView(0);
        rootParameters[1].InitAsConstantBufferView(1);
        rootSignatureDesc.Init(2, rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
    }
    else if (bEnableDepth)
    {
        // One CBV: MVP (b0)
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
    if (bEnableLighting)
    {
        psoDesc.InputLayout = { litInputElementDescs, _countof(litInputElementDescs) };
    }
    else
    {
        psoDesc.InputLayout = { unlitInputElementDescs, _countof(unlitInputElementDescs) };
    }
    psoDesc.pRootSignature = rootSignature.Get();
    psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());
    psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get());
    
    // Rasterizer state
    CD3DX12_RASTERIZER_DESC rasterizerDesc(D3D12_DEFAULT);
    rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;  // Don't cull any faces for now
    if (bWireframe)
    {
        rasterizerDesc.FillMode = D3D12_FILL_MODE_WIREFRAME;
    }
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
    psoDesc.PrimitiveTopologyType = bLineTopology ? D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE : D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
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
