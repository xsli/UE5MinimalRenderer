#include "../Game/Game.h"
#include "../Renderer/Renderer.h"
#include "../Renderer/RTPool.h"
#include <Windows.h>
#include <windowsx.h>
#include <chrono>
#include <cstdio>

// Input state tracking
struct FInputState 
{
    bool bLeftMouseDown = false;
    bool bRightMouseDown = false;
    bool bMiddleMouseDown = false;
    int LastMouseX = 0;
    int LastMouseY = 0;
    
    // Keyboard state
    bool bKeyW = false;
    bool bKeyA = false;
    bool bKeyS = false;
    bool bKeyD = false;
    bool bKeyQ = false;
    bool bKeyE = false;
};

// Camera sensitivity settings - UE5 Perspective Viewport Style
namespace CameraSettings 
{
    constexpr float MovementSpeed = 0.005f;   // Mouse drag movement (reduced)
    constexpr float RotationSpeed = 0.003f;   // Mouse rotation sensitivity (reduced for precision)
    constexpr float PanSpeed = 0.008f;        // Pan movement (reduced)
    constexpr float ZoomSpeed = 0.3f;         // Zoom/scroll speed (reduced)
    constexpr float KeyboardMoveSpeed = 3.0f; // WASD movement speed (units per second)
}

// Global game instance and timing
static FGame* g_Game = nullptr;
static HWND g_HWND = nullptr;
static auto g_LastTime = std::chrono::high_resolution_clock::now();
static int g_FrameCount = 0;
static FInputState g_InputState;

// Update window title with RT pool stats
static void UpdateWindowTitle()
{
    if (!g_Game || !g_HWND) return;
    
    FRenderer* renderer = g_Game->GetRenderer();
    if (!renderer) return;
    
    uint32 drawCalls = renderer->GetDrawCallCount();
    float fps = renderer->GetStats().GetFPS();
    
    const FRTPoolStats* poolStats = renderer->GetRTPoolStats();
    uint32 rtPooled = poolStats ? poolStats->TotalPooledRTs : 0;
    uint32 rtActive = poolStats ? poolStats->ActiveRTs : 0;
    
    char title[256];
    snprintf(title, sizeof(title), 
             "UE5 Minimal Renderer - Shadow Mapping | FPS: %.1f | DrawCalls: %u | RT Pool: %u/%u", 
             fps, drawCalls, rtActive, rtPooled);
    
    SetWindowTextA(g_HWND, title);
}

