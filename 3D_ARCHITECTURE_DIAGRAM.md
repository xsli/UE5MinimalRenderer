# 3D Rendering Architecture Diagram

## Coordinate Space Transformation Pipeline

```
┌─────────────────────────────────────────────────────────────────────┐
│                        CUBE GEOMETRY DEFINITION                      │
│                                                                       │
│  24 Vertices (4 per face) + 36 Indices (6 faces × 2 triangles)      │
│                                                                       │
│  Front: Red      Back: Green     Top: Blue                           │
│  Bottom: Yellow  Right: Magenta  Left: Cyan                          │
└─────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────┐
│                        OBJECT/LOCAL SPACE                            │
│                                                                       │
│  Vertices defined relative to object center (origin)                 │
│  Cube corners: ±0.5 in X, Y, Z                                      │
└─────────────────────────────────────────────────────────────────────┘
                                    │
                         [ MODEL MATRIX (M) ]
                         - Rotation: Spin cube
                         - Translation: Position
                         - Scaling: Size
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────┐
│                           WORLD SPACE                                │
│                                                                       │
│  All objects positioned in the shared world                          │
│  Cube rotates at 0.5 radians/second                                 │
└─────────────────────────────────────────────────────────────────────┘
                                    │
                          [ VIEW MATRIX (V) ]
                          - Camera position: (0, 0, -5)
                          - Look at: (0, 0, 0)
                          - Up vector: (0, 1, 0)
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────┐
│                        CAMERA/VIEW SPACE                             │
│                                                                       │
│  Vertices relative to camera                                         │
│  Camera at origin, looking down +Z axis                             │
└─────────────────────────────────────────────────────────────────────┘
                                    │
                       [ PROJECTION MATRIX (P) ]
                       - FOV: 45° (π/4 radians)
                       - Aspect: 16:9
                       - Near: 0.1, Far: 100.0
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────┐
│                         CLIP SPACE (Homogeneous)                     │
│                                                                       │
│  4D coordinates with perspective divide pending                      │
│  W component carries depth for perspective                          │
└─────────────────────────────────────────────────────────────────────┘
                                    │
                      [ GPU: Perspective Divide ]
                      - Divide X, Y, Z by W
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────┐
│                  NORMALIZED DEVICE COORDINATES (NDC)                 │
│                                                                       │
│  X, Y, Z in range [-1, 1] (DirectX: Z in [0, 1])                   │
└─────────────────────────────────────────────────────────────────────┘
                                    │
                        [ Viewport Transform ]
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────┐
│                          SCREEN SPACE                                │
│                                                                       │
│  Final pixel coordinates for rasterization                          │
│  Depth tested against depth buffer                                  │
└─────────────────────────────────────────────────────────────────────┘
```

## Rendering Pipeline Flow

```
┌──────────────┐
│  Game Tick   │  Updates rotation angle
└──────┬───────┘
       │
       ▼
┌──────────────────────────────────────────────────────────────┐
│              RENDERER: RenderFrame()                         │
│                                                              │
│  1. BeginFrame()                                            │
│  2. Clear Render Target (0.2, 0.3, 0.4)                    │
│  3. Clear Depth Stencil (1.0)                              │
│                                                              │
│  4. RenderScene() ──────────────────────────────┐           │
│     │                                            │           │
│     └─► For each FCubeMeshProxy:                │           │
│         - Calculate MVP = M × V × P             │           │
│         - Transpose MVP (HLSL column-major)     │           │
│         - Update Constant Buffer                │           │
│         - SetPipelineState()                    │           │
│         - SetConstantBuffer(MVP, slot 0)        │           │
│         - SetVertexBuffer(24 vertices)          │           │
│         - SetIndexBuffer(36 indices)            │           │
│         - DrawIndexedPrimitive()  ──────────────┼───┐       │
│                                                  │   │       │
│  5. FlushCommandsFor2D()                        │   │       │
│     - Submit 3D commands to GPU                 │   │       │
│     - Wait for completion                       │   │       │
│                                                  │   │       │
│  6. RenderStats()                               │   │       │
│     - Draw text overlay (D2D/DWrite)            │   │       │
│                                                  │   │       │
│  7. EndFrame()                                  │   │       │
│  8. Present()                                   │   │       │
└──────────────────────────────────────────────────┼───┼───────┘
                                                   │   │
                                                   ▼   ▼
                                           ┌──────────────────┐
                                           │   GPU PIPELINE   │
                                           ├──────────────────┤
                                           │ Vertex Shader:   │
                                           │   MVP × Vertex   │
                                           ├──────────────────┤
                                           │ Rasterization    │
                                           ├──────────────────┤
                                           │ Depth Test       │
                                           ├──────────────────┤
                                           │ Pixel Shader:    │
                                           │   Interpolate    │
                                           │   Color          │
                                           ├──────────────────┤
                                           │ Depth Write      │
                                           ├──────────────────┤
                                           │ Color Write      │
                                           └──────────────────┘
                                                   │
                                                   ▼
                                           ┌──────────────────┐
                                           │  Final Image:    │
                                           │  Rotating Cube   │
                                           │  + Stats Overlay │
                                           └──────────────────┘
```

