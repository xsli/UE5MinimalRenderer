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

# Find DirectXMath on non-Windows platforms (for testing)
if(NOT WIN32)
    # Try to find DirectXMath in common locations
    find_path(DIRECTXMATH_INCLUDE_DIR DirectXMath.h
        PATHS
            /tmp/DirectXMath/Inc
            /usr/include
            /usr/local/include
        PATH_SUFFIXES directxmath
    )
    
    if(DIRECTXMATH_INCLUDE_DIR)
        message(STATUS "Found DirectXMath at: ${DIRECTXMATH_INCLUDE_DIR}")
        target_include_directories(Core PUBLIC ${DIRECTXMATH_INCLUDE_DIR})
    endif()
endif()
