#include "../Game/Game.h"
#include <Windows.h>
#include <chrono>

// Global game instance and timing
static FGame* g_Game = nullptr;
static auto g_LastTime = std::chrono::high_resolution_clock::now();
static int g_FrameCount = 0;

// Window procedure
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
            
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
