# UE5MinimalRenderer
Minimal renderer mimicking UE5 parallel rendering architecture

## Overview
This project demonstrates a simplified version of Unreal Engine 5's parallel rendering architecture with the following layers:
- **Game Layer**: Game logic and tick loop
- **Renderer Layer**: Scene management and render command processing
- **RHI (Render Hardware Interface)**: Platform-agnostic rendering interface
- **DX12 Backend**: DirectX 12 implementation

## Demo
The project includes a simple demo that renders a colored triangle:
- Red vertex at the top
- Green vertex at the bottom right
- Blue vertex at the bottom left

## Architecture
```
Game Tick → Renderer → RHI Interface → DX12 Implementation → GPU
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
├── Core/           # Core types and utilities
├── RHI/            # Render Hardware Interface (platform-agnostic)
├── RHI_DX12/       # DirectX 12 implementation
│   ├── d3dx12*.h   # Microsoft DirectX 12 helper headers (included)
│   └── DX12RHI.*   # DX12 RHI implementation
├── Renderer/       # Rendering layer
├── Game/           # Game logic layer
└── Runtime/        # Application entry point
```

## Dependencies
All required dependencies are included in the repository:
- **d3dx12.h and related headers**: Microsoft's DirectX 12 helper headers from [DirectX-Headers](https://github.com/microsoft/DirectX-Headers) (MIT License)
- **Windows SDK**: Provides d3d12.lib, dxgi.lib, d3dcompiler.lib (installed with Visual Studio)

## Flow
1. **Game Thread**: Creates game objects and ticks the world
2. **Renderer**: Processes scene proxies and generates render commands
3. **RHI**: Provides platform-agnostic rendering interface
4. **DX12**: Executes commands on GPU using DirectX 12
5. **Output**: Rendered frame displayed on screen
