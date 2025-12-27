#pragma once

#include "../Core/CoreTypes.h"
#include "../RHI/RHI.h"
#include "../Lighting/Light.h"  // Includes FMaterial, FLightScene

// Forward declarations
class FSceneProxy;
class FRHI;
class FCamera;

/**
 * FTransform - Transform component for primitives
 */
struct FTransform 
{
    FVector Position;
    FVector Rotation;  // Euler angles in radians
    FVector Scale;

    FTransform()
        : Position(0.0f, 0.0f, 0.0f)
        , Rotation(0.0f, 0.0f, 0.0f)
        , Scale(1.0f, 1.0f, 1.0f)
    {
    }

    // Get transformation matrix
    FMatrix4x4 GetMatrix() const
    {
        FMatrix4x4 scale = FMatrix4x4::Scaling(Scale.X, Scale.Y, Scale.Z);
        FMatrix4x4 rotationX = FMatrix4x4::RotationX(Rotation.X);
        FMatrix4x4 rotationY = FMatrix4x4::RotationY(Rotation.Y);
        FMatrix4x4 rotationZ = FMatrix4x4::RotationZ(Rotation.Z);
        FMatrix4x4 translation = FMatrix4x4::Translation(Position.X, Position.Y, Position.Z);

        return scale * rotationX * rotationY * rotationZ * translation;
    }
};

// FMaterial is defined in ../Lighting/Light.h

/**
 * EPrimitiveType - Type of primitive for rendering mode selection
 */
enum class EPrimitiveType
{
    Lit,      // Uses lighting calculations (default)
    Unlit,    // No lighting, uses vertex colors directly
    Wireframe // Line rendering for debug visualization
};

/**
 * FPrimitive - Base class for all scene primitives
 * Supports both lit and unlit rendering modes
 */
class FPrimitive 
{
public:
    FPrimitive();
    virtual ~FPrimitive() = default;

    // Game thread update
    virtual void Tick(float DeltaTime);

    // Create render thread proxy
    virtual FSceneProxy* CreateSceneProxy(FRHI* RHI, FLightScene* LightScene) = 0;

    // Transform accessors
    void SetTransform(const FTransform& InTransform) { Transform = InTransform; MarkTransformDirty(); }
    const FTransform& GetTransform() const { return Transform; }
    FTransform& GetTransform() { return Transform; }
    
    void SetPosition(const FVector& InPosition) { Transform.Position = InPosition; MarkTransformDirty(); }
    const FVector& GetPosition() const { return Transform.Position; }
    
    void SetRotation(const FVector& InRotation) { Transform.Rotation = InRotation; MarkTransformDirty(); }
    const FVector& GetRotation() const { return Transform.Rotation; }
    
    void SetScale(const FVector& InScale) { Transform.Scale = InScale; MarkTransformDirty(); }
    const FVector& GetScale() const { return Transform.Scale; }
    
    FMatrix4x4 GetTransformMatrix() const { return Transform.GetMatrix(); }

    // Material accessors
    void SetMaterial(const FMaterial& InMaterial) { Material = InMaterial; MarkDirty(); }
    const FMaterial& GetMaterial() const { return Material; }
    FMaterial& GetMaterial() { return Material; }

    // Color accessor (for unlit mode)
    void SetColor(const FColor& InColor) { Color = InColor; MarkDirty(); }
    const FColor& GetColor() const { return Color; }

    // Primitive type
    EPrimitiveType GetPrimitiveType() const { return PrimitiveType; }
    void SetPrimitiveType(EPrimitiveType InType) { PrimitiveType = InType; MarkDirty(); }

    // Dirty tracking
    bool IsDirty() const { return bIsDirty; }
    bool IsTransformDirty() const { return bTransformDirty; }
    void MarkDirty() { bIsDirty = true; bTransformDirty = false; }
    void MarkTransformDirty() { bTransformDirty = true; }
    void ClearDirty() { bIsDirty = false; bTransformDirty = false; }

protected:
    FTransform Transform;
    FMaterial Material;
    FColor Color;
    EPrimitiveType PrimitiveType;
    bool bIsDirty;
    bool bTransformDirty;
};

// ============================================================================
// LIT PRIMITIVES (Default)
// ============================================================================

/**
 * FCubePrimitive - Lit cube with per-face normals
 */
class FCubePrimitive : public FPrimitive
{
public:
    FCubePrimitive();
    virtual ~FCubePrimitive() override = default;

    virtual void Tick(float DeltaTime) override;
    virtual FSceneProxy* CreateSceneProxy(FRHI* RHI, FLightScene* LightScene) override;

    void SetAutoRotate(bool bEnable) { bAutoRotate = bEnable; }
    bool IsAutoRotating() const { return bAutoRotate; }

private:
    bool bAutoRotate;
    float RotationSpeed;
};

/**
 * FSpherePrimitive - Lit sphere with smooth normals
 */
class FSpherePrimitive : public FPrimitive
{
public:
    FSpherePrimitive(uint32 InSegments = 24, uint32 InRings = 16);
    virtual ~FSpherePrimitive() override = default;

