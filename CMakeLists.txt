cmake_minimum_required(VERSION 3.15)
project(UE5MinimalRenderer VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Source files
set(GAME_SOURCES
    Source/Game/Game.cpp
    Source/Game/Game.h
)

set(RENDERER_SOURCES
    Source/Renderer/Renderer.cpp
    Source/Renderer/Renderer.h
)

set(RHI_SOURCES
    Source/RHI/RHI.cpp
    Source/RHI/RHI.h
    Source/RHI/DX12/DX12RHI.cpp
    Source/RHI/DX12/DX12RHI.h
)

set(MAIN_SOURCES
    Source/Main.cpp
)

# Create executable
add_executable(${PROJECT_NAME}
    ${GAME_SOURCES}
    ${RENDERER_SOURCES}
    ${RHI_SOURCES}
    ${MAIN_SOURCES}
)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/Source
)

# Enable threading support
find_package(Threads REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE Threads::Threads)

# Platform-specific settings
if(WIN32)
    # Windows-specific settings
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        PLATFORM_WINDOWS=1
    )
elseif(UNIX AND NOT APPLE)
    # Linux-specific settings
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        PLATFORM_LINUX=1
    )
elseif(APPLE)
    # macOS-specific settings
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        PLATFORM_MAC=1
    )
endif()

# Compiler warnings
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -pedantic)
endif()

# Print configuration info
message(STATUS "")
message(STATUS "========================================")
message(STATUS "UE5 Minimal Renderer Configuration")
message(STATUS "========================================")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Output directory: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
message(STATUS "========================================")
message(STATUS "")
