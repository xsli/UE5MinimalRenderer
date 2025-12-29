#pragma once

#include "../Core/CoreTypes.h"
#include "../RHI/RHI.h"
#include "../Lighting/Light.h"
#include "../Asset/OBJLoader.h"
#include "../Asset/TextureLoader.h"
#include "ScenePrimitive.h"

/**
 * FOBJPrimitive - Primitive that loads and renders OBJ model files
 * Supports diffuse textures via material's map_Kd property
 */
class FOBJPrimitive : public FPrimitive
{
public:
    /**
     * Construct an OBJ primitive
     * @param InFilename - Path to OBJ file
     * @param InRHI - RHI instance for texture creation
     */
    FOBJPrimitive(const std::string& InFilename, FRHI* InRHI);
    virtual ~FOBJPrimitive() override;
    
    virtual void Tick(float DeltaTime) override;
    virtual FSceneProxy* CreateSceneProxy(FRHI* RHI, FLightScene* LightScene) override;
    
    // Check if model loaded successfully
    bool IsValid() const { return MeshData.IsValid(); }
    
    // Get mesh data
    const FMeshData& GetMeshData() const { return MeshData; }
    
    // Get diffuse texture (may be null)
    FRHITexture* GetDiffuseTexture() const { return DiffuseTexture; }
    
    // Auto-rotation for demo
    void SetAutoRotate(bool bEnable) { bAutoRotate = bEnable; }
    bool IsAutoRotating() const { return bAutoRotate; }
    void SetRotationSpeed(float Speed) { RotationSpeed = Speed; }
    
private:
    std::string Filename;
    FMeshData MeshData;
    FRHITexture* DiffuseTexture;
    FRHI* RHIRef;
    bool bAutoRotate;
    float RotationSpeed;
};
