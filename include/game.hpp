// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2026 Ingo Ruhnke <grumbel@gmail.com>
#pragma once

#include "audio.hpp"
#include "config.hpp"
#include "gpu_raytracer.hpp"
#include "raytracer.hpp"
#include "scene.hpp"
#include <SDL.h>

enum class RenderBackend { Cpu, Gpu };

enum class GameState {
    Attract,
    Intro,
    Play,
    Pause,
    GameOver
};

enum class Difficulty { Easy, Normal, Hard };

enum class CameraMode {
    Paddle,
    High,
    Sideline
};

enum class Theme {
    Classic,
    Neon,
    Ice,
    Ember
};

class Game {
public:
    explicit Game(RenderBackend backend);
    ~Game();

    bool init();
    void run();
    void shutdown();

private:
    RenderBackend backend_;
    GameState state_ = GameState::Attract;
    Difficulty difficulty_ = Difficulty::Normal;
    CameraMode cameraMode_ = CameraMode::Paddle;
    bool twoPlayer_ = false;
    int maxBounces_ = 3;
    bool vsync_ = true;
    int targetFps_ = 60;
    int shadowSamples_ = 4;
    float exposure_ = 1.0f;
    Theme theme_ = Theme::Classic;
    float replayTimer_ = 0.0f;
    float replayDuration_ = 1.8f;
    bool slowmoReplay_ = true;
    bool replayTowardPlayer_ = false; // ball exited near player end

    SDL_Window* window_ = nullptr;
    SDL_Renderer* sdlRenderer_ = nullptr;
    SDL_Texture* texture_ = nullptr;
    uint32_t* framebuffer_ = nullptr;
    int fbW_ = 0;
    int fbH_ = 0;

    CpuRaytracer cpuRt_;
    GpuRaytracer gpuRt_;
    Audio audio_;
    Scene scene_;
    Camera camera_;

    float playerX_ = 0.0f;
    float player2X_ = 0.0f; // AI or second human (far paddle)
    float ballX_ = 0.0f;
    float ballZ_ = 0.0f;
    float ballVX_ = 0.0f;
    float ballVZ_ = 0.0f;

    int playerScore_ = 0;
    int aiScore_ = 0;
    int pointsToWin_ = 11;

    bool running_ = false;

    float shake_ = 0.0f;
    float shakeOffsetX_ = 0.0f;
    float shakeOffsetY_ = 0.0f;

    float serveTimer_ = 0.0f;
    bool nextServeTowardPlayer_ = false;
    float ballFlash_ = 0.0f;
    float fpsSmooth_ = 0.0f;

    float attractTime_ = 0.0f;
    float introT_ = 0.0f;
    static constexpr float INTRO_DURATION = 2.2f;

    // Attract demo: light AI-vs-AI motion
    bool demoActive_ = true;
    float gameOverTime_ = 0.0f;
    float animTime_ = 0.0f;
    bool playerWon_ = false;

    static constexpr float FIELD_W = 8.0f;
    static constexpr float FIELD_L = 16.0f;
    static constexpr float PADDLE_W = 1.6f;
    static constexpr float PADDLE_H = 0.4f;
    static constexpr float PADDLE_D = 0.3f;
    static constexpr float BALL_R = 0.35f;
    static constexpr float WALL_H = 3.5f;
    static constexpr float SERVE_DELAY = 0.75f;

    bool initWindowAndBackend();
    void shutdownBackend();
    bool switchBackend(RenderBackend next);
    bool ensureCpuFramebuffer(int w, int h);

    float aiSpeedForDifficulty() const;
    void cycleDifficulty();
    void cyclePointsToWin();
    void cycleCameraMode();
    void toggleTwoPlayer();

    void resetBall(bool towardPlayer);
    void queueServe(bool towardPlayer);
    void startMatch();
    void updateDemo(float dt);
    void update(float dt);
    void handleInput(float dt);
    void buildScene();
    void addScoreboard(Scene& scene) const;
    void addOptionsGeometry(Scene& scene) const;
    void addTitleGeometry(Scene& scene) const;
    void addServeCountdown(Scene& scene) const;
    void addMatchPointBanner(Scene& scene) const;
    void addGameOverBanner(Scene& scene) const;
    void cycleShadowSamples();
    void cycleTheme();
    void adjustExposure(float delta);
    void paddleColors(Vec3& player, Vec3& farPad) const;
    void applyVsync();
    bool isMatchPoint() const;
    void applyConfig(const AppConfig& cfg);
    AppConfig currentConfig() const;
    void persistConfig();
    void cycleBounces();
    void addDigitBoxes(Scene& scene, float ox, float oy, float oz, int digit,
                       const Vec3& color) const;
    void presentCpu();
    void updateHud();
    void toggleFullscreen();
    void triggerShake(float amount);
    void updateShake(float dt);
    void updateCamera(float dt);
};
