# Build and Test Instructions

This project is Windows-only and requires DirectX 12. The implementation includes:

## New Features Added

### 1. Text Rendering API
- Added `DrawText` method to `FRHICommandList` interface
- Implemented using DirectWrite and Direct2D on DirectX 12
- Supports configurable:
  - Text content (std::string)
  - Position (FVector2D)
  - Font size (float)
  - Color (FColor with RGBA)

### 2. Statistics Tracking
- Created `FRenderStats` class to track:
  - Frame count (total frames rendered)
  - FPS (frames per second, updated every 0.5s)
  - Frame time (milliseconds per frame)
  - Triangle count (triangles rendered per frame)

### 3. Statistics Display
- Statistics are rendered as an overlay in yellow text
- Display includes:
  - Frame: [frame number]
  - FPS: [frames per second]
  - Frame Time: [ms]
  - Triangles: [triangle count]

## Building on Windows

```batch
# Run the build script
build.bat

# Or manually:
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

## Running

```batch
cd build\Source\Runtime\Release
UE5MinimalRenderer.exe
```

## Expected Result

You should see:
1. The colored triangle rendering as before
2. A yellow statistics overlay in the top-left corner showing:
   - Current frame number
   - FPS (updated every 0.5 seconds)
   - Frame time in milliseconds
   - Number of triangles (should be 1 for the single triangle)

## Implementation Details

### Text Rendering Architecture
- Uses D3D11on12 to bridge DirectX 12 and Direct2D/DirectWrite
- Wraps D3D12 render targets for D2D rendering
- Text is rendered after 3D scene, before present
- Each text draw call:
  1. Acquires wrapped D3D11 resource
  2. Sets D2D render target
  3. Creates text format and brush
  4. Draws text
  5. Releases wrapped resource

### Statistics Tracking
- `BeginFrame()`: Starts timing and resets triangle count
- `EndFrame()`: Calculates frame time and updates FPS
- `AddTriangles()`: Accumulates triangles from scene proxies
- FPS calculation: Averaged over 0.5-second intervals

### Integration Points
- `Renderer.cpp`: Calls stats methods and renders overlay
- `FSceneProxy`: Added `GetTriangleCount()` virtual method
- `FTriangleMeshProxy`: Returns vertex count / 3
