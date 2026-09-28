// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2026 Ingo Ruhnke <grumbel@gmail.com>
#pragma once

#include "audio.hpp"
#include "gpu_raytracer.hpp"
#include "raytracer.hpp"
#include "scene.hpp"
#include <SDL.h>

enum class RenderBackend { Cpu, Gpu };

class Game {
public:
    explicit Game(RenderBackend backend);
    ~Game();

    bool init();
    void run();
    void shutdown();

private:
    RenderBackend backend_;

    SDL_Window* window_ = nullptr;
    SDL_Renderer* sdlRenderer_ = nullptr;
    SDL_Texture* texture_ = nullptr;
    uint32_t* framebuffer_ = nullptr;

    CpuRaytracer cpuRt_;
    GpuRaytracer gpuRt_;
    Audio audio_;
    Scene scene_;
    Camera camera_;

    float playerX_ = 0.0f;
    float aiX_ = 0.0f;
    float ballX_ = 0.0f;
    float ballZ_ = 0.0f;
    float ballVX_ = 0.0f;
    float ballVZ_ = 0.0f;

    int playerScore_ = 0;
    int aiScore_ = 0;

    bool running_ = false;
    bool paused_ = false;

    // Camera shake
    float shake_ = 0.0f;
    float shakeOffsetX_ = 0.0f;
    float shakeOffsetY_ = 0.0f;

    // Serve pause after a point (seconds remaining)
    float serveTimer_ = 0.0f;
    bool nextServeTowardPlayer_ = false;

    // Ball impact flash (seconds remaining)
    float ballFlash_ = 0.0f;

    // FPS display (smoothed)
    float fpsSmooth_ = 0.0f;

    static constexpr float FIELD_W = 8.0f;
    static constexpr float FIELD_L = 16.0f;
    static constexpr float PADDLE_W = 1.6f;
    static constexpr float PADDLE_H = 0.4f;
    static constexpr float PADDLE_D = 0.3f;
    static constexpr float BALL_R = 0.35f;
    static constexpr float WALL_H = 3.0f;
    static constexpr float SERVE_DELAY = 0.75f;

    void resetBall(bool towardPlayer);
    void queueServe(bool towardPlayer);
    void update(float dt);
    void handleInput(float dt);
    void buildScene();
    void presentCpu();
    void updateHud();
    void toggleFullscreen();
    void triggerShake(float amount);
    void updateShake(float dt);
};
