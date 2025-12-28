#include "ShaderCompiler.h"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>
#include <regex>
#include <Windows.h>

// Helper function to get the executable directory
static std::string GetExecutableDirectory()
{
    char buffer[MAX_PATH];
    DWORD result = GetModuleFileNameA(NULL, buffer, MAX_PATH);
    
    // Check for errors
    if (result == 0 || result == MAX_PATH)
    {
        FLog::Log(ELogLevel::Warning, "GetModuleFileNameA failed, using current directory");
        return ".";
    }
    
    std::string path(buffer);
    
    // Find last backslash or forward slash
    size_t lastSlash = path.find_last_of("\\/");
    if (lastSlash != std::string::npos)
    {
        path = path.substr(0, lastSlash);
    }
    
    // Normalize to forward slashes
    std::replace(path.begin(), path.end(), '\\', '/');
    
    return path;
}

// Helper function to resolve shader directory relative to executable
static std::string ResolveShaderDirectory(const std::string& RelativePath)
{
    std::string exeDir = GetExecutableDirectory();
    
    // If the path is absolute, use it directly
    if (RelativePath.length() > 1 && (RelativePath[1] == ':' || RelativePath[0] == '/'))
    {
        return RelativePath;
    }
    
    // For relative paths, first try relative to executable
    std::string fullPath = exeDir + "/" + RelativePath;
    
    // Check if the directory exists
    if (std::filesystem::exists(fullPath) && std::filesystem::is_directory(fullPath))
    {
        FLog::Log(ELogLevel::Info, "Found shader directory relative to executable: " + fullPath);
        return fullPath;
    }
    
    // Try going up directories from the executable to find Source/Shaders
    // Build structure: build/Source/Runtime/Release/UE5MinimalRenderer.exe
    // We need to go up to find Source/Shaders
    std::string currentDir = exeDir;
    for (int i = 0; i < 5; ++i)  // Try up to 5 parent directories
    {
        // Try Source/Shaders relative to current directory
        std::string testPath = currentDir + "/Source/Shaders";
        if (std::filesystem::exists(testPath) && std::filesystem::is_directory(testPath))
        {
            FLog::Log(ELogLevel::Info, "Found shader directory by searching parents: " + testPath);
            return testPath;
        }
        
        // Go up one directory
        size_t lastSlash = currentDir.find_last_of("/");
        if (lastSlash == std::string::npos || lastSlash == 0)
        {
            break;
        }
        currentDir = currentDir.substr(0, lastSlash);
    }
    
    // Fall back to original path (might be relative to current working directory)
    FLog::Log(ELogLevel::Warning, "Could not find shader directory, using fallback: " + RelativePath);
    return RelativePath;
}

// Shader compile flags
static UINT GetShaderCompileFlags()
{
    UINT flags = 0;
#ifdef _DEBUG
    flags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
    flags = D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif
    return flags;
}

// FShaderCompiler implementation
FShaderCompiler::FShaderCompiler()
{
    // Default shader directory (can be overridden)
    ShaderDirectory = "./Shaders";
}

FShaderCompiler::~FShaderCompiler()
{
}

void FShaderCompiler::SetShaderDirectory(const std::string& Directory)
{
    // Resolve the shader directory using executable path
    ShaderDirectory = ResolveShaderDirectory(Directory);
    
    // Normalize path separators
    std::replace(ShaderDirectory.begin(), ShaderDirectory.end(), '\\', '/');
    // Remove trailing slash if present
    if (!ShaderDirectory.empty() && ShaderDirectory.back() == '/')
    {
        ShaderDirectory.pop_back();
    }
    
    FLog::Log(ELogLevel::Info, "Shader directory set to: " + ShaderDirectory);
}

const char* FShaderCompiler::GetShaderTarget(EShaderType ShaderType) const
{
    switch (ShaderType)
    {
        case EShaderType::Vertex:   return "vs_5_0";
        case EShaderType::Pixel:    return "ps_5_0";
        case EShaderType::Compute:  return "cs_5_0";
        case EShaderType::Geometry: return "gs_5_0";
        case EShaderType::Hull:     return "hs_5_0";
        case EShaderType::Domain:   return "ds_5_0";
        default:                    return "vs_5_0";
    }
}

