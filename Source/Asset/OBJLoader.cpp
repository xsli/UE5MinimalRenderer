#define TINYOBJLOADER_IMPLEMENTATION
#include "../ThirdParty/tiny_obj_loader.h"
#include "OBJLoader.h"
#include <unordered_map>
#include <cmath>

// Hash function for position-based vertex lookup (for smooth normal generation)
struct PositionHash
{
    size_t operator()(const FVector& v) const
    {
        // Quantize to avoid floating-point precision issues
        auto quantize = [](float f) { return static_cast<int>(f * 10000.0f); };
        auto h1 = std::hash<int>()(quantize(v.X));
        auto h2 = std::hash<int>()(quantize(v.Y));
        auto h3 = std::hash<int>()(quantize(v.Z));
        
        size_t seed = 0;
        seed ^= h1 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= h2 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= h3 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        return seed;
    }
};

// Equality function for position-based vertex lookup
struct PositionEqual
{
    bool operator()(const FVector& a, const FVector& b) const
    {
        const float eps = 1e-4f;
        return std::abs(a.X - b.X) < eps &&
               std::abs(a.Y - b.Y) < eps &&
               std::abs(a.Z - b.Z) < eps;
    }
};

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

// Normalize a vector
static FVector Normalize(const FVector& v)
{
    float len = std::sqrt(v.X * v.X + v.Y * v.Y + v.Z * v.Z);
    if (len > 1e-8f)
    {
        return FVector(v.X / len, v.Y / len, v.Z / len);
    }
    return FVector(0.0f, 1.0f, 0.0f);  // Return up vector if degenerate
}

// Compute dot product of two vectors
static float Dot(const FVector& a, const FVector& b)
{
    return a.X * b.X + a.Y * b.Y + a.Z * b.Z;
}

// Compute the angle at a vertex in a triangle (in radians)
static float ComputeVertexAngle(const FVector& v, const FVector& v0, const FVector& v1)
{
    // Edge vectors from v to v0 and v1
    FVector e0(v0.X - v.X, v0.Y - v.Y, v0.Z - v.Z);
    FVector e1(v1.X - v.X, v1.Y - v.Y, v1.Z - v.Z);
    
    // Normalize edges
    e0 = Normalize(e0);
    e1 = Normalize(e1);
    
    // Clamp dot product to avoid numerical issues with acos
    float dot = Dot(e0, e1);
    dot = std::max(-1.0f, std::min(1.0f, dot));
    
    return std::acos(dot);
}

// Compute face normal from three vertices (unnormalized for area weighting)
static FVector ComputeFaceNormalUnnormalized(const FVector& v0, const FVector& v1, const FVector& v2)
{
    // Edge vectors
    FVector e1(v1.X - v0.X, v1.Y - v0.Y, v1.Z - v0.Z);
    FVector e2(v2.X - v0.X, v2.Y - v0.Y, v2.Z - v0.Z);
    
    // Cross product (magnitude is proportional to triangle area)
    return FVector(
        e1.Y * e2.Z - e1.Z * e2.Y,
        e1.Z * e2.X - e1.X * e2.Z,
        e1.X * e2.Y - e1.Y * e2.X
    );
}

// Compute face normal from three vertices (normalized)
static FVector ComputeFaceNormal(const FVector& v0, const FVector& v1, const FVector& v2)
{
    return Normalize(ComputeFaceNormalUnnormalized(v0, v1, v2));
}

