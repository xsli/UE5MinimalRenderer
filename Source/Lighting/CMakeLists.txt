# Lighting module - light types, materials, and lighting constants
# Note: LitSceneProxy has been moved to the Scene module
add_library(Lighting STATIC
    Light.cpp
    Light.h
    LightingConstants.h
    LightVisualization.cpp
    LightVisualization.h
)

target_include_directories(Lighting PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(Lighting PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../Game)
target_include_directories(Lighting PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../Scene)
target_link_libraries(Lighting PUBLIC Core RHI Renderer)