## Data Structures

```
┌────────────────────────────────────────────────────────────────┐
│                        FVertex                                  │
├────────────────────────────────────────────────────────────────┤
│  FVector Position;  // X, Y, Z (12 bytes)                      │
│  FColor Color;      // R, G, B, A (16 bytes)                   │
│  Total: 28 bytes                                               │
└────────────────────────────────────────────────────────────────┘

┌────────────────────────────────────────────────────────────────┐
│                      FMatrix4x4                                 │
├────────────────────────────────────────────────────────────────┤
│  DirectX::XMMATRIX Matrix;  // 4×4 floats (64 bytes)          │
│                                                                 │
│  Methods:                                                       │
│  - static Identity()                                            │
│  - static Translation(X, Y, Z)                                  │
│  - static RotationX/Y/Z(Angle)                                  │
│  - static PerspectiveFovLH(...)                                │
│  - static LookAtLH(eye, target, up)                            │
│  - operator*(other)     // Matrix multiplication               │
│  - Transpose()                                                  │
└────────────────────────────────────────────────────────────────┘

┌────────────────────────────────────────────────────────────────┐
│                        FCamera                                  │
├────────────────────────────────────────────────────────────────┤
│  FVector Position;              // Camera location              │
│  FVector LookAtTarget;          // What camera looks at         │
│  FVector UpVector;              // Up direction                 │
│  float FovY;                    // Field of view                │
│  float AspectRatio;             // Width / Height               │
│  float NearPlane, FarPlane;     // Clipping planes             │
│  FMatrix4x4 ViewMatrix;         // Cached view matrix          │
│  FMatrix4x4 ProjectionMatrix;   // Cached projection           │
│  bool bViewMatrixDirty;         // Update flag                 │
│  bool bProjectionMatrixDirty;   // Update flag                 │
└────────────────────────────────────────────────────────────────┘

┌────────────────────────────────────────────────────────────────┐
│                     FCubeMeshProxy                              │
├────────────────────────────────────────────────────────────────┤
│  FRHIBuffer* VertexBuffer;      // 24 vertices × 28 bytes      │
│  FRHIBuffer* IndexBuffer;       // 36 indices × 4 bytes        │
│  FRHIBuffer* ConstantBuffer;    // MVP matrix (256 bytes)      │
│  FRHIPipelineState* PipelineState;  // Shaders + state         │
│  uint32 IndexCount;             // 36                          │
│  FMatrix4x4 ModelMatrix;        // Object transform            │
└────────────────────────────────────────────────────────────────┘
```

## Memory Layout

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
│ (2 tri) │ (2 tri) │ (2 tri) │     │ (2 tri) │
└─────────┴─────────┴─────────┴─────┴─────────┘

Constant Buffer (256 bytes, aligned):
┌──────────────────────────┬────────────────┐
│ MVP Matrix (64 bytes)     │  Padding       │
└──────────────────────────┴────────────────┘
```

## DirectX 12 Resource Binding

```
Root Signature Layout (3D Mode):
┌───────────────────────────────────────┐
│  Root Parameter 0: CBV                │
│  - Register: b0                       │
│  - Shader Visibility: Vertex          │
│  - Contains: MVP Matrix               │
└───────────────────────────────────────┘

Vertex Shader Input:
┌───────────────────────────────────────┐
│  Input Layout:                        │
│  - POSITION: R32G32B32_FLOAT @ 0      │
│  - COLOR: R32G32B32A32_FLOAT @ 12     │
└───────────────────────────────────────┘

Pipeline State:
┌───────────────────────────────────────┐
│  Rasterizer: Cull None                │
│  Depth: Enabled, Less, Write All      │
│  Blend: Disabled                      │
│  Topology: Triangle List              │
└───────────────────────────────────────┘
```
