# GitHub Copilot Instructions for UE5MinimalRenderer

## Project Overview

UE5MinimalRenderer is a minimal educational renderer that demonstrates Unreal Engine 5's parallel rendering architecture. The project implements a simplified version of UE5's multi-layered rendering system with DirectX 12 backend.

### Core Architecture Layers
1. **Application Layer** (Runtime/Main.cpp) - Window management and main loop
2. **Game Layer** (Game/) - Game logic, world, and primitives
3. **Renderer Layer** (Renderer/) - Scene management and render command processing
4. **RHI Layer** (RHI/) - Platform-agnostic rendering interface
5. **DX12 Backend** (RHI_DX12/) - DirectX 12 implementation

### Key Design Principles
- **Separation of Concerns**: Game logic is completely decoupled from rendering
- **Proxy Pattern**: Scene proxies represent game objects on the render thread
- **Command Pattern**: RHI command lists record and execute rendering commands
- **Factory Pattern**: Platform-specific RHI implementations created via factory functions
- **Left-Handed Coordinate System**: DirectX standard (Y-up, Z-forward, X-right)

## Coding Style - Unreal Engine Conventions

### Naming Conventions

#### Prefixes
- **F** prefix for classes/structs: `FGame`, `FRenderer`, `FCamera`, `FVector`
- **E** prefix for enums: `ELogLevel`, `EBufferType`
- **b** prefix for booleans: `bLeftMouseDown`, `bEnableDepth`, `bAutoRotate`
- **I** prefix for interfaces: `IRHI` (though we use abstract classes)
- **In** prefix for function parameters: `InPosition`, `InColor`, `InTransform`

#### Class Members
- Public member variables use PascalCase: `Position`, `Rotation`, `Scale`
- Private/protected members have no prefix (but could use lowercase or camelCase for locals)
- Use meaningful, descriptive names: `VertexBuffer`, `PipelineState`, `ModelMatrix`

#### Functions
- Use PascalCase for all functions: `Initialize()`, `Tick()`, `RenderFrame()`, `CreateSceneProxy()`
- Getters/Setters: `GetPosition()`, `SetColor()`, `GetTransform()`
- Boolean queries: `IsValid()`, `IsInitialized()` (not used extensively in this codebase)

#### Constants and Namespaces
- Use `constexpr` for compile-time constants
- Group related constants in namespaces: `namespace CameraSettings { constexpr float MovementSpeed = 0.01f; }`

### Code Structure

#### Headers
```cpp
#pragma once

#include "CoreTypes.h"  // Local includes first
#include <memory>       // Standard library after
#include <Windows.h>    // Platform headers last

// Forward declarations before class definitions
class FRenderer;
class FRHI;

// Class definition
class FGame 
{
public:
    FGame();
    ~FGame();
    
    bool Initialize(void* WindowHandle, uint32 Width, uint32 Height);
    void Tick(float DeltaTime);
    
private:
    std::unique_ptr<FRHI> RHI;
    std::unique_ptr<FRenderer> Renderer;
};
```

#### Implementation Files
```cpp
#include "ClassName.h"  // Own header first
#include "OtherLocal.h"  // Other local headers
#include <iostream>      // Standard library last

// Implementation
FClassName::FClassName()
{
    // Constructor implementation
}
```

#### Braces and Indentation
- **Opening brace on new line** (Allman style) for functions and control structures:
```cpp
void Function()
{
    if (condition)
    {
        // code
    }
}
```
- Use **tabs** for indentation (UE convention, though this project may use spaces - follow existing style)
- Consistent indentation throughout

#### Comments
- Use `//` for single-line comments
- Use `/** */` or `/* */` for multi-line documentation
- Add comments for complex algorithms and non-obvious code
- Document public API functions
- Keep comments concise and meaningful

### Type Usage
- Prefer UE types: `uint32`, `uint8`, `int32` over standard `unsigned int`, etc.
- Use `float` for floating-point (not double unless necessary)
- Use `FVector`, `FColor`, `FMatrix4x4` wrapper types
- Use `std::unique_ptr` for ownership, raw pointers for non-owning references

### Memory Management
- Use RAII principles (constructors/destructors)
- `std::unique_ptr` for single ownership
- `std::shared_ptr` sparingly (prefer clear ownership)
- Manual `new`/`delete` only when interfacing with C APIs or when ownership is complex
- Note: RHI resources use manual `new`/`delete` as they follow RHI ownership pattern

## Main Loop Design - UE5 Pattern

### Game Loop Structure
The main loop follows UE5's tick-based architecture:

```cpp
while (Running) 
{
    // 1. Process input/events
    ProcessWindowMessages();
    
    // 2. Calculate delta time
    float DeltaTime = CalculateDeltaTime();
    
    // 3. Game tick (game thread simulation)
    Game->Tick(DeltaTime);
    
    // 4. Render frame (simulated render thread)
    // In UE5, this would be on a separate thread
}
```