    virtual void Tick(float DeltaTime) override;
    virtual FSceneProxy* CreateSceneProxy(FRHI* RHI, FLightScene* LightScene) override;

    void SetAutoRotate(bool bEnable) { bAutoRotate = bEnable; }

private:
    uint32 Segments;
    uint32 Rings;
    bool bAutoRotate;
    float RotationSpeed;
};

/**
 * FPlanePrimitive - Lit plane (ground plane)
 */
class FPlanePrimitive : public FPrimitive
{
public:
    FPlanePrimitive(uint32 InSubdivisions = 1);
    virtual ~FPlanePrimitive() override = default;

    virtual void Tick(float DeltaTime) override;
    virtual FSceneProxy* CreateSceneProxy(FRHI* RHI, FLightScene* LightScene) override;

private:
    uint32 Subdivisions;
};

/**
 * FCylinderPrimitive - Lit cylinder
 */
class FCylinderPrimitive : public FPrimitive
{
public:
    FCylinderPrimitive(uint32 InSegments = 24);
    virtual ~FCylinderPrimitive() override = default;

    virtual void Tick(float DeltaTime) override;
    virtual FSceneProxy* CreateSceneProxy(FRHI* RHI, FLightScene* LightScene) override;

    void SetAutoRotate(bool bEnable) { bAutoRotate = bEnable; }

private:
    uint32 Segments;
    bool bAutoRotate;
    float RotationSpeed;
};

// ============================================================================
// UNLIT PRIMITIVES
// ============================================================================

/**
 * FUnlitCubePrimitive - Unlit cube (uses vertex colors)
 */
class FUnlitCubePrimitive : public FPrimitive
{
public:
    FUnlitCubePrimitive();
    virtual ~FUnlitCubePrimitive() override = default;

    virtual void Tick(float DeltaTime) override;
    virtual FSceneProxy* CreateSceneProxy(FRHI* RHI, FLightScene* LightScene) override;

    void SetAutoRotate(bool bEnable) { bAutoRotate = bEnable; }

private:
    bool bAutoRotate;
    float RotationSpeed;
};

/**
 * FUnlitSpherePrimitive - Unlit sphere (uses vertex colors)
 */
class FUnlitSpherePrimitive : public FPrimitive
{
public:
    FUnlitSpherePrimitive(uint32 InSegments = 16, uint32 InRings = 16);
    virtual ~FUnlitSpherePrimitive() override = default;

    virtual void Tick(float DeltaTime) override;
    virtual FSceneProxy* CreateSceneProxy(FRHI* RHI, FLightScene* LightScene) override;

    void SetAutoRotate(bool bEnable) { bAutoRotate = bEnable; }

private:
    uint32 Segments;
    uint32 Rings;
    bool bAutoRotate;
    float RotationSpeed;
};

// ============================================================================
// SPECIAL PRIMITIVES
// ============================================================================

/**
 * FGizmoPrimitive - UE-style coordinate axis visualization
 * X axis = Red, Y axis = Green, Z axis = Blue
 * Special primitive that can render in screen space
 */
class FGizmoPrimitive : public FPrimitive
{
public:
    FGizmoPrimitive(float InAxisLength = 1.5f);
    virtual ~FGizmoPrimitive() override = default;

    virtual void Tick(float DeltaTime) override;
    virtual FSceneProxy* CreateSceneProxy(FRHI* RHI, FLightScene* LightScene) override;

    // Screen-space rendering mode (renders in a corner)
    void SetScreenSpace(bool bEnable) { bScreenSpace = bEnable; }
    bool IsScreenSpace() const { return bScreenSpace; }
    
    // Set screen corner (0=top-left, 1=top-right, 2=bottom-left, 3=bottom-right)
    void SetScreenCorner(int Corner) { ScreenCorner = Corner; }
    int GetScreenCorner() const { return ScreenCorner; }

private:
    float AxisLength;
    bool bScreenSpace;
    int ScreenCorner;  // 3 = bottom-right (default)
};

/**
 * Animation type for demo primitives
 */
enum class EAnimationType
{
    None,
    RotateX,
    RotateY,
    RotateZ,
    TranslateX,
    TranslateY,
    TranslateZ,
    TranslateDiagonal,
    Scale
};

/**
 * FDemoCubePrimitive - Demo cube with configurable animation
 */
class FDemoCubePrimitive : public FPrimitive
{
public:
    FDemoCubePrimitive();
    virtual ~FDemoCubePrimitive() override = default;

    virtual void Tick(float DeltaTime) override;
    virtual FSceneProxy* CreateSceneProxy(FRHI* RHI, FLightScene* LightScene) override;

    void SetAnimationType(EAnimationType InType) { AnimationType = InType; }
    void SetAnimationSpeed(float InSpeed) { AnimationSpeed = InSpeed; }
    void SetBasePosition(const FVector& InPos) { BasePosition = InPos; }
    void SetBaseScale(const FVector& InScale) { BaseScale = InScale; }

private:
    EAnimationType AnimationType;
    float AnimationSpeed;
    float AnimationTime;
    FVector BasePosition;
    FVector BaseScale;
};
