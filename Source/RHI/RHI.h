#pragma once

#include <memory>
#include <cstdint>

// Render Hardware Interface - Abstract layer for graphics API
class IRHI
{
public:
    virtual ~IRHI() = default;

    // Initialization and shutdown
    virtual bool Initialize() = 0;
    virtual void Shutdown() = 0;

    // Frame management
    virtual void BeginFrame() = 0;
    virtual void EndFrame() = 0;
    virtual void Present() = 0;

    // Command submission
    virtual void ExecuteCommands() = 0;

    // Query capabilities
    virtual const char* GetAPIName() const = 0;
};

// Factory function to create RHI instance
std::unique_ptr<IRHI> CreateRHI();
