# UE5MinimalRenderer

Minimal renderer mimicking UE5 parallel rendering architecture.

## Overview

This project demonstrates a simplified version of Unreal Engine 5's parallel rendering architecture with the following layers:
- **Game Layer**: Scene management with primitives (cube, sphere, cylinder, plane)
- **Renderer Layer**: Camera system, scene proxies, shadow mapping, and render command processing
- **TaskGraph Layer**: Multi-threading with dedicated Game, Render, and RHI threads
- **RHI (Render Hardware Interface)**: Platform-agnostic rendering interface
- **DX12 Backend**: DirectX 12 implementation with depth buffer, shadow maps, and text rendering

## Demo

The project renders a **3D scene with multiple rotating primitives**:
- **Red Cube**: Auto-rotating box
- **Blue Sphere**: UV sphere with smooth shading
- **Green Cylinder**: Cylindrical primitive
- **Gray Ground Plane**: Static ground surface
- Additional colored objects demonstrating the scene system

Features demonstrated:
- **Multi-threaded rendering**: Separate Game, Render, and RHI threads
- **Shadow Mapping**: Directional light and point light shadows with PCF filtering
- **RT Pool System**: Efficient render texture management with pooling
- 3D camera system with perspective projection
- Model-View-Projection (MVP) matrix transformations
- Depth testing and depth buffer
- Indexed rendering with multiple primitive types
- UE5-style interactive camera controls
- Real-time statistics overlay (FPS, frame time, triangle count, draw calls, RT pool stats)

## Shadow Mapping System

### Features
- **Directional Light Shadows**: Orthographic projection shadow maps (1024x1024 D32_FLOAT)
- **Point Light Shadows**: Omnidirectional shadow mapping with 6-face atlas (512x512 per face, 3x2 grid)
- **Shadow Bias**: Configurable constant bias and slope-scaled bias to eliminate shadow acne
- **RHI Shadow Pass API**: BeginShadowPass/EndShadowPass for custom render target binding
- **Viewport Control**: Dynamic viewport setting for atlas region rendering

### Shadow Map Constant Buffer Structure (HLSL)
```hlsl
cbuffer ShadowBuffer : register(b2)
{
    // Directional light shadow matrix (world -> light clip space)
    float4x4 DirLightViewProj;        // 64 bytes
    
    // Point light shadow matrices (6 faces for omnidirectional)
    float4x4 PointLight0ViewProj[6];  // 384 bytes
    float4x4 PointLight1ViewProj[6];  // 384 bytes
    
    // Shadow parameters: x = constant bias, y = slope bias, z = PCF radius, w = strength
    float4 ShadowParams;              // 16 bytes
    
    // Shadow info: x = enabled, y = map size, z = near, w = far
    float4 DirShadowInfo;             // 16 bytes
    float4 PointLight0ShadowInfo;     // 16 bytes
    float4 PointLight1ShadowInfo;     // 16 bytes
    
    // Atlas offsets for point light faces (UV offset/scale)
    float4 PointLight0AtlasOffsets[6]; // 96 bytes
    float4 PointLight1AtlasOffsets[6]; // 96 bytes
};
```

## RT Pool System

The render texture pool efficiently manages GPU texture resources through pooling and lifecycle management.

### Features
- **Descriptor-Based Lookup**: Textures are pooled by (Width, Height, Format, MipLevels, ArraySize, SampleCount)
- **Automatic Reuse**: Matching textures are reused across frames
- **Lifecycle Management**: Unused textures are automatically cleaned up after timeout
- **Capacity Limits**: Configurable maximum pool size to prevent VRAM overflow

### Pool Flow
```
BeginFrame(frameNumber)
    └── Mark pool ready for new frame

Fetch(descriptor)
    ├── Look for matching idle RT in pool
    │   └── If found: mark as active, return
    └── If not found: create new RT (if under capacity)

Release(RT)
    └── Mark RT as idle, available for reuse

EndFrame()
    └── Cleanup stale RTs (unused for 60+ frames)
```

