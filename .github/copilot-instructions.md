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
- **Opening brace on same line** for functions and control structures
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

## Documentation Requirements

**Critical**: Every code change MUST update relevant documentation.

### When to Update Documentation

#### Adding New Features
- Update **README.md**: Add to features list, update build instructions if needed
- Create/update **IMPLEMENTATION.md**: Describe implementation details
- Update **ARCHITECTURE.md**: If adding new modules or changing architecture
- Add code comments for non-trivial implementations

#### Modifying Existing Features
- Update all affected documentation files
- Keep documentation in sync with code changes
- Update code comments if behavior changes

#### Bug Fixes
- Document the fix in commit messages
- Update documentation if the bug revealed incorrect documentation
- Add comments explaining workarounds or non-obvious fixes

### Documentation Files Structure

#### Primary Documentation
- **README.md**: Project overview, build instructions, features, controls
- **ARCHITECTURE.md**: System architecture, layer descriptions, design patterns
- **IMPLEMENTATION.md**: Implementation details, technical decisions

#### Feature-Specific Documentation
- **3D_IMPLEMENTATION.md**: 3D rendering system details
- **CAMERA_CONTROLS.md**: Interactive camera controls
- **BUILD_TEST.md**: Build and testing procedures
- **TESTING.md**: Testing strategies and results

#### Architecture Diagrams
- **ARCHITECTURE_DIAGRAM.md**: Text-based architecture diagrams
- **3D_ARCHITECTURE_DIAGRAM.md**: 3D rendering pipeline visualization

### Documentation Style

#### Markdown Formatting
- Use clear headers (# ## ###)
- Code blocks with language hints: ```cpp, ```hlsl, ```bash
- Lists for step-by-step instructions
- Tables for comparisons or feature matrices

#### Technical Writing
- Be precise and concise
- Use active voice
- Include code examples for APIs
- Document parameters and return values
- Explain "why" not just "what"

#### Keep Updated
```
Before Commit Checklist:
[ ] Code changes implemented
[ ] Relevant documentation updated
[ ] Code comments added/updated
[ ] Architecture diagrams reflect changes (if applicable)
[ ] README reflects new features or changes
```

## Development Workflow

### Making Changes

1. **Understand the architecture** - Review ARCHITECTURE.md and related docs
2. **Identify affected layers** - Changes should respect layer boundaries
3. **Update RHI if needed** - Platform-agnostic interface changes first
4. **Implement in DX12 backend** - Platform-specific implementation
5. **Update game/renderer layers** - Use new RHI features
6. **Test thoroughly** - Build and run, verify visual output
7. **Update documentation** - All relevant docs must be updated
8. **Commit with descriptive messages** - Explain what and why

### Adding New Primitives

1. Create primitive class in `Game/Primitive.h` (inherit from `FPrimitive`)
2. Implement `CreateSceneProxy()` to create render representation
3. Create proxy class in `Renderer/Renderer.h` (inherit from `FPrimitiveSceneProxy`)
4. Implement `Render()` method to execute draw calls
5. Instantiate in `FGame::Initialize()` or `FScene::Initialize()`
6. Update documentation with new primitive type

### Adding New RHI Features

1. Add method to `FRHI` and `FRHICommandList` interfaces in `RHI/RHI.h`
2. Implement in `FDX12RHI` and `FDX12CommandList` in `RHI_DX12/DX12RHI.cpp`
3. Update renderer layer to use new feature
4. Test with simple use case
5. Document in ARCHITECTURE.md and IMPLEMENTATION.md

### Shader Changes

1. Modify shader code in `DX12RHI.cpp` (inline HLSL strings)
2. Update pipeline state creation if input/output changed
3. Update vertex structures if shader inputs changed
4. Rebuild and test visually
5. Document shader behavior in relevant docs

## Common Patterns and Idioms

### Error Handling
```cpp
// DirectX HRESULT checking
inline void ThrowIfFailed(HRESULT hr)
{
    if (FAILED(hr))
    {
        throw std::runtime_error("DirectX 12 operation failed");
    }
}

// Usage
ThrowIfFailed(Device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&CommandQueue)));
```

### Logging
```cpp
FLog::Log(ELogLevel::Info, "Initializing game...");
FLog::Log(ELogLevel::Warning, "Depth buffer not enabled");
FLog::Log(ELogLevel::Error, "Failed to initialize RHI");
```

### Resource Creation Pattern
```cpp
// RHI interface (platform-agnostic)
virtual FRHIBuffer* CreateVertexBuffer(const void* Data, uint32 Size) = 0;

// DX12 implementation (platform-specific)
FRHIBuffer* FDX12RHI::CreateVertexBuffer(const void* Data, uint32 Size)
{
    // Create D3D12 resource
    // Upload data
    // Return wrapped buffer
    return new FDX12Buffer(resource, EBufferType::Vertex);
}
```

### Proxy Pattern
```cpp
// Game object (game thread)
class FCubePrimitive : public FPrimitive 
{
public:
    FPrimitiveSceneProxy* CreateSceneProxy(FRHI* RHI) override;
};

// Scene proxy (render thread representation)
class FCubeMeshProxy : public FPrimitiveSceneProxy 
{
public:
    void Render(FRHICommandList* RHICmdList) override;
private:
    FRHIBuffer* VertexBuffer;
    FRHIPipelineState* PipelineState;
};
```

## Testing and Validation

### Build and Run
```bash
# From repository root
mkdir build
cd build
cmake ..
cmake --build . --config Release
./Source/Runtime/Release/UE5MinimalRenderer.exe
```

### Visual Validation
- Verify 3D scene renders correctly
- Check camera controls respond properly
- Confirm statistics overlay displays accurate information
- Test all mouse controls (LMB, RMB, MMB, wheel)
- Verify keyboard controls (WASD, QE)

### Code Quality
- No compiler warnings (treat warnings as errors)
- Proper memory management (no leaks)
- Follow UE coding conventions consistently
- All public APIs documented
- Architecture layers respected (no cross-layer violations)

## Project-Specific Notes

### DirectX 12 Considerations
- Use left-handed coordinate system
- Matrices are row-major in CPU, column-major in HLSL (transpose when uploading)
- Constant buffers must be 256-byte aligned
- Resources must be in correct state before use (barrier transitions)
- Synchronization via fences required for CPU/GPU coordination

### Camera System
- Default position: (0, 0, -5) looking at origin
- FOV: 45 degrees, Near: 0.1, Far: 100
- UE5-style controls: LMB (move/yaw), RMB (free-look), MMB (pan), Wheel (zoom)
- Lazy matrix updates (only when dirty)

### Performance Considerations
- Single-threaded (render thread simulated)
- Synchronous GPU waits (could optimize with triple buffering)
- Text rendering uses D3D11on12 interop (minor overhead)
- No resource pooling or batching (simple implementation)

## Summary Checklist

When making any changes to this codebase:

- [ ] Follow UE5 naming conventions (F prefix, PascalCase, etc.)
- [ ] Respect architecture layer boundaries
- [ ] Follow main loop and tick-based design patterns
- [ ] Update all relevant documentation files
- [ ] Add/update code comments for complex logic
- [ ] Test build and runtime behavior
- [ ] Verify visual output (if rendering changes)
- [ ] Write clear commit messages
- [ ] Keep documentation synchronized with code

**Remember**: This is an educational project. Code clarity and architectural demonstration are more important than performance optimization.
