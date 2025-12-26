add_library(TaskGraph STATIC
    TaskGraph.cpp
    TaskGraph.h
    RenderCommands.cpp
    RenderCommands.h
)

# Organize files in Visual Studio filters
source_group("Header Files" FILES 
    TaskGraph.h
    RenderCommands.h
)

source_group("Source Files" FILES 
    TaskGraph.cpp
    RenderCommands.cpp
)

target_include_directories(TaskGraph PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(TaskGraph PUBLIC Core)
