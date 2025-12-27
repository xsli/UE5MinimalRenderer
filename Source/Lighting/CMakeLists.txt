# Lighting module - light types, materials, and lit rendering
add_library(Lighting STATIC
    Light.cpp
    Light.h
    LightingConstants.h
    LightVisualization.cpp
    LightVisualization.h
    LitPrimitive.cpp
    LitPrimitive.h
    LitSceneProxy.cpp
    LitSceneProxy.h
)

target_include_directories(Lighting PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(Lighting PUBLIC Core RHI Renderer Game)
