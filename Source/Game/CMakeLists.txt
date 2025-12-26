add_library(Game STATIC
    Game.cpp
    Scene.cpp
    Primitive.cpp
    PrimitiveSceneProxy.cpp
)

target_include_directories(Game PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(Game PUBLIC Renderer RHI Core)
