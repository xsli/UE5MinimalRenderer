add_library(Game STATIC
    Game.cpp
    Game.h
    GameGlobals.h
)

# Note: Primitive and Scene have been moved to the Scene module

# Organize files in Visual Studio filters
source_group("Header Files" FILES 
    Game.h
    GameGlobals.h
)

source_group("Source Files" FILES 
    Game.cpp
)

target_include_directories(Game PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(Game PUBLIC Renderer RHI Core TaskGraph Lighting Scene)
