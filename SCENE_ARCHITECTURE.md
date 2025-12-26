# Scene Management and Primitive System

## Overview
This implementation follows Unreal Engine 5's scene management architecture, separating game thread logic from render thread representation.

## Architecture

### Game Thread (FScene + FPrimitive)
```
FScene (Game Thread)
  └── FPrimitive[] (Game objects)
       ├── FCubePrimitive
       ├── FSpherePrimitive
       ├── FCylinderPrimitive
       └── FPlanePrimitive
```

**FScene**
- Owns game thread primitives
- Updates primitives each frame
- Synchronizes with render scene

**FPrimitive**
- Base class for renderable objects
- Contains transform (position, rotation, scale)
- Has Tick() for game logic
- Creates FPrimitiveSceneProxy for rendering

### Render Thread (FRenderScene + FPrimitiveSceneProxy)
```
FRenderScene (Render Thread)
  └── FPrimitiveSceneProxy[] (Render representations)
```

**FRenderScene**
- Owns render thread proxies
- Renders all proxies each frame
- Independent from game objects

**FPrimitiveSceneProxy**
- Unified proxy for all primitive types
- Contains GPU resources (buffers, PSO)
- Renders geometry with MVP transformation

## Synchronization Flow

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

## Primitive Types

### FCubePrimitive
- 24 vertices (4 per face for proper coloring)
- 36 indices (12 triangles)
- Auto-rotation support

### FSpherePrimitive
- UV sphere algorithm
- Configurable segments (longitude) and rings (latitude)
- Default: 24 segments × 16 rings
- Auto-rotation support

### FCylinderPrimitive
- Circular top and bottom caps
- Configurable segment count
- Default: 16 segments
- Auto-rotation support

### FPlanePrimitive
- Grid-based subdivision
- Configurable subdivision count
- Ideal for ground planes

## Transform System

Each primitive has an FTransform:
```cpp
struct FTransform {
    FVector Position;   // World position
    FVector Rotation;   // Euler angles (radians)
    FVector Scale;      // Scale factors
};
```

Transformation order: **Scale → Rotate → Translate**

## Usage Example

```cpp
// Create scene
FScene* scene = new FScene(RHI);

// Add a cube
FCubePrimitive* cube = new FCubePrimitive();
cube->SetColor(FColor(1.0f, 0.3f, 0.3f, 1.0f));
cube->GetTransform().Position = FVector(-2.0f, 0.5f, 0.0f);
cube->GetTransform().Scale = FVector(0.8f, 0.8f, 0.8f);
cube->SetAutoRotate(true);
scene->AddPrimitive(cube);

// Add a sphere
FSpherePrimitive* sphere = new FSpherePrimitive(24, 16);
sphere->SetColor(FColor(0.3f, 0.7f, 1.0f, 1.0f));
sphere->GetTransform().Position = FVector(2.0f, 0.5f, 0.0f);
scene->AddPrimitive(sphere);

// Update render scene
renderer->UpdateFromScene(scene);

// Game loop
while (running) {
    scene->Tick(deltaTime);
    renderer->UpdateFromScene(scene);  // Sync point
    renderer->RenderFrame();
}
```

## Key Benefits

1. **Thread Safety**: Clear separation between game and render data
2. **Flexibility**: Easy to add new primitive types
3. **UE5 Pattern**: Follows industry-standard architecture
4. **Transform System**: Unified transform handling
5. **Resource Management**: Proxies own GPU resources
6. **Scalability**: Can extend to complex scene graphs

## Comparison with UE5

| Feature | UE5 | This Implementation |
|---------|-----|---------------------|
| FScene | ✓ | ✓ |
| FRenderScene | ✓ | ✓ |
| Primitive Components | ✓ | ✓ (Simplified) |
| Scene Proxies | ✓ | ✓ |
| Multi-threading | ✓ | ✗ (Simulated) |
| Dirty Tracking | ✓ | ✗ (Full rebuild) |
| Octree/BVH | ✓ | ✗ |
| Material System | ✓ | ✗ |

## Future Enhancements

- [ ] Dirty tracking for efficient updates
- [ ] Per-object materials
- [ ] Lighting support
- [ ] Shadow mapping
- [ ] Culling system
- [ ] Scene graph hierarchy
- [ ] Actual multi-threading
- [ ] Instanced rendering
- [ ] LOD system
