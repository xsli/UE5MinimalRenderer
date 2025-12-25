# UE5MinimalRenderer Architecture

## Overview
This project implements a minimal version of Unreal Engine 5's parallel rendering architecture, demonstrating the separation of game logic from rendering through multiple abstraction layers.

## Layer Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                        Application                          │
│                      (Runtime/Main.cpp)                     │
│  • Window creation & management                             │
│  • Main game loop                                           │
│  • Message pump                                             │
└────────────────────────┬────────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────────────┐
│                        Game Layer                           │
│                      (Game/Game.cpp)                        │
│  • FGame: Main game class                                   │
│  • FGameWorld: Manages game objects                         │
│  • FGameObject: Base for renderable entities                │
│  • FTriangleObject: Example game entity                     │
└────────────────────────┬────────────────────────────────────┘
                         │ Creates scene proxies
                         ▼
┌─────────────────────────────────────────────────────────────┐
│                      Renderer Layer                         │
│                   (Renderer/Renderer.cpp)                   │
│  • FRenderer: Main renderer class                           │
│  • FSceneProxy: Renderable representation                   │
│  • FTriangleMeshProxy: Triangle rendering proxy             │
│  • Simulates render thread (UE5 uses separate thread)       │
└────────────────────────┬────────────────────────────────────┘
                         │ RHI abstraction
                         ▼
┌─────────────────────────────────────────────────────────────┐
│                    RHI Abstraction Layer                    │
│                       (RHI/RHI.h)                           │
│  • FRHI: Platform-agnostic interface                        │
│  • FRHICommandList: Command recording                       │
│  • FRHIBuffer: Buffer abstraction                           │
│  • FRHIPipelineState: Pipeline state abstraction            │
└────────────────────────┬────────────────────────────────────┘
                         │ Platform-specific implementation
                         ▼
┌─────────────────────────────────────────────────────────────┐
│                   DirectX 12 Backend                        │
│                  (RHI_DX12/DX12RHI.cpp)                     │
│  • FDX12RHI: DirectX 12 implementation                      │
│  • FDX12CommandList: DX12 command list wrapper              │
│  • FDX12Buffer: DX12 buffer implementation                  │
│  • FDX12PipelineState: PSO management                       │
└────────────────────────┬────────────────────────────────────┘
                         │ DirectX 12 API
                         ▼
┌─────────────────────────────────────────────────────────────┐
│                          GPU                                │
│                  (DirectX 12 Runtime)                       │
│  • Command queue execution                                  │
│  • Vertex processing                                        │
│  • Pixel shading                                            │
│  • Frame presentation                                       │
└─────────────────────────────────────────────────────────────┘
```

## Module Dependencies

```
Runtime
  └── Game
       └── Renderer
            └── RHI_DX12
                 ├── RHI (interface)
                 └── Core
```

## Data Flow: Triangle Rendering

### 1. Initialization Phase
```
WinMain (Main.cpp)
  └── FGame::Initialize()
       ├── CreateDX12RHI()
       │    └── Initialize D3D12 device, swap chain, command queue
       ├── FRenderer::Initialize()
       └── FGameWorld::Initialize()
            └── FTriangleObject created
                 └── CreateSceneProxy() called
                      ├── Create vertex buffer (3 vertices with positions & colors)
                      ├── Create pipeline state (shaders, input layout)
                      └── Return FTriangleMeshProxy
```

### 2. Per-Frame Rendering
```
Game Loop (Main.cpp)
  └── FGame::Tick(deltaTime)
       ├── FGameWorld::Tick(deltaTime)
       │    └── FTriangleObject::Tick() [Game thread logic]
       │
       └── FRenderer::RenderFrame() [Simulated render thread]
            ├── FRHI::BeginFrame()
            ├── FRHICommandList::BeginFrame()
            │    ├── Reset command allocator/list
            │    ├── Set viewport & scissor
            │    └── Transition RT to RENDER_TARGET state
            │
            ├── FRHICommandList::ClearRenderTarget()
            │    └── Clear to dark blue color
            │
            ├── FRenderer::RenderScene()
            │    └── For each FSceneProxy:
            │         └── FTriangleMeshProxy::Render()
            │              ├── SetPipelineState() [shaders, input layout]
            │              ├── SetVertexBuffer() [triangle vertices]
            │              └── DrawPrimitive(3, 0) [draw 3 vertices]
            │
            ├── FRHICommandList::EndFrame()
            │    ├── Transition RT to PRESENT state
            │    ├── Close command list
            │    └── Submit to command queue
            │
            ├── FRHI::EndFrame()
            └── FRHICommandList::Present()
                 ├── Swap buffers (vsync)
                 └── Wait for GPU