### Configuration
- `MaxCapacity`: Default 64 textures
- `CleanupTimeoutFrames`: Default 60 frames (~1 second at 60fps)

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
├── Shaders/        # HLSL shader files (Unreal Engine naming convention)
│   ├── Common.ush  # Shared vertex structures
│   ├── LightingCommon.ush # Lighting constant buffers and functions
│   ├── BasePassVertexShader.usf # Unlit base pass
│   ├── ShadowDepthVertexShader.usf # Shadow depth pass
│   ├── ForwardLightingPixelShader.usf # Lit forward rendering
│   └── ShaderCompiler.* # Shader compilation and caching system
├── RHI/            # Render Hardware Interface (platform-agnostic)
├── RHI_DX12/       # DirectX 12 implementation
│   ├── d3dx12*.h   # Microsoft DirectX 12 helper headers (included)
│   └── DX12RHI.*   # DX12 RHI implementation with depth buffer support
├── Renderer/       # Rendering layer with 3D camera system
│   ├── Camera.*    # 3D camera with view/projection matrices
│   ├── Renderer.*  # Scene management and rendering
│   ├── RTPool.*    # Render texture pool system
│   └── ShadowMapping.* # Shadow map generation and sampling
├── Lighting/       # Lighting system
│   ├── Light.*     # Light types (directional, point)
│   └── LightingConstants.* # GPU constant buffer for lighting
├── Scene/          # Scene management
│   ├── Scene.*     # Scene container
│   ├── ScenePrimitive.* # Primitive types
│   ├── LitSceneProxy.* # Lit rendering proxy
│   └── UnlitSceneProxy.* # Unlit rendering proxy
├── Game/           # Game logic layer
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

### Shadow Mapping
- **Directional Light Shadows**: Orthographic projection for sun-like lights
- **Point Light Shadows**: 6-face omnidirectional shadow atlas
- **Shadow Bias**: Constant and slope-scaled bias for artifact reduction
- **PCF Filtering**: Smooth shadow edges with 3x3 kernel

### Resource Management
- **RT Pool**: Pooled render texture allocation with automatic cleanup
- **Lifecycle Tracking**: Frame-based timeout for unused resources
- **Memory Limits**: Configurable pool capacity

### Scene Management
- **FScene**: Game thread primitive management
- **FPrimitive**: Base class with transform system
- **FPrimitiveSceneProxy**: Unified render thread representation
- **Auto-rotation**: Per-primitive rotation support

### Rendering Pipeline
- **Vertex Shader**: MVP matrix transformation in HLSL
- **Pixel Shader**: Per-vertex color interpolation with lighting
- **Constant Buffers**: 256-byte aligned GPU buffers
- **Text Overlay**: Statistics rendered with Direct2D/DirectWrite

### Graphics Debugging with RenderDoc

This project supports [RenderDoc](https://renderdoc.org/) for frame capture and graphics debugging.

**How to Use:**
1. Download and install RenderDoc from https://renderdoc.org/
2. Launch RenderDoc
3. Go to: **File > Launch Application**
4. Set **Executable Path** to: `UE5MinimalRenderer.exe`
5. Set **Working Directory** to the build directory
6. Click **Launch**
7. In the application, press **F12** to capture a frame
8. Click on the capture in RenderDoc to analyze

**Debugging Shadow Maps:**
1. Capture a frame with F12
2. In the Event Browser, find "BeginShadowPass" or "ClearDepthStencilView" events
3. Click on draw calls during the shadow pass
4. Go to the **Texture Viewer** tab
5. Select the depth attachment to view shadow map content
6. White = far depth (1.0), Black = near depth (0.0)

**Troubleshooting:**
- **Shadow map is all white (empty)**: Objects may be outside the light frustum, or bCastShadow is false
- **Shadow map has content but shadows don't appear**: Check shader shadow sampling code
- **No shadow pass events**: Verify shadow system initialization and RenderShadowPasses is called

For detailed implementation information, see [ARCHITECTURE.md](ARCHITECTURE.md).

## Flow
1. **Game Thread**: Creates game objects, ticks the world, enqueues render commands
2. **Render Thread**: Processes render commands, updates scene proxies, generates RHI commands
3. **RHI Thread**: Executes RHI commands, submits to GPU
4. **GPU**: Executes DirectX 12 commands, presents frame
