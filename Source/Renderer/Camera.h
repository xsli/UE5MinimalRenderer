#pragma once

#include "../Core/CoreTypes.h"

// 3D Camera using DirectX coordinate system (left-handed, Y-up)
class FCamera {
public:
    FCamera();
    ~FCamera();
    
    // Set camera position
    void SetPosition(const FVector& Position);
    
    // Set camera look target
    void SetLookAt(const FVector& Target);
    
    // Set camera up vector (default is Y-up)
    void SetUpVector(const FVector& Up);
    
    // Set perspective projection parameters
    void SetPerspective(float FovYRadians, float AspectRatio, float NearPlane, float FarPlane);
    
    // Get view matrix (world to camera space)
    FMatrix4x4 GetViewMatrix() const;
    
    // Get projection matrix (camera to clip space)
    FMatrix4x4 GetProjectionMatrix() const;
    
    // Get combined view-projection matrix
    FMatrix4x4 GetViewProjectionMatrix() const;
    
    // Get camera position
    const FVector& GetPosition() const { return Position; }
    
private:
    void UpdateMatrices() const;
    
    FVector Position;
    FVector LookAtTarget;
    FVector UpVector;
    
    float FovY;
    float AspectRatio;
    float NearPlane;
    float FarPlane;
    
    mutable FMatrix4x4 ViewMatrix;
    mutable FMatrix4x4 ProjectionMatrix;
    
    mutable bool bViewMatrixDirty;
    mutable bool bProjectionMatrixDirty;
};
