cmake_minimum_required(VERSION 3.20)
project(UE5MinimalRenderer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Option to build tests
option(BUILD_TESTS "Build unit tests" ON)

# Add source directories
add_subdirectory(Source)

# Add tests if enabled
if(BUILD_TESTS)
    add_subdirectory(Tests)
endif()
