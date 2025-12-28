#include "OBJPrimitive.h"
#include "TexturedSceneProxy.h"
#include "../Game/GameGlobals.h"

FOBJPrimitive::FOBJPrimitive(const std::string& InFilename, FRHI* InRHI)
    : Filename(InFilename)
    , DiffuseTexture(nullptr)
    , RHIRef(InRHI)
    , bAutoRotate(false)
    , RotationSpeed(0.5f)
{
    // Set primitive type to Lit (textured uses lit rendering with texture sampling)
    PrimitiveType = EPrimitiveType::Lit;
    
    // Load OBJ file
    if (!FOBJLoader::LoadFromFile(Filename, MeshData))
    {
        FLog::Log(ELogLevel::Error, "Failed to load OBJ: " + Filename);
        return;
    }
    
    // Set material from loaded mesh data
    Material.DiffuseColor = MeshData.Material.DiffuseColor;
    Material.SpecularColor = MeshData.Material.SpecularColor;
    Material.AmbientColor = MeshData.Material.AmbientColor;
    Material.Shininess = MeshData.Material.Shininess;
    
    // Load diffuse texture if available
    if (MeshData.Material.HasDiffuseTexture() && RHIRef)
    {
        DiffuseTexture = FTextureLoader::CreateTextureFromFile(RHIRef, MeshData.Material.DiffuseTexturePath);
        if (DiffuseTexture)
        {
            FLog::Log(ELogLevel::Info, "Loaded diffuse texture: " + MeshData.Material.DiffuseTexturePath);
        }
        else
        {
            FLog::Log(ELogLevel::Warning, "Failed to load diffuse texture, using checker pattern fallback");
            // Create a checker pattern texture as fallback
            DiffuseTexture = FTextureLoader::CreateCheckerTexture(RHIRef, 256, 32,
                FColor(1.0f, 1.0f, 1.0f, 1.0f), FColor(0.8f, 0.8f, 0.8f, 1.0f));
        }
    }
    else if (RHIRef)
    {
        // Create checker texture as default for untextured OBJ
        DiffuseTexture = FTextureLoader::CreateCheckerTexture(RHIRef, 256, 32,
            FColor(0.9f, 0.9f, 0.95f, 1.0f), FColor(0.7f, 0.75f, 0.85f, 1.0f));
    }
    
    FLog::Log(ELogLevel::Info, "FOBJPrimitive created: " + Filename + 
              " (" + std::to_string(MeshData.GetTriangleCount()) + " triangles)");
}

FOBJPrimitive::~FOBJPrimitive()
{
    delete DiffuseTexture;
    FLog::Log(ELogLevel::Info, "FOBJPrimitive destroyed");
}

void FOBJPrimitive::Tick(float DeltaTime)
{
    if (bAutoRotate)
    {
        Transform.Rotation.Y += RotationSpeed * DeltaTime;
        MarkTransformDirty();
    }
}

FSceneProxy* FOBJPrimitive::CreateSceneProxy(FRHI* RHI, FLightScene* LightScene)
{
    if (!MeshData.IsValid())
    {
        FLog::Log(ELogLevel::Error, "FOBJPrimitive::CreateSceneProxy - Invalid mesh data");
        return nullptr;
    }
    
    FLog::Log(ELogLevel::Info, "Creating textured scene proxy for OBJ model");
    
    // Create RHI resources
    FRHIBuffer* vertexBuffer = RHI->CreateVertexBuffer(
        MeshData.Vertices.size() * sizeof(FTexturedVertex),
        MeshData.Vertices.data());
    
    FRHIBuffer* indexBuffer = RHI->CreateIndexBuffer(
        MeshData.Indices.size() * sizeof(uint32),
        MeshData.Indices.data());
    
    FRHIBuffer* mvpBuffer = RHI->CreateConstantBuffer(sizeof(FMatrix4x4));
    FRHIBuffer* lightingBuffer = RHI->CreateConstantBuffer(sizeof(FLightingConstants));
    
    // Create pipeline states
    EPipelineFlags flags = EPipelineFlags::EnableDepth | EPipelineFlags::EnableLighting | EPipelineFlags::EnableTextures;
    FRHIPipelineState* pso = RHI->CreateGraphicsPipelineStateEx(flags);
    
    // Shadow PSO (depth-only)
    EPipelineFlags shadowFlags = EPipelineFlags::EnableDepth | EPipelineFlags::DepthOnly;
    FRHIPipelineState* shadowPSO = RHI->CreateGraphicsPipelineStateEx(shadowFlags);
    
    // Create the proxy
    FTexturedSceneProxy* proxy = new FTexturedSceneProxy(
        vertexBuffer, indexBuffer, mvpBuffer, lightingBuffer,
        pso, shadowPSO,
        MeshData.GetIndexCount(),
        g_Camera, Transform, LightScene, Material,
        DiffuseTexture, RHI);
    
    return proxy;
}
