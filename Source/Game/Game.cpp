#include "Game.h"
#include "../Renderer/Renderer.h"
#include <iostream>
#include <chrono>
#include <thread>

Game::Game()
    : bIsRunning(false)
    , FrameCount(0)
    , MaxFrames(10) // Run 10 frames for demonstration
{
}

Game::~Game()
{
    if (bIsRunning)
    {
        Shutdown();
    }
}

bool Game::Initialize()
{
    std::cout << "[Game] Initializing Game..." << std::endl;

    // Create and initialize renderer
    GameRenderer = std::make_unique<Renderer>();
    if (!GameRenderer->Initialize())
    {
        std::cerr << "[Game] Failed to initialize Renderer" << std::endl;
        return false;
    }

    bIsRunning = true;

    std::cout << "[Game] Game initialized successfully" << std::endl;
    return true;
}

void Game::Shutdown()
{
    std::cout << "[Game] Shutting down Game..." << std::endl;

    bIsRunning = false;

    // Shutdown renderer
    if (GameRenderer)
    {
        GameRenderer->Shutdown();
        GameRenderer.reset();
    }

    std::cout << "[Game] Game shut down" << std::endl;
}

void Game::Run()
{
    std::cout << "[Game] Starting main game loop..." << std::endl;
    std::cout << "[Game] Will run for " << MaxFrames << " frames" << std::endl;

    auto LastTime = std::chrono::high_resolution_clock::now();

    while (bIsRunning && FrameCount < MaxFrames)
    {
        auto CurrentTime = std::chrono::high_resolution_clock::now();
        float DeltaTime = std::chrono::duration<float>(CurrentTime - LastTime).count();
        LastTime = CurrentTime;

        Tick(DeltaTime);

        // Simulate frame rate (roughly 60 FPS)
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    std::cout << "[Game] Main game loop finished" << std::endl;
}

void Game::Tick(float DeltaTime)
{
    std::cout << "\n[Game] ===== Game Thread Frame " << FrameCount 
              << " (DeltaTime: " << DeltaTime << "s) =====" << std::endl;

    // Begin frame on game thread
    GameRenderer->BeginFrame();

    // Simulate game logic
    std::cout << "[Game] Processing game logic..." << std::endl;

    // Enqueue render commands to the render thread
    // This demonstrates parallel rendering - game thread continues while render thread processes
    uint32_t CapturedFrameCount = FrameCount;
    GameRenderer->EnqueueRenderCommand(RenderCommand{
        [CapturedFrameCount]() {
            std::cout << "[Game::RenderThread] Processing render commands for frame " 
                      << CapturedFrameCount << std::endl;
            
            // Simulate rendering work on render thread
            // In a real implementation, this would include:
            // - Scene traversal
            // - Culling
            // - Draw call submission
            
            // For now, we'll just demonstrate the RHI calls
            // Note: In a real implementation, RHI would only be called from render thread
        }
    });

    // End frame on game thread
    GameRenderer->EndFrame();

    FrameCount++;

    std::cout << "[Game] Game thread frame " << (FrameCount - 1) << " complete" << std::endl;
}
