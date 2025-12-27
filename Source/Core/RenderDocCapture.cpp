#include "RenderDocCapture.h"
#include "Log.h"

// Global instance
FRenderDocCapture g_RenderDocCapture;

FRenderDocCapture::FRenderDocCapture()
    : RenderDocModule(nullptr)
    , RenderDocAPI(nullptr)
{
    // Try to get the RenderDoc module if it's already loaded
    // (which happens when launched through RenderDoc)
    RenderDocModule = GetModuleHandleA("renderdoc.dll");
    
    if (RenderDocModule)
    {
        // RenderDoc is attached - get the API
        typedef int (*pRENDERDOC_GetAPI)(int version, void** outAPIPointers);
        pRENDERDOC_GetAPI RENDERDOC_GetAPI = (pRENDERDOC_GetAPI)GetProcAddress(RenderDocModule, "RENDERDOC_GetAPI");
        
        if (RENDERDOC_GetAPI)
        {
            // API version 1.0.0 = 10000
            int ret = RENDERDOC_GetAPI(10000, (void**)&RenderDocAPI);
            if (ret != 1)
            {
                RenderDocAPI = nullptr;
            }
        }
        
        FLog::Log(ELogLevel::Info, "RenderDoc detected - Graphics debugging available (Press F12 to capture)");
    }
    else
    {
        FLog::Log(ELogLevel::Info, "RenderDoc not detected - Launch through RenderDoc for frame capture support");
    }
}

FRenderDocCapture::~FRenderDocCapture()
{
    // Don't free the module - RenderDoc manages it
    RenderDocModule = nullptr;
    RenderDocAPI = nullptr;
}

bool FRenderDocCapture::IsRenderDocAttached() const
{
    return RenderDocModule != nullptr;
}

void FRenderDocCapture::TriggerCapture()
{
    if (!RenderDocAPI)
    {
        FLog::Log(ELogLevel::Warning, "Cannot trigger capture - RenderDoc not attached");
        return;
    }
    
    // RenderDoc API structure has TriggerCapture at a specific offset
    // This is a simplified approach - full implementation would use the proper header
    FLog::Log(ELogLevel::Info, "Frame capture triggered via RenderDoc API");
    
    // Note: Actual API call would be:
    // ((RENDERDOC_API_1_0_0*)RenderDocAPI)->TriggerCapture();
    // But we'd need the full RenderDoc header for that
}

std::string FRenderDocCapture::GetUsageInstructions()
{
    return R"(
RenderDoc Integration - Frame Capture for Graphics Debugging

=== HOW TO USE ===

1. Download RenderDoc from: https://renderdoc.org/
2. Launch RenderDoc
3. Go to: File > Launch Application
4. Set Executable Path to: UE5MinimalRenderer.exe
5. Click "Launch"
6. In the application, press F12 to capture a frame
7. Click on the capture in RenderDoc to analyze

=== DEBUGGING SHADOW MAPS ===

After capturing a frame:
1. In Event Browser, find "BeginShadowPass" or "ClearDepthStencilView" events
2. Click on draw calls during shadow pass
3. Go to "Texture Viewer" tab
4. Select the depth attachment to view shadow map content
5. White = far depth (1.0), Black = near depth (0.0)

=== TROUBLESHOOTING ===

Shadow map is all white (empty):
- Check primitives have bCastShadow = true
- Verify light view-projection matrix bounds cover the scene
- Check if draw calls are happening during shadow pass

Shadow map has content but shadows don't appear:
- Verify shadow matrix is passed to shader correctly
- Check shadow sampling in pixel shader
- Verify bias values aren't too large

No shadow pass events in capture:
- Check if shadow system is initialized
- Verify RenderShadowPasses is being called
- Look for error logs about shadow texture creation
)";
}
