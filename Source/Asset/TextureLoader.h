#pragma once

#include "../Core/CoreTypes.h"
#include "../RHI/RHI.h"
#include <string>
#include <vector>

/**
 * FTextureData - Raw texture data loaded from file
 */
struct FTextureData
{
    std::vector<uint8> Pixels;   // RGBA8 format (4 bytes per pixel)
    uint32 Width;
    uint32 Height;
    uint32 Channels;
    
    FTextureData() : Width(0), Height(0), Channels(0) {}
    
    bool IsValid() const { return !Pixels.empty() && Width > 0 && Height > 0; }
};

/**
 * FTextureLoader - Utility class for loading textures from files
 * Supports PNG, JPEG, BMP, TGA formats via stb_image
 */
class FTextureLoader
{
public:
    /**
     * Load texture data from file
     * @param Filename - Path to texture file
     * @param OutData - Output texture data (RGBA8 format)
     * @return true if successful
     */
    static bool LoadFromFile(const std::string& Filename, FTextureData& OutData);
    
    /**
     * Create RHI texture from file
     * @param RHI - RHI instance for resource creation
     * @param Filename - Path to texture file
     * @return RHI texture, or nullptr on failure
     */
    static FRHITexture* CreateTextureFromFile(FRHI* RHI, const std::string& Filename);
    
    /**
     * Create a 1x1 solid color texture (for fallback)
     * @param RHI - RHI instance for resource creation
     * @param Color - Solid color for the texture
     * @return RHI texture
     */
    static FRHITexture* CreateSolidColorTexture(FRHI* RHI, const FColor& Color);
    
    /**
     * Create a procedural checker pattern texture
     * @param RHI - RHI instance for resource creation
     * @param Size - Texture size (square)
     * @param CheckerSize - Size of each checker square
     * @param Color1 - First checker color
     * @param Color2 - Second checker color
     * @return RHI texture
     */
    static FRHITexture* CreateCheckerTexture(FRHI* RHI, uint32 Size, uint32 CheckerSize, 
                                              const FColor& Color1, const FColor& Color2);
};
