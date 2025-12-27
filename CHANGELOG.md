# Changelog

All notable changes to the UE5MinimalRenderer project are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]

### Planned
- See [TODO.md](TODO.md) for planned features

---

## [0.7.0] - Lighting System

### Added
- **Lighting Module (Lighting/)**
  - `FLight` base class for all light types
  - `FDirectionalLight` - Parallel light rays (sunlight)
  - `FPointLight` - Omni-directional point source with attenuation
  - `FLightScene` - Scene light management container
  - `FLightingConstants` - GPU constant buffer data structure

- **Material System**
  - `FMaterial` struct with Phong properties
  - Diffuse, Specular, Ambient colors
  - Shininess/specular power
  - Factory methods: `Diffuse()`, `Glossy()`, `Metal()`

- **Lit Rendering**
  - `FLitVertex` - Vertex format with position, normal, color
  - `FLitPrimitiveSceneProxy` - Lit primitive rendering proxy
  - `FLitCubePrimitive` - Lit cube with per-face normals
  - `FLitSpherePrimitive` - Lit sphere with smooth normals
  - `FLitPlanePrimitive` - Lit ground plane
  - `FLitCylinderPrimitive` - Lit cylinder

- **Phong/Blinn-Phong Shader**
  - Directional light with diffuse and specular
  - Point lights (up to 4) with radius-based attenuation
  - Ambient lighting contribution
  - Blinn-Phong specular highlights (half-vector method)

- **Light Visualization**
  - `FLightVisualization` helper class
  - Wireframe arrow for directional lights
  - Wireframe sphere showing point light radius
  - Cross marker at light position

- **RHI Extensions**
  - `EPipelineFlags` enum for flexible PSO creation
  - `CreateGraphicsPipelineStateEx()` with flags
  - `SetPrimitiveTopology()` for line/triangle switching
  - Support for line list primitive topology

- **Demo Scene**
  - Daylight scene with sun (directional) and fill light
  - Two point lights (warm orange, cool blue)
  - Cubes with various materials (matte, glossy, metallic)
  - Spheres and cylinders showcasing lighting

### Technical Details
- Two constant buffer slots: b0 (MVP), b1 (Lighting data)
- Normal transformation in world space
- Smooth attenuation with inverse-square falloff
- 384-byte lighting constant buffer (256-byte aligned)

---

## [0.6.0] - Multi-Threading Architecture

### Added
- **TaskGraph System (TaskGraph/)**
  - `FTaskGraph` class - Worker thread pool for parallel task execution
  - `FTask` base class - Executable task with completion event
  - `FLambdaTask` - Lambda-based task for quick task creation
  - `FTaskEvent` - Synchronization primitive for task completion
  - Auto-detection of hardware thread count

- **Named Thread Management**
  - `FThreadManager` class - Manages named threads (Game, Render, RHI)
  - `ENamedThreads` enum - Thread type identification
  - Thread registration and validation macros

- **Render Command System**
  - `FRenderCommandQueue` - Thread-safe queue for render commands
  - `FRenderCommandBase` - Base class for render commands
  - `FLambdaRenderCommand` - Lambda-based render command
  - `ENQUEUE_RENDER_COMMAND` macro - UE5-style command enqueuing

- **Render Thread**
  - `FRenderThread` class - Dedicated render thread management
  - Command processing loop
  - Frame-based synchronization

- **RHI Thread**
  - `FRHIThread` class - Dedicated RHI/GPU command thread
  - Work queue for RHI operations
  - Separate GPU submission

- **Frame Synchronization**
  - `FFrameSyncManager` class - Game/Render/RHI synchronization
  - 1-frame lead support (Game can be 1 frame ahead of Render)
  - `FRenderFence` - Fence for synchronization between threads

### Changed
- `FGame` class now supports both single-threaded and multi-threaded modes
- Added `TickSingleThreaded()` and `TickMultiThreaded()` methods
- Multi-threading is enabled by default
- Updated layer architecture to include TaskGraph module

### Technical Details
- Game thread can lead render thread by up to 1 frame
- Condition variable-based synchronization for frame boundaries
- Worker thread pool reserves threads for named threads
- Thread-safe command queue with producer-consumer pattern

---

## [0.5.0] - Scene Management System

### Added
- **Scene Management Architecture**
  - `FScene` class for game thread primitive management
  - `FRenderScene` class for render thread proxy management
  - `FPrimitive` base class with transform system (position, rotation, scale)
  - `FPrimitiveSceneProxy` unified render representation
  - Synchronization flow between game and render scenes

- **Multiple Primitive Types**
  - `FCubePrimitive` - 24 vertices, 36 indices, box shape
  - `FSpherePrimitive` - UV sphere with configurable segments/rings
  - `FCylinderPrimitive` - Circular caps with configurable segments
  - `FPlanePrimitive` - Grid-based subdivision for ground planes

- **Transform System**
  - `FTransform` struct with Position, Rotation (Euler), Scale
  - Transformation order: Scale → Rotate → Translate
  - Per-primitive auto-rotation support

- **Demo Scene**
  - 6 primitive objects with different colors and transformations
  - Red cube, blue sphere, green cylinder, gray ground plane, yellow/magenta small objects

### Changed
- Refactored `FGameObject` to `FPrimitive` pattern
- Updated renderer to use unified `FPrimitiveSceneProxy`

---

## [0.4.0] - Interactive Camera Controls

