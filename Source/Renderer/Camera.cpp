#include "Camera.h"
#include <cmath>

FCamera::FCamera()
    : Position(0.0f, 0.0f, -5.0f)
    , LookAtTarget(0.0f, 0.0f, 0.0f)
    , UpVector(0.0f, 1.0f, 0.0f)
    , Pitch(0.0f)
    , Yaw(0.0f)
    , Forward(0.0f, 0.0f, 1.0f)
    , Right(1.0f, 0.0f, 0.0f)
    , Up(0.0f, 1.0f, 0.0f)
    , FovY(DirectX::XM_PIDIV4)  // 45 degrees
    , AspectRatio(16.0f / 9.0f)
    , NearPlane(0.1f)
    , FarPlane(100.0f)
    , bViewMatrixDirty(true)
    , bProjectionMatrixDirty(true)
{
    // Calculate initial pitch and yaw based on current position and look target
    FVector direction;
    direction.X = LookAtTarget.X - Position.X;
    direction.Y = LookAtTarget.Y - Position.Y;
    direction.Z = LookAtTarget.Z - Position.Z;
    
    // Normalize
    float length = std::sqrt(direction.X * direction.X + direction.Y * direction.Y + direction.Z * direction.Z);
    if (length > 0.0001f) {
        direction.X /= length;
        direction.Y /= length;
        direction.Z /= length;
    }
    
    // Calculate yaw and pitch from direction
    // DirectX left-handed coordinate system: Z is forward, Y is up, X is right
    Yaw = std::atan2(direction.X, direction.Z);
    // Clamp direction.Y to valid range for asin [-1, 1]
    float clampedY = direction.Y;
    if (clampedY > 1.0f) clampedY = 1.0f;
    if (clampedY < -1.0f) clampedY = -1.0f;
    Pitch = std::asin(-clampedY);
    
    UpdateOrientation();
}

FCamera::~FCamera() {
}

void FCamera::SetPosition(const FVector& InPosition) {
    Position = InPosition;
    bViewMatrixDirty = true;
}

void FCamera::SetLookAt(const FVector& Target) {
    LookAtTarget = Target;
    bViewMatrixDirty = true;
}

void FCamera::SetUpVector(const FVector& Up) {
    UpVector = Up;
    bViewMatrixDirty = true;
}

void FCamera::SetPerspective(float FovYRadians, float InAspectRatio, float InNearPlane, float InFarPlane) {
    FovY = FovYRadians;
    AspectRatio = InAspectRatio;
    NearPlane = InNearPlane;
    FarPlane = InFarPlane;
    bProjectionMatrixDirty = true;
}

FMatrix4x4 FCamera::GetViewMatrix() const {
    if (bViewMatrixDirty) {
        UpdateMatrices();
    }
    return ViewMatrix;
}

FMatrix4x4 FCamera::GetProjectionMatrix() const {
    if (bProjectionMatrixDirty) {
        UpdateMatrices();
    }
    return ProjectionMatrix;
}

FMatrix4x4 FCamera::GetViewProjectionMatrix() const {
    return GetViewMatrix() * GetProjectionMatrix();
}

void FCamera::UpdateMatrices() const {
    if (bViewMatrixDirty) {
        ViewMatrix = FMatrix4x4::LookAtLH(Position, LookAtTarget, UpVector);
        bViewMatrixDirty = false;
    }
    
    if (bProjectionMatrixDirty) {
        ProjectionMatrix = FMatrix4x4::PerspectiveFovLH(FovY, AspectRatio, NearPlane, FarPlane);
        bProjectionMatrixDirty = false;
    }
}

void FCamera::UpdateOrientation() {
    // Calculate forward vector from pitch and yaw
    Forward.X = std::sin(Yaw) * std::cos(Pitch);
    Forward.Y = -std::sin(Pitch);
    Forward.Z = std::cos(Yaw) * std::cos(Pitch);
    
    // Normalize forward
    float length = std::sqrt(Forward.X * Forward.X + Forward.Y * Forward.Y + Forward.Z * Forward.Z);
    if (length > 0.0001f) {
        Forward.X /= length;
        Forward.Y /= length;
        Forward.Z /= length;
    }
    
    // Calculate right vector (cross product of world up and forward)
    FVector worldUp(0.0f, 1.0f, 0.0f);
    Right.X = worldUp.Y * Forward.Z - worldUp.Z * Forward.Y;
    Right.Y = worldUp.Z * Forward.X - worldUp.X * Forward.Z;
    Right.Z = worldUp.X * Forward.Y - worldUp.Y * Forward.X;
    
    // Normalize right
    length = std::sqrt(Right.X * Right.X + Right.Y * Right.Y + Right.Z * Right.Z);
    if (length > 0.0001f) {
        Right.X /= length;
        Right.Y /= length;
        Right.Z /= length;
    }
    
    // Calculate up vector (cross product of forward and right)
    Up.X = Forward.Y * Right.Z - Forward.Z * Right.Y;
    Up.Y = Forward.Z * Right.X - Forward.X * Right.Z;
    Up.Z = Forward.X * Right.Y - Forward.Y * Right.X;
    
    // Update look target based on position and forward direction
    LookAtTarget.X = Position.X + Forward.X;
    LookAtTarget.Y = Position.Y + Forward.Y;
    LookAtTarget.Z = Position.Z + Forward.Z;
    
    UpVector = Up;
    bViewMatrixDirty = true;
}

// UE5-style camera controls

// LMB drag: Move forward/backward
void FCamera::MoveForwardBackward(float Delta) {
    Position.X += Forward.X * Delta;
    Position.Y += Forward.Y * Delta;
    Position.Z += Forward.Z * Delta;
    UpdateOrientation();
}

// LMB drag: Rotate left/right (yaw)
void FCamera::RotateYaw(float Delta) {
    Yaw += Delta;
    UpdateOrientation();
}

// RMB drag: Rotate camera pitch
void FCamera::RotatePitch(float Delta) {
    Pitch += Delta;
    
    // Clamp pitch to avoid flipping
    const float MaxPitch = DirectX::XM_PIDIV2 - 0.01f;  // ~89 degrees
    if (Pitch > MaxPitch) Pitch = MaxPitch;
    if (Pitch < -MaxPitch) Pitch = -MaxPitch;
    
    UpdateOrientation();
}

// LMB+RMB or MMB drag: Pan right
void FCamera::PanRight(float Delta) {
    Position.X += Right.X * Delta;
    Position.Y += Right.Y * Delta;
    Position.Z += Right.Z * Delta;
    UpdateOrientation();
}

// LMB+RMB or MMB drag: Pan up
void FCamera::PanUp(float Delta) {
    Position.X += Up.X * Delta;
    Position.Y += Up.Y * Delta;
    Position.Z += Up.Z * Delta;
    UpdateOrientation();
}

// Mouse wheel: Zoom
void FCamera::Zoom(float Delta) {
    Position.X += Forward.X * Delta;
    Position.Y += Forward.Y * Delta;
    Position.Z += Forward.Z * Delta;
    UpdateOrientation();
}
