# Camera Controls Documentation

This document describes the UE5-style camera controls implemented in the UE5MinimalRenderer.

## Overview

The camera system now supports interactive navigation using mouse inputs, mimicking Unreal Engine 5's viewport controls. The implementation allows users to:
- Rotate the camera to view the scene from different angles
- Move the camera through the scene
- Pan the camera horizontally and vertically
- Zoom in and out

## Controls

### Mouse Left Button (LMB)
**Action**: Press and hold LMB, then drag the mouse

**Effect**:
- **Horizontal drag (X-axis)**: Rotates camera left/right (yaw rotation)
- **Vertical drag (Y-axis)**: Moves camera forward/backward

This allows you to navigate through the scene while naturally turning to face the direction of movement.

### Mouse Right Button (RMB)
**Action**: Press and hold RMB, then drag the mouse

**Effect**:
- **Horizontal drag (X-axis)**: Rotates camera left/right (yaw rotation)
- **Vertical drag (Y-axis)**: Rotates camera up/down (pitch rotation)

This is the "free look" mode - rotate the camera to look around without moving.

### Mouse Middle Button (MMB) or LMB + RMB
**Action**: Press and hold MMB (or both LMB and RMB together), then drag the mouse

**Effect**:
- **Horizontal drag (X-axis)**: Pans camera left/right
- **Vertical drag (Y-axis)**: Pans camera up/down

The camera position moves but doesn't rotate. Perfect for adjusting the framing without changing the viewing angle.

### Mouse Wheel
**Action**: Scroll the mouse wheel up or down

**Effect**:
- **Scroll up**: Zooms in (moves camera forward)
- **Scroll down**: Zooms out (moves camera backward)

Quick zooming along the camera's forward direction.

## Technical Implementation

### Camera Class Extensions

The `FCamera` class has been extended with the following methods:

```cpp
// Movement
void MoveForwardBackward(float Delta);  // Move along forward vector
void PanRight(float Delta);              // Move along right vector
void PanUp(float Delta);                 // Move along up vector
void Zoom(float Delta);                  // Move forward/backward quickly

// Rotation
void RotateYaw(float Delta);             // Rotate around Y-axis (left/right)
void RotatePitch(float Delta);           // Rotate around X-axis (up/down)
```

### Camera Orientation System

The camera now maintains:
- **Pitch**: Rotation around the right axis (up/down view angle)
- **Yaw**: Rotation around the up axis (left/right view angle)
- **Forward, Right, Up vectors**: Computed from pitch/yaw to define camera orientation

The `UpdateOrientation()` method recalculates these vectors whenever the camera moves or rotates, ensuring the view matrix is always correct.

### Input State Tracking

A global `FInputState` structure tracks:
- Which mouse buttons are currently pressed
- Last known mouse position for delta calculations
- Button state combinations (e.g., LMB + RMB)

### Input Processing

The Windows message loop (`WindowProc` in `Main.cpp`) handles:

1. **WM_LBUTTONDOWN/UP, WM_RBUTTONDOWN/UP, WM_MBUTTONDOWN/UP**: Track button states and capture/release mouse
2. **WM_MOUSEMOVE**: Calculate mouse delta and apply appropriate camera transformations based on button combinations
3. **WM_MOUSEWHEEL**: Apply zoom based on scroll amount

### Sensitivity Settings

Current sensitivity values (in `Main.cpp`, `CameraSettings` namespace):
```cpp
namespace CameraSettings {
    constexpr float MovementSpeed = 0.01f;   // Forward/backward movement
    constexpr float RotationSpeed = 0.005f;  // Camera rotation
    constexpr float PanSpeed = 0.01f;        // Pan movement
    constexpr float ZoomSpeed = 0.5f;        // Zoom/scroll speed
}
```

These can be adjusted for different feel preferences.

## Usage Example

1. **Launch the application**: The cube will be visible with the camera at its initial position
2. **Hold RMB and drag**: Look around the cube from all angles
3. **Hold LMB and drag up**: Move closer to the cube while maintaining your view direction
4. **Hold LMB and drag left/right**: Orbit around while moving
5. **Hold MMB and drag**: Reposition the cube in the frame without rotating view
6. **Scroll wheel**: Quick zoom in/out

## Differences from UE5

This implementation closely follows UE5's viewport controls with minor simplifications:
- No keyboard modifiers (Alt, Shift, Ctrl) are currently implemented
- No orbit-around-point mode (camera rotates around itself, not a target point)
- Simplified sensitivity settings (UE5 has extensive customization options)

## Future Enhancements

Potential improvements:
- Add keyboard movement (WASD keys)
- Add orbit mode (rotate around a selected point)
- Add camera speed multipliers (Shift for fast, Ctrl for slow)
- Add camera bookmarks/saved positions
- Add smooth camera interpolation for less jarring movement
- Add configurable sensitivity settings via UI or config file
