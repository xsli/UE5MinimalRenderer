add_executable(UE5MinimalRenderer WIN32
    Main.cpp
)

target_link_libraries(UE5MinimalRenderer PRIVATE
    Game
    Renderer
    RHI_DX12
    RHI
    Core
    TaskGraph
)

# Set as startup project
set_property(DIRECTORY ${CMAKE_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT UE5MinimalRenderer)
