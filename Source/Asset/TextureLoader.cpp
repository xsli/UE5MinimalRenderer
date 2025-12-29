#define STB_IMAGE_IMPLEMENTATION
#include "../ThirdParty/stb_image.h"
#include "TextureLoader.h"

bool FTextureLoader::LoadFromFile(const std::string& Filename, FTextureData& OutData)
{
    FLog::Log(ELogLevel::Info, "Loading texture from file: " + Filename);
    
    int width, height, channels;
    
    // Force RGBA output (4 channels)
    stbi_uc* data = stbi_load(Filename.c_str(), &width, &height, &channels, 4);
    
    if (!data)
    {
        FLog::Log(ELogLevel::Error, "Failed to load texture: " + Filename + " - " + stbi_failure_reason());
        return false;
    }
    
    OutData.Width = static_cast<uint32>(width);
    OutData.Height = static_cast<uint32>(height);
    OutData.Channels = 4;  // Always RGBA
    
    // Copy pixel data
    size_t dataSize = width * height * 4;
    OutData.Pixels.resize(dataSize);
    memcpy(OutData.Pixels.data(), data, dataSize);
    
    // Free stb_image data
    stbi_image_free(data);
    
    FLog::Log(ELogLevel::Info, "Texture loaded: " + std::to_string(width) + "x" + std::to_string(height) + 
              " (original " + std::to_string(channels) + " channels)");
    
    return true;
}

FRHITexture* FTextureLoader::CreateTextureFromFile(FRHI* RHI, const std::string& Filename)
{
    if (!RHI)
    {
        FLog::Log(ELogLevel::Error, "CreateTextureFromFile: RHI is null");
        return nullptr;
    }
    
    FTextureData textureData;
    if (!LoadFromFile(Filename, textureData))
    {
        return nullptr;
    }
    
    return RHI->CreateTexture2D(textureData.Width, textureData.Height, textureData.Pixels.data());
}

FRHITexture* FTextureLoader::CreateSolidColorTexture(FRHI* RHI, const FColor& Color)
{
    if (!RHI)
    {
        FLog::Log(ELogLevel::Error, "CreateSolidColorTexture: RHI is null");
        return nullptr;
    }
    
    // Create 1x1 texture with the specified color
    uint8 pixels[4] = {
        static_cast<uint8>(Color.R * 255.0f),
        static_cast<uint8>(Color.G * 255.0f),
        static_cast<uint8>(Color.B * 255.0f),
        static_cast<uint8>(Color.A * 255.0f)
    };
    
    return RHI->CreateTexture2D(1, 1, pixels);
}

FRHITexture* FTextureLoader::CreateCheckerTexture(FRHI* RHI, uint32 Size, uint32 CheckerSize,
                                                   const FColor& Color1, const FColor& Color2)
{
    if (!RHI)
    {
        FLog::Log(ELogLevel::Error, "CreateCheckerTexture: RHI is null");
        return nullptr;
    }
    
    FLog::Log(ELogLevel::Info, "Creating checker texture: " + std::to_string(Size) + "x" + std::to_string(Size));
    
    // Generate checker pattern
    std::vector<uint8> pixels(Size * Size * 4);
    
    uint8 c1[4] = {
        static_cast<uint8>(Color1.R * 255.0f),
        static_cast<uint8>(Color1.G * 255.0f),
        static_cast<uint8>(Color1.B * 255.0f),
        static_cast<uint8>(Color1.A * 255.0f)
    };
    
    uint8 c2[4] = {
        static_cast<uint8>(Color2.R * 255.0f),
        static_cast<uint8>(Color2.G * 255.0f),
        static_cast<uint8>(Color2.B * 255.0f),
        static_cast<uint8>(Color2.A * 255.0f)
    };
    
    for (uint32 y = 0; y < Size; ++y)
    {
        for (uint32 x = 0; x < Size; ++x)
        {
            uint32 idx = (y * Size + x) * 4;
            bool isChecker = ((x / CheckerSize) + (y / CheckerSize)) % 2 == 0;
            uint8* color = isChecker ? c1 : c2;
            
            pixels[idx + 0] = color[0];
            pixels[idx + 1] = color[1];
            pixels[idx + 2] = color[2];
            pixels[idx + 3] = color[3];
        }
    }
    
    return RHI->CreateTexture2D(Size, Size, pixels.data());
}
