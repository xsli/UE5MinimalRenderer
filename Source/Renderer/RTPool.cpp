#include "RTPool.h"
#include <algorithm>

// Static instance
FRTPool* FRTPool::GInstance = nullptr;

FRTPool::FRTPool(FRHI* InRHI)
    : RHI(InRHI)
    , CurrentFrameNumber(0)
    , MaxCapacity(DEFAULT_MAX_CAPACITY)
    , CleanupTimeoutFrames(DEFAULT_CLEANUP_TIMEOUT_FRAMES)
{
    FLog::Log(ELogLevel::Info, "FRTPool: Initialized with max capacity " + std::to_string(MaxCapacity));
}

FRTPool::~FRTPool()
{
    ClearAll();
}

FRTPool* FRTPool::Get()
{
    return GInstance;
}

void FRTPool::Initialize(FRHI* InRHI)
{
    if (!GInstance && InRHI)
    {
        GInstance = new FRTPool(InRHI);
    }
}

void FRTPool::Shutdown()
{
    if (GInstance)
    {
        delete GInstance;
        GInstance = nullptr;
    }
}

void FRTPool::BeginFrame(uint64 FrameNumber)
{
    CurrentFrameNumber = FrameNumber;
    
    // Reset per-frame stats
    Stats.CreatedThisFrame = 0;
    Stats.ReusedThisFrame = 0;
    
    // Mark all RTs as not in use for this frame
    // (They stay marked until Release() is called)
}

void FRTPool::EndFrame()
{
    // Perform cleanup of stale RTs
    Cleanup(false);
    
    // Update statistics
    UpdateStats();
}

FPooledRT* FRTPool::Fetch(const FRTDescriptor& Descriptor)
{
    // Look for an available RT with matching descriptor
    auto it = Pool.find(Descriptor);
    if (it != Pool.end())
    {
        for (FPooledRT* RT : it->second)
        {
            if (!RT->bInUse)
            {
                // Found an available RT - reuse it
                RT->bInUse = true;
                RT->LastUsedFrame = CurrentFrameNumber;
                Stats.ReusedThisFrame++;
                Stats.ActiveRTs++;
                Stats.IdleRTs--;
                
                FLog::Log(ELogLevel::Info, "FRTPool: Reused RT " + 
                    std::to_string(Descriptor.Width) + "x" + std::to_string(Descriptor.Height));
                return RT;
            }
        }
    }
    
    // No available RT found - create a new one if under capacity
    if (AllRTs.size() >= MaxCapacity)
    {
        FLog::Log(ELogLevel::Warning, "FRTPool: Max capacity reached, cannot allocate new RT");
        return nullptr;
    }
    
    FPooledRT* NewRT = CreateRT(Descriptor);
    if (NewRT)
    {
        NewRT->bInUse = true;
        NewRT->LastUsedFrame = CurrentFrameNumber;
        
        // Add to pool
        Pool[Descriptor].push_back(NewRT);
        AllRTs.push_back(NewRT);
        
        Stats.CreatedThisFrame++;
        Stats.ActiveRTs++;
        Stats.TotalPooledRTs++;
        
        FLog::Log(ELogLevel::Info, "FRTPool: Created new RT " + 
            std::to_string(Descriptor.Width) + "x" + std::to_string(Descriptor.Height) +
            " (total: " + std::to_string(AllRTs.size()) + ")");
    }
    
    return NewRT;
}

void FRTPool::Release(FPooledRT* RT)
{
    if (RT && RT->bInUse)
    {
        RT->bInUse = false;
        RT->LastUsedFrame = CurrentFrameNumber;
        
        Stats.ActiveRTs--;
        Stats.IdleRTs++;
    }
}

