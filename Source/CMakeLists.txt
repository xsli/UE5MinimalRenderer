# Core modules (now compiled into Runtime executable)
# Keep subdirectories for include paths and organization
# The actual compilation is handled by Runtime/CMakeLists.txt

# Core library is still needed for unit tests
add_subdirectory(Core)

# Add Runtime which compiles everything else into a single executable
add_subdirectory(Runtime)
