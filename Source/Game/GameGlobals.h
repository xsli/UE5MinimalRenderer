#pragma once

// Forward declarations
class FCamera;
class FLightScene;

// Global camera pointer for primitive proxy creation
// TODO: This is a temporary workaround that should be removed.
// In a production engine, the camera should be passed via:
//   1. CreateSceneProxy() parameters, OR
//   2. A scene context object, OR
//   3. Stored in FRenderScene with a getter method
extern FCamera* g_Camera;

// Global light scene pointer for lit primitive proxy creation
// TODO: This is a temporary workaround similar to g_Camera.
// In a production engine, the light scene should be passed via
// the renderer context or scene management system.
extern FLightScene* g_LightScene;