std::string FShaderCompiler::LoadShaderFile(const std::string& FilePath)
{
    std::string fullPath = ShaderDirectory + "/" + FilePath;
    
    FLog::Log(ELogLevel::Info, "Loading shader file: " + fullPath);
    
    std::ifstream file(fullPath);
    if (!file.is_open())
    {
        FLog::Log(ELogLevel::Error, "Failed to open shader file: " + fullPath);
        return "";
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

std::string FShaderCompiler::ProcessIncludes(const std::string& Source, const std::string& CurrentDir)
{
    std::string result;
    std::istringstream stream(Source);
    std::string line;
    
    // Regex to match #include "filename"
    std::regex includeRegex(R"(#include\s*[\"<]([^\"<>]+)[\">])");
    
    while (std::getline(stream, line))
    {
        std::smatch match;
        if (std::regex_search(line, match, includeRegex))
        {
            std::string includePath = match[1].str();
            
            // Load the included file
            std::string includeContent = LoadShaderFile(includePath);
            
            if (!includeContent.empty())
            {
                // Recursively process includes
                std::string includeDir = includePath;
                size_t lastSlash = includeDir.find_last_of("/\\");
                if (lastSlash != std::string::npos)
                {
                    includeDir = includeDir.substr(0, lastSlash);
                }
                else
                {
                    includeDir = CurrentDir;
                }
                
                result += ProcessIncludes(includeContent, includeDir);
                result += "\n";
            }
            else
            {
                // Keep the include directive if file not found (let compiler handle it)
                result += line + "\n";
            }
        }
        else
        {
            result += line + "\n";
        }
    }
    
    return result;
}

FShaderCompileResult FShaderCompiler::CompileFromFile(const std::string& FilePath,
                                                       const std::string& EntryPoint,
                                                       EShaderType ShaderType)
{
    FShaderCompileResult result;
    
    // Load shader source with include processing
    std::string sourceCode = LoadShaderFile(FilePath);
    if (sourceCode.empty())
    {
        result.ErrorMessage = "Failed to load shader file: " + FilePath;
        result.bSuccess = false;
        return result;
    }
    
    // Process includes
    sourceCode = ProcessIncludes(sourceCode, "");
    
    return CompileFromSource(sourceCode, FilePath, EntryPoint, ShaderType);
}

FShaderCompileResult FShaderCompiler::CompileFromSource(const std::string& SourceCode,
                                                         const std::string& SourceName,
                                                         const std::string& EntryPoint,
                                                         EShaderType ShaderType)
{
    FShaderCompileResult result;
    
    const char* target = GetShaderTarget(ShaderType);
    UINT flags = GetShaderCompileFlags();
    
    FLog::Log(ELogLevel::Info, "Compiling shader: " + SourceName + " EntryPoint: " + EntryPoint + " Target: " + target);
    
    // Build defines array
    std::vector<D3D_SHADER_MACRO> macros;
    for (const auto& define : Defines)
    {
        D3D_SHADER_MACRO macro = {};
        macro.Name = define.first.c_str();
        macro.Definition = define.second.c_str();
        macros.push_back(macro);
    }
    // Null terminator
    macros.push_back({nullptr, nullptr});
    
    ComPtr<ID3DBlob> shaderBlob;
    ComPtr<ID3DBlob> errorBlob;
    
    HRESULT hr = D3DCompile(
        SourceCode.c_str(),
        SourceCode.length(),
        SourceName.c_str(),
        macros.data(),
        nullptr,  // No include handler (we process includes ourselves)
        EntryPoint.c_str(),
        target,
        flags,
        0,
        &shaderBlob,
        &errorBlob
    );
    
    if (FAILED(hr))
    {
        if (errorBlob)
        {
            result.ErrorMessage = static_cast<const char*>(errorBlob->GetBufferPointer());
        }
        else
        {
            result.ErrorMessage = "Unknown shader compilation error";
        }
        result.bSuccess = false;
        FLog::Log(ELogLevel::Error, "Shader compilation failed: " + result.ErrorMessage);
        return result;
    }
    
    result.Bytecode = FShaderBytecode(shaderBlob);
    result.bSuccess = true;
    FLog::Log(ELogLevel::Info, "Shader compiled successfully: " + SourceName);
    
    return result;
}

void FShaderCompiler::AddDefine(const std::string& Name, const std::string& Value)
{
    Defines.push_back({Name, Value});
}

void FShaderCompiler::ClearDefines()
{
    Defines.clear();
}

// FShaderManager implementation
FShaderManager& FShaderManager::Get()
{
    static FShaderManager instance;
    return instance;
}

void FShaderManager::Initialize(const std::string& ShaderDirectory)
{
    if (bInitialized)
    {
        FLog::Log(ELogLevel::Warning, "FShaderManager already initialized");
        return;
    }
    
    Compiler.SetShaderDirectory(ShaderDirectory);
    bInitialized = true;
    FLog::Log(ELogLevel::Info, "FShaderManager initialized with directory: " + ShaderDirectory);
}

void FShaderManager::Shutdown()
{
    ClearCache();
    bInitialized = false;
    FLog::Log(ELogLevel::Info, "FShaderManager shutdown");
}

FShaderBytecode FShaderManager::GetShader(const std::string& FilePath,
                                           const std::string& EntryPoint,
                                           EShaderType ShaderType)
{
    // Create cache key
    FShaderKey key = {FilePath, EntryPoint, ShaderType};
    
    // Check cache
    auto it = ShaderCache.find(key);
    if (it != ShaderCache.end())
    {
        FLog::Log(ELogLevel::Info, "Shader cache hit: " + FilePath + ":" + EntryPoint);
        return it->second;
    }
    
    // Compile shader
    FShaderCompileResult result = Compiler.CompileFromFile(FilePath, EntryPoint, ShaderType);
    
    if (result.bSuccess)
    {
        // Cache the result
        ShaderCache[key] = result.Bytecode;
        return result.Bytecode;
    }
    
    // Return invalid bytecode on failure
    return FShaderBytecode();
}

FShaderBytecode FShaderManager::CompileShaderFromSource(const std::string& SourceCode,
                                                         const std::string& SourceName,
                                                         const std::string& EntryPoint,
                                                         EShaderType ShaderType)
{
    FShaderCompileResult result = Compiler.CompileFromSource(SourceCode, SourceName, EntryPoint, ShaderType);
    return result.bSuccess ? result.Bytecode : FShaderBytecode();
}

void FShaderManager::ClearCache()
{
    ShaderCache.clear();
    FLog::Log(ELogLevel::Info, "Shader cache cleared");
}
