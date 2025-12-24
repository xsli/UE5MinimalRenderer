# UE5MinimalRenderer

A minimal renderer mimicking Unreal Engine 5's parallel rendering architecture.

## Architecture

This project demonstrates the core parallel rendering pattern used in UE5, with separate threads for game logic and rendering:

### Layers

1. **Game Layer** (`Source/Game/`)
   - Main application loop running on the game thread
   - Game logic and simulation
   - Enqueues render commands to the render thread

2. **Renderer Layer** (`Source/Renderer/`)
   - Manages the render thread
   - Processes render commands asynchronously
   - Coordinates frame synchronization between game and render threads

3. **RHI Layer** (`Source/RHI/`)
   - Render Hardware Interface - abstraction layer for graphics APIs
   - Provides platform-agnostic rendering interface

4. **DX12 Layer** (`Source/RHI/DX12/`)
   - DirectX 12 implementation of the RHI
   - Currently a stub implementation for demonstration

## Parallel Rendering

The architecture demonstrates UE5's parallel rendering model:
- Game thread runs game logic and prepares render commands
- Render thread processes rendering commands independently
- Commands are queued from game thread to render thread
- Both threads can run in parallel, improving performance

## Building

### Prerequisites
- CMake 3.15 or higher
- C++17 compatible compiler
- Threading support (pthreads on Linux/Mac, native on Windows)

### Build Steps

```bash
# Create build directory
mkdir Build
cd Build

# Configure with CMake
cmake ..

# Build
cmake --build .

# Run
./bin/UE5MinimalRenderer
```

### Platform-Specific Notes

**Linux/macOS:**
```bash
cmake -DCMAKE_BUILD_TYPE=Release ..
make
```

**Windows (Visual Studio):**
```bash
cmake -G "Visual Studio 16 2019" ..
cmake --build . --config Release
```

## Output

When run, the application will:
1. Initialize the Game, Renderer, and RHI layers
2. Start the render thread
3. Run the game loop for a fixed number of frames (10 by default)
4. Demonstrate parallel execution of game and render threads
5. Clean shutdown of all systems

## Future Extensions

This minimal framework can be extended with:
- Actual DirectX 12 / Vulkan implementation
- Scene graph and entity management
- Material and shader system
- Command buffer recording
- GPU resource management
- Window and input handling