// Window procedure
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        
        // Mouse button down events
        case WM_LBUTTONDOWN:
            g_InputState.bLeftMouseDown = true;
            g_InputState.LastMouseX = GET_X_LPARAM(lParam);
            g_InputState.LastMouseY = GET_Y_LPARAM(lParam);
            SetCapture(hwnd);  // Capture mouse to receive events outside window
            return 0;
            
        case WM_RBUTTONDOWN:
            g_InputState.bRightMouseDown = true;
            g_InputState.LastMouseX = GET_X_LPARAM(lParam);
            g_InputState.LastMouseY = GET_Y_LPARAM(lParam);
            SetCapture(hwnd);
            return 0;
            
        case WM_MBUTTONDOWN:
            g_InputState.bMiddleMouseDown = true;
            g_InputState.LastMouseX = GET_X_LPARAM(lParam);
            g_InputState.LastMouseY = GET_Y_LPARAM(lParam);
            SetCapture(hwnd);
            return 0;
        
        // Mouse button up events
        case WM_LBUTTONUP:
            g_InputState.bLeftMouseDown = false;
            if (!g_InputState.bRightMouseDown && !g_InputState.bMiddleMouseDown)
            {
                ReleaseCapture();
            }
            return 0;
            
        case WM_RBUTTONUP:
            g_InputState.bRightMouseDown = false;
            if (!g_InputState.bLeftMouseDown && !g_InputState.bMiddleMouseDown)
            {
                ReleaseCapture();
            }
            return 0;
            
        case WM_MBUTTONUP:
            g_InputState.bMiddleMouseDown = false;
            if (!g_InputState.bLeftMouseDown && !g_InputState.bRightMouseDown)
            {
                ReleaseCapture();
            }
            return 0;
        
        // Mouse move event - UE5 Perspective Viewport Style
        case WM_MOUSEMOVE:
            {
                if (g_Game && (g_InputState.bLeftMouseDown || g_InputState.bRightMouseDown || g_InputState.bMiddleMouseDown))
                {
                    FCamera* camera = g_Game->GetCamera();
                    if (camera)
                    {
                        int currentX = GET_X_LPARAM(lParam);
                        int currentY = GET_Y_LPARAM(lParam);
                        int deltaX = currentX - g_InputState.LastMouseX;
                        int deltaY = currentY - g_InputState.LastMouseY;
                        
                        // UE5 Perspective Viewport Controls:
                        // - RMB: Free look (rotate pitch and yaw)
                        // - LMB + RMB or MMB: Pan camera
                        // - LMB alone: In perspective view, this also does free look (same as RMB)
                        
                        if ((g_InputState.bLeftMouseDown && g_InputState.bRightMouseDown) || g_InputState.bMiddleMouseDown)
                        {
                            // LMB + RMB or MMB: Pan camera (translate left/right and up/down)
                            camera->PanRight(-deltaX * CameraSettings::PanSpeed);  // Inverted for natural feel
                            camera->PanUp(deltaY * CameraSettings::PanSpeed);      // Inverted for natural feel
                        }
                        else if (g_InputState.bRightMouseDown)
                        {
                            // RMB only: Free look (rotate camera)
                            camera->RotateYaw(deltaX * CameraSettings::RotationSpeed);
                            camera->RotatePitch(deltaY * CameraSettings::RotationSpeed);
                        }
                        else if (g_InputState.bLeftMouseDown)
                        {
                            // LMB only: In UE5 perspective, this is typically orbit or nothing
                            // For simplicity, we'll make LMB also do free look (common in many 3D apps)
                            camera->RotateYaw(deltaX * CameraSettings::RotationSpeed);
                            camera->RotatePitch(deltaY * CameraSettings::RotationSpeed);
                        }
                        
                        g_InputState.LastMouseX = currentX;
                        g_InputState.LastMouseY = currentY;
                    }
                }
                return 0;
            }
        
        // Mouse wheel event
        case WM_MOUSEWHEEL:
            {
                if (g_Game)
                {
                    FCamera* camera = g_Game->GetCamera();
                    if (camera)
                    {
                        int delta = GET_WHEEL_DELTA_WPARAM(wParam);
                        float zoomAmount = delta / 120.0f;  // Standard wheel delta is 120 per notch
                        camera->Zoom(zoomAmount * CameraSettings::ZoomSpeed);
                    }
                }
                return 0;
            }
        
        // Keyboard events
        case WM_KEYDOWN:
            {
                switch (wParam)
                {
                    case 'W': g_InputState.bKeyW = true; return 0;
                    case 'A': g_InputState.bKeyA = true; return 0;
                    case 'S': g_InputState.bKeyS = true; return 0;
                    case 'D': g_InputState.bKeyD = true; return 0;
                    case 'Q': g_InputState.bKeyQ = true; return 0;
                    case 'E': g_InputState.bKeyE = true; return 0;
                }
                return 0;
            }
        
        case WM_KEYUP:
            {
                switch (wParam)
                {
                    case 'W': g_InputState.bKeyW = false; return 0;
                    case 'A': g_InputState.bKeyA = false; return 0;
                    case 'S': g_InputState.bKeyS = false; return 0;
                    case 'D': g_InputState.bKeyD = false; return 0;
                    case 'Q': g_InputState.bKeyQ = false; return 0;
                    case 'E': g_InputState.bKeyE = false; return 0;
                }
                return 0;
            }
            
        case WM_PAINT:
            {
                // Don't use BeginPaint/EndPaint as we're doing our own rendering
                ValidateRect(hwnd, NULL);
                
                if (g_Game)
                {
                    // Calculate delta time
                    auto currentTime = std::chrono::high_resolution_clock::now();
                    float deltaTime = std::chrono::duration<float>(currentTime - g_LastTime).count();
                    g_LastTime = currentTime;
                    
                    // Log first few frames
                    g_FrameCount++;
                    if (g_FrameCount <= 3)
                    {
                        FLog::Log(ELogLevel::Info, std::string("Frame ") + std::to_string(g_FrameCount) + 
                            " - DeltaTime: " + std::to_string(deltaTime));
                    }
                    
                    // Process keyboard input for camera movement
                    // UE5 Style: WASD works best when holding RMB (fly mode)
                    // But also works without RMB for general navigation
                    FCamera* camera = g_Game->GetCamera();
                    if (camera)
                    {
                        float moveAmount = CameraSettings::KeyboardMoveSpeed * deltaTime;
                        
                        // WASD movement (UE5 style - works with or without RMB)
                        if (g_InputState.bKeyW)
                        {
                            camera->MoveForwardBackward(moveAmount);
                        }
                        if (g_InputState.bKeyS)
                        {
                            camera->MoveForwardBackward(-moveAmount);
                        }
                        if (g_InputState.bKeyD)
                        {
                            camera->PanRight(moveAmount);
                        }
                        if (g_InputState.bKeyA)
                        {
                            camera->PanRight(-moveAmount);
                        }
                        
                        // QE for up/down movement (world space up/down)
                        if (g_InputState.bKeyE)
                        {
                            camera->PanUp(moveAmount);
                        }
                        if (g_InputState.bKeyQ)
                        {
                            camera->PanUp(-moveAmount);
                        }
                    }
                    
                    // Game tick (includes rendering)
                    try
                    {
                        g_Game->Tick(deltaTime);
                        
                        // Update window title every 10 frames
                        if (g_FrameCount % 10 == 0)
                        {
                            UpdateWindowTitle();
                        }
                    }
                    catch (const std::exception& e)
                    {
                        FLog::Log(ELogLevel::Error, std::string("Exception in game.Tick: ") + e.what());
                        PostQuitMessage(1);
                    }
                    catch (...)
                    {
                        FLog::Log(ELogLevel::Error, "Unknown exception in game.Tick");
                        PostQuitMessage(1);
                    }
                    
                    // Invalidate to trigger continuous rendering
                    InvalidateRect(hwnd, NULL, FALSE);
                }
                
                return 0;
            }
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

