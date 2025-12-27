# Camera Controls Documentation

This document describes the UE5-style camera controls implemented in the UE5MinimalRenderer.

## Overview

The camera system supports interactive navigation using mouse and keyboard inputs, closely mimicking Unreal Engine 5's perspective viewport controls.

## Controls

### Mouse Right Button (RMB) - Primary Look Control
**Action**: Press and hold RMB, then drag the mouse

**Effect**:
- **Horizontal drag (X-axis)**: Rotates camera left/right (yaw rotation)
- **Vertical drag (Y-axis)**: Rotates camera up/down (pitch rotation)

This is the main "free look" mode - rotate the camera to look around without moving. Matches UE5 perspective viewport.

### Mouse Left Button (LMB) - Alternative Look Control
**Action**: Press and hold LMB, then drag the mouse

**Effect**:
- Same as RMB - free look rotation

### Mouse Middle Button (MMB) or LMB + RMB - Pan
**Action**: Press and hold MMB (or both LMB and RMB together), then drag the mouse

**Effect**:
- **Horizontal drag (X-axis)**: Pans camera left/right
- **Vertical drag (Y-axis)**: Pans camera up/down

The camera position moves without rotating. Perfect for adjusting framing.

### Mouse Wheel - Zoom
**Action**: Scroll the mouse wheel up or down

**Effect**:
- **Scroll up**: Zooms in (moves camera forward)
- **Scroll down**: Zooms out (moves camera backward)

### Keyboard WASD - Movement (UE5 Fly Mode)
**Action**: Press W, A, S, D keys

**Effect**:
- **W**: Move forward
- **S**: Move backward
- **A**: Strafe left
- **D**: Strafe right

Works best when combined with RMB for "fly mode" navigation (like UE5).

### Keyboard Q/E - Vertical Movement
**Action**: Press Q or E keys

**Effect**:
- **E**: Move up
- **Q**: Move down

## Sensitivity Settings

Current sensitivity values (in `Main.cpp`, `CameraSettings` namespace):
```cpp
namespace CameraSettings {
    constexpr float MovementSpeed = 0.005f;    // Mouse drag movement
    constexpr float RotationSpeed = 0.003f;    // Camera rotation
    constexpr float PanSpeed = 0.008f;         // Pan movement
    constexpr float ZoomSpeed = 0.3f;          // Zoom/scroll speed
    constexpr float KeyboardMoveSpeed = 3.0f;  // WASD movement (units/sec)
}
```

## Usage Example

1. **Launch the application**: Scene will be visible with camera at initial position
2. **Hold RMB and drag**: Look around the scene freely
3. **Hold RMB + press WASD**: Fly through the scene (UE5 fly mode)
4. **Hold MMB and drag**: Pan to reposition the view
5. **Scroll wheel**: Quick zoom in/out

## Comparison with UE5

| Control | UE5 Perspective | This Renderer |
|---------|-----------------|---------------|
| RMB drag | Free look | Free look ✓ |
| LMB drag | (Context dependent) | Free look |
| MMB drag | Pan | Pan ✓ |
| LMB+RMB drag | Pan | Pan ✓ |
| Scroll | Dolly/Zoom | Dolly/Zoom ✓ |
| RMB+WASD | Fly mode | WASD always works ✓ |
| Q/E | Up/Down | Up/Down ✓ |
