#pragma once

#include <memory>
#include <atomic>

class Renderer;

// Main Game class - Runs on the game thread
class Game
{
public:
    Game();
    ~Game();

    // Initialization and shutdown
    bool Initialize();
    void Shutdown();

    // Main game loop
    void Run();

    // Frame update
    void Tick(float DeltaTime);

    // Check if game should continue running
    bool IsRunning() const { return bIsRunning; }

private:
    std::unique_ptr<Renderer> GameRenderer;
    std::atomic<bool> bIsRunning;
    uint32_t FrameCount;
    uint32_t MaxFrames; // For testing, we'll run a limited number of frames
};
