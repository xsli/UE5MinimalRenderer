#include "DX12RHI.h"
#include "d3dx12.h"
#include <d3dcompiler.h>
#include <stdexcept>
#include <cstring>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

// Helper function for checking HRESULT
inline void ThrowIfFailed(HRESULT hr) {
    if (FAILED(hr)) {
        throw std::runtime_error("DirectX 12 operation failed");
    }
}

// FDX12Buffer implementation
FDX12Buffer::FDX12Buffer(ID3D12Resource* InResource)
    : Resource(InResource) {
    
    D3D12_RESOURCE_DESC desc = Resource->GetDesc();
    VertexBufferView.BufferLocation = Resource->GetGPUVirtualAddress();
    VertexBufferView.SizeInBytes = static_cast<UINT>(desc.Width);
    VertexBufferView.StrideInBytes = sizeof(FVertex);
    
    FLog::Log(ELogLevel::Info, std::string("FDX12Buffer created - GPU Address: 0x") + 
        std::to_string(VertexBufferView.BufferLocation) + 
        ", Size: " + std::to_string(VertexBufferView.SizeInBytes) + 
        ", Stride: " + std::to_string(VertexBufferView.StrideInBytes));
}

FDX12Buffer::~FDX12Buffer() {
}

void* FDX12Buffer::Map() {
    void* pData = nullptr;
    CD3DX12_RANGE readRange(0, 0);
    ThrowIfFailed(Resource->Map(0, &readRange, &pData));
    return pData;
}

void FDX12Buffer::Unmap() {
    Resource->Unmap(0, nullptr);
}

// FDX12PipelineState implementation
FDX12PipelineState::FDX12PipelineState(ID3D12PipelineState* InPSO, ID3D12RootSignature* InRootSig)
    : PSO(InPSO), RootSignature(InRootSig) {
}

FDX12PipelineState::~FDX12PipelineState() {
}

// FDX12CommandList implementation
FDX12CommandList::FDX12CommandList(ID3D12Device* InDevice, ID3D12CommandQueue* InQueue, IDXGISwapChain3* InSwapChain)
    : Device(InDevice), CommandQueue(InQueue), SwapChain(InSwapChain), FrameIndex(0), FenceValue(0) {
    
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
    for (uint32 i = 0; i < FrameCount; i++) {
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
    if (FenceEvent == nullptr) {
        ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
    }
}

FDX12CommandList::~FDX12CommandList() {
    WaitForGPU();
    CloseHandle(FenceEvent);
}

void FDX12CommandList::BeginFrame() {
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

void FDX12CommandList::EndFrame() {
    // Transition render target to present state
    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        RenderTargets[FrameIndex].Get(),
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        D3D12_RESOURCE_STATE_PRESENT);
    GraphicsCommandList->ResourceBarrier(1, &barrier);
    
    ThrowIfFailed(GraphicsCommandList->Close());
    
    ID3D12CommandList* ppCommandLists[] = { GraphicsCommandList.Get() };
    CommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
}

void FDX12CommandList::ClearRenderTarget(const FColor& Color) {
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(RTVHeap->GetCPUDescriptorHandleForHeapStart(), FrameIndex, RTVDescriptorSize);
    
    const float clearColor[] = { Color.R, Color.G, Color.B, Color.A };
    FLog::Log(ELogLevel::Info, std::string("ClearRenderTarget - Color: ") + 
        std::to_string(Color.R) + ", " + std::to_string(Color.G) + ", " + std::to_string(Color.B));
    
    GraphicsCommandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
    GraphicsCommandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);
}

void FDX12CommandList::SetPipelineState(FRHIPipelineState* PipelineState) {
    FLog::Log(ELogLevel::Info, "SetPipelineState called");
    FDX12PipelineState* DX12PSO = static_cast<FDX12PipelineState*>(PipelineState);
    GraphicsCommandList->SetPipelineState(DX12PSO->GetPSO());
    GraphicsCommandList->SetGraphicsRootSignature(DX12PSO->GetRootSignature());
}

void FDX12CommandList::SetVertexBuffer(FRHIBuffer* VertexBuffer, uint32 Offset, uint32 Stride) {
    FLog::Log(ELogLevel::Info, std::string("SetVertexBuffer - Stride: ") + std::to_string(Stride));
    FDX12Buffer* DX12Buffer = static_cast<FDX12Buffer*>(VertexBuffer);
    D3D12_VERTEX_BUFFER_VIEW vbv = DX12Buffer->GetVertexBufferView();
    FLog::Log(ELogLevel::Info, std::string("  VBV - Location: 0x") + 
        std::to_string(vbv.BufferLocation) + 
        ", Size: " + std::to_string(vbv.SizeInBytes) + 
        ", Stride: " + std::to_string(vbv.StrideInBytes));
    GraphicsCommandList->IASetVertexBuffers(0, 1, &vbv);
}

void FDX12CommandList::DrawPrimitive(uint32 VertexCount, uint32 StartVertex) {
    FLog::Log(ELogLevel::Info, std::string("DrawPrimitive - VertexCount: ") + std::to_string(VertexCount) + 
        ", StartVertex: " + std::to_string(StartVertex));
    
    GraphicsCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    GraphicsCommandList->DrawInstanced(VertexCount, 1, StartVertex, 0);
}

