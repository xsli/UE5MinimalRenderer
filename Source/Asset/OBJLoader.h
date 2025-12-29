#pragma once

#include "../Core/CoreTypes.h"
#include "../RHI/RHI.h"
#include <string>
#include <vector>
#include <map>

/**
 * FMeshMaterial - Material data loaded from MTL file
 */
struct FMeshMaterial
{
    std::string Name;
    std::string DiffuseTexturePath;  // Path to diffuse texture (map_Kd)
    FColor DiffuseColor;             // Kd
    FColor SpecularColor;            // Ks
    FColor AmbientColor;             // Ka
    float Shininess;                 // Ns (specular exponent)
    
    FMeshMaterial()
        : DiffuseColor(0.8f, 0.8f, 0.8f, 1.0f)
        , SpecularColor(1.0f, 1.0f, 1.0f, 1.0f)
        , AmbientColor(0.2f, 0.2f, 0.2f, 1.0f)
        , Shininess(32.0f)
    {}
    
    bool HasDiffuseTexture() const { return !DiffuseTexturePath.empty(); }
};

/**
 * FMeshData - Mesh data loaded from OBJ file
 * Contains textured vertices and indices
 */
struct FMeshData
{
    std::vector<FTexturedVertex> Vertices;  // Vertices with position, normal, UV, color
    std::vector<uint32> Indices;
    FMeshMaterial Material;
    
    bool IsValid() const { return !Vertices.empty() && !Indices.empty(); }
    
    uint32 GetVertexCount() const { return static_cast<uint32>(Vertices.size()); }
    uint32 GetIndexCount() const { return static_cast<uint32>(Indices.size()); }
    uint32 GetTriangleCount() const { return GetIndexCount() / 3; }
};

/**
 * FOBJLoader - Utility class for loading OBJ model files
 * Uses tinyobjloader for parsing
 */
class FOBJLoader
{
public:
    /**
     * Load mesh data from OBJ file
     * @param Filename - Path to OBJ file
     * @param OutMeshData - Output mesh data
     * @return true if successful
     */
    static bool LoadFromFile(const std::string& Filename, FMeshData& OutMeshData);
    
    /**
     * Load mesh data with base path for textures
     * @param Filename - Path to OBJ file
     * @param BasePath - Base path for resolving texture paths
     * @param OutMeshData - Output mesh data
     * @return true if successful
     */
    static bool LoadFromFileWithBasePath(const std::string& Filename, const std::string& BasePath, FMeshData& OutMeshData);
};
