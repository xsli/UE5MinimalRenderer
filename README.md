# UE5MinimalRenderer

Minimal renderer mimicking UE5 parallel rendering architecture.

## Overview

This project demonstrates a simplified version of Unreal Engine 5's parallel rendering architecture with the following layers:
- **Game Layer**: Scene management with primitives (cube, sphere, cylinder, plane)
- **Renderer Layer**: Camera system, scene proxies, and render command processing
- **TaskGraph Layer**: Multi-threading with dedicated Game, Render, and RHI threads
- **RHI (Render Hardware Interface)**: Platform-agnostic rendering interface
- **DX12 Backend**: DirectX 12 implementation with depth buffer and text rendering

## Demo

The project renders a **3D scene with multiple rotating primitives**:
- **Red Cube**: Auto-rotating box
- **Blue Sphere**: UV sphere with smooth shading
- **Green Cylinder**: Cylindrical primitive
- **Gray Ground Plane**: Static ground surface
- Additional colored objects demonstrating the scene system

Features demonstrated:
- **Multi-threaded rendering**: Separate Game, Render, and RHI threads
- 3D camera system with perspective projection
- Model-View-Projection (MVP) matrix transformations
- Depth testing and depth buffer
- Indexed rendering with multiple primitive types
- UE5-style interactive camera controls
- Real-time statistics overlay (FPS, frame time, triangle count)

## Documentation

| Document | Description |
|----------|-------------|
| [ARCHITECTURE.md](ARCHITECTURE.md) | System architecture and design patterns |
| [CHANGELOG.md](CHANGELOG.md) | Version history and release notes |
| [TODO.md](TODO.md) | Planned features and improvements |
| [CAMERA_CONTROLS.md](CAMERA_CONTROLS.md) | Interactive camera controls guide |
| [BUILD_TEST.md](BUILD_TEST.md) | Build and testing instructions |

## Interactive Camera Controls

The application supports **UE5-style viewport controls** for interactive camera navigation:

- **Left Mouse Button (LMB) + Drag**: Move forward/backward and rotate left/right
- **Right Mouse Button (RMB) + Drag**: Free-look camera rotation (pitch and yaw)
- **Middle Mouse Button (MMB) + Drag** or **LMB + RMB + Drag**: Pan camera without rotation
- **Mouse Wheel Scroll**: Zoom in/out

For detailed control information, see [CAMERA_CONTROLS.md](CAMERA_CONTROLS.md).

## Architecture
```
Game Thread → [ENQUEUE_RENDER_COMMAND] → Render Thread → RHI Thread → GPU
     ↓                                         ↓              ↓
Scene::Tick()                          RenderFrame()     Present()
```

## Building

### Prerequisites
- Windows 10/11
- Visual Studio 2019 or later with C++ development tools
- CMake 3.20 or later
- Windows SDK with DirectX 12 support

### Build Steps
```bash
# Create build directory
mkdir build
cd build

# Generate Visual Studio solution
cmake ..

# Build
cmake --build . --config Release

# Or open the solution in Visual Studio
start UE5MinimalRenderer.sln
```

### Running
```bash
# From build directory
./Source/Runtime/Release/UE5MinimalRenderer.exe
```

## Project Structure
```
Source/
├── Core/           # Core types, math library (DirectXMath wrapper)
├── TaskGraph/      # Multi-threading system
│   ├── TaskGraph.* # Worker thread pool and task system
│   └── RenderCommands.* # Render command queue and thread management
├── RHI/            # Render Hardware Interface (platform-agnostic)
├── RHI_DX12/       # DirectX 12 implementation
│   ├── d3dx12*.h   # Microsoft DirectX 12 helper headers (included)
│   └── DX12RHI.*   # DX12 RHI implementation with depth buffer support
├── Renderer/       # Rendering layer with 3D camera system
│   ├── Camera.*    # 3D camera with view/projection matrices
│   └── Renderer.*  # Scene management and rendering
├── Game/           # Game logic layer
│   ├── Scene.*     # Scene and primitive management
│   ├── Primitive.* # Cube, sphere, cylinder, plane primitives
│   └── Game.*      # Main game class
└── Runtime/        # Application entry point
```

## Dependencies
All required dependencies are included in the repository:
- **DirectXMath**: Microsoft's SIMD-optimized math library (part of Windows SDK)
- **d3dx12.h and related headers**: Microsoft's DirectX 12 helper headers from [DirectX-Headers](https://github.com/microsoft/DirectX-Headers) (MIT License)
- **Windows SDK**: Provides d3d12.lib, dxgi.lib, d3dcompiler.lib, d2d1.lib, dwrite.lib (installed with Visual Studio)

## Features

### Multi-Threading
- **TaskGraph System**: Worker thread pool for parallel task execution
- **Separate Threads**: Dedicated Game, Render, and RHI threads
- **Frame Synchronization**: Game can lead Render by 1 frame
- **Render Commands**: ENQUEUE_RENDER_COMMAND macro for thread-safe command queueing

### 3D Rendering
- **Math Library**: DirectXMath wrapper with matrix operations
- **Camera System**: Perspective projection with configurable FOV, aspect ratio, near/far planes
- **Interactive Controls**: UE5-style mouse controls for camera navigation
- **Transformation Pipeline**: Model → View → Projection transformations
- **Depth Testing**: Proper occlusion with 32-bit depth buffer
- **Indexed Rendering**: Efficient rendering with index buffers
- **Multiple Primitives**: Cube, sphere, cylinder, and plane support

### Scene Management
- **FScene**: Game thread primitive management
- **FPrimitive**: Base class with transform system
- **FPrimitiveSceneProxy**: Unified render thread representation
- **Auto-rotation**: Per-primitive rotation support

### Rendering Pipeline
- **Vertex Shader**: MVP matrix transformation in HLSL
- **Pixel Shader**: Per-vertex color interpolation
- **Constant Buffers**: 256-byte aligned GPU buffers
- **Text Overlay**: Statistics rendered with Direct2D/DirectWrite

For detailed implementation information, see [ARCHITECTURE.md](ARCHITECTURE.md).

## Flow
1. **Game Thread**: Creates game objects, ticks the world, enqueues render commands
2. **Render Thread**: Processes render commands, updates scene proxies, generates RHI commands
3. **RHI Thread**: Executes RHI commands, submits to GPU
4. **GPU**: Executes DirectX 12 commands, presents frame
