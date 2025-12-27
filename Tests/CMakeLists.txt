# Unit tests for UE5MinimalRenderer
include(FetchContent)

# Download and configure GoogleTest
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

# Matrix transformation tests (Core module only)
add_executable(MatrixTests
    MatrixTests.cpp
)

target_include_directories(MatrixTests PRIVATE 
    ${CMAKE_SOURCE_DIR}/Source/Core
)

target_link_libraries(MatrixTests
    GTest::gtest_main
    Core
)

# Organize files in Visual Studio
source_group("Test Files" FILES MatrixTests.cpp)

include(GoogleTest)
gtest_discover_tests(MatrixTests)