### Tick Flow
```
Main Loop
  └── FGame::Tick(DeltaTime)
       ├── FScene::Tick(DeltaTime)       // Game thread: Update game objects
       │    └── FPrimitive::Tick()        // Individual object updates
       │         └── Auto-rotation, physics, etc.
       │
       └── FRenderer::RenderFrame()       // Simulated render thread
            ├── BeginFrame()               // Prepare rendering
            ├── RenderScene()              // Process scene proxies
            │    └── Proxy::Render()       // Execute draw calls
            ├── RenderStats()              // Overlay text
            └── EndFrame()                 // Present frame
```

### Key Concepts

#### Game Thread vs Render Thread
- **Game Thread**: Updates game logic, physics, AI (FPrimitive::Tick())
- **Render Thread**: Processes rendering commands (FRenderer::RenderFrame())
- **Proxy Pattern**: Scene proxies allow game objects to be destroyed while still rendering
- **Current Implementation**: Single-threaded (simulated), but architecture supports multi-threading

#### Frame Synchronization
- Double buffering for render targets
- Fence-based GPU/CPU synchronization
- Command allocator/list management per frame
- Clear separation between game state and render state

#### Resource Lifecycle
```
Game Object (FPrimitive)
  └── CreateSceneProxy()  // Called once or when dirty
       └── Create RHI resources (buffers, pipeline states)
       └── Return FPrimitiveSceneProxy

Per Frame:
  └── Proxy->Render(RHICmdList)  // Record draw commands
       └── SetPipelineState()
       └── SetBuffers()
       └── Draw()
```

## Documentation

### Documentation Files
- **README.md**: Project overview, build instructions, features, controls
- **ARCHITECTURE.md**: System architecture, layer descriptions, design patterns
- **CHANGELOG.md**: Version history and release notes
- **TODO.md**: Planned features and improvements
- **CAMERA_CONTROLS.md**: Interactive camera controls guide
- **BUILD_TEST.md**: Build and testing instructions

### Documentation Style
- Use clear headers (# ## ###)
- Code blocks with language hints: ```cpp, ```hlsl, ```bash
- Lists for step-by-step instructions
- Be precise and concise

## Using Unreal Engine Documentation

When working with UE5-specific patterns, use the **Context7 MCP tool** to access official documentation.

### Context7 Usage
- Use `resolve-library-id` with "unreal engine" to get the library ID
- Use `get-library-docs` with topics like: `FScene`, `FPrimitiveSceneProxy`, `FRHICommandList`, `RDG`
- Mode: "code" for API references, "info" for conceptual guides

**Note**: This project is educational - adapt complex UE5 patterns to our simplified architecture.

## Development Workflow

### Making Changes
1. Review ARCHITECTURE.md to understand the layer structure
2. Respect layer boundaries (Game → Renderer → RHI → DX12)
3. Test thoroughly - build and run, verify visual output
4. Commit with descriptive messages

### Adding New Primitives
1. Create primitive class in `Game/Primitive.h` (inherit from `FPrimitive`)
2. Implement `CreateSceneProxy()` to create render representation
3. Create proxy class in `Renderer/Renderer.h` (inherit from `FPrimitiveSceneProxy`)
4. Implement `Render()` method to execute draw calls
5. Instantiate in `FGame::Initialize()` or `FScene::Initialize()`

### Adding New RHI Features
1. Add method to `FRHI` and `FRHICommandList` interfaces in `RHI/RHI.h`
2. Implement in `FDX12RHI` and `FDX12CommandList` in `RHI_DX12/DX12RHI.cpp`
3. Update renderer layer to use new feature

## Common Patterns

### Error Handling
```cpp
inline void ThrowIfFailed(HRESULT hr)
{
    if (FAILED(hr))
    {
        throw std::runtime_error("DirectX 12 operation failed");
    }
}
```

### Logging
```cpp
FLog::Log(ELogLevel::Info, "Initializing game...");
FLog::Log(ELogLevel::Error, "Failed to initialize RHI");
```

### Proxy Pattern
```cpp
// Game object creates scene proxy for render thread
class FCubePrimitive : public FPrimitive 
{
public:
    FPrimitiveSceneProxy* CreateSceneProxy(FRHI* RHI) override;
};
```

## Testing and Validation

### Build and Run
```bash
mkdir build
cd build
cmake ..
cmake --build . --config Release
./Source/Runtime/Release/UE5MinimalRenderer.exe
```

### Visual Validation
- Verify 3D scene renders correctly
- Check camera controls respond properly
- Confirm statistics overlay displays

## Project-Specific Notes

### DirectX 12
- Left-handed coordinate system
- Matrices: row-major in CPU, column-major in HLSL (transpose when uploading)
- Constant buffers: 256-byte aligned

### Camera System
- Default: (0, 0, -5) looking at origin
- FOV: 45°, Near: 0.1, Far: 100
- UE5-style controls: LMB (move/yaw), RMB (free-look), MMB (pan), Wheel (zoom)

## Summary

When making changes:
- Follow UE5 naming conventions (F prefix, PascalCase)
- Respect architecture layer boundaries
- Test build and visual output

**Remember**: This is an educational project. Code clarity and architectural demonstration are more important than performance optimization.
