// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2026 Ingo Ruhnke <grumbel@gmail.com>
#pragma once

#include "audio.hpp"
#include "config.hpp"
#include "gpu_raytracer.hpp"
#include "raytracer.hpp"
#include "scene.hpp"
#include <SDL.h>

#include <string>
#include <utility>
#include <vector>

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

enum class Quality {
    Low,
    Medium,
    High,
    Ultra
};

/** Non-persistent options for testing / automation (never written to the config file). */
struct DevOptions {
    int windowW = 0;                                   // initial window size (0 = default)
    int windowH = 0;
    bool autoStart = false;                            // skip attract mode, start a match
    float fixedDt = 0.0f;                              // > 0: deterministic simulation step (s)
    int quitAfterFrames = 0;                           // > 0: exit after this many frames
    std::vector<std::pair<int, std::string>> shots;    // (frame index, BMP path)
};

class Game {
public:
    explicit Game(RenderBackend backend);
    ~Game();

    void setDevOptions(const DevOptions& dev) { dev_ = dev; }

    /** Load config file, then apply optional CLI overrides (mask bits: CliOverride). */
    bool init(const AppConfig* cli = nullptr, unsigned cliMask = 0);
    void applyCliOverrides(const AppConfig& cfg, unsigned mask);
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
    Quality quality_ = Quality::High;
    float replayTimer_ = 0.0f;
    float replayDuration_ = 1.8f;
    bool slowmoReplay_ = true;
    int cpuMaxWidth_ = 1280;
    int cpuMaxHeight_ = 720;
    float cpuScale_ = 1.0f;
    bool replayTowardPlayer_ = false; // ball exited near player end

    SDL_Window* window_ = nullptr;
    SDL_Renderer* sdlRenderer_ = nullptr;
    SDL_Texture* texture_ = nullptr;
    uint32_t* framebuffer_ = nullptr;
    int fbW_ = 0;
    int fbH_ = 0;
    // Debounced resize: avoid thrashing FB/FBO while the user drags the window
    int appliedWinW_ = 0;
    int appliedWinH_ = 0;
    int pendingWinW_ = 0;
    int pendingWinH_ = 0;
    float resizeCooldown_ = 0.0f;
    static constexpr float kResizeSettle = 0.10f; // seconds after last size change

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

    DevOptions dev_;
    long frameIndex_ = 0;
    bool screenshotRequested_ = false;

    float shake_ = 0.0f;
    float shakeOffsetX_ = 0.0f;
    float shakeOffsetY_ = 0.0f;
    float shakeAge_ = 0.0f;   // seconds since the last impact (drives the oscillation)

    // Camera rig: each shot defines an ideal pose; the rendered pose follows it
    // through critically damped smoothing so every cut becomes a glide.
    struct CamPose {
        Vec3 pos;
        Vec3 look;
        float fov = 60.0f;
    };
    CamPose cam_;
    Vec3 camPosVel_;
    Vec3 camLookVel_;
    float camFovVel_ = 0.0f;
    bool camInit_ = false;
    int camShot_ = -1;
    float camGlide_ = 0.0f;   // 1 right after a big shot change, decays to 0
    float fovKick_ = 0.0f;    // impact FOV punch (degrees)
    float camRecoil_ = 0.0f;  // impact dolly-back (0..1)
    float camRoll_ = 0.0f;    // strafe lean (radians)
    float playerVelX_ = 0.0f;
    float prevPlayerX_ = 0.0f;

    float serveTimer_ = 0.0f;
    bool nextServeTowardPlayer_ = false;
    float ballFlash_ = 0.0f;
    static constexpr int TRAIL_LEN = 4;
    static constexpr float TRAIL_STEP = 0.022f;  // seconds between trail samples
    float trailX_[TRAIL_LEN] = {};
    float trailZ_[TRAIL_LEN] = {};
    float trailTimer_ = 0.0f;
    void pushTrail(float dt);

    // Short-lived feedback animations (1 → 0)
    float playerRecoil_ = 0.0f;  // paddle kick-back after hitting the ball
    float aiRecoil_ = 0.0f;
    float playerPop_ = 0.0f;     // score digit pop after scoring
    float aiPop_ = 0.0f;
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
    static constexpr float SERVE_DELAY = 1.35f;   // 3 - 2 - 1, 0.45 s each
    // Distance from each paddle line to the end wall (near wall at z = -END_WALL,
    // far wall at z = FIELD_L + END_WALL). Goals are scored at GOAL_MARGIN.
    static constexpr float END_WALL = 1.6f;
    static constexpr float GOAL_MARGIN = 1.0f;

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
    void enterAttract();
    void updateDemo(float dt);
    void update(float dt);
    void updateGoalDrift(float dt);
    void handleInput(float dt);
    void buildScene();
    void addScoreboard(Scene& scene) const;
    void addOptionsGeometry(Scene& scene) const;
    void addTitleGeometry(Scene& scene) const;
    void addServeCountdown(Scene& scene) const;
    void addMatchPointBanner(Scene& scene) const;
    void addGameOverBanner(Scene& scene) const;
    void addAttractHint(Scene& scene) const;
    void addPauseBanner(Scene& scene) const;
    void cycleShadowSamples();
    void cycleTheme();
    void cycleQuality();
    void applyQualityPreset();
    void syncGpuScale();
    const char* qualityLabel() const;
    void adjustExposure(float delta);
    void paddleColors(Vec3& player, Vec3& farPad) const;
    void applyVsync();
    bool isMatchPoint() const;
    void applyConfig(const AppConfig& cfg);
    AppConfig currentConfig() const;
    void persistConfig();
    void cycleBounces();
    void presentCpu();
    void updateHud();
    void toggleFullscreen();
    void noteWindowSize(int w, int h);
    void flushPendingResize(float dt);
    void applyWindowSize(int w, int h);
    void triggerShake(float amount);
    void cameraImpulse(float fovDegrees, float recoil);
    void updateShake(float dt);
    void updateCamera(float dt);
    void handleScreenshots();
    bool saveScreenshot(const std::string& path);
    std::string nextScreenshotPath() const;
};
