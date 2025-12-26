#pragma once

// Forward declarations
class FCamera;

// Global camera pointer for primitive proxy creation
// TODO: This is a temporary workaround that should be removed.
// In a production engine, the camera should be passed via:
//   1. CreateSceneProxy() parameters, OR
//   2. A scene context object, OR
//   3. Stored in FRenderScene with a getter method
extern FCamera* g_Camera;
