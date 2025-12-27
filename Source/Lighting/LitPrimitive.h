#pragma once

#include "../Core/CoreTypes.h"
#include "../RHI/RHI.h"
#include "../Game/Primitive.h"  // For FTransform
#include "Light.h"

// Forward declarations
class FSceneProxy;
class FRHI;
class FLightScene;

/**
 * FLitPrimitive - Base class for primitives with lighting support
 * Uses FLitVertex format (position, normal, color) for Phong shading
 */
class FLitPrimitive
{
public:
    FLitPrimitive();
    virtual ~FLitPrimitive() = default;

    // Game thread update
    virtual void Tick(float DeltaTime);

    // Create render thread proxy
    virtual FSceneProxy* CreateSceneProxy(FRHI* RHI, FLightScene* LightScene) = 0;

    // Transform accessors
    void SetPosition(const FVector& InPosition) { Position = InPosition; MarkTransformDirty(); }
    const FVector& GetPosition() const { return Position; }
    
    void SetRotation(const FVector& InRotation) { Rotation = InRotation; MarkTransformDirty(); }
    const FVector& GetRotation() const { return Rotation; }
    
    void SetScale(const FVector& InScale) { Scale = InScale; MarkTransformDirty(); }
    const FVector& GetScale() const { return Scale; }

    // Material accessors
    void SetMaterial(const FMaterial& InMaterial) { Material = InMaterial; MarkDirty(); }
    const FMaterial& GetMaterial() const { return Material; }
    FMaterial& GetMaterial() { return Material; }

    // Dirty tracking
    bool IsDirty() const { return bIsDirty; }
    bool IsTransformDirty() const { return bTransformDirty; }
    void MarkDirty() { bIsDirty = true; bTransformDirty = false; }
    void MarkTransformDirty() { bTransformDirty = true; }
    void ClearDirty() { bIsDirty = false; bTransformDirty = false; }

    // Get transform matrix
    FMatrix4x4 GetTransformMatrix() const;
    
    // Get transform struct (for passing to scene proxy)
    FTransform GetTransform() const;

protected:
    FVector Position;
    FVector Rotation;  // Euler angles in radians
    FVector Scale;
    FMaterial Material;
    bool bIsDirty;
    bool bTransformDirty;
};

/**
 * FLitCubePrimitive - Lit cube with per-face normals
 */
class FLitCubePrimitive : public FLitPrimitive
{
public:
    FLitCubePrimitive();
    virtual ~FLitCubePrimitive() override = default;

    virtual void Tick(float DeltaTime) override;
    virtual FSceneProxy* CreateSceneProxy(FRHI* RHI, FLightScene* LightScene) override;

    void SetAutoRotate(bool bEnable) { bAutoRotate = bEnable; }

private:
    bool bAutoRotate;
    float RotationSpeed;
};

/**
 * FLitSpherePrimitive - Lit sphere with smooth normals
 */
class FLitSpherePrimitive : public FLitPrimitive
{
public:
    FLitSpherePrimitive(uint32 InSegments = 24, uint32 InRings = 16);
    virtual ~FLitSpherePrimitive() override = default;

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
 * FLitPlanePrimitive - Lit plane (ground plane)
 */
class FLitPlanePrimitive : public FLitPrimitive
{
public:
    FLitPlanePrimitive(uint32 InSubdivisions = 1);
    virtual ~FLitPlanePrimitive() override = default;

    virtual void Tick(float DeltaTime) override;
    virtual FSceneProxy* CreateSceneProxy(FRHI* RHI, FLightScene* LightScene) override;

private:
    uint32 Subdivisions;
};

/**
 * FLitCylinderPrimitive - Lit cylinder
 */
class FLitCylinderPrimitive : public FLitPrimitive
{
public:
    FLitCylinderPrimitive(uint32 InSegments = 24);
    virtual ~FLitCylinderPrimitive() override = default;

    virtual void Tick(float DeltaTime) override;
    virtual FSceneProxy* CreateSceneProxy(FRHI* RHI, FLightScene* LightScene) override;

    void SetAutoRotate(bool bEnable) { bAutoRotate = bEnable; }

private:
    uint32 Segments;
    bool bAutoRotate;
    float RotationSpeed;
};
