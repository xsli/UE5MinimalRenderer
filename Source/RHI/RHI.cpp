#include "DX12/DX12RHI.h"
#include "RHI.h"

// Factory function to create the appropriate RHI implementation
std::unique_ptr<IRHI> CreateRHI()
{
    // For now, we default to DX12
    return std::make_unique<DX12RHI>();
}
