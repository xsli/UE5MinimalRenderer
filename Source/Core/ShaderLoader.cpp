#include "ShaderLoader.h"
#include "CoreTypes.h"
#include <fstream>
#include <sstream>
#include <filesystem>

// Default shader directory - can be changed at runtime
std::string FShaderLoader::ShaderDirectory = "Shaders";

std::string FShaderLoader::LoadShaderFromFile(const std::string& ShaderName)
{
    // Build full path: ShaderDirectory/ShaderName.usf
    std::string filePath = ShaderDirectory + "/" + ShaderName + ".usf";
    
    FLog::Log(ELogLevel::Info, "Loading shader: " + filePath);
    
    // Try to open the file
    std::ifstream file(filePath);
    if (!file.is_open())
    {
        // Try alternate paths (relative to executable)
        std::string alternatePaths[] = {
            "../Shaders/" + ShaderName + ".usf",
            "../../Shaders/" + ShaderName + ".usf",
            "../../../Shaders/" + ShaderName + ".usf"
        };
        
        for (const auto& altPath : alternatePaths)
        {
            file.open(altPath);
            if (file.is_open())
            {
                FLog::Log(ELogLevel::Info, "Found shader at alternate path: " + altPath);
                break;
            }
        }
        
        if (!file.is_open())
        {
            FLog::Log(ELogLevel::Error, "Failed to open shader file: " + filePath);
            return "";
        }
    }
    
    // Read entire file into string
    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();
    
    std::string source = buffer.str();
    FLog::Log(ELogLevel::Info, "Loaded shader '" + ShaderName + "' (" + 
              std::to_string(source.size()) + " bytes)");
    
    return source;
}

void FShaderLoader::SetShaderDirectory(const std::string& Path)
{
    ShaderDirectory = Path;
    FLog::Log(ELogLevel::Info, "Shader directory set to: " + ShaderDirectory);
}

const std::string& FShaderLoader::GetShaderDirectory()
{
    return ShaderDirectory;
}

bool FShaderLoader::ShaderExists(const std::string& ShaderName)
{
    std::string filePath = ShaderDirectory + "/" + ShaderName + ".usf";
    
    std::ifstream file(filePath);
    if (file.is_open())
    {
        file.close();
        return true;
    }
    
    // Try alternate paths
    std::string alternatePaths[] = {
        "../Shaders/" + ShaderName + ".usf",
        "../../Shaders/" + ShaderName + ".usf",
        "../../../Shaders/" + ShaderName + ".usf"
    };
    
    for (const auto& altPath : alternatePaths)
    {
        file.open(altPath);
        if (file.is_open())
        {
            file.close();
            return true;
        }
    }
    
    return false;
}
