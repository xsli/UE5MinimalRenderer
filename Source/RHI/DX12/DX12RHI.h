#pragma once

#include "../RHI.h"

// DirectX 12 implementation of RHI
class DX12RHI : public IRHI
{
public:
    DX12RHI();
    virtual ~DX12RHI();

    // IRHI interface implementation
    virtual bool Initialize() override;
    virtual void Shutdown() override;
    virtual void BeginFrame() override;
    virtual void EndFrame() override;
    virtual void Present() override;
    virtual void ExecuteCommands() override;
    virtual const char* GetAPIName() const override;

private:
    bool bIsInitialized;
    uint32_t FrameIndex;
};
