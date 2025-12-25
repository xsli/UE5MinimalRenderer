add_library(RHI INTERFACE)

target_include_directories(RHI INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(RHI INTERFACE Core)
