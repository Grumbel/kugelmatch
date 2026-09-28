// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2024–2026 Ingo Ruhnke <grumbel@gmail.com>
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
    SDL_Renderer* sdlRenderer_ = nullptr; // CPU path only
    SDL_Texture* texture_ = nullptr;      // CPU path only
    uint32_t* framebuffer_ = nullptr;     // CPU path only

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

    // Camera shake (world units), decays over time
    float shake_ = 0.0f;
    float shakeTime_ = 0.0f;
    float shakeOffsetX_ = 0.0f;
    float shakeOffsetY_ = 0.0f;

    static constexpr float FIELD_W = 8.0f;
    static constexpr float FIELD_L = 16.0f;
    static constexpr float PADDLE_W = 1.6f;
    static constexpr float PADDLE_H = 0.4f;
    static constexpr float PADDLE_D = 0.3f;
    static constexpr float BALL_R = 0.35f;
    static constexpr float WALL_H = 3.0f;

    void resetBall(bool towardPlayer);
    void update(float dt);
    void handleInput(float dt);
    void buildScene();
    void presentCpu();
    void updateHud();
    void triggerShake(float amount);
    void updateShake(float dt);
};
