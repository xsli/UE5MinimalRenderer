# UE5MinimalRenderer Architecture

## Overview

This project implements a minimal version of Unreal Engine 5's parallel rendering architecture, demonstrating the separation of game logic from rendering through multiple abstraction layers. It serves as an educational tool for understanding modern game engine rendering systems.

## Table of Contents

1. [Layer Architecture](#layer-architecture)
2. [Scene Management System](#scene-management-system)
3. [3D Rendering Pipeline](#3d-rendering-pipeline)
4. [Camera System](#camera-system)
5. [Data Structures](#data-structures)
6. [Key Design Patterns](#key-design-patterns)
7. [DirectX 12 Implementation](#directx-12-specifics)
8. [Text Rendering System](#text-rendering-system)
9. [UE5 Comparison](#ue5-parallel-rendering-comparison)

---

## Layer Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                         APPLICATION                              │
│                      (Runtime/Main.cpp)                          │
│  • Window creation & management                                  │
│  • Main game loop with delta time                                │
│  • Message pump & input handling                                 │
│  • UE5-style mouse controls                                      │
└───────────────────────────┬─────────────────────────────────────┘
                            │
                            ▼
┌─────────────────────────────────────────────────────────────────┐
│                         GAME LAYER                               │
│                        (Game/Game.cpp)                           │
│  • FGame: Main game class                                        │
│  • FScene: Manages FPrimitive objects                            │
│  • FPrimitive: Base class for game objects                       │
│     ├── FCubePrimitive                                           │
│     ├── FSpherePrimitive                                         │
│     ├── FCylinderPrimitive                                       │
│     └── FPlanePrimitive                                          │
└───────────────────────────┬─────────────────────────────────────┘
                            │ Creates scene proxies
                            ▼
┌─────────────────────────────────────────────────────────────────┐
│                       RENDERER LAYER                             │
│                   (Renderer/Renderer.cpp)                        │
│  • FRenderer: Main renderer class                                │
│  • FRenderScene: Render thread proxy management                  │
│  • FPrimitiveSceneProxy: Unified render representation           │
│  • FCamera: 3D camera with view/projection matrices              │
│  • FRenderStats: Performance tracking                            │
│  • Simulates render thread (UE5 uses separate thread)            │
└───────────────────────────┬─────────────────────────────────────┘
                            │ RHI abstraction
                            ▼
┌─────────────────────────────────────────────────────────────────┐
│                    RHI ABSTRACTION LAYER                         │
│                        (RHI/RHI.h)                               │
│  • FRHI: Platform-agnostic interface                             │
│  • FRHICommandList: Command recording                            │
│  • FRHIBuffer: Buffer abstraction (Vertex, Index, Constant)      │
│  • FRHIPipelineState: Pipeline state abstraction                 │
└───────────────────────────┬─────────────────────────────────────┘
                            │ Platform-specific implementation
                            ▼
┌─────────────────────────────────────────────────────────────────┐
│                   DIRECTX 12 BACKEND                             │
│                  (RHI_DX12/DX12RHI.cpp)                          │
│  • FDX12RHI: DirectX 12 implementation                           │
│  • FDX12CommandList: DX12 command list wrapper                   │
│  • FDX12Buffer: DX12 buffer implementation                       │
│  • FDX12PipelineState: PSO management                            │
│  • D3D11on12 interop for text rendering                          │
└───────────────────────────┬─────────────────────────────────────┘
                            │ DirectX 12 API
                            ▼
┌─────────────────────────────────────────────────────────────────┐
│                           GPU                                    │
│  • Command queue execution                                       │
│  • Vertex processing (MVP transform)                             │
│  • Depth testing                                                 │
│  • Pixel shading (color interpolation)                           │
│  • Frame presentation (double-buffered)                          │
└─────────────────────────────────────────────────────────────────┘
```

### Module Dependencies

```
Runtime
  └── Game
       ├── Scene (FScene, FPrimitive)
       └── Renderer
            ├── Camera
            ├── RenderStats
            └── RHI_DX12
                 ├── RHI (interface)
                 └── Core (FVector, FMatrix4x4, etc.)
```

---

## Scene Management System

Following UE5's architecture, the scene is split between game thread and render thread representations.

### Game Thread (FScene + FPrimitive)

```
FScene (Game Thread)
  └── FPrimitive[] (Game objects)
       ├── FCubePrimitive     - Box shape, 24 vertices, 36 indices
       ├── FSpherePrimitive   - UV sphere, configurable segments
       ├── FCylinderPrimitive - Cylinder with caps
       └── FPlanePrimitive    - Grid-based ground plane
```

### Render Thread (FRenderScene + FPrimitiveSceneProxy)

```
FRenderScene (Render Thread)
  └── FPrimitiveSceneProxy[] (Render representations)
       └── Contains GPU resources: VertexBuffer, IndexBuffer, 
           ConstantBuffer, PipelineState
```

### Synchronization Flow

```
Game Thread                    Render Thread
-----------                    -------------
FScene::Tick()
  └─> FPrimitive::Tick()
       (Update transforms)

FGame::Tick()
  └─> Renderer::UpdateFromScene()
       └─> Scene::UpdateRenderScene()
            └─> Create/Update Proxies
                                   FRenderer::RenderFrame()
                                     └─> RenderScene::Render()
                                          └─> Proxy::Render()
```

---

## 3D Rendering Pipeline

### Coordinate Space Transformation

```
OBJECT/LOCAL SPACE (Vertices defined at origin)
        │
        │  [ MODEL MATRIX (M) ]
        │  - Rotation: Object spin
        │  - Translation: World position
        │  - Scaling: Object size
        ▼
WORLD SPACE (All objects in shared world)
        │
        │  [ VIEW MATRIX (V) ]
        │  - Camera position: (0, 0, -5)
        │  - Look at: (0, 0, 0)
        │  - Up vector: (0, 1, 0)
        ▼
CAMERA/VIEW SPACE (Relative to camera)
        │
        │  [ PROJECTION MATRIX (P) ]
        │  - FOV: 45° (π/4 radians)
        │  - Aspect: 16:9
        │  - Near: 0.1, Far: 100.0
        ▼
CLIP SPACE (Homogeneous coordinates)
        │
        │  [ GPU: Perspective Divide ]
        ▼
NORMALIZED DEVICE COORDINATES (NDC)
        │
        │  [ Viewport Transform ]
        ▼
SCREEN SPACE (Final pixels)
```

### Per-Frame Rendering Flow

```
1. Game::Tick()
   └─ Update primitive transforms (rotation, etc.)
   
2. Renderer::RenderFrame()
   ├─ Stats.BeginFrame()           ← Start timing
   ├─ BeginFrame()
   ├─ Clear Render Target (0.2, 0.3, 0.4)
   ├─ Clear Depth Stencil (1.0)
   │
   ├─ RenderScene()
   │   └─ For each FPrimitiveSceneProxy:
   │       - Calculate MVP = M × V × P
   │       - Transpose MVP (HLSL column-major)
   │       - Update Constant Buffer
   │       - SetPipelineState()
   │       - SetConstantBuffer(MVP, slot 0)
   │       - SetVertexBuffer()
   │       - SetIndexBuffer()
   │       - DrawIndexedPrimitive()
   │       - Stats.AddTriangles()
   │
   ├─ FlushCommandsFor2D()         ← Submit 3D, reset for 2D
   ├─ RenderStats()                ← Text overlay
   ├─ EndFrame()
   ├─ Present()
   └─ Stats.EndFrame()             ← Calculate FPS
```

---

## Camera System

### FCamera Properties

```cpp
class FCamera {
    FVector Position;           // Camera location (default: 0, 0, -5)
    FVector LookAtTarget;       // Target point (default: 0, 0, 0)
    FVector UpVector;           // Up direction (default: 0, 1, 0)
    float FovY;                 // Field of view (default: 45°)
    float AspectRatio;          // Width/Height (default: 16:9)
    float NearPlane, FarPlane;  // Clipping (default: 0.1, 100)
    
    // Orientation for mouse control
    float Pitch, Yaw;           // Rotation angles
    FVector Forward, Right, Up; // Direction vectors
    
    // Cached matrices with dirty flags
    FMatrix4x4 ViewMatrix;
    FMatrix4x4 ProjectionMatrix;
};
```

### Coordinate System

Uses **left-handed coordinate system** (DirectX standard):
- **Y-up**: Positive Y is up
- **Z-forward**: Positive Z is into the screen
- **X-right**: Positive X is to the right

```
    Y (Up)
    │
    │
    │
    └────── X (Right)
   ╱
  ╱
 Z (Forward into screen)
```

---

## Data Structures

### FVertex
```cpp
struct FVertex {
    FVector Position;  // X, Y, Z (12 bytes)
    FColor Color;      // R, G, B, A (16 bytes)
    // Total: 28 bytes
};
```

### FMatrix4x4 (DirectXMath Wrapper)
```cpp
struct FMatrix4x4 {
    DirectX::XMMATRIX Matrix;  // 4×4 floats (64 bytes)
    
    // Factory methods
    static Identity();
    static Translation(X, Y, Z);
    static RotationX/Y/Z(Angle);
    static PerspectiveFovLH(...);
    static LookAtLH(eye, target, up);
    
    // Operations
    operator*(other);   // Matrix multiplication
    Transpose();        // For HLSL upload
};
```

### FTransform
```cpp
struct FTransform {
    FVector Position;   // World position
    FVector Rotation;   // Euler angles (radians)
    FVector Scale;      // Scale factors
    // Transform order: Scale → Rotate → Translate
};
```

### Memory Layout (Cube Example)

```
Vertex Buffer (672 bytes = 24 × 28):
┌────────┬────────┬────────┬─────┬────────┐
│ Face 1 │ Face 2 │ Face 3 │ ... │ Face 6 │
│ 4 verts│ 4 verts│ 4 verts│     │ 4 verts│
└────────┴────────┴────────┴─────┴────────┘

Index Buffer (144 bytes = 36 × 4):
┌─────────┬─────────┬─────────┬─────┬─────────┐
│ Face 1  │ Face 2  │ Face 3  │ ... │ Face 6  │
│ 6 idx   │ 6 idx   │ 6 idx   │     │ 6 idx   │
└─────────┴─────────┴─────────┴─────┴─────────┘

Constant Buffer (256 bytes, aligned):
┌──────────────────────────┬────────────────┐
│ MVP Matrix (64 bytes)    │  Padding       │
└──────────────────────────┴────────────────┘
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

---

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

---

## Text Rendering System

Text rendering uses D3D11on12 to bridge DirectX 12 and Direct2D/DirectWrite.

### Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    DirectX 12 Side                          │
│  ┌───────────────────────────────────────────────────────┐  │
│  │ D3D12 Render Targets (2 back buffers)                │  │
│  └───────────────────────┬───────────────────────────────┘  │
│                          │ Wrapped by D3D11On12Device       │
│                          ▼                                   │
│  ┌───────────────────────────────────────────────────────┐  │
│  │ Wrapped Resources (D3D11/D3D12 bridge)               │  │
│  │  • ID3D11Resource                                    │  │
│  │  • Acquire/Release for interop                       │  │
│  └───────────────────────┬───────────────────────────────┘  │
└────────────────────────────┼───────────────────────────────┘
                             │
┌────────────────────────────┼───────────────────────────────┐
│                    Direct2D Side                            │
│                            ▼                                │
│  ┌───────────────────────────────────────────────────────┐  │
│  │ D2D Bitmap Render Targets                            │  │
│  └───────────────────────┬───────────────────────────────┘  │
│                          ▼                                   │
│  ┌───────────────────────────────────────────────────────┐  │
│  │ ID2D1DeviceContext2                                  │  │
│  │  • BeginDraw()                                       │  │
│  │  • DrawTextW()                                       │  │
│  │  • EndDraw()                                         │  │
│  └───────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
```

### Statistics Tracking

```cpp
class FRenderStats {
    uint64 FrameCount;      // Total frames since start
    float FPS;              // Updated every 0.5 seconds
    float FrameTimeMs;      // Current frame time
    uint32 TriangleCount;   // Per-frame accumulation
};
```

---

## UE5 Parallel Rendering Comparison

| Feature | UE5 | This Demo |
|---------|-----|-----------|
| Render Thread | Separate thread | Simulated (same thread) |
| Command Buffering | Multi-frame | Single frame |
| Scene Representation | Complex proxy system | Simple proxy |
| RHI Abstraction | Full multi-platform | DX12 only (but extensible) |
| Resource Streaming | Async streaming | Immediate creation |
| Shader System | Material system | Hardcoded HLSL |

---

## Extension Points

To extend this renderer:

1. **Add new backends**: Implement FRHI interface for Vulkan/Metal
2. **Add new primitives**: Create new FGameObject/FSceneProxy types
3. **Add textures**: Extend RHI with texture support
4. **Add transforms**: Add matrix support to shaders
5. **Add multi-threading**: Move renderer to separate thread
6. **Add async resource loading**: Stream resources on background thread

---

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

---

## Related Documentation

- [README.md](README.md) - Project overview and quick start
- [CHANGELOG.md](CHANGELOG.md) - Version history
- [TODO.md](TODO.md) - Planned features
- [CAMERA_CONTROLS.md](CAMERA_CONTROLS.md) - Input controls documentation
- [BUILD_TEST.md](BUILD_TEST.md) - Build and testing instructions
