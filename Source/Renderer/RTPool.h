#pragma once

#include "../Core/CoreTypes.h"
#include "../RHI/RHI.h"
#include <unordered_map>
#include <vector>
#include <functional>

/**
 * FRTDescriptor - Description for a render texture
 * Used as key for RT pool lookup
 */
struct FRTDescriptor
{
    uint32 Width;
    uint32 Height;
    ERTFormat Format;
    uint32 MipLevels;
    uint32 ArraySize;     // For texture arrays/cubemaps (6 for point light shadow)
    uint32 SampleCount;   // MSAA sample count

    FRTDescriptor()
        : Width(0)
        , Height(0)
        , Format(ERTFormat::R8G8B8A8_UNORM)
        , MipLevels(1)
        , ArraySize(1)
        , SampleCount(1)
    {
    }

    FRTDescriptor(uint32 InWidth, uint32 InHeight, ERTFormat InFormat,
                  uint32 InMips = 1, uint32 InArraySize = 1, uint32 InSamples = 1)
        : Width(InWidth)
        , Height(InHeight)
        , Format(InFormat)
        , MipLevels(InMips)
        , ArraySize(InArraySize)
        , SampleCount(InSamples)
    {
    }

    // Equality operator for hash map lookup
    bool operator==(const FRTDescriptor& Other) const
    {
        return Width == Other.Width &&
               Height == Other.Height &&
               Format == Other.Format &&
               MipLevels == Other.MipLevels &&
               ArraySize == Other.ArraySize &&
               SampleCount == Other.SampleCount;
    }
};

// Hash function for FRTDescriptor
struct FRTDescriptorHash
{
    size_t operator()(const FRTDescriptor& Desc) const
    {
        size_t hash = 0;
        hash ^= std::hash<uint32>()(Desc.Width) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
        hash ^= std::hash<uint32>()(Desc.Height) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
        hash ^= std::hash<int>()(static_cast<int>(Desc.Format)) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
        hash ^= std::hash<uint32>()(Desc.MipLevels) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
        hash ^= std::hash<uint32>()(Desc.ArraySize) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
        hash ^= std::hash<uint32>()(Desc.SampleCount) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
        return hash;
    }
};

/**
 * FPooledRT - A render texture managed by the RT pool
 */
struct FPooledRT
{
    FRHITexture* Texture;
    FRTDescriptor Descriptor;
    uint64 LastUsedFrame;      // Frame number when last used
    bool bInUse;               // Currently checked out

    FPooledRT()
        : Texture(nullptr)
        , LastUsedFrame(0)
        , bInUse(false)
    {
    }
};

/**
 * FRTPoolStats - Statistics for the RT pool
 */
struct FRTPoolStats
{
    uint32 TotalPooledRTs;     // Total RTs in pool
    uint32 ActiveRTs;          // Currently checked out
    uint32 IdleRTs;            // Available for reuse
    uint32 CreatedThisFrame;   // New allocations this frame
    uint32 ReusedThisFrame;    // Reused from pool this frame
    uint64 TotalMemoryBytes;   // Estimated VRAM usage

    FRTPoolStats()
        : TotalPooledRTs(0)
        , ActiveRTs(0)
        , IdleRTs(0)
        , CreatedThisFrame(0)
        , ReusedThisFrame(0)
        , TotalMemoryBytes(0)
    {
    }
};

/**
 * FRTPool - Render Texture Pool Manager
 * 
 * Manages pooled render textures for efficient VRAM usage.
 * Implements:
 *  - Allocation by descriptor (width, height, format, etc.)
 *  - Automatic reuse of matching textures
 *  - Per-frame lifecycle (mark/sweep)
 *  - Timeout-based cleanup (configurable frames/seconds)
 *  - Max capacity limit to prevent VRAM overflow
 */
class FRTPool
{
public:
    // Configuration
    static constexpr uint32 DEFAULT_MAX_CAPACITY = 64;
    static constexpr uint32 DEFAULT_CLEANUP_TIMEOUT_FRAMES = 60;
    static constexpr float DEFAULT_CLEANUP_TIMEOUT_SECONDS = 10.0f;

    FRTPool(FRHI* InRHI);
    ~FRTPool();

    // Singleton access (optional, for global access)
    static FRTPool* Get();
    static void Initialize(FRHI* InRHI);
    static void Shutdown();

    // Frame lifecycle
    void BeginFrame(uint64 FrameNumber);
    void EndFrame();

    // RT allocation/release
    FPooledRT* Fetch(const FRTDescriptor& Descriptor);
    void Release(FPooledRT* RT);

    // Cleanup
    void Cleanup(bool bForce = false);
    void ClearAll();

    // Configuration
    void SetMaxCapacity(uint32 Capacity) { MaxCapacity = Capacity; }
    uint32 GetMaxCapacity() const { return MaxCapacity; }
    void SetCleanupTimeoutFrames(uint32 Frames) { CleanupTimeoutFrames = Frames; }
    uint32 GetCleanupTimeoutFrames() const { return CleanupTimeoutFrames; }

    // Statistics
    const FRTPoolStats& GetStats() const { return Stats; }
    uint32 GetPooledCount() const;
    uint32 GetActiveCount() const;

private:
    FPooledRT* CreateRT(const FRTDescriptor& Descriptor);
    void DestroyRT(FPooledRT* RT);
    uint64 EstimateMemoryUsage(const FRTDescriptor& Descriptor) const;
    void UpdateStats();

    FRHI* RHI;
    uint64 CurrentFrameNumber;
    uint32 MaxCapacity;
    uint32 CleanupTimeoutFrames;

    // Pool storage: descriptor -> list of pooled RTs
    std::unordered_map<FRTDescriptor, std::vector<FPooledRT*>, FRTDescriptorHash> Pool;
    
    // All allocated RTs (for cleanup tracking)
    std::vector<FPooledRT*> AllRTs;

    FRTPoolStats Stats;
    static FRTPool* GInstance;
};
