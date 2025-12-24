# UE5MinimalRenderer Testing Guide

## System Requirements

### Operating System
- Windows 10 version 1903 (May 2019 Update) or later
- Windows 11 (recommended)

### Software Requirements
- **Visual Studio 2019 or 2022** with:
  - Desktop development with C++
  - Windows 10 SDK (version 10.0.19041.0 or later)
  - C++ CMake tools for Windows
- **CMake 3.20 or later**
- **DirectX 12 capable GPU** (Most modern GPUs from 2015 onwards)

### Hardware Requirements
- DirectX 12 capable graphics card
  - NVIDIA: GeForce GTX 900 series or newer
  - AMD: Radeon R9 200 series or newer
  - Intel: 6th generation Core processors (Skylake) or newer with integrated graphics

## Build Instructions

### Option 1: Using the Build Script
```batch
build.bat
```

This will:
1. Create a `build` directory
2. Generate Visual Studio solution
3. Build the Release configuration
4. Output the executable to `build/Source/Runtime/Release/UE5MinimalRenderer.exe`

### Option 2: Manual CMake Build
```batch
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

### Option 3: Using Visual Studio IDE
```batch
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64
start UE5MinimalRenderer.sln
```

Then build in Visual Studio (Ctrl+Shift+B).

## Running the Demo

From the build directory:
```batch
cd Source\Runtime\Release
UE5MinimalRenderer.exe
```

Or simply double-click the executable in Windows Explorer.

## Expected Output

When running successfully, you should see:

1. **Console Output:**
   ```
   [INFO] Initializing game...
   [INFO] DX12 RHI initialized successfully
   [INFO] Renderer initialized
   [INFO] Game world initialized
   [INFO] Game initialized successfully
   [INFO] Starting main loop...
   ```

2. **Window Display:**
   - Window title: "UE5 Minimal Renderer - Colored Triangle Demo"
   - Window size: 1280x720
   - Background color: Dark blue (RGB: 0.2, 0.3, 0.4)

3. **Rendered Triangle:**
   - **Top vertex**: Red color
   - **Bottom-right vertex**: Green color
   - **Bottom-left vertex**: Blue color
   - The triangle should be centered in the window

## Verification Checklist

- [ ] Build completes without errors
- [ ] Application window opens
- [ ] Dark blue background is visible
- [ ] Colored triangle is rendered correctly
- [ ] Triangle has smooth color gradients between vertices
- [ ] No console errors during runtime
- [ ] Application exits cleanly when window is closed

## Troubleshooting

### Build Errors

**Error: "Cannot find d3d12.lib"**
- Solution: Install Windows 10 SDK through Visual Studio Installer

**Error: "CMake not found"**
- Solution: Install CMake from https://cmake.org/download/ or through Visual Studio

**Error: "MSBuild not found"**
- Solution: Run from Visual Studio Developer Command Prompt

### Runtime Errors

**Error: "Failed to create D3D12 device"**
- Your GPU may not support DirectX 12
- Update your graphics drivers
- Check if your GPU is in the compatible hardware list above

**Error: "DirectX 12 operation failed"**
- Update graphics drivers to the latest version
- Enable Developer Mode in Windows Settings
- Check Windows Update for graphics driver updates

**Black Screen / No Triangle**
- This may indicate shader compilation issues
- Check console for error messages
- Verify DirectX 12 runtime is installed

## Debug Build

For more detailed error messages:
```batch
cmake --build . --config Debug
cd Source\Runtime\Debug
UE5MinimalRenderer.exe
```

Debug builds enable D3D12 debug layer which provides detailed validation messages.

## Performance Notes

- Expected frame rate: 60 FPS (vsync enabled)
- GPU usage should be minimal for this simple demo
- CPU usage should be low

## Architecture Flow Validation

The rendering pipeline follows this flow:

1. **Game Layer**: 
   - Creates FTriangleObject
   - Tick() called each frame
   
2. **Renderer Layer**:
   - Processes scene proxies
   - Issues render commands
   
3. **RHI Layer**:
   - Platform-agnostic interface
   - Translates to DX12 calls
   
4. **DX12 Layer**:
   - Command list recording
   - GPU execution
   - Present to screen

Each layer should log its initialization in the console output.
