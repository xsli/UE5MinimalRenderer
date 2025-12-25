#include "../Game/Game.h"
#include <Windows.h>
#include <windowsx.h>
#include <chrono>

// Input state tracking
struct FInputState {
    bool bLeftMouseDown = false;
    bool bRightMouseDown = false;
    bool bMiddleMouseDown = false;
    int LastMouseX = 0;
    int LastMouseY = 0;
};

// Camera sensitivity settings
namespace CameraSettings {
    constexpr float MovementSpeed = 0.01f;
    constexpr float RotationSpeed = 0.005f;
    constexpr float PanSpeed = 0.01f;
    constexpr float ZoomSpeed = 0.5f;
}

// Global game instance and timing
static FGame* g_Game = nullptr;
static auto g_LastTime = std::chrono::high_resolution_clock::now();
static int g_FrameCount = 0;
static FInputState g_InputState;

// Window procedure
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
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
            if (!g_InputState.bRightMouseDown && !g_InputState.bMiddleMouseDown) {
                ReleaseCapture();
            }
            return 0;
            
        case WM_RBUTTONUP:
            g_InputState.bRightMouseDown = false;
            if (!g_InputState.bLeftMouseDown && !g_InputState.bMiddleMouseDown) {
                ReleaseCapture();
            }
            return 0;
            
        case WM_MBUTTONUP:
            g_InputState.bMiddleMouseDown = false;
            if (!g_InputState.bLeftMouseDown && !g_InputState.bRightMouseDown) {
                ReleaseCapture();
            }
            return 0;
        
        // Mouse move event
        case WM_MOUSEMOVE:
            {
                if (g_Game && (g_InputState.bLeftMouseDown || g_InputState.bRightMouseDown || g_InputState.bMiddleMouseDown)) {
                    FCamera* camera = g_Game->GetCamera();
                    if (camera) {
                        int currentX = GET_X_LPARAM(lParam);
                        int currentY = GET_Y_LPARAM(lParam);
                        int deltaX = currentX - g_InputState.LastMouseX;
                        int deltaY = currentY - g_InputState.LastMouseY;
                        
                        // LMB + RMB or MMB: Pan camera
                        if ((g_InputState.bLeftMouseDown && g_InputState.bRightMouseDown) || g_InputState.bMiddleMouseDown) {
                            camera->PanRight(deltaX * CameraSettings::PanSpeed);
                            camera->PanUp(-deltaY * CameraSettings::PanSpeed);  // Invert Y for natural movement
                        }
                        // LMB only: Move forward/backward and rotate left/right
                        else if (g_InputState.bLeftMouseDown && !g_InputState.bRightMouseDown) {
                            camera->MoveForwardBackward(-deltaY * CameraSettings::MovementSpeed);  // Y movement controls forward/back
                            camera->RotateYaw(deltaX * CameraSettings::RotationSpeed);  // X movement controls left/right rotation
                        }
                        // RMB only: Rotate camera
                        else if (g_InputState.bRightMouseDown && !g_InputState.bLeftMouseDown) {
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
                if (g_Game) {
                    FCamera* camera = g_Game->GetCamera();
                    if (camera) {
                        int delta = GET_WHEEL_DELTA_WPARAM(wParam);
                        float zoomAmount = delta / 120.0f;  // Standard wheel delta is 120 per notch
                        camera->Zoom(zoomAmount * CameraSettings::ZoomSpeed);
                    }
                }
                return 0;
            }
            
        case WM_PAINT:
            {
                // Don't use BeginPaint/EndPaint as we're doing our own rendering
                ValidateRect(hwnd, NULL);
                
                if (g_Game) {
                    // Calculate delta time
                    auto currentTime = std::chrono::high_resolution_clock::now();
                    float deltaTime = std::chrono::duration<float>(currentTime - g_LastTime).count();
                    g_LastTime = currentTime;
                    
                    // Log first few frames
                    g_FrameCount++;
                    if (g_FrameCount <= 3) {
                        FLog::Log(ELogLevel::Info, std::string("Frame ") + std::to_string(g_FrameCount) + 
                            " - DeltaTime: " + std::to_string(deltaTime));
                    }
                    
                    // Game tick (includes rendering)
                    try {
                        g_Game->Tick(deltaTime);
                    } catch (const std::exception& e) {
                        FLog::Log(ELogLevel::Error, std::string("Exception in game.Tick: ") + e.what());
                        PostQuitMessage(1);
                    } catch (...) {
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
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
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
    
    if (!RegisterClassExA(&wc)) {
        MessageBoxA(NULL, "Window Registration Failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }
    
    // Create window
    RECT windowRect = { 0, 0, static_cast<LONG>(Width), static_cast<LONG>(Height) };
    AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);
    
    HWND hwnd = CreateWindowExA(
        0,
        "UE5MinimalRendererClass",
        "UE5 Minimal Renderer - Colored Triangle Demo",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        windowRect.right - windowRect.left,
        windowRect.bottom - windowRect.top,
        NULL, NULL, hInstance, NULL
    );
    
    if (!hwnd) {
        MessageBoxA(NULL, "Window Creation Failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }
    
    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);
    
    // Initialize game
    FGame game;
    g_Game = &game;
    
    if (!game.Initialize(hwnd, Width, Height)) {
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
    while (GetMessage(&msg, NULL, 0, 0)) {
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
