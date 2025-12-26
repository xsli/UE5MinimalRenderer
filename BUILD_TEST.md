# Build and Test Instructions

This document provides comprehensive build, run, and testing instructions for UE5MinimalRenderer.

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
- **DirectX 12 capable GPU**

### Hardware Requirements
- DirectX 12 capable graphics card:
  - NVIDIA: GeForce GTX 900 series or newer
  - AMD: Radeon R9 200 series or newer
  - Intel: 6th generation Core (Skylake) or newer with integrated graphics

---

## Building

### Option 1: Using the Build Script (Recommended)
```batch
build.bat
```

This will:
1. Create a `build` directory
2. Generate Visual Studio solution
3. Build the Release configuration
4. Output executable to `build/Source/Runtime/Release/UE5MinimalRenderer.exe`

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

---

## Running

From the build directory:
```batch
cd Source\Runtime\Release
UE5MinimalRenderer.exe
```

Or double-click the executable in Windows Explorer.

---

## Expected Output

### Console Output
```
[INFO] Initializing game...
[INFO] DX12 RHI initialized successfully
[INFO] Renderer initialized
[INFO] Scene initialized with primitives
[INFO] Game initialized successfully
[INFO] Starting main loop...
```

### Window Display
- **Window Title**: "UE5 Minimal Renderer"
- **Window Size**: 1280x720
- **Background Color**: Dark blue (RGB: 0.2, 0.3, 0.4)

### 3D Scene
- Multiple rotating primitives (cube, sphere, cylinder, etc.)
- Proper depth sorting (faces occlude correctly)
- Each primitive with distinct color

### Statistics Overlay (Top-Left)
```
Frame: 12345
FPS: 60.0
Frame Time: 16.67 ms
Triangles: [count]
```

---

## Verification Checklist

- [ ] Build completes without errors
- [ ] Application window opens
- [ ] Dark blue background is visible
- [ ] 3D primitives are rendered correctly
- [ ] Primitives rotate smoothly
- [ ] Depth testing works (no face overlap artifacts)
- [ ] Statistics overlay appears in top-left (yellow text)
- [ ] Camera controls respond to mouse input
- [ ] No console errors during runtime
- [ ] Application exits cleanly when window is closed

---

## Camera Controls Testing

Test each control mode:
1. **LMB + Drag**: Move forward/backward, rotate left/right
2. **RMB + Drag**: Free-look (pitch and yaw)
3. **MMB + Drag** or **LMB+RMB + Drag**: Pan without rotation
4. **Mouse Wheel**: Zoom in/out

See [CAMERA_CONTROLS.md](CAMERA_CONTROLS.md) for detailed information.

---

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
- Check if your GPU is in the compatible hardware list

**Error: "DirectX 12 operation failed"**
- Update graphics drivers to the latest version
- Enable Developer Mode in Windows Settings
- Check Windows Update for graphics driver updates

**Black Screen / No Primitives**
- Check console for error messages
- Verify DirectX 12 runtime is installed
- Try Debug build for more detailed errors

---

## Debug Build

For more detailed error messages:
```batch
cmake --build . --config Debug
cd Source\Runtime\Debug
UE5MinimalRenderer.exe
```

Debug builds enable D3D12 debug layer which provides detailed validation messages.

---

## Performance Notes

- Expected frame rate: 60 FPS (vsync enabled)
- GPU usage should be minimal for this demo
- CPU usage should be low
- Text rendering adds ~0.4ms per frame overhead
