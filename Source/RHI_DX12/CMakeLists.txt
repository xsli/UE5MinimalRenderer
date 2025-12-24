add_library(RHI_DX12 STATIC
    DX12RHI.cpp
)

target_include_directories(RHI_DX12 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(RHI_DX12 PUBLIC RHI Core d3d12.lib dxgi.lib d3dcompiler.lib)
