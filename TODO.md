# TODO - Future Features

This document lists planned features and improvements for the UE5MinimalRenderer project.

## High Priority

### Rendering Features
- [x] **Lighting System**
  - [x] Directional light support
  - [x] Point light support (up to 4)
  - [x] Ambient lighting
  - [x] Phong/Blinn-Phong shading model
  - [x] Light visualization (wireframe debug rendering)
  - [x] Per-object material properties (diffuse, specular, ambient, shininess)

- [ ] **Shadow Mapping**
  - Shadow map generation pass
  - Percentage closer filtering (PCF)
  - Cascaded shadow maps for large scenes

- [ ] **Texture Support**
  - Texture loading (PNG, JPEG, DDS)
  - Texture sampling in shaders
  - Mipmapping
  - Texture filtering (bilinear, trilinear, anisotropic)

- [ ] **Material System Enhancements**
  - Roughness and metallic parameters (PBR preparation)
  - Material instances
  - Normal mapping support

### Architecture Improvements
- [x] **Multi-threading**
  - [x] Separate render thread (true UE5 parallel rendering)
  - [x] Separate RHI thread for GPU command submission
  - [x] Thread-safe resource management
  - [x] Job system for parallel work (FTaskGraph)
  - [x] Lock-free command queues (FRenderCommandQueue)
  - [x] Game/Render frame synchronization with 1-frame lead

- [ ] **Resource Management**
  - Resource pooling for buffers and textures
  - Async resource loading/streaming
  - Resource reference counting
  - Automatic garbage collection

## Medium Priority

### Camera & Controls
- [ ] **Keyboard Movement (WASD)**
  - Forward/backward movement
  - Strafe left/right
  - Up/down movement (Q/E)
  - Sprint modifier (Shift)

- [ ] **Orbit Camera Mode**
  - Rotate around a selected point
  - Focus on object functionality
  - Smooth camera interpolation

- [ ] **Camera Bookmarks**
  - Save/load camera positions
  - Quick switch between views
  - Named camera presets

### Geometry & Primitives
- [ ] **Additional Primitives**
  - Cone primitive
  - Torus primitive
  - Capsule primitive
  - Custom mesh loading (OBJ, FBX)

- [ ] **Instanced Rendering**
  - GPU instancing for repeated objects
  - Instance data buffers
  - Reduced draw calls

- [ ] **LOD System**
  - Level of detail meshes
  - Distance-based LOD switching
  - Automatic LOD generation

### Scene Management
- [ ] **Dirty Tracking**
  - Track which objects need updates
  - Incremental scene updates
  - Avoid full scene rebuilds

- [ ] **Scene Graph Hierarchy**
  - Parent-child relationships
  - Transform inheritance
  - Object grouping

- [ ] **Culling System**
  - Frustum culling
  - Occlusion culling
  - Spatial partitioning (Octree/BVH)

## Low Priority

### Visual Effects
- [ ] **Post-Processing**
  - Bloom effect
  - Tone mapping
  - Color grading
  - Anti-aliasing (FXAA, TAA)

- [ ] **Normal Mapping**
  - Tangent space calculation
  - Normal map sampling
  - Enhanced surface detail

- [ ] **PBR Materials**
  - Physically based rendering
  - IBL (Image-based lighting)
  - Environment maps

### Debug & Tools
- [ ] **Debug Visualization**
  - Wireframe rendering mode
  - Bounding box display
  - Normal visualization
  - Depth buffer visualization

- [ ] **Performance Profiling**
  - GPU timing queries
  - Memory usage tracking
  - Draw call analysis
  - Bottleneck identification

- [ ] **Statistics Enhancements**
  - Performance graphs
  - Toggle statistics display (F1 key)
  - Configurable stats position/color
  - Additional metrics (GPU time, memory)

### Platform & Backend
- [ ] **Vulkan Backend**
  - Alternative to DirectX 12
  - Cross-platform support
  - VkRHI implementation

- [ ] **Configuration System**
  - Graphics quality settings
  - Resolution settings
  - VSync toggle
  - Config file loading

- [ ] **Window Management**
  - Fullscreen toggle
  - Resolution switching
  - Multi-monitor support

## Documentation

- [ ] **Tutorial Series**
  - Getting started guide
  - Adding new primitives tutorial
  - Extending the RHI tutorial
  - Shader programming guide

- [ ] **API Documentation**
  - Doxygen comments
  - Class reference
  - Code examples

- [ ] **Video Walkthrough**
  - Architecture overview video
  - Build and run demonstration
  - Feature showcase

## Code Quality

- [ ] **Unit Tests**
  - Math library tests
  - Transform tests
  - RHI abstraction tests

- [ ] **Continuous Integration**
  - GitHub Actions workflow
  - Automated builds
  - Code quality checks

- [ ] **Code Cleanup**
  - Consistent coding style
  - Remove dead code
  - Optimize includes

---

## Contribution Guidelines

If you'd like to contribute:

1. Pick an item from this list
2. Create a feature branch
3. Implement the feature
4. Update documentation
5. Submit a pull request

## Priority Legend

| Priority | Meaning |
|----------|---------|
| High | Core functionality, significant impact |
| Medium | Nice to have, improves usability |
| Low | Future enhancements, polish items |

## Notes

- This list is not exhaustive and may be updated
- Features may be reprioritized based on feedback
- Some features may require prerequisite work
- Contributions welcome!
