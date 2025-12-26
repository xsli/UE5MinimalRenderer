add_library(Renderer STATIC
    Renderer.cpp
    Renderer.h
    RenderStats.cpp
    RenderStats.h
    Camera.cpp
    Camera.h
)

# Organize files in Visual Studio filters
source_group("Header Files" FILES 
    Renderer.h
    RenderStats.h
    Camera.h
)

source_group("Source Files" FILES 
    Renderer.cpp
    RenderStats.cpp
    Camera.cpp
)

target_include_directories(Renderer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(Renderer PUBLIC RHI Core TaskGraph)