// Generate smooth normals for mesh using angle-weighted position-based averaging
// This properly averages normals for all vertices at the same position, even if duplicated
static void GenerateSmoothNormals(FMeshData& meshData)
{
    if (meshData.Vertices.empty() || meshData.Indices.empty())
        return;
    
    FLog::Log(ELogLevel::Info, "Generating angle-weighted smooth normals for " + std::to_string(meshData.GetTriangleCount()) + " triangles");
    
    // Map from position to accumulated normal (for position-based averaging)
    std::unordered_map<FVector, FVector, PositionHash, PositionEqual> positionNormals;
    
    // First pass: Accumulate angle-weighted face normals per position
    for (size_t i = 0; i + 2 < meshData.Indices.size(); i += 3)
    {
        uint32 i0 = meshData.Indices[i];
        uint32 i1 = meshData.Indices[i + 1];
        uint32 i2 = meshData.Indices[i + 2];
        
        const FVector& v0 = meshData.Vertices[i0].Position;
        const FVector& v1 = meshData.Vertices[i1].Position;
        const FVector& v2 = meshData.Vertices[i2].Position;
        
        // Compute face normal (normalized)
        FVector faceNormal = ComputeFaceNormal(v0, v1, v2);
        
        // Compute angle at each vertex
        float angle0 = ComputeVertexAngle(v0, v1, v2);
        float angle1 = ComputeVertexAngle(v1, v2, v0);
        float angle2 = ComputeVertexAngle(v2, v0, v1);
        
        // Accumulate angle-weighted face normal for each position
        auto accumulateNormal = [&](const FVector& pos, float angle) {
            FVector weighted(faceNormal.X * angle, faceNormal.Y * angle, faceNormal.Z * angle);
            auto it = positionNormals.find(pos);
            if (it == positionNormals.end())
            {
                positionNormals[pos] = weighted;
            }
            else
            {
                it->second.X += weighted.X;
                it->second.Y += weighted.Y;
                it->second.Z += weighted.Z;
            }
        };
        
        accumulateNormal(v0, angle0);
        accumulateNormal(v1, angle1);
        accumulateNormal(v2, angle2);
    }
    
    // Normalize all accumulated normals in the map
    for (auto& pair : positionNormals)
    {
        pair.second = Normalize(pair.second);
    }
    
    // Second pass: Assign the averaged normal to all vertices at each position
    for (auto& vertex : meshData.Vertices)
    {
        auto it = positionNormals.find(vertex.Position);
        if (it != positionNormals.end())
        {
            vertex.Normal = it->second;
        }
        else
        {
            // Fallback (shouldn't happen)
            vertex.Normal = FVector(0.0f, 1.0f, 0.0f);
        }
    }
    
    FLog::Log(ELogLevel::Info, "Angle-weighted smooth normals generated successfully for " + 
              std::to_string(positionNormals.size()) + " unique positions");
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
    
    bool hasNormals = !attrib.normals.empty();
    
    FLog::Log(ELogLevel::Info, "OBJ loaded: " + std::to_string(attrib.vertices.size() / 3) + " vertices, " +
              std::to_string(attrib.normals.size() / 3) + " normals, " +
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
            
            // Get material for this face to use its diffuse color
            int materialId = -1;
            if (faceIdx < shape.mesh.material_ids.size())
            {
                materialId = shape.mesh.material_ids[faceIdx];
            }
            
            FColor faceColor = OutMeshData.Material.DiffuseColor;
            if (materialId >= 0 && materialId < static_cast<int>(materials.size()))
            {
                const auto& faceMat = materials[materialId];
                faceColor = FColor(faceMat.diffuse[0], faceMat.diffuse[1], faceMat.diffuse[2], 1.0f);
            }
            
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
                if (idx.normal_index >= 0 && hasNormals)
                {
                    vertex.Normal.X = attrib.normals[3 * idx.normal_index + 0];
                    vertex.Normal.Y = attrib.normals[3 * idx.normal_index + 1];
                    vertex.Normal.Z = attrib.normals[3 * idx.normal_index + 2];
                }
                else
                {
                    // Will be computed later
                    vertex.Normal = FVector(0.0f, 0.0f, 0.0f);
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
                
                // Vertex color (use per-face material diffuse color)
                vertex.Color = faceColor;
                
                // Don't deduplicate if we need to generate normals (would break smooth normals)
                // Only deduplicate if normals are already present
                if (hasNormals)
                {
                    if (uniqueVertices.count(vertex) == 0)
                    {
                        uniqueVertices[vertex] = static_cast<uint32>(OutMeshData.Vertices.size());
                        OutMeshData.Vertices.push_back(vertex);
                    }
                    OutMeshData.Indices.push_back(uniqueVertices[vertex]);
                }
                else
                {
                    // Push each vertex separately for proper normal calculation
                    OutMeshData.Indices.push_back(static_cast<uint32>(OutMeshData.Vertices.size()));
                    OutMeshData.Vertices.push_back(vertex);
                }
            }
            
            indexOffset += faceVertices;
        }
    }
    
    // Generate normals if not present in the file
    if (!hasNormals)
    {
        GenerateSmoothNormals(OutMeshData);
    }
    
    FLog::Log(ELogLevel::Info, "OBJ processed: " + std::to_string(OutMeshData.Vertices.size()) + " unique vertices, " +
              std::to_string(OutMeshData.Indices.size()) + " indices, " +
              std::to_string(OutMeshData.GetTriangleCount()) + " triangles");
    
    return true;
}
