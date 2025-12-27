#pragma once

#include "CoreTypes.h"
#include <string>

/**
 * FShaderLoader - Simple shader file loading system
 * 
 * Follows UE convention with .usf (Unreal Shader File) extension
 * Loads shader source code from the Shaders directory
 */
class FShaderLoader
{
public:
    /**
     * Load shader source code from a .usf file
     * @param ShaderName Name of the shader (without extension), e.g., "Lit", "DepthOnly"
     * @return Shader source code as string, empty if file not found
     */
    static std::string LoadShaderFromFile(const std::string& ShaderName);
    
    /**
     * Set the shader directory path
     * @param Path Path to shader directory (default: "Shaders")
     */
    static void SetShaderDirectory(const std::string& Path);
    
    /**
     * Get current shader directory
     */
    static const std::string& GetShaderDirectory();
    
    /**
     * Check if a shader file exists
     * @param ShaderName Name of the shader (without extension)
     */
    static bool ShaderExists(const std::string& ShaderName);
    
private:
    static std::string ShaderDirectory;
};
