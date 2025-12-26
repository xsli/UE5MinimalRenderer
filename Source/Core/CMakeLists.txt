add_library(Core STATIC
    CoreTypes.cpp
    CoreTypes.h
)

# Organize files in Visual Studio filters
source_group("Header Files" FILES 
    CoreTypes.h
)

source_group("Source Files" FILES 
    CoreTypes.cpp
)

target_include_directories(Core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
