#include "Camera.h"
#include <cmath>

FCamera::FCamera()
    : Position(0.0f, 0.0f, -5.0f)
    , LookAtTarget(0.0f, 0.0f, 0.0f)
    , UpVector(0.0f, 1.0f, 0.0f)
    , FovY(DirectX::XM_PIDIV4)  // 45 degrees
    , AspectRatio(16.0f / 9.0f)
    , NearPlane(0.1f)
    , FarPlane(100.0f)
    , bViewMatrixDirty(true)
    , bProjectionMatrixDirty(true)
{
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
        const_cast<FCamera*>(this)->UpdateMatrices();
    }
    return ViewMatrix;
}

FMatrix4x4 FCamera::GetProjectionMatrix() const {
    if (bProjectionMatrixDirty) {
        const_cast<FCamera*>(this)->UpdateMatrices();
    }
    return ProjectionMatrix;
}

FMatrix4x4 FCamera::GetViewProjectionMatrix() const {
    return GetViewMatrix() * GetProjectionMatrix();
}

void FCamera::UpdateMatrices() {
    if (bViewMatrixDirty) {
        ViewMatrix = FMatrix4x4::LookAtLH(Position, LookAtTarget, UpVector);
        bViewMatrixDirty = false;
    }
    
    if (bProjectionMatrixDirty) {
        ProjectionMatrix = FMatrix4x4::PerspectiveFovLH(FovY, AspectRatio, NearPlane, FarPlane);
        bProjectionMatrixDirty = false;
    }
}