void FDX12CommandList::Present() {
    FLog::Log(ELogLevel::Info, "Presenting frame...");
    ThrowIfFailed(SwapChain->Present(1, 0));
    WaitForGPU();
    FLog::Log(ELogLevel::Info, "Frame presented");
}

void FDX12CommandList::WaitForGPU() {
    const UINT64 currentFenceValue = FenceValue;
    ThrowIfFailed(CommandQueue->Signal(Fence.Get(), currentFenceValue));
    FenceValue++;
    
    if (Fence->GetCompletedValue() < currentFenceValue) {
        ThrowIfFailed(Fence->SetEventOnCompletion(currentFenceValue, FenceEvent));
        WaitForSingleObjectEx(FenceEvent, INFINITE, FALSE);
    }
}

// FDX12RHI implementation
FDX12RHI::FDX12RHI() : Width(0), Height(0) {
}

FDX12RHI::~FDX12RHI() {
    Shutdown();
}

bool FDX12RHI::Initialize(void* WindowHandle, uint32 InWidth, uint32 InHeight) {
    Width = InWidth;
    Height = InHeight;
    
    try {
        UINT dxgiFactoryFlags = 0;
        
#ifdef _DEBUG
        // Enable debug layer
        ComPtr<ID3D12Debug> debugController;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
            debugController->EnableDebugLayer();
            dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
        }
#endif
        
        // Create DXGI factory
        ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&Factory)));
        
        // Create device
        ComPtr<IDXGIAdapter1> hardwareAdapter;
        for (UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != Factory->EnumAdapters1(adapterIndex, &hardwareAdapter); ++adapterIndex) {
            DXGI_ADAPTER_DESC1 desc;
            hardwareAdapter->GetDesc1(&desc);
            
            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
                continue;
            }
            
            if (SUCCEEDED(D3D12CreateDevice(hardwareAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&Device)))) {
                break;
            }
        }
        
        if (!Device) {
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
        CommandList = std::make_unique<FDX12CommandList>(Device.Get(), CommandQueue.Get(), SwapChain.Get());
        
        FLog::Log(ELogLevel::Info, "DX12 RHI initialized successfully");
        return true;
        
    } catch (const std::exception& e) {
        FLog::Log(ELogLevel::Error, std::string("Failed to initialize DX12: ") + e.what());
        return false;
    }
}

void FDX12RHI::Shutdown() {
    CommandList.reset();
    SwapChain.Reset();
    CommandQueue.Reset();
    Device.Reset();
    Factory.Reset();
}

FRHICommandList* FDX12RHI::GetCommandList() {
    return CommandList.get();
}

FRHIBuffer* FDX12RHI::CreateVertexBuffer(uint32 Size, const void* Data) {
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
    if (Data) {
        void* pVertexDataBegin;
        CD3DX12_RANGE readRange(0, 0);
        ThrowIfFailed(vertexBuffer->Map(0, &readRange, &pVertexDataBegin));
        memcpy(pVertexDataBegin, Data, Size);
        vertexBuffer->Unmap(0, nullptr);
        FLog::Log(ELogLevel::Info, "Vertex data copied to buffer");
    }
    
    FLog::Log(ELogLevel::Info, "Vertex buffer created successfully");
    return new FDX12Buffer(vertexBuffer.Detach());
}

FRHIPipelineState* FDX12RHI::CreateGraphicsPipelineState() {
    FLog::Log(ELogLevel::Info, "Creating graphics pipeline state...");
    
    // Simple shader code
    const char* shaderCode = R"(
        struct VSInput {
            float3 position : POSITION;
            float4 color : COLOR;
        };
        
        struct PSInput {
            float4 position : SV_POSITION;
            float4 color : COLOR;
        };
        
        PSInput VSMain(VSInput input) {
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
                         "VSMain", "vs_5_0", 0, 0, &vertexShader, &error))) {
        if (error) {
            FLog::Log(ELogLevel::Error, std::string("Vertex shader compile error: ") + static_cast<char*>(error->GetBufferPointer()));
        }
        return nullptr;
    }
    FLog::Log(ELogLevel::Info, "Vertex shader compiled successfully");
    
    // Compile pixel shader
    if (FAILED(D3DCompile(shaderCode, strlen(shaderCode), nullptr, nullptr, nullptr,
                         "PSMain", "ps_5_0", 0, 0, &pixelShader, &error))) {
        if (error) {
            FLog::Log(ELogLevel::Error, std::string("Pixel shader compile error: ") + static_cast<char*>(error->GetBufferPointer()));
        }
        return nullptr;
    }
    FLog::Log(ELogLevel::Info, "Pixel shader compiled successfully");
    
    // Create root signature
    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    rootSignatureDesc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
    
    ComPtr<ID3DBlob> signature;
    ComPtr<ID3D12RootSignature> rootSignature;
    ThrowIfFailed(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
    ThrowIfFailed(Device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&rootSignature)));
    
    FLog::Log(ELogLevel::Info, "Root signature created");
    
    // Define input layout
    D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
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
    psoDesc.DepthStencilState.DepthEnable = FALSE;
    psoDesc.DepthStencilState.StencilEnable = FALSE;
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

// Factory function
FRHI* CreateDX12RHI() {
    return new FDX12RHI();
}
