# UE5MinimalRenderer
Minimal renderer mimicking UE5 parallel rendering architecture

## Overview
This project demonstrates a simplified version of Unreal Engine 5's parallel rendering architecture with the following layers:
- **Game Layer**: Game logic and tick loop
- **Renderer Layer**: Scene management and render command processing
- **RHI (Render Hardware Interface)**: Platform-agnostic rendering interface
- **DX12 Backend**: DirectX 12 implementation

## Demo
The project renders a **3D rotating cube** with different colored faces:
- Front face: Red
- Back face: Green
- Top face: Blue
- Bottom face: Yellow
- Right face: Magenta
- Left face: Cyan

The cube rotates continuously and demonstrates:
- 3D camera system with perspective projection
- Model-View-Projection (MVP) matrix transformations
- Depth testing and depth buffer
- Indexed rendering
- DirectX left-handed coordinate system

A real-time statistics overlay displays FPS, frame time, and triangle count.

## Interactive Camera Controls

The application supports **UE5-style viewport controls** for interactive camera navigation:

- **Left Mouse Button (LMB) + Drag**: Move forward/backward and rotate left/right
- **Right Mouse Button (RMB) + Drag**: Free-look camera rotation (pitch and yaw)
- **Middle Mouse Button (MMB) + Drag** or **LMB + RMB + Drag**: Pan camera without rotation
- **Mouse Wheel Scroll**: Zoom in/out

For detailed control information, see [CAMERA_CONTROLS.md](CAMERA_CONTROLS.md).

## Architecture
```
Game Tick → Renderer → RHI Interface → DX12 Implementation → GPU
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
├── RHI/            # Render Hardware Interface (platform-agnostic)
├── RHI_DX12/       # DirectX 12 implementation
│   ├── d3dx12*.h   # Microsoft DirectX 12 helper headers (included)
│   └── DX12RHI.*   # DX12 RHI implementation with depth buffer support
├── Renderer/       # Rendering layer with 3D camera system
│   ├── Camera.*    # 3D camera with view/projection matrices
│   └── Renderer.*  # Scene management and rendering
├── Game/           # Game logic layer
│   └── Game.*      # Cube object and game world
└── Runtime/        # Application entry point
```

## Dependencies
All required dependencies are included in the repository:
- **DirectXMath**: Microsoft's SIMD-optimized math library (part of Windows SDK)
- **d3dx12.h and related headers**: Microsoft's DirectX 12 helper headers from [DirectX-Headers](https://github.com/microsoft/DirectX-Headers) (MIT License)
- **Windows SDK**: Provides d3d12.lib, dxgi.lib, d3dcompiler.lib, d2d1.lib, dwrite.lib (installed with Visual Studio)

## Features

### 3D Rendering
- **Math Library**: DirectXMath wrapper with matrix operations
- **Camera System**: Perspective projection with configurable FOV, aspect ratio, near/far planes
- **Interactive Controls**: UE5-style mouse controls for camera navigation
- **Transformation Pipeline**: Model → View → Projection transformations
- **Depth Testing**: Proper occlusion with 32-bit depth buffer
- **Indexed Rendering**: Efficient cube rendering with index buffer

### Rendering Pipeline
- **Vertex Shader**: MVP matrix transformation in HLSL
- **Pixel Shader**: Per-vertex color interpolation
- **Constant Buffers**: 256-byte aligned GPU buffers
- **Text Overlay**: Statistics rendered with Direct2D/DirectWrite

For detailed implementation information, see [3D_IMPLEMENTATION.md](3D_IMPLEMENTATION.md).

## Flow
1. **Game Thread**: Creates game objects and ticks the world
2. **Renderer**: Processes scene proxies and generates render commands
3. **RHI**: Provides platform-agnostic rendering interface
4. **DX12**: Executes commands on GPU using DirectX 12
5. **Output**: Rendered frame displayed on screen