// Main entry point
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    // Window dimensions
    const uint32 Width = 1280;
    const uint32 Height = 720;
    
    // Register window class
    WNDCLASSEXA wc = {};
    wc.cbSize = sizeof(WNDCLASSEXA);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = "UE5MinimalRendererClass";
    
    if (!RegisterClassExA(&wc))
    {
        MessageBoxA(NULL, "Window Registration Failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }
    
    // Create window
    RECT windowRect = { 0, 0, static_cast<LONG>(Width), static_cast<LONG>(Height) };
    AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);
    
    HWND hwnd = CreateWindowExA(
        0,
        "UE5MinimalRendererClass",
        "UE5 Minimal Renderer - Shadow Mapping",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        windowRect.right - windowRect.left,
        windowRect.bottom - windowRect.top,
        NULL, NULL, hInstance, NULL
    );
    
    if (!hwnd)
    {
        MessageBoxA(NULL, "Window Creation Failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }
    
    // Store global HWND for title updates
    g_HWND = hwnd;
    
    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);
    
    // Initialize game
    FGame game;
    g_Game = &game;
    
    if (!game.Initialize(hwnd, Width, Height))
    {
        MessageBoxA(NULL, "Game Initialization Failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }
    
    FLog::Log(ELogLevel::Info, "Starting main loop...");
    
    // Initialize timing
    g_LastTime = std::chrono::high_resolution_clock::now();
    
    // Trigger first paint to start the render loop
    InvalidateRect(hwnd, NULL, FALSE);
    
    // Standard Windows message loop
    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    FLog::Log(ELogLevel::Info, std::string("Main loop exited after ") + std::to_string(g_FrameCount) + " frames");
    
    // Shutdown
    g_Game = nullptr;
    game.Shutdown();
    
    FLog::Log(ELogLevel::Info, "Application exiting");
    return static_cast<int>(msg.wParam);
}
