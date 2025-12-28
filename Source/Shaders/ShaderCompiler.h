#pragma once

#include "../Core/CoreTypes.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <d3d12.h>
#include <d3dcompiler.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

// Forward declarations
class FShaderManager;

/**
 * EShaderType - Types of shaders supported
 */
enum class EShaderType
{
    Vertex,
    Pixel,
    Compute,
    Geometry,
    Hull,
    Domain
};

/**
 * FShaderBytecode - Container for compiled shader bytecode
 */
class FShaderBytecode
{
public:
    FShaderBytecode() = default;
    FShaderBytecode(ComPtr<ID3DBlob> InBytecode) : Bytecode(InBytecode) {}
    
    bool IsValid() const { return Bytecode != nullptr; }
    ID3DBlob* GetBlob() const { return Bytecode.Get(); }
    
    void* GetBufferPointer() const { return Bytecode ? Bytecode->GetBufferPointer() : nullptr; }
    SIZE_T GetBufferSize() const { return Bytecode ? Bytecode->GetBufferSize() : 0; }
    
    D3D12_SHADER_BYTECODE GetShaderBytecode() const
    {
        D3D12_SHADER_BYTECODE bc = {};
        if (Bytecode)
        {
            bc.pShaderBytecode = Bytecode->GetBufferPointer();
            bc.BytecodeLength = Bytecode->GetBufferSize();
        }
        return bc;
    }
    
private:
    ComPtr<ID3DBlob> Bytecode;
};

/**
 * FShaderCompileResult - Result of shader compilation
 */
struct FShaderCompileResult
{
    FShaderBytecode Bytecode;
    std::string ErrorMessage;
    bool bSuccess = false;
};

/**
 * FShaderCompiler - Compiles HLSL shaders from files or source strings
 * Similar to Unreal Engine's FShaderCompilerDefinitions and compilation pipeline
 */
class FShaderCompiler
{
public:
    FShaderCompiler();
    ~FShaderCompiler();
    
    /**
     * Set the base shader directory for file loading
     */
    void SetShaderDirectory(const std::string& Directory);
    
    /**
     * Compile shader from file
     * @param FilePath - Path to shader file (relative to shader directory)
     * @param EntryPoint - Shader entry point function name
     * @param ShaderType - Type of shader (vertex, pixel, etc.)
     * @return Compiled shader bytecode
     */
    FShaderCompileResult CompileFromFile(const std::string& FilePath, 
                                          const std::string& EntryPoint,
                                          EShaderType ShaderType);
    
    /**
     * Compile shader from source string
     * @param SourceCode - HLSL source code
     * @param SourceName - Name for error reporting
     * @param EntryPoint - Shader entry point function name
     * @param ShaderType - Type of shader (vertex, pixel, etc.)
     * @return Compiled shader bytecode
     */
    FShaderCompileResult CompileFromSource(const std::string& SourceCode,
                                            const std::string& SourceName,
                                            const std::string& EntryPoint,
                                            EShaderType ShaderType);
    
    /**
     * Add a macro definition for shader compilation
     */
    void AddDefine(const std::string& Name, const std::string& Value = "1");
    
    /**
     * Clear all macro definitions
     */
    void ClearDefines();
    
private:
    /**
     * Get the shader model string for a shader type
     */
    const char* GetShaderTarget(EShaderType ShaderType) const;
    
    /**
     * Load shader file content with include handling
     */
    std::string LoadShaderFile(const std::string& FilePath);
    
    /**
     * Process #include directives in shader source
     */
    std::string ProcessIncludes(const std::string& Source, const std::string& CurrentDir);
    
    std::string ShaderDirectory;
    std::vector<std::pair<std::string, std::string>> Defines;
};

/**
 * FShaderKey - Key for shader cache lookup
 */
struct FShaderKey
{
    std::string FilePath;
    std::string EntryPoint;
    EShaderType ShaderType;
    
    bool operator==(const FShaderKey& Other) const
    {
        return FilePath == Other.FilePath && 
               EntryPoint == Other.EntryPoint && 
               ShaderType == Other.ShaderType;
    }
};

// Hash function for FShaderKey
struct FShaderKeyHash
{
    size_t operator()(const FShaderKey& Key) const
    {
        size_t h1 = std::hash<std::string>()(Key.FilePath);
        size_t h2 = std::hash<std::string>()(Key.EntryPoint);
        size_t h3 = std::hash<int>()(static_cast<int>(Key.ShaderType));
        return h1 ^ (h2 << 1) ^ (h3 << 2);
    }
};

/**
 * FShaderManager - Manages shader compilation and caching
 * Similar to Unreal Engine's FShaderManager
 */
class FShaderManager
{
public:
    static FShaderManager& Get();
    
    /**
     * Initialize the shader manager
     * @param ShaderDirectory - Base directory for shader files
     */
    void Initialize(const std::string& ShaderDirectory);
    
    /**
     * Shutdown the shader manager
     */
    void Shutdown();
    
    /**
     * Get or compile a shader from file
     * Results are cached for subsequent calls
     */
    FShaderBytecode GetShader(const std::string& FilePath,
                              const std::string& EntryPoint,
                              EShaderType ShaderType);
    
    /**
     * Compile shader directly from source (not cached)
     */
    FShaderBytecode CompileShaderFromSource(const std::string& SourceCode,
                                            const std::string& SourceName,
                                            const std::string& EntryPoint,
                                            EShaderType ShaderType);
    
    /**
     * Clear the shader cache
     */
    void ClearCache();
    
    /**
     * Get the shader compiler for advanced usage
     */
    FShaderCompiler& GetCompiler() { return Compiler; }
    
private:
    FShaderManager() = default;
    ~FShaderManager() = default;
    
    FShaderCompiler Compiler;
    std::unordered_map<FShaderKey, FShaderBytecode, FShaderKeyHash> ShaderCache;
    bool bInitialized = false;
};