void FRTPool::Cleanup(bool bForce)
{
    if (bForce)
    {
        ClearAll();
        return;
    }
    
    // Remove RTs that haven't been used for CleanupTimeoutFrames
    uint64 timeoutThreshold = 0;
    if (CurrentFrameNumber > CleanupTimeoutFrames)
    {
        timeoutThreshold = CurrentFrameNumber - CleanupTimeoutFrames;
    }
    
    std::vector<FPooledRT*> ToRemove;
    
    for (FPooledRT* RT : AllRTs)
    {
        // Only cleanup idle RTs that are past the timeout
        if (!RT->bInUse && RT->LastUsedFrame < timeoutThreshold)
        {
            ToRemove.push_back(RT);
        }
    }
    
    for (FPooledRT* RT : ToRemove)
    {
        // Remove from pool map
        auto it = Pool.find(RT->Descriptor);
        if (it != Pool.end())
        {
            auto& vec = it->second;
            vec.erase(std::remove(vec.begin(), vec.end(), RT), vec.end());
            if (vec.empty())
            {
                Pool.erase(it);
            }
        }
        
        // Remove from all RTs list
        AllRTs.erase(std::remove(AllRTs.begin(), AllRTs.end(), RT), AllRTs.end());
        
        // Destroy the RT
        DestroyRT(RT);
        
        Stats.TotalPooledRTs--;
        Stats.IdleRTs--;
    }
    
    if (!ToRemove.empty())
    {
        FLog::Log(ELogLevel::Info, "FRTPool: Cleaned up " + std::to_string(ToRemove.size()) + 
            " stale RTs (remaining: " + std::to_string(AllRTs.size()) + ")");
    }
}

void FRTPool::ClearAll()
{
    for (FPooledRT* RT : AllRTs)
    {
        DestroyRT(RT);
    }
    AllRTs.clear();
    Pool.clear();
    
    Stats = FRTPoolStats();
    
    FLog::Log(ELogLevel::Info, "FRTPool: Cleared all pooled RTs");
}

FPooledRT* FRTPool::CreateRT(const FRTDescriptor& Descriptor)
{
    if (!RHI)
    {
        return nullptr;
    }
    
    // Create the RHI texture resource
    FRHITexture* Texture = RHI->CreateDepthTexture(
        Descriptor.Width,
        Descriptor.Height,
        Descriptor.Format,
        Descriptor.ArraySize
    );
    
    if (!Texture)
    {
        FLog::Log(ELogLevel::Error, "FRTPool: Failed to create texture " +
            std::to_string(Descriptor.Width) + "x" + std::to_string(Descriptor.Height));
        return nullptr;
    }
    
    FPooledRT* RT = new FPooledRT();
    RT->Texture = Texture;
    RT->Descriptor = Descriptor;
    RT->LastUsedFrame = CurrentFrameNumber;
    RT->bInUse = false;
    
    Stats.TotalMemoryBytes += EstimateMemoryUsage(Descriptor);
    
    return RT;
}

void FRTPool::DestroyRT(FPooledRT* RT)
{
    if (RT)
    {
        Stats.TotalMemoryBytes -= EstimateMemoryUsage(RT->Descriptor);
        
        if (RT->Texture)
        {
            delete RT->Texture;
        }
        delete RT;
    }
}

uint64 FRTPool::EstimateMemoryUsage(const FRTDescriptor& Descriptor) const
{
    uint32 bytesPerPixel = 4; // Default
    
    switch (Descriptor.Format)
    {
        case ERTFormat::R8G8B8A8_UNORM:
            bytesPerPixel = 4;
            break;
        case ERTFormat::R16G16B16A16_FLOAT:
            bytesPerPixel = 8;
            break;
        case ERTFormat::R32_FLOAT:
        case ERTFormat::D32_FLOAT:
            bytesPerPixel = 4;
            break;
        case ERTFormat::D16_UNORM:
            bytesPerPixel = 2;
            break;
        case ERTFormat::D24_UNORM_S8_UINT:
            bytesPerPixel = 4;
            break;
    }
    
    uint64 baseSize = static_cast<uint64>(Descriptor.Width) * 
                      static_cast<uint64>(Descriptor.Height) * 
                      bytesPerPixel *
                      Descriptor.ArraySize *
                      Descriptor.SampleCount;
    
    // Account for mip chain (roughly 1.33x for full mip chain)
    if (Descriptor.MipLevels > 1)
    {
        baseSize = static_cast<uint64>(baseSize * 1.33);
    }
    
    return baseSize;
}

void FRTPool::UpdateStats()
{
    uint32 activeCount = 0;
    uint32 idleCount = 0;
    
    for (FPooledRT* RT : AllRTs)
    {
        if (RT->bInUse)
        {
            activeCount++;
        }
        else
        {
            idleCount++;
        }
    }
    
    Stats.TotalPooledRTs = static_cast<uint32>(AllRTs.size());
    Stats.ActiveRTs = activeCount;
    Stats.IdleRTs = idleCount;
}

uint32 FRTPool::GetPooledCount() const
{
    return static_cast<uint32>(AllRTs.size());
}

uint32 FRTPool::GetActiveCount() const
{
    uint32 count = 0;
    for (FPooledRT* RT : AllRTs)
    {
        if (RT->bInUse)
        {
            count++;
        }
    }
    return count;
}
