# Scene module - unified scene and primitive management
add_library(Scene STATIC
    Scene.cpp
    Scene.h
    ScenePrimitive.cpp
    ScenePrimitive.h
    UnlitSceneProxy.cpp
    UnlitSceneProxy.h
)

source_group("Header Files" FILES 
    Scene.h
    ScenePrimitive.h
    UnlitSceneProxy.h
)

source_group("Source Files" FILES 
    Scene.cpp
    ScenePrimitive.cpp
    UnlitSceneProxy.cpp
)

target_include_directories(Scene PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(Scene PUBLIC Core RHI Renderer Lighting)
