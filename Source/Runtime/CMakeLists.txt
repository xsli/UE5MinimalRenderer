# UE5MinimalRenderer - Single executable with all source files
# (Excluding unit tests which remain as a separate target)

add_executable(UE5MinimalRenderer WIN32
    Main.cpp
    
    # Core
    ../Core/CoreTypes.cpp
    ../Core/CoreTypes.h
    
    # TaskGraph
    ../TaskGraph/TaskGraph.cpp
    ../TaskGraph/TaskGraph.h
    ../TaskGraph/RenderCommands.cpp
    ../TaskGraph/RenderCommands.h
    
    # RHI
    ../RHI/RHI.cpp
    ../RHI/RHI.h
    
    # RHI_DX12
    ../RHI_DX12/DX12RHI.cpp
    ../RHI_DX12/DX12RHI.h
    
    # Renderer
    ../Renderer/Renderer.cpp
    ../Renderer/Renderer.h
    ../Renderer/RenderStats.cpp
    ../Renderer/RenderStats.h
    ../Renderer/Camera.cpp
    ../Renderer/Camera.h
    ../Renderer/RTPool.cpp
    ../Renderer/RTPool.h
    ../Renderer/ShadowMapping.cpp
    ../Renderer/ShadowMapping.h
    
    # Lighting
    ../Lighting/Light.cpp
    ../Lighting/Light.h
    ../Lighting/LightingConstants.h
    ../Lighting/LightVisualization.cpp
    ../Lighting/LightVisualization.h
    
    # Scene
    ../Scene/Scene.cpp
    ../Scene/Scene.h
    ../Scene/ScenePrimitive.cpp
    ../Scene/ScenePrimitive.h
    ../Scene/UnlitSceneProxy.cpp
    ../Scene/UnlitSceneProxy.h
    ../Scene/LitSceneProxy.cpp
    ../Scene/LitSceneProxy.h
    
    # Game
    ../Game/Game.cpp
    ../Game/Game.h
    ../Game/GameGlobals.h
)

# Include directories
target_include_directories(UE5MinimalRenderer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../Core
    ${CMAKE_CURRENT_SOURCE_DIR}/../TaskGraph
    ${CMAKE_CURRENT_SOURCE_DIR}/../RHI
    ${CMAKE_CURRENT_SOURCE_DIR}/../RHI_DX12
    ${CMAKE_CURRENT_SOURCE_DIR}/../Renderer
    ${CMAKE_CURRENT_SOURCE_DIR}/../Lighting
    ${CMAKE_CURRENT_SOURCE_DIR}/../Scene
    ${CMAKE_CURRENT_SOURCE_DIR}/../Game
)

# Link DirectX 12 libraries
target_link_libraries(UE5MinimalRenderer PRIVATE
    d3d12.lib
    dxgi.lib
    d3dcompiler.lib
)

# Organize files in Visual Studio filters
source_group("Runtime" FILES Main.cpp)
source_group("Core" FILES ../Core/CoreTypes.cpp ../Core/CoreTypes.h)
source_group("TaskGraph" FILES 
    ../TaskGraph/TaskGraph.cpp ../TaskGraph/TaskGraph.h
    ../TaskGraph/RenderCommands.cpp ../TaskGraph/RenderCommands.h)
source_group("RHI" FILES ../RHI/RHI.cpp ../RHI/RHI.h)
source_group("RHI_DX12" FILES ../RHI_DX12/DX12RHI.cpp ../RHI_DX12/DX12RHI.h)
source_group("Renderer" FILES 
    ../Renderer/Renderer.cpp ../Renderer/Renderer.h
    ../Renderer/RenderStats.cpp ../Renderer/RenderStats.h
    ../Renderer/Camera.cpp ../Renderer/Camera.h
    ../Renderer/RTPool.cpp ../Renderer/RTPool.h
    ../Renderer/ShadowMapping.cpp ../Renderer/ShadowMapping.h)
source_group("Lighting" FILES 
    ../Lighting/Light.cpp ../Lighting/Light.h
    ../Lighting/LightingConstants.h
    ../Lighting/LightVisualization.cpp ../Lighting/LightVisualization.h)
source_group("Scene" FILES 
    ../Scene/Scene.cpp ../Scene/Scene.h
    ../Scene/ScenePrimitive.cpp ../Scene/ScenePrimitive.h
    ../Scene/UnlitSceneProxy.cpp ../Scene/UnlitSceneProxy.h
    ../Scene/LitSceneProxy.cpp ../Scene/LitSceneProxy.h)
source_group("Game" FILES 
    ../Game/Game.cpp ../Game/Game.h
    ../Game/GameGlobals.h)

# Set as startup project
set_property(DIRECTORY ${CMAKE_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT UE5MinimalRenderer)
