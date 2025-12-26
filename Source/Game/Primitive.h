#pragma once

#include "../Core/CoreTypes.h"

// Forward declarations
class FPrimitiveSceneProxy;
class FRHI;

// Transform structure for primitives
struct FTransform {
    FVector Position;
    FVector Rotation;  // Euler angles in radians
    FVector Scale;
    
    FTransform()
        : Position(0.0f, 0.0f, 0.0f)
        , Rotation(0.0f, 0.0f, 0.0f)
        , Scale(1.0f, 1.0f, 1.0f) {
    }
    
    // Get transformation matrix
    FMatrix4x4 GetMatrix() const {
        FMatrix4x4 scale = FMatrix4x4::Scaling(Scale.X, Scale.Y, Scale.Z);
        FMatrix4x4 rotationX = FMatrix4x4::RotationX(Rotation.X);
        FMatrix4x4 rotationY = FMatrix4x4::RotationY(Rotation.Y);
        FMatrix4x4 rotationZ = FMatrix4x4::RotationZ(Rotation.Z);
        FMatrix4x4 translation = FMatrix4x4::Translation(Position.X, Position.Y, Position.Z);
        
        // Standard graphics transformation order: Scale -> Rotate -> Translate
        // Matrix multiplication is right-to-left: Translation * (RotZ * RotY * RotX) * Scale
        // Rotation order is Z*Y*X (Yaw*Pitch*Roll convention)
        return translation * rotationZ * rotationY * rotationX * scale;
    }
};

// Base primitive class - represents game objects
class FPrimitive {
public:
    FPrimitive();
    virtual ~FPrimitive() = default;
    
    // Game thread update
    virtual void Tick(float DeltaTime);
    
    // Create render thread proxy
    virtual FPrimitiveSceneProxy* CreateSceneProxy(FRHI* RHI) = 0;
    
    // Transform accessors
    void SetTransform(const FTransform& InTransform) { Transform = InTransform; }
    const FTransform& GetTransform() const { return Transform; }
    FTransform& GetTransform() { return Transform; }
    
    // Color accessor
    void SetColor(const FColor& InColor) { Color = InColor; }
    const FColor& GetColor() const { return Color; }
    
protected:
    FTransform Transform;
    FColor Color;
};

// Cube primitive
class FCubePrimitive : public FPrimitive {
public:
    FCubePrimitive();
    virtual ~FCubePrimitive() override;
    
    virtual void Tick(float DeltaTime) override;
    virtual FPrimitiveSceneProxy* CreateSceneProxy(FRHI* RHI) override;
    
    void SetAutoRotate(bool bEnable) { bAutoRotate = bEnable; }
    
private:
    bool bAutoRotate;
    float RotationSpeed;
};

// Sphere primitive
class FSpherePrimitive : public FPrimitive {
public:
    FSpherePrimitive(uint32 InSegments = 16, uint32 InRings = 16);
    virtual ~FSpherePrimitive() override;
    
    virtual void Tick(float DeltaTime) override;
    virtual FPrimitiveSceneProxy* CreateSceneProxy(FRHI* RHI) override;
    
    void SetAutoRotate(bool bEnable) { bAutoRotate = bEnable; }
    
private:
    uint32 Segments;  // Longitude divisions
    uint32 Rings;     // Latitude divisions
    bool bAutoRotate;
    float RotationSpeed;
};

// Cylinder primitive
class FCylinderPrimitive : public FPrimitive {
public:
    FCylinderPrimitive(uint32 InSegments = 16);
    virtual ~FCylinderPrimitive() override;
    
    virtual void Tick(float DeltaTime) override;
    virtual FPrimitiveSceneProxy* CreateSceneProxy(FRHI* RHI) override;
    
    void SetAutoRotate(bool bEnable) { bAutoRotate = bEnable; }
    
private:
    uint32 Segments;  // Circle divisions
    bool bAutoRotate;
    float RotationSpeed;
};

// Plane primitive
class FPlanePrimitive : public FPrimitive {
public:
    FPlanePrimitive(uint32 InSubdivisions = 1);
    virtual ~FPlanePrimitive() override;
    
    virtual void Tick(float DeltaTime) override;
    virtual FPrimitiveSceneProxy* CreateSceneProxy(FRHI* RHI) override;
    
private:
    uint32 Subdivisions;
};
