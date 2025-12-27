#pragma once

/**
 * RenderDoc Integration
 * 
 * This module provides optional RenderDoc frame capture support.
 * RenderDoc is a free frame debugger for DirectX 12, Vulkan, OpenGL, and other graphics APIs.
 * 
 * To use RenderDoc with this application:
 * 
 * METHOD 1: Launch through RenderDoc (Recommended)
 * 1. Open RenderDoc
 * 2. Go to File > Launch Application
 * 3. Set "Executable Path" to UE5MinimalRenderer.exe
 * 4. Set "Working Directory" to the build directory
 * 5. Click "Launch"
 * 6. Press F12 (or PrintScreen) in the app to capture a frame
 * 7. Click on the captured frame in RenderDoc to analyze
 * 
 * METHOD 2: Inject into running application
 * 1. Launch UE5MinimalRenderer.exe normally
 * 2. Open RenderDoc
 * 3. Go to File > Inject into Process
 * 4. Select UE5MinimalRenderer.exe from the process list
 * 5. Click "Inject"
 * 6. Press F12 to capture frames
 * 
 * DEBUGGING SHADOW MAPS:
 * 1. Capture a frame with F12
 * 2. In the Event Browser, look for "BeginShadowPass" events
 * 3. Click on any draw call during the shadow pass
 * 4. Go to Texture Viewer and select the depth attachment
 * 5. You should see the shadow map content (white = far, black = near)
 * 
 * If shadow map is empty (all white/cleared):
 * - Check that primitives have bCastShadow = true
 * - Check that the light view-projection matrix is correct
 * - Check that the shadow PSO is correctly configured
 * - Verify viewport is set correctly for the shadow map size
 * 
 * COMMON ISSUES:
 * - Shadow map shows all white: Objects are outside the light frustum
 * - Shadow map shows all black: Near plane is too close, depth buffer issues
 * - Shadow not visible in final image: Shader not sampling shadow map correctly
 */

#include <Windows.h>
#include <string>

// RenderDoc API types (simplified - full API header available from RenderDoc repository)
typedef void* RENDERDOC_API_1_0_0;

/**
 * FRenderDocCapture - RenderDoc frame capture helper
 * 
 * This class provides a simple interface to interact with RenderDoc
 * when the application is launched through RenderDoc or injected.
 */
class FRenderDocCapture
{
public:
    FRenderDocCapture();
    ~FRenderDocCapture();
    
    /**
     * Check if RenderDoc is attached to this process
     */
    bool IsRenderDocAttached() const;
    
    /**
     * Trigger a frame capture (same as pressing F12 in RenderDoc)
     * Only works if RenderDoc is attached
     */
    void TriggerCapture();
    
    /**
     * Get instructions for using RenderDoc
     */
    static std::string GetUsageInstructions();
    
private:
    HMODULE RenderDocModule;
    RENDERDOC_API_1_0_0* RenderDocAPI;
};

// Global instance for easy access
extern FRenderDocCapture g_RenderDocCapture;

/**
 * Helper macro to log RenderDoc status
 */
#define LOG_RENDERDOC_STATUS() \
    if (g_RenderDocCapture.IsRenderDocAttached()) { \
        FLog::Log(ELogLevel::Info, "RenderDoc is attached - Press F12 to capture frames"); \
    } else { \
        FLog::Log(ELogLevel::Info, "RenderDoc not detected - Launch through RenderDoc for graphics debugging"); \
    }