```

## Key Design Patterns

### 1. Factory Pattern
- `CreateDX12RHI()` creates platform-specific RHI implementation
- Allows easy addition of other backends (Vulkan, Metal, etc.)

### 2. Command Pattern
- `FRHICommandList` records commands
- Commands executed when submitted to GPU
- Mimics UE5's deferred command execution

### 3. Proxy Pattern
- `FSceneProxy` represents game objects in renderer
- Decouples game logic from rendering
- Allows game objects to be destroyed while proxy still renders

### 4. Interface Segregation
- RHI provides minimal platform-agnostic interface
- Platform-specific details hidden in implementation
- Easy to swap backends without changing upper layers

## Resource Lifecycle

### Vertex Buffer
```
Create: FTriangleObject::CreateSceneProxy()
  └── FRHI::CreateVertexBuffer()
       └── FDX12RHI::CreateVertexBuffer()
            └── ID3D12Resource created in upload heap

Use: Per-frame
  └── FRHICommandList::SetVertexBuffer()
       └── IASetVertexBuffers()

Destroy: FTriangleMeshProxy::~FTriangleMeshProxy()
  └── FDX12Buffer::~FDX12Buffer()
       └── ComPtr auto-releases
```

### Pipeline State
```
Create: FTriangleObject::CreateSceneProxy()
  └── FRHI::CreateGraphicsPipelineState()
       └── Compile shaders (VS/PS)
       └── Create root signature
       └── Create PSO with input layout

Use: Per-frame
  └── FRHICommandList::SetPipelineState()
       └── SetPipelineState() + SetGraphicsRootSignature()

Destroy: FTriangleMeshProxy::~FTriangleMeshProxy()
  └── FDX12PipelineState::~FDX12PipelineState()
       └── ComPtr auto-releases
```

## Shader Pipeline

### Vertex Shader
```hlsl
Input: 
  - float3 position (POSITION)
  - float4 color (COLOR)

Processing:
  - Transform to clip space: float4(position, 1.0)
  - Pass through color

Output:
  - float4 position (SV_POSITION)
  - float4 color (COLOR)
```

### Pixel Shader
```hlsl
Input:
  - float4 color (COLOR) [interpolated]

Processing:
  - Pass through color (no lighting)

Output:
  - float4 color (SV_TARGET)
```

## DirectX 12 Specifics

### Command Queue Execution
1. Reset command allocator/list
2. Record commands (clear, draw, etc.)
3. Close command list
4. ExecuteCommandLists() on command queue
5. Signal fence
6. Wait for fence completion

### Synchronization
- Double buffering (2 render targets)
- Fence per frame for GPU/CPU sync
- Waits for GPU before presenting next frame

### Memory Management
- Upload heap for vertex buffer (CPU writable)
- No intermediate copy in this simple demo
- ComPtr for automatic reference counting

## UE5 Parallel Rendering Comparison

| Feature | UE5 | This Demo |
|---------|-----|-----------|
| Render Thread | Separate thread | Simulated (same thread) |
| Command Buffering | Multi-frame | Single frame |
| Scene Representation | Complex proxy system | Simple proxy |
| RHI Abstraction | Full multi-platform | DX12 only (but extensible) |
| Resource Streaming | Async streaming | Immediate creation |
| Shader System | Material system | Hardcoded HLSL |

## Extension Points

To extend this renderer:

1. **Add new backends**: Implement FRHI interface for Vulkan/Metal
2. **Add new primitives**: Create new FGameObject/FSceneProxy types
3. **Add textures**: Extend RHI with texture support
4. **Add transforms**: Add matrix support to shaders
5. **Add multi-threading**: Move renderer to separate thread
6. **Add async resource loading**: Stream resources on background thread

## Performance Considerations

### Current Implementation
- Synchronous rendering (waits for GPU each frame)
- No resource pooling
- Immediate vertex buffer upload
- Single-threaded

### Potential Optimizations
- Triple buffering for less GPU waiting
- Command allocator/list pooling
- Separate upload thread for resources
- Parallel command list recording
- Culling & batching
