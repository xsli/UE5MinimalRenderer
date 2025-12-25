add_library(Renderer STATIC
    Renderer.cpp
    RenderStats.cpp
)

target_include_directories(Renderer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(Renderer PUBLIC RHI Core)
