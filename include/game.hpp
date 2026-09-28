#pragma once
#include "raytracer.hpp"
#include <SDL.h>
#include <string>

class Game {
public:
    Game();
    ~Game();

    bool init();
    void run();
    void shutdown();

private:
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_Texture* texture = nullptr;
    uint32_t* framebuffer = nullptr;

    Raytracer rt;
    Scene scene;

    // Game state (playfield in XZ, Y is up)
    // Player paddle at near end (negative Z), AI at far end (positive Z)
    float playerX = 0.0f;
    float aiX = 0.0f;
    float ballX = 0.0f;
    float ballZ = 0.0f;
    float ballVX = 0.0f;
    float ballVZ = 0.0f;

    int playerScore = 0;
    int aiScore = 0;

    bool running = false;
    bool paused = false;

    // Playfield dimensions
    static constexpr float FIELD_W = 8.0f;   // X extent (-4 .. +4)
    static constexpr float FIELD_L = 16.0f;  // Z extent (0 near player .. 16 far)
    static constexpr float PADDLE_W = 1.6f;
    static constexpr float PADDLE_H = 0.4f;
    static constexpr float PADDLE_D = 0.3f;
    static constexpr float BALL_R = 0.35f;
    static constexpr float WALL_H = 3.0f;

    void resetBall(bool towardPlayer);
    void update(float dt);
    void handleInput(float dt);
    void buildScene();
    void present();
    void drawHUD();
};
