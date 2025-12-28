#define TINYOBJLOADER_IMPLEMENTATION
#include "../ThirdParty/tiny_obj_loader.h"
#include "OBJLoader.h"
#include <unordered_map>
#include <cmath>

// Hash function for vertex deduplication
struct VertexHash
{
    size_t operator()(const FTexturedVertex& v) const
    {
        // Simple hash combining position, normal, and UV
        auto h1 = std::hash<float>()(v.Position.X);
        auto h2 = std::hash<float>()(v.Position.Y);
        auto h3 = std::hash<float>()(v.Position.Z);
        auto h4 = std::hash<float>()(v.Normal.X);
        auto h5 = std::hash<float>()(v.Normal.Y);
        auto h6 = std::hash<float>()(v.Normal.Z);
        auto h7 = std::hash<float>()(v.TexCoord.X);
        auto h8 = std::hash<float>()(v.TexCoord.Y);
        
        size_t seed = 0;
        seed ^= h1 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= h2 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= h3 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= h4 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= h5 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= h6 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= h7 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= h8 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        return seed;
    }
};

// Equality function for vertex deduplication
struct VertexEqual
{
    bool operator()(const FTexturedVertex& a, const FTexturedVertex& b) const
    {
        const float eps = 1e-6f;
        return std::abs(a.Position.X - b.Position.X) < eps &&
               std::abs(a.Position.Y - b.Position.Y) < eps &&
               std::abs(a.Position.Z - b.Position.Z) < eps &&
               std::abs(a.Normal.X - b.Normal.X) < eps &&
               std::abs(a.Normal.Y - b.Normal.Y) < eps &&
               std::abs(a.Normal.Z - b.Normal.Z) < eps &&
               std::abs(a.TexCoord.X - b.TexCoord.X) < eps &&
               std::abs(a.TexCoord.Y - b.TexCoord.Y) < eps;
    }
};

// Get directory from file path
static std::string GetDirectory(const std::string& filepath)
{
    size_t lastSlash = filepath.find_last_of("/\\");
    if (lastSlash != std::string::npos)
    {
        return filepath.substr(0, lastSlash + 1);
    }
    return "";
}

bool FOBJLoader::LoadFromFile(const std::string& Filename, FMeshData& OutMeshData)
{
    return LoadFromFileWithBasePath(Filename, GetDirectory(Filename), OutMeshData);
}

bool FOBJLoader::LoadFromFileWithBasePath(const std::string& Filename, const std::string& BasePath, FMeshData& OutMeshData)
{
    FLog::Log(ELogLevel::Info, "Loading OBJ file: " + Filename);
    
    tinyobj::ObjReaderConfig readerConfig;
    readerConfig.mtl_search_path = BasePath;
    readerConfig.triangulate = true;  // Triangulate polygons
    
    tinyobj::ObjReader reader;
    
    if (!reader.ParseFromFile(Filename, readerConfig))
    {
        if (!reader.Error().empty())
        {
            FLog::Log(ELogLevel::Error, "TinyObjReader error: " + reader.Error());
        }
        return false;
    }
    
    if (!reader.Warning().empty())
    {
        FLog::Log(ELogLevel::Warning, "TinyObjReader warning: " + reader.Warning());
    }
    
    const auto& attrib = reader.GetAttrib();
    const auto& shapes = reader.GetShapes();
    const auto& materials = reader.GetMaterials();
    
    FLog::Log(ELogLevel::Info, "OBJ loaded: " + std::to_string(attrib.vertices.size() / 3) + " vertices, " +
              std::to_string(shapes.size()) + " shapes, " + std::to_string(materials.size()) + " materials");
    
    // Load material information (use first material if available)
    if (!materials.empty())
    {
        const auto& mat = materials[0];
        OutMeshData.Material.Name = mat.name;
        OutMeshData.Material.DiffuseColor = FColor(mat.diffuse[0], mat.diffuse[1], mat.diffuse[2], 1.0f);
        OutMeshData.Material.SpecularColor = FColor(mat.specular[0], mat.specular[1], mat.specular[2], 1.0f);
        OutMeshData.Material.AmbientColor = FColor(mat.ambient[0], mat.ambient[1], mat.ambient[2], 1.0f);
        OutMeshData.Material.Shininess = mat.shininess > 0 ? mat.shininess : 32.0f;
        
        // Get diffuse texture path
        if (!mat.diffuse_texname.empty())
        {
            OutMeshData.Material.DiffuseTexturePath = BasePath + mat.diffuse_texname;
            FLog::Log(ELogLevel::Info, "Diffuse texture: " + OutMeshData.Material.DiffuseTexturePath);
        }
    }
    
    // Vertex deduplication map
    std::unordered_map<FTexturedVertex, uint32, VertexHash, VertexEqual> uniqueVertices;
    
    // Process all shapes
    for (const auto& shape : shapes)
    {
        size_t indexOffset = 0;
        
        for (size_t faceIdx = 0; faceIdx < shape.mesh.num_face_vertices.size(); ++faceIdx)
        {
            size_t faceVertices = shape.mesh.num_face_vertices[faceIdx];
            
            // Process each vertex in the face (should be 3 after triangulation)
            for (size_t v = 0; v < faceVertices; ++v)
            {
                tinyobj::index_t idx = shape.mesh.indices[indexOffset + v];
                
                FTexturedVertex vertex;
                
                // Position
                vertex.Position.X = attrib.vertices[3 * idx.vertex_index + 0];
                vertex.Position.Y = attrib.vertices[3 * idx.vertex_index + 1];
                vertex.Position.Z = attrib.vertices[3 * idx.vertex_index + 2];
                
                // Normal (if available)
                if (idx.normal_index >= 0)
                {
                    vertex.Normal.X = attrib.normals[3 * idx.normal_index + 0];
                    vertex.Normal.Y = attrib.normals[3 * idx.normal_index + 1];
                    vertex.Normal.Z = attrib.normals[3 * idx.normal_index + 2];
                }
                else
                {
                    // Default to up vector if no normal
                    vertex.Normal = FVector(0.0f, 1.0f, 0.0f);
                }
                
                // Texture coordinates (if available)
                if (idx.texcoord_index >= 0)
                {
                    vertex.TexCoord.X = attrib.texcoords[2 * idx.texcoord_index + 0];
                    // Flip Y for DirectX (OBJ uses OpenGL convention)
                    vertex.TexCoord.Y = 1.0f - attrib.texcoords[2 * idx.texcoord_index + 1];
                }
                else
                {
                    vertex.TexCoord = FVector2D(0.0f, 0.0f);
                }
                
                // Vertex color (use material diffuse color)
                vertex.Color = OutMeshData.Material.DiffuseColor;
                
                // Vertex deduplication
                if (uniqueVertices.count(vertex) == 0)
                {
                    uniqueVertices[vertex] = static_cast<uint32>(OutMeshData.Vertices.size());
                    OutMeshData.Vertices.push_back(vertex);
                }
                
                OutMeshData.Indices.push_back(uniqueVertices[vertex]);
            }
            
            indexOffset += faceVertices;
        }
    }
    
    FLog::Log(ELogLevel::Info, "OBJ processed: " + std::to_string(OutMeshData.Vertices.size()) + " unique vertices, " +
              std::to_string(OutMeshData.Indices.size()) + " indices, " +
              std::to_string(OutMeshData.GetTriangleCount()) + " triangles");
    
    return true;
}
