add_library(Game STATIC
    Game.cpp
    Game.h
    Scene.cpp
    Scene.h
    Primitive.cpp
    Primitive.h
    PrimitiveSceneProxy.cpp
    PrimitiveSceneProxy.h
    GameGlobals.h
)

# Organize files in Visual Studio filters
source_group("Header Files" FILES 
    Game.h
    Scene.h
    Primitive.h
    PrimitiveSceneProxy.h
    GameGlobals.h
)

source_group("Source Files" FILES 
    Game.cpp
    Scene.cpp
    Primitive.cpp
    PrimitiveSceneProxy.cpp
)

target_include_directories(Game PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(Game PUBLIC Renderer RHI Core TaskGraph Lighting)
