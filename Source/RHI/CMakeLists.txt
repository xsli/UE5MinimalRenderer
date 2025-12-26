add_library(RHI STATIC
    RHI.cpp
    RHI.h
)

# Organize files in Visual Studio filters
source_group("Header Files" FILES 
    RHI.h
)

source_group("Source Files" FILES 
    RHI.cpp
)

target_include_directories(RHI PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(RHI PUBLIC Core)
