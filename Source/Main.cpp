#include "Game/Game.h"
#include <iostream>
#include <memory>

int main(int argc [[maybe_unused]], char* argv[] [[maybe_unused]])
{
    std::cout << "========================================" << std::endl;
    std::cout << "UE5 Minimal Renderer - Parallel Architecture Demo" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;

    // Create game instance
    auto GameInstance = std::make_unique<Game>();

    // Initialize the game (this will initialize renderer and RHI)
    if (!GameInstance->Initialize())
    {
        std::cerr << "Failed to initialize game!" << std::endl;
        return 1;
    }

    std::cout << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Initialization Complete - Starting Game Loop" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;

    // Run the main game loop
    GameInstance->Run();

    std::cout << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Shutting Down" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;

    // Shutdown
    GameInstance->Shutdown();

    std::cout << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Application Exited Successfully" << std::endl;
    std::cout << "========================================" << std::endl;

    return 0;
}