### Added
- **UE5-Style Mouse Controls**
  - Left Mouse Button (LMB) + Drag: Move forward/backward and rotate left/right
  - Right Mouse Button (RMB) + Drag: Free-look camera rotation (pitch and yaw)
  - Middle Mouse Button (MMB) or LMB+RMB + Drag: Pan camera without rotation
  - Mouse Wheel: Zoom in/out

- **Camera Orientation System**
  - Pitch/Yaw rotation system replacing look-at target
  - Forward/Right/Up direction vectors
  - Pitch clamping to ±89° to prevent gimbal lock

- **Input Processing**
  - `FInputState` structure for mouse state tracking
  - Mouse capture for dragging outside window
  - Configurable sensitivity constants

### Changed
- Enhanced `FCamera` class with movement and rotation methods
- Added `GetCamera()` method to `FGame` for input access
- Updated `Main.cpp` with Windows message handlers for mouse input

---

## [0.3.0] - 3D Rendering System

### Added
- **Math Library (DirectXMath Wrapper)**
  - `FMatrix4x4` struct with SIMD-optimized operations
  - Identity, Translation, Rotation, Scaling matrix factories
  - `PerspectiveFovLH` and `LookAtLH` for camera matrices
  - Matrix multiplication and transpose operations

- **3D Camera System**
  - `FCamera` class with View and Projection matrices
  - Configurable FOV, aspect ratio, near/far planes
  - Lazy matrix updates with dirty flags
  - Left-handed coordinate system (DirectX standard)

- **Cube Geometry**
  - 24 vertices (4 per face) with distinct face colors
  - 36 indices for indexed rendering
  - Face colors: Red (front), Green (back), Blue (top), Yellow (bottom), Magenta (right), Cyan (left)

- **Depth Buffer Support**
  - 32-bit float depth buffer (D32_FLOAT)
  - Depth testing with LESS comparison
  - Depth clear to 1.0 each frame

- **RHI Extensions**
  - `CreateIndexBuffer()`, `CreateConstantBuffer()`
  - `SetIndexBuffer()`, `SetConstantBuffer()`
  - `ClearDepthStencil()`, `DrawIndexedPrimitive()`
  - Pipeline state with optional depth testing

- **MVP Shader System**
  - Constant buffer for MVP matrix (256-byte aligned)
  - Vertex shader with matrix transformation
  - Matrix transposition for HLSL column-major layout

### Changed
- Updated demo from 2D triangle to 3D rotating cube
- Triangle count in statistics now shows 12 (for cube)

---

## [0.2.0] - Text Rendering and Statistics

### Added
- **Text Rendering API**
  - `DrawText(text, position, fontSize, color)` in RHI interface
  - Configurable position, font size, and RGBA color
  - Implementation using DirectWrite + Direct2D via D3D11on12

- **Statistics Tracking System**
  - `FRenderStats` class for performance metrics
  - Frame count (total frames since start)
  - FPS (frames per second, 0.5-second averaging)
  - Frame time (milliseconds per frame)
  - Triangle count (per frame accumulation)

- **Visual Statistics Overlay**
  - Yellow text in top-left corner (10, 10)
  - Arial 18pt font
  - Real-time display of all tracked metrics

### Technical Details
- D3D11on12 interop for DX12/D2D bridging
- Wrapped back buffers for D2D bitmap render targets
- High-resolution clock for accurate timing

---

## [0.1.0] - Initial Release

### Added
- **Four-Layer Architecture**
  - Application Layer (Runtime/Main.cpp) - Window management, main loop
  - Game Layer (Game/) - Game logic, world, objects
  - Renderer Layer (Renderer/) - Scene management, render commands
  - RHI Layer (RHI/) - Platform-agnostic rendering interface
  - DX12 Backend (RHI_DX12/) - DirectX 12 implementation

- **Core Components**
  - `FVector`, `FColor`, `FVector2D` types
  - Logging system with log levels
  - DirectX 12 device, command queue, swap chain

- **Rendering Pipeline**
  - Vertex buffer creation and upload
  - HLSL shader compilation (vertex + pixel)
  - Pipeline state object (PSO) management
  - Double-buffered swap chain with VSync
  - GPU/CPU synchronization with fences

- **Demo Application**
  - Colored triangle with vertex colors (red, green, blue)
  - 1280x720 window with dark blue background
  - Smooth color interpolation between vertices

- **Build System**
  - CMake configuration with modular structure
  - `build.bat` script for Windows
  - Auto-detection of Visual Studio version

### Technical Details
- DirectX 12 with hardware adapter selection
- Render target view (RTV) descriptor heap
- Resource state transitions (RENDER_TARGET ↔ PRESENT)
- Microsoft DirectX helper headers (d3dx12*.h) included

---

## Version History Summary

| Version | Release | Key Features |
|---------|---------|--------------|
| 0.7.0 | Lighting System | Phong shading, directional/point lights, materials, light visualization |
| 0.6.0 | Multi-Threading | TaskGraph, FRenderThread, FRHIThread, 1-frame lead sync |
| 0.5.0 | Scene Management | FScene, FPrimitive, multiple primitive types |
| 0.4.0 | Camera Controls | UE5-style mouse controls, camera orientation |
| 0.3.0 | 3D Rendering | DirectXMath, camera system, depth buffer, cube |
| 0.2.0 | Text & Stats | DrawText API, FRenderStats, overlay display |
| 0.1.0 | Initial | 4-layer architecture, DX12 backend, triangle demo |
