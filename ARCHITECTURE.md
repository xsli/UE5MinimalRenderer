# UE5MinimalRenderer Architecture

## Overview
This document describes the architecture of the UE5MinimalRenderer framework.

## Architecture Diagram

```
┌─────────────────────────────────────────────────────────────┐
│                        Application                           │
│                       (Main.cpp)                             │
└────────────────────────┬────────────────────────────────────┘
                         │
                         │ Creates & Runs
                         ▼
┌─────────────────────────────────────────────────────────────┐
│                    Game Layer                                │
│                  (Source/Game/)                              │
│  - Main game loop (Game Thread)                              │
│  - Tick-based updates                                        │
│  - Enqueues render commands                                  │
└────────────────────────┬────────────────────────────────────┘
                         │
                         │ Owns & Commands
                         ▼
┌─────────────────────────────────────────────────────────────┐
│                  Renderer Layer                              │
│                (Source/Renderer/)                            │
│  ┌──────────────────┐         ┌──────────────────┐          │
│  │  Game Thread     │         │  Render Thread   │          │
│  │  - BeginFrame()  │────────▶│  - Command Queue │          │
│  │  - EndFrame()    │  Enqueue│  - Execute Cmds  │          │
│  │  - Commands      │────────▶│  - RHI Calls     │          │
│  └──────────────────┘         └────────┬─────────┘          │
└────────────────────────────────────────┼────────────────────┘
                                         │
                                         │ Uses
                                         ▼
┌─────────────────────────────────────────────────────────────┐
│                     RHI Layer                                │
│                   (Source/RHI/)                              │
│  - Abstract rendering interface (IRHI)                       │
│  - Platform-agnostic API                                     │
│  - BeginFrame/EndFrame/Present/ExecuteCommands               │
└────────────────────────┬────────────────────────────────────┘
                         │
                         │ Implements
                         ▼
┌─────────────────────────────────────────────────────────────┐
│                   DX12 Layer                                 │
│                (Source/RHI/DX12/)                            │
│  - DirectX 12 implementation of IRHI                         │
│  - (Currently stub for demonstration)                        │
└─────────────────────────────────────────────────────────────┘
```

## Thread Model

### Game Thread
- Runs game logic and simulation
- Updates at ~60 FPS (configurable)
- Enqueues render commands to render thread
- Can continue executing while render thread processes previous frame

### Render Thread
- Runs independently from game thread
- Processes queued render commands
- Makes RHI calls to submit GPU work
- Synchronizes with game thread via command queue

## Key Components

### 1. Game (Source/Game/Game.h, Game.cpp)
**Responsibilities:**
- Initialize and shutdown the application
- Run main game loop
- Tick game logic each frame
- Submit render commands to renderer

**Key Methods:**
- `Initialize()` - Set up game and renderer
- `Run()` - Main game loop
- `Tick(float DeltaTime)` - Per-frame update
- `Shutdown()` - Clean shutdown

### 2. Renderer (Source/Renderer/Renderer.h, Renderer.cpp)
**Responsibilities:**
- Manage render thread lifecycle
- Queue and execute render commands
- Coordinate frame synchronization
- Interface with RHI

**Key Methods:**
- `Initialize()` - Create RHI and start render thread
- `StartRenderThread()` - Spawn render thread
- `EnqueueRenderCommand()` - Add command to queue
- `RenderThreadMain()` - Render thread loop
- `StopRenderThread()` - Join render thread

### 3. IRHI (Source/RHI/RHI.h, RHI.cpp)
**Responsibilities:**
- Define abstract rendering interface
- Provide factory for creating RHI implementations

**Key Methods:**
- `Initialize()` - Set up graphics API
- `BeginFrame()` - Start frame rendering
- `EndFrame()` - End frame rendering
- `ExecuteCommands()` - Submit GPU commands
- `Present()` - Present rendered frame

### 4. DX12RHI (Source/RHI/DX12/DX12RHI.h, DX12RHI.cpp)
**Responsibilities:**
- Implement IRHI interface for DirectX 12
- (Current implementation is a stub for demonstration)

## Parallel Rendering Flow

```
Time ──────────────────────────────────────────────────▶

Game Thread:    [Frame N]────[Frame N+1]────[Frame N+2]
                   │             │              │
                   │ Enqueue     │ Enqueue      │ Enqueue
                   │ Commands    │ Commands     │ Commands
                   ▼             ▼              ▼
Render Thread:  [Process N]───[Process N+1]─[Process N+2]
```

## Command Queue Pattern

The renderer uses a thread-safe command queue to enable parallel execution:

1. **Game thread** creates render commands with captured state
2. Commands are **enqueued** to thread-safe queue
3. Game thread **continues** without blocking
4. **Render thread** dequeues and executes commands
5. Condition variable signals when commands available

## Build System

- **CMake 3.15+** for cross-platform builds
- **C++17** standard
- **Threading library** for std::thread support
- Organized source structure with separate directories per layer

## Future Extensions

This framework can be extended with:

1. **Real DX12 Implementation**
   - D3D12 device and command queue creation
   - Swap chain management
   - Resource creation and binding
   - Pipeline state objects

2. **Scene Management**
   - Entity component system
   - Scene graph traversal
   - Frustum culling

3. **Material System**
   - Shader compilation
   - Material parameter binding
   - Render state management

4. **Resource Management**
   - Texture loading and management
   - Buffer allocation
   - Descriptor heap management

5. **Additional RHI Backends**
   - Vulkan implementation
   - Metal implementation (macOS)
   - OpenGL fallback
