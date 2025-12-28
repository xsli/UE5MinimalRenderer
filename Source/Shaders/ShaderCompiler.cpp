#include "ShaderCompiler.h"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>
#include <regex>

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
    ShaderDirectory = Directory;
    // Normalize path separators
    std::replace(ShaderDirectory.begin(), ShaderDirectory.end(), '\\', '/');
    // Remove trailing slash if present
    if (!ShaderDirectory.empty() && ShaderDirectory.back() == '/')
    {
        ShaderDirectory.pop_back();
    }
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
