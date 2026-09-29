// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2026 Ingo Ruhnke <grumbel@gmail.com>
#include "game.hpp"
#include "glyphs.hpp"
#include "version.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

namespace {

void pushBox(Scene& scene, const Vec3& minb, const Vec3& maxb, const Vec3& color,
             float reflectivity) {
    Box b;
    b.minb = minb;
    b.maxb = maxb;
    b.color = color;
    b.reflectivity = reflectivity;
    scene.boxes.push_back(b);
}

} // namespace

Game::Game(RenderBackend backend) : backend_(backend) {
    std::srand(static_cast<unsigned>(std::time(nullptr)));
}

Game::~Game() {
    shutdown();
}

bool Game::initWindowAndBackend() {
    Uint32 winFlags = SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE;
    if (backend_ == RenderBackend::Gpu) {
        winFlags |= SDL_WINDOW_OPENGL;
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    }

    if (!window_) {
        window_ = SDL_CreateWindow(
            "KugelMatch",
            SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
            GpuRaytracer::DEFAULT_WIDTH, GpuRaytracer::DEFAULT_HEIGHT,
            winFlags);
        if (!window_) {
            std::fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
            return false;
        }
    }

    if (backend_ == RenderBackend::Gpu) {
        if (!gpuRt_.init(window_)) {
            std::fprintf(stderr, "GPU raytracer init failed.\n");
            return false;
        }
        syncGpuScale();
        applyVsync();
    } else {
        sdlRenderer_ = SDL_CreateRenderer(
            window_, -1,
            SDL_RENDERER_ACCELERATED | (vsync_ ? SDL_RENDERER_PRESENTVSYNC : 0));
        if (!sdlRenderer_) {
            sdlRenderer_ = SDL_CreateRenderer(window_, -1, 0);
        }
        if (!sdlRenderer_) {
            std::fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
            return false;
        }
        int w = 0, h = 0;
        SDL_GetWindowSize(window_, &w, &h);
        if (!ensureCpuFramebuffer(std::max(1, w), std::max(1, h))) {
            return false;
        }
    }
    return true;
}

void Game::shutdownBackend() {
    if (backend_ == RenderBackend::Gpu) {
        gpuRt_.shutdown();
    } else {
        if (texture_) {
            SDL_DestroyTexture(texture_);
            texture_ = nullptr;
        }
        if (sdlRenderer_) {
            SDL_DestroyRenderer(sdlRenderer_);
            sdlRenderer_ = nullptr;
        }
        delete[] framebuffer_;
        framebuffer_ = nullptr;
        fbW_ = fbH_ = 0;
    }
}

bool Game::switchBackend(RenderBackend next) {
    if (next == backend_) {
        return true;
    }
    int w = 0, h = 0;
    SDL_GetWindowSize(window_, &w, &h);
    Uint32 flags = SDL_GetWindowFlags(window_);
    bool fs = (flags & (SDL_WINDOW_FULLSCREEN | SDL_WINDOW_FULLSCREEN_DESKTOP)) != 0;

    shutdownBackend();
    if (window_) {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
    }

    backend_ = next;
    if (!initWindowAndBackend()) {
        // Try to recover original
        backend_ = (next == RenderBackend::Gpu) ? RenderBackend::Cpu : RenderBackend::Gpu;
        if (!initWindowAndBackend()) {
            return false;
        }
    }
    if (fs) {
        SDL_SetWindowFullscreen(window_, SDL_WINDOW_FULLSCREEN_DESKTOP);
    } else if (w > 0 && h > 0) {
        SDL_SetWindowSize(window_, w, h);
    }
    if (backend_ == RenderBackend::Gpu) {
        gpuRt_.onResize(w, h);
    }
    return true;
}

bool Game::ensureCpuFramebuffer(int w, int h) {
    // Optional supersample / undersample before max clamp
    w = static_cast<int>(w * cpuScale_ + 0.5f);
    h = static_cast<int>(h * cpuScale_ + 0.5f);
    if (cpuMaxWidth_ > 0 && w > cpuMaxWidth_) {
        w = cpuMaxWidth_;
    }
    if (cpuMaxHeight_ > 0 && h > cpuMaxHeight_) {
        h = cpuMaxHeight_;
    }
    w = std::max(1, w);
    h = std::max(1, h);
    if (w == fbW_ && h == fbH_ && framebuffer_ && texture_) {
        return true;
    }
    if (texture_) {
        SDL_DestroyTexture(texture_);
        texture_ = nullptr;
    }
    delete[] framebuffer_;
    framebuffer_ = nullptr;
    fbW_ = w;
    fbH_ = h;
    framebuffer_ = new uint32_t[static_cast<size_t>(w) * static_cast<size_t>(h)];
    std::memset(framebuffer_, 0, sizeof(uint32_t) * static_cast<size_t>(w) * static_cast<size_t>(h));
    texture_ = SDL_CreateTexture(sdlRenderer_, SDL_PIXELFORMAT_ARGB8888,
                                 SDL_TEXTUREACCESS_STREAMING, w, h);
    if (!texture_) {
        std::fprintf(stderr, "SDL_CreateTexture failed: %s\n", SDL_GetError());
        return false;
    }
    return true;
}

bool Game::init() {
    Uint32 sdlFlags = SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO;
    if (SDL_Init(sdlFlags) != 0) {
        std::fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return false;
    }

    if (!initWindowAndBackend()) {
        return false;
    }

    if (!audio_.init()) {
        std::fprintf(stderr, "Warning: audio init failed; continuing without sound.\n");
    }

    {
        AppConfig cfg;
        std::string from;
        if (loadConfig(cfg, &from)) {
            std::fprintf(stderr, "Loaded config from %s\n", from.c_str());
            applyConfig(cfg);
            applyVsync();
            // Backend is chosen at construction; optional switch if mismatch
            if (cfg.useGpu && backend_ != RenderBackend::Gpu) {
                switchBackend(RenderBackend::Gpu);
            } else if (!cfg.useGpu && backend_ != RenderBackend::Cpu) {
                switchBackend(RenderBackend::Cpu);
            }
            if (cfg.fullscreen) {
                SDL_SetWindowFullscreen(window_, SDL_WINDOW_FULLSCREEN_DESKTOP);
            }
        }
    }

    playerX_ = 0.0f;
    player2X_ = 0.0f;
    ballX_ = 0.0f;
    ballZ_ = FIELD_L * 0.5f;
    ballVX_ = ballVZ_ = 0.0f;
    state_ = GameState::Attract;
    attractTime_ = 0.0f;

    scene_.lightPos = Vec3(0.0f, 6.0f, 8.0f);
    scene_.lightColor = Vec3(1.2f, 1.15f, 1.05f);
    scene_.ambient = Vec3(0.12f, 0.12f, 0.15f);
    scene_.skyColor = Vec3(0.02f, 0.02f, 0.05f);

    running_ = true;
    return true;
}

void Game::shutdown() {
    persistConfig();
    audio_.shutdown();
    shutdownBackend();
    if (window_) {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
    }
    SDL_Quit();
}

void Game::resetBall(bool towardPlayer) {
    ballX_ = 0.0f;
    ballZ_ = FIELD_L * 0.5f;
    float speed = 6.5f + (std::rand() % 100) * 0.01f;
    ballVX_ = ((std::rand() % 200) - 100) * 0.02f;
    ballVZ_ = towardPlayer ? -speed : speed;
    ballFlash_ = 0.0f;
}

void Game::queueServe(bool towardPlayer) {
    nextServeTowardPlayer_ = towardPlayer;
    serveTimer_ = SERVE_DELAY;
    ballX_ = 0.0f;
    ballZ_ = FIELD_L * 0.5f;
    ballVX_ = 0.0f;
    ballVZ_ = 0.0f;
    ballFlash_ = 0.0f;
    for (int i = 0; i < TRAIL_LEN; ++i) {
        trailX_[i] = ballX_;
        trailZ_[i] = ballZ_;
    }
}


float Game::aiSpeedForDifficulty() const {
    switch (difficulty_) {
    case Difficulty::Easy:
        return 4.5f;
    case Difficulty::Hard:
        return 11.0f;
    default:
        return 7.5f;
    }
}

void Game::cycleDifficulty() {
    switch (difficulty_) {
    case Difficulty::Easy:
        difficulty_ = Difficulty::Normal;
        break;
    case Difficulty::Normal:
        difficulty_ = Difficulty::Hard;
        break;
    default:
        difficulty_ = Difficulty::Easy;
        break;
    }
    audio_.playSoftThud(1.2f, 0.2f);
    persistConfig();
}

void Game::cyclePointsToWin() {
    static const int opts[] = {7, 11, 15, 21};
    int idx = 0;
    for (int i = 0; i < 4; ++i) {
        if (opts[i] == pointsToWin_) {
            idx = i;
            break;
        }
    }
    pointsToWin_ = opts[(idx + 1) % 4];
    audio_.playSoftThud(0.9f, 0.2f);
    persistConfig();
}

void Game::cycleCameraMode() {
    switch (cameraMode_) {
    case CameraMode::Paddle:
        cameraMode_ = CameraMode::High;
        break;
    case CameraMode::High:
        cameraMode_ = CameraMode::Sideline;
        break;
    default:
        cameraMode_ = CameraMode::Paddle;
        break;
    }
    audio_.playSoftThud(1.0f, 0.2f);
    persistConfig();
}

void Game::toggleTwoPlayer() {
    twoPlayer_ = !twoPlayer_;
    audio_.playClank(twoPlayer_ ? 1.1f : 0.8f, 0.35f);
    persistConfig();
}


void Game::cycleBounces() {
    maxBounces_ = (maxBounces_ + 1) % 4; // 0..3
    audio_.playSoftThud(0.7f + maxBounces_ * 0.15f, 0.25f);
    persistConfig();
}

void Game::cycleShadowSamples() {
    static const int opts[] = {1, 2, 4, 8};
    int idx = 0;
    for (int i = 0; i < 4; ++i) {
        if (opts[i] == shadowSamples_) {
            idx = i;
            break;
        }
    }
    shadowSamples_ = opts[(idx + 1) % 4];
    audio_.playSoftThud(0.8f + shadowSamples_ * 0.05f, 0.2f);
    persistConfig();
}

void Game::cycleTheme() {
    theme_ = static_cast<Theme>((static_cast<int>(theme_) + 1) % 4);
    audio_.playSoftThud(0.9f + static_cast<float>(theme_) * 0.08f, 0.22f);
    persistConfig();
}

void Game::applyQualityPreset() {
    switch (quality_) {
    case Quality::Low:
        maxBounces_ = 0;
        shadowSamples_ = 1;
        cpuScale_ = 0.5f;
        break;
    case Quality::Medium:
        maxBounces_ = 1;
        shadowSamples_ = 2;
        cpuScale_ = 0.75f;
        break;
    case Quality::High:
        maxBounces_ = 2;
        shadowSamples_ = 4;
        cpuScale_ = 1.0f;
        break;
    case Quality::Ultra:
    default:
        maxBounces_ = 3;
        shadowSamples_ = 8;
        cpuScale_ = 1.25f;
        break;
    }
    syncGpuScale();
}

void Game::cycleQuality() {
    quality_ = static_cast<Quality>((static_cast<int>(quality_) + 1) % 4);
    applyQualityPreset();
    // Force CPU FB rebuild at new scale
    fbW_ = fbH_ = 0;
    audio_.playSoftThud(0.65f + static_cast<float>(quality_) * 0.12f, 0.25f);
    persistConfig();
}

void Game::syncGpuScale() {
    if (!gpuRt_.ready()) {
        return;
    }
    gpuRt_.setRenderScale(cpuScale_);
    gpuRt_.setMaxResolution(cpuMaxWidth_, cpuMaxHeight_);
}

const char* Game::qualityLabel() const {
    // Reflect actual RT settings; show CUST when keys 5/7 diverged from the preset.
    auto matches = [&](int bounces, int shadows, float scale) {
        return maxBounces_ == bounces && shadowSamples_ == shadows
            && std::fabs(cpuScale_ - scale) < 0.02f;
    };
    switch (quality_) {
    case Quality::Low:
        if (matches(0, 1, 0.5f)) return "LOW";
        break;
    case Quality::Medium:
        if (matches(1, 2, 0.75f)) return "MED";
        break;
    case Quality::High:
        if (matches(2, 4, 1.0f)) return "HIGH";
        break;
    case Quality::Ultra:
        if (matches(3, 8, 1.25f)) return "ULTRA";
        break;
    }
    if (matches(0, 1, 0.5f)) return "LOW";
    if (matches(1, 2, 0.75f)) return "MED";
    if (matches(2, 4, 1.0f)) return "HIGH";
    if (matches(3, 8, 1.25f)) return "ULTRA";
    return "CUST";
}

void Game::adjustExposure(float delta) {
    exposure_ += delta;
    if (exposure_ < 0.1f) exposure_ = 0.1f;
    if (exposure_ > 2.5f) exposure_ = 2.5f;
}

void Game::paddleColors(Vec3& player, Vec3& farPad) const {
    switch (theme_) {
    case Theme::Neon:
        player = Vec3(0.3f, 1.0f, 0.85f);
        farPad = Vec3(1.0f, 0.25f, 0.9f);
        break;
    case Theme::Ice:
        player = Vec3(0.75f, 0.9f, 1.0f);
        farPad = Vec3(0.55f, 0.7f, 0.95f);
        break;
    case Theme::Ember:
        player = Vec3(1.0f, 0.7f, 0.25f);
        farPad = Vec3(0.95f, 0.25f, 0.15f);
        break;
    default: // Classic
        player = Vec3(0.7f, 0.75f, 0.9f);
        farPad = Vec3(0.9f, 0.4f, 0.35f);
        break;
    }
}

AppConfig Game::currentConfig() const {
    AppConfig c;
    c.useGpu = (backend_ == RenderBackend::Gpu);
    c.volume = audio_.masterVolume();
    c.mute = audio_.muted();
    c.difficulty = static_cast<int>(difficulty_);
    c.pointsToWin = pointsToWin_;
    c.cameraMode = static_cast<int>(cameraMode_);
    c.twoPlayer = twoPlayer_;
    c.maxBounces = maxBounces_;
    c.vsync = vsync_;
    c.targetFps = targetFps_;
    c.shadowSamples = shadowSamples_;
    c.exposure = exposure_;
    c.theme = static_cast<int>(theme_);
    c.quality = static_cast<int>(quality_);
    c.slowmoReplay = slowmoReplay_;
    c.cpuMaxWidth = cpuMaxWidth_;
    c.cpuMaxHeight = cpuMaxHeight_;
    c.cpuScale = cpuScale_;
    if (window_) {
        Uint32 flags = SDL_GetWindowFlags(window_);
        c.fullscreen = (flags & (SDL_WINDOW_FULLSCREEN | SDL_WINDOW_FULLSCREEN_DESKTOP)) != 0;
    }
    return c;
}

void Game::applyConfig(const AppConfig& cfg) {
    difficulty_ = static_cast<Difficulty>(std::max(0, std::min(2, cfg.difficulty)));
    pointsToWin_ = cfg.pointsToWin;
    if (pointsToWin_ != 7 && pointsToWin_ != 11 && pointsToWin_ != 15 && pointsToWin_ != 21) {
        pointsToWin_ = 11;
    }
    cameraMode_ = static_cast<CameraMode>(std::max(0, std::min(2, cfg.cameraMode)));
    twoPlayer_ = cfg.twoPlayer;
    maxBounces_ = std::max(0, std::min(3, cfg.maxBounces));
    vsync_ = cfg.vsync;
    targetFps_ = cfg.targetFps;
    if (targetFps_ < 0) targetFps_ = 0;
    if (targetFps_ > 300) targetFps_ = 300;
    shadowSamples_ = std::max(1, std::min(8, cfg.shadowSamples));
    exposure_ = cfg.exposure;
    if (exposure_ < 0.1f) exposure_ = 0.1f;
    if (exposure_ > 3.0f) exposure_ = 3.0f;
    theme_ = static_cast<Theme>(std::max(0, std::min(3, cfg.theme)));
    quality_ = static_cast<Quality>(std::max(0, std::min(3, cfg.quality)));
    slowmoReplay_ = cfg.slowmoReplay;
    cpuMaxWidth_ = cfg.cpuMaxWidth;
    cpuMaxHeight_ = cfg.cpuMaxHeight;
    // Start from the named preset (scale + defaults), then re-apply explicit
    // bounces/shadows/scale from the file so custom values survive.
    applyQualityPreset();
    maxBounces_ = std::max(0, std::min(3, cfg.maxBounces));
    shadowSamples_ = std::max(1, std::min(8, cfg.shadowSamples));
    if (cfg.cpuScale > 0.0f) {
        cpuScale_ = cfg.cpuScale;
    }
    if (cpuScale_ < 0.25f) cpuScale_ = 0.25f;
    if (cpuScale_ > 2.0f) cpuScale_ = 2.0f;
    syncGpuScale();
    audio_.setMasterVolume(cfg.volume);
    audio_.setMuted(cfg.mute);
}

void Game::persistConfig() {
    saveConfig(currentConfig(), nullptr);
}

void Game::applyVsync() {
    if (backend_ == RenderBackend::Gpu) {
        SDL_GL_SetSwapInterval(vsync_ ? 1 : 0);
    }
    // CPU path uses PRESENTVSYNC flag at renderer create; recreate if needed is heavy —
    // store preference for next backend init.
}

bool Game::isMatchPoint() const {
    return (playerScore_ == pointsToWin_ - 1) || (aiScore_ == pointsToWin_ - 1);
}



void Game::updateDemo(float dt) {
    // Slow AI-vs-AI rally for attract mode
    float half = FIELD_W * 0.5f - PADDLE_W * 0.5f;
    float spd = 5.0f;

    auto track = [&](float& px, float target) {
        if (px < target - 0.12f) {
            px += spd * dt;
        } else if (px > target + 0.12f) {
            px -= spd * dt;
        }
        px = std::max(-half, std::min(half, px));
    };

    if (std::abs(ballVX_) < 0.01f && std::abs(ballVZ_) < 0.01f) {
        resetBall(false);
        ballVZ_ *= 0.7f;
        ballVX_ *= 0.7f;
    }

    track(playerX_, ballX_ + (ballVZ_ < 0.0f ? ballVX_ * 0.3f : 0.0f));
    track(player2X_, ballX_ + (ballVZ_ > 0.0f ? ballVX_ * 0.3f : 0.0f));

    ballX_ += ballVX_ * dt;
    ballZ_ += ballVZ_ * dt;

    // Soft ghost trail (raytraced spheres)
    for (int i = TRAIL_LEN - 1; i > 0; --i) {
        trailX_[i] = trailX_[i - 1];
        trailZ_[i] = trailZ_[i - 1];
    }
    trailX_[0] = ballX_;
    trailZ_[0] = ballZ_;

    float wall = FIELD_W * 0.5f - BALL_R;
    if (ballX_ < -wall) {
        ballX_ = -wall;
        ballVX_ = -ballVX_;
        audio_.playClank(0.9f, 0.18f);
        ballFlash_ = 0.08f;
    } else if (ballX_ > wall) {
        ballX_ = wall;
        ballVX_ = -ballVX_;
        audio_.playClank(0.9f, 0.18f);
        ballFlash_ = 0.08f;
    }

    float pz = 0.4f;
    if (ballZ_ - BALL_R < pz + PADDLE_D * 0.5f && ballVZ_ < 0.0f &&
        ballX_ + BALL_R > playerX_ - PADDLE_W * 0.5f &&
        ballX_ - BALL_R < playerX_ + PADDLE_W * 0.5f) {
        ballZ_ = pz + PADDLE_D * 0.5f + BALL_R;
        ballVZ_ = std::abs(ballVZ_);
        ballVX_ += (ballX_ - playerX_) * 1.5f;
        audio_.playClank(1.05f, 0.22f);
        ballFlash_ = 0.1f;
    }
    float az = FIELD_L - 0.4f;
    if (ballZ_ + BALL_R > az - PADDLE_D * 0.5f && ballVZ_ > 0.0f &&
        ballX_ + BALL_R > player2X_ - PADDLE_W * 0.5f &&
        ballX_ - BALL_R < player2X_ + PADDLE_W * 0.5f) {
        ballZ_ = az - PADDLE_D * 0.5f - BALL_R;
        ballVZ_ = -std::abs(ballVZ_);
        ballVX_ += (ballX_ - player2X_) * 1.5f;
        audio_.playClank(0.85f, 0.2f);
        ballFlash_ = 0.1f;
    }
    // Wrap if escapes
    if (ballZ_ < -2.0f || ballZ_ > FIELD_L + 2.0f) {
        resetBall(ballZ_ > FIELD_L * 0.5f);
        ballVZ_ *= 0.65f;
    }
}

void Game::startMatch() {
    playerScore_ = 0;
    aiScore_ = 0;
    playerX_ = 0.0f;
    player2X_ = 0.0f;
    state_ = GameState::Intro;
    introT_ = 0.0f;
    attractTime_ = 0.0f;
    replayTimer_ = 0.0f;
    serveTimer_ = 0.0f;
    for (int i = 0; i < TRAIL_LEN; ++i) {
        trailX_[i] = 0.0f;
        trailZ_[i] = FIELD_L * 0.5f;
    }
    queueServe(false);
}

void Game::enterAttract() {
    // Clean slate so scoreboard / banners do not linger from a paused or finished match
    state_ = GameState::Attract;
    attractTime_ = 0.0f;
    introT_ = 0.0f;
    replayTimer_ = 0.0f;
    serveTimer_ = 0.0f;
    gameOverTime_ = 0.0f;
    playerScore_ = 0;
    aiScore_ = 0;
    playerX_ = 0.0f;
    player2X_ = 0.0f;
    shake_ = 0.0f;
    ballFlash_ = 0.0f;
    resetBall(false);
    ballVZ_ *= 0.7f;
    ballVX_ *= 0.7f;
    for (int i = 0; i < TRAIL_LEN; ++i) {
        trailX_[i] = ballX_;
        trailZ_[i] = ballZ_;
    }
}

void Game::handleInput(float dt) {
    const Uint8* keys = SDL_GetKeyboardState(nullptr);

    // Volume always available
    if (keys[SDL_SCANCODE_EQUALS] || keys[SDL_SCANCODE_KP_PLUS]) {
        audio_.setMasterVolume(audio_.masterVolume() + 0.5f * dt);
    }
    if (keys[SDL_SCANCODE_MINUS] || keys[SDL_SCANCODE_KP_MINUS]) {
        audio_.setMasterVolume(audio_.masterVolume() - 0.5f * dt);
    }
    // Hold-to-ramp exposure
    if (keys[SDL_SCANCODE_8] || keys[SDL_SCANCODE_LEFTBRACKET]) {
        adjustExposure(-0.35f * dt);
    }
    if (keys[SDL_SCANCODE_9] || keys[SDL_SCANCODE_RIGHTBRACKET]) {
        adjustExposure(0.35f * dt);
    }

    if (state_ == GameState::Attract || state_ == GameState::Intro ||
        state_ == GameState::GameOver || state_ == GameState::Pause) {
        return;
    }

    // Play: P1 uses A/D (and arrows if single-player)
    float speed = 9.0f;
    float half = FIELD_W * 0.5f - PADDLE_W * 0.5f;

    if (keys[SDL_SCANCODE_A]) {
        playerX_ += speed * dt;
    }
    if (keys[SDL_SCANCODE_D]) {
        playerX_ -= speed * dt;
    }
    if (!twoPlayer_) {
        if (keys[SDL_SCANCODE_LEFT]) {
            playerX_ += speed * dt;
        }
        if (keys[SDL_SCANCODE_RIGHT]) {
            playerX_ -= speed * dt;
        }
    } else {
        // P2 (far paddle): arrow keys
        if (keys[SDL_SCANCODE_LEFT]) {
            player2X_ += speed * dt;
        }
        if (keys[SDL_SCANCODE_RIGHT]) {
            player2X_ -= speed * dt;
        }
        player2X_ = std::max(-half, std::min(half, player2X_));
    }
    playerX_ = std::max(-half, std::min(half, playerX_));
}

void Game::triggerShake(float amount) {
    if (amount > shake_) {
        shake_ = amount;
    }
    float angle = (static_cast<float>(std::rand() % 1000) / 1000.0f) * 6.2831853f;
    shakeOffsetX_ = std::cos(angle);
    shakeOffsetY_ = std::sin(angle) * 0.6f;
}

void Game::updateShake(float dt) {
    if (shake_ <= 0.001f) {
        shake_ = 0.0f;
        return;
    }
    shake_ *= std::exp(-dt * 8.0f);
    if (shake_ < 0.001f) {
        shake_ = 0.0f;
    }
}

void Game::update(float dt) {
    if (state_ == GameState::Play && replayTimer_ > 0.0f && slowmoReplay_) {
        animTime_ += dt * 0.4f;
    } else {
        animTime_ += dt;
    }
    updateShake(dt);

    if (ballFlash_ > 0.0f) {
        ballFlash_ -= dt;
        if (ballFlash_ < 0.0f) {
            ballFlash_ = 0.0f;
        }
    }

    if (state_ == GameState::Attract) {
        attractTime_ += dt;
        if (demoActive_) {
            updateDemo(dt);
        }
        return;
    }

    if (state_ == GameState::GameOver) {
        gameOverTime_ += dt;
        attractTime_ += dt * 0.5f; // keep some motion continuity if returning to attract
        return;
    }

    if (state_ == GameState::Intro) {
        attractTime_ += dt; // keep orbit angle advancing for fly-in continuity
        introT_ += dt / INTRO_DURATION;
        if (introT_ >= 1.0f) {
            introT_ = 1.0f;
            state_ = GameState::Play;
            audio_.playSoftThud(1.0f, 0.3f);
        }
        return;
    }

    if (state_ == GameState::Pause) {
        return;
    }

    // Post-goal replay beat (frozen ball, cinematic cam handled in updateCamera)
    if (state_ == GameState::Play && replayTimer_ > 0.0f) {
        replayTimer_ -= dt;
        if (replayTimer_ <= 0.0f) {
            replayTimer_ = 0.0f;
            queueServe(nextServeTowardPlayer_);
        }
        return;
    }

    // Play
    if (serveTimer_ > 0.0f) {
        serveTimer_ -= dt;
        if (serveTimer_ <= 0.0f) {
            serveTimer_ = 0.0f;
            resetBall(nextServeTowardPlayer_);
            audio_.playSoftThud(1.1f, 0.25f);
        }
        float half = FIELD_W * 0.5f - PADDLE_W * 0.5f;
        if (!twoPlayer_) {
            if (player2X_ < -0.1f) {
                player2X_ += 4.0f * dt;
            } else if (player2X_ > 0.1f) {
                player2X_ -= 4.0f * dt;
            }
            player2X_ = std::max(-half, std::min(half, player2X_));
        }
        return;
    }

    float target = ballX_;
    if (ballVZ_ > 0.0f) {
        float t = (FIELD_L - ballZ_) / std::max(0.1f, ballVZ_);
        target = ballX_ + ballVX_ * t * 0.7f;
    }
    float half = FIELD_W * 0.5f - PADDLE_W * 0.5f;
    if (!twoPlayer_) {
        float aiSpeed = aiSpeedForDifficulty();
        // Hard: stronger prediction weight
        float pred = (difficulty_ == Difficulty::Hard) ? 0.95f
                    : (difficulty_ == Difficulty::Easy) ? 0.35f : 0.7f;
        if (ballVZ_ > 0.0f) {
            float tt = (FIELD_L - ballZ_) / std::max(0.1f, ballVZ_);
            target = ballX_ + ballVX_ * tt * pred;
        }
        target = std::max(-half, std::min(half, target));
        if (player2X_ < target - 0.15f) {
            player2X_ += aiSpeed * dt;
        } else if (player2X_ > target + 0.15f) {
            player2X_ -= aiSpeed * dt;
        }
        player2X_ = std::max(-half, std::min(half, player2X_));
    }

    // Substep when the ball would travel more than ~half a paddle depth in one frame
    // (avoids tunneling through paddles at high speed or after a long frame hitch).
    {
        float spd = std::sqrt(ballVX_ * ballVX_ + ballVZ_ * ballVZ_);
        const float maxStepDist = PADDLE_D * 0.45f;
        int steps = 1;
        if (spd * dt > maxStepDist && spd > 1e-4f) {
            steps = static_cast<int>(std::ceil(spd * dt / maxStepDist));
            if (steps > 8) {
                steps = 8;
            }
        }
        const float sdt = dt / static_cast<float>(steps);
        bool scored = false;
        for (int step = 0; step < steps && !scored; ++step) {
            ballX_ += ballVX_ * sdt;
            ballZ_ += ballVZ_ * sdt;

            float wall = FIELD_W * 0.5f - BALL_R;
            if (ballX_ < -wall) {
                ballX_ = -wall;
                ballVX_ = -ballVX_;
                float speed = std::sqrt(ballVX_ * ballVX_ + ballVZ_ * ballVZ_);
                audio_.playClank(0.95f + std::min(0.25f, speed * 0.02f), 0.45f);
                triggerShake(0.04f);
                ballFlash_ = 0.12f;
            } else if (ballX_ > wall) {
                ballX_ = wall;
                ballVX_ = -ballVX_;
                float speed = std::sqrt(ballVX_ * ballVX_ + ballVZ_ * ballVZ_);
                audio_.playClank(0.95f + std::min(0.25f, speed * 0.02f), 0.45f);
                triggerShake(0.04f);
                ballFlash_ = 0.12f;
            }

            float pz = 0.4f;
            if (ballZ_ - BALL_R < pz + PADDLE_D * 0.5f && ballZ_ + BALL_R > pz - PADDLE_D * 0.5f &&
                ballVZ_ < 0.0f) {
                if (ballX_ + BALL_R > playerX_ - PADDLE_W * 0.5f &&
                    ballX_ - BALL_R < playerX_ + PADDLE_W * 0.5f) {
                    ballZ_ = pz + PADDLE_D * 0.5f + BALL_R;
                    ballVZ_ = -ballVZ_ * 1.05f;
                    float offset = (ballX_ - playerX_) / (PADDLE_W * 0.5f);
                    ballVX_ += offset * 2.5f;
                    float sp = std::sqrt(ballVX_ * ballVX_ + ballVZ_ * ballVZ_);
                    if (sp > 14.0f) {
                        ballVX_ *= 14.0f / sp;
                        ballVZ_ *= 14.0f / sp;
                    }
                    if (std::abs(ballVZ_) < 4.0f) {
                        ballVZ_ = (ballVZ_ >= 0.0f ? 4.0f : -4.0f);
                    }
                    if (std::abs(ballVX_) > std::abs(ballVZ_) * 1.6f) {
                        ballVX_ = (ballVX_ >= 0.0f ? 1.0f : -1.0f) * std::abs(ballVZ_) * 1.6f;
                    }
                    float pitch = 0.85f + std::min(0.4f, sp * 0.03f) + std::abs(offset) * 0.15f;
                    audio_.playClank(pitch, 0.75f);
                    triggerShake(0.12f + std::min(0.08f, sp * 0.008f));
                    ballFlash_ = 0.2f;
                }
            }

            float az = FIELD_L - 0.4f;
            if (ballZ_ + BALL_R > az - PADDLE_D * 0.5f && ballZ_ - BALL_R < az + PADDLE_D * 0.5f &&
                ballVZ_ > 0.0f) {
                if (ballX_ + BALL_R > player2X_ - PADDLE_W * 0.5f &&
                    ballX_ - BALL_R < player2X_ + PADDLE_W * 0.5f) {
                    ballZ_ = az - PADDLE_D * 0.5f - BALL_R;
                    ballVZ_ = -ballVZ_ * 1.05f;
                    float offset = (ballX_ - player2X_) / (PADDLE_W * 0.5f);
                    ballVX_ += offset * 2.5f;
                    float sp = std::sqrt(ballVX_ * ballVX_ + ballVZ_ * ballVZ_);
                    if (sp > 14.0f) {
                        ballVX_ *= 14.0f / sp;
                        ballVZ_ *= 14.0f / sp;
                    }
                    if (std::abs(ballVZ_) < 4.0f) {
                        ballVZ_ = (ballVZ_ >= 0.0f ? 4.0f : -4.0f);
                    }
                    if (std::abs(ballVX_) > std::abs(ballVZ_) * 1.6f) {
                        ballVX_ = (ballVX_ >= 0.0f ? 1.0f : -1.0f) * std::abs(ballVZ_) * 1.6f;
                    }
                    audio_.playClank(0.75f + std::min(0.35f, sp * 0.025f), 0.55f);
                    triggerShake(0.05f);
                    ballFlash_ = 0.18f;
                }
            }

            if (ballZ_ < -1.0f || ballZ_ > FIELD_L + 1.0f) {
                scored = true;
            }
        }

        // Soft ghost trail (once per frame after integration)
        for (int i = TRAIL_LEN - 1; i > 0; --i) {
            trailX_[i] = trailX_[i - 1];
            trailZ_[i] = trailZ_[i - 1];
        }
        trailX_[0] = ballX_;
        trailZ_[0] = ballZ_;
    }

    if (ballZ_ < -1.0f) {
        aiScore_++;
        audio_.playSoftThud(0.6f, 0.35f);
        triggerShake(0.06f);
        ballVX_ = 0.0f;
        ballVZ_ = 0.0f;
        ballZ_ = -0.5f;
        if (aiScore_ >= pointsToWin_) {
            state_ = GameState::GameOver;
            gameOverTime_ = 0.0f;
            playerWon_ = false;
            audio_.playClank(0.5f, 0.6f);
        } else {
            replayTimer_ = replayDuration_;
            replayTowardPlayer_ = true; // ball left near player; serve toward far? 
            // nextServeTowardPlayer_: false means ball goes toward AI (away from player)
            nextServeTowardPlayer_ = false;
        }
    } else if (ballZ_ > FIELD_L + 1.0f) {
        playerScore_++;
        audio_.playSoftThud(0.7f, 0.35f);
        triggerShake(0.06f);
        ballVX_ = 0.0f;
        ballVZ_ = 0.0f;
        ballZ_ = FIELD_L + 0.5f;
        if (playerScore_ >= pointsToWin_) {
            state_ = GameState::GameOver;
            gameOverTime_ = 0.0f;
            playerWon_ = true;
            audio_.playClank(1.4f, 0.7f);
        } else {
            replayTimer_ = replayDuration_;
            replayTowardPlayer_ = false; // exited near AI end
            nextServeTowardPlayer_ = true;
        }
    }
}

void Game::addScoreboard(Scene& scene) const {
    // Hanging board above midfield, facing down the long axis slightly
    const float boardY = WALL_H - 0.55f;
    const float boardZ = FIELD_L * 0.5f;
    const float boardW = 2.8f;
    const float boardH = 1.0f;
    const float boardD = 0.12f;

    // Panel (dark reflective)
    pushBox(scene,
            Vec3(-boardW * 0.5f, boardY - boardH * 0.5f, boardZ - boardD * 0.5f),
            Vec3(boardW * 0.5f, boardY + boardH * 0.5f, boardZ + boardD * 0.5f),
            Vec3(0.08f, 0.09f, 0.12f), 0.35f);

    // Frame rim
    const float rim = 0.06f;
    pushBox(scene,
            Vec3(-boardW * 0.5f - rim, boardY - boardH * 0.5f - rim, boardZ - boardD * 0.5f - 0.02f),
            Vec3(boardW * 0.5f + rim, boardY - boardH * 0.5f, boardZ + boardD * 0.5f + 0.02f),
            Vec3(0.55f, 0.5f, 0.35f), 0.5f);
    pushBox(scene,
            Vec3(-boardW * 0.5f - rim, boardY + boardH * 0.5f, boardZ - boardD * 0.5f - 0.02f),
            Vec3(boardW * 0.5f + rim, boardY + boardH * 0.5f + rim, boardZ + boardD * 0.5f + 0.02f),
            Vec3(0.55f, 0.5f, 0.35f), 0.5f);

    // Suspension cables to ceiling
    pushBox(scene, Vec3(-1.0f, boardY + boardH * 0.5f, boardZ - 0.03f),
            Vec3(-0.95f, WALL_H, boardZ + 0.03f), Vec3(0.4f, 0.4f, 0.45f), 0.3f);
    pushBox(scene, Vec3(0.95f, boardY + boardH * 0.5f, boardZ - 0.03f),
            Vec3(1.0f, WALL_H, boardZ + 0.03f), Vec3(0.4f, 0.4f, 0.45f), 0.3f);

    // Center divider bar
    pushBox(scene, Vec3(-0.04f, boardY - 0.35f, boardZ + boardD * 0.5f),
            Vec3(0.04f, boardY + 0.35f, boardZ + boardD * 0.5f + 0.04f),
            Vec3(0.7f, 0.7f, 0.75f), 0.4f);

    // Scores as 7-segment digits (player left, AI right) — front face of board
    const float digitZ = boardZ + boardD * 0.5f + 0.06f;
    const float digitY = boardY;
    Vec3 colPlayer(0.35f, 0.85f, 1.0f);
    Vec3 colAi(1.0f, 0.45f, 0.35f);
    if (isMatchPoint() && state_ == GameState::Play) {
        float g = 1.15f + 0.2f * (0.5f + 0.5f * std::sin(animTime_ * 6.0f));
        colPlayer = colPlayer * g;
        colAi = colAi * g;
    }

    int pTens = (playerScore_ / 10) % 10;
    int pOnes = playerScore_ % 10;
    int aTens = (aiScore_ / 10) % 10;
    int aOnes = aiScore_ % 10;

    // Always show ones; show tens if >= 10 or always for classic look
    glyphs::addDigit7(scene, -1.05f, digitY, digitZ, pTens, colPlayer);
    glyphs::addDigit7(scene, -0.45f, digitY, digitZ, pOnes, colPlayer);
    glyphs::addDigit7(scene, 0.45f, digitY, digitZ, aTens, colAi);
    glyphs::addDigit7(scene, 1.05f, digitY, digitZ, aOnes, colAi);
}






void Game::addGameOverBanner(Scene& scene) const {
    if (state_ != GameState::GameOver) {
        return;
    }
    const char* word = playerWon_ ? "WIN" : (twoPlayer_ ? "P2" : "LOSE");
    float pulse = 0.5f + 0.5f * std::sin(gameOverTime_ * 5.0f);
    float cell = 0.16f + 0.03f * pulse;
    float gap = 0.2f;
    int len = 0;
    for (const char* q = word; *q; ++q) {
        ++len;
    }
    float width = len * (5 * cell + gap) - gap;
    float startX = -width * 0.5f;
    float baseY = 2.1f + 0.15f * pulse;
    float z = FIELD_L * 0.5f - 1.2f;
    Vec3 col = playerWon_ ? Vec3(0.45f, 1.0f, 0.55f)
                          : Vec3(1.0f, 0.35f + 0.2f * pulse, 0.3f);
    glyphs::addWord(scene, word, startX, baseY, z, cell, gap, col, 0.65f);
}

void Game::addMatchPointBanner(Scene& scene) const {
    if (state_ != GameState::Play || !isMatchPoint()) {
        return;
    }
    float pulse = 0.5f + 0.5f * std::sin(animTime_ * 7.0f);
    float cell = 0.11f + 0.02f * pulse;
    float gap = 0.16f;
    const char* word = "MATCH";
    float width = 5 * (5 * cell + gap) - gap;
    float startX = -width * 0.5f;
    float baseY = 2.3f + 0.12f * pulse;
    float z = FIELD_L * 0.5f - 0.8f;
    Vec3 col(1.0f, 0.55f + 0.35f * pulse, 0.25f + 0.2f * pulse);
    glyphs::addWord(scene, word, startX, baseY, z, cell, gap, col, 0.6f);
}

void Game::addServeCountdown(Scene& scene) const {
    if (serveTimer_ <= 0.0f || state_ != GameState::Play) {
        return;
    }
    // Map remaining time to 3,2,1
    int n = 1;
    if (serveTimer_ > SERVE_DELAY * (2.0f / 3.0f)) {
        n = 3;
    } else if (serveTimer_ > SERVE_DELAY * (1.0f / 3.0f)) {
        n = 2;
    }
    const float pulse = 0.15f * std::sin(serveTimer_ * 12.0f);
    glyphs::addDigit7(scene, 0.0f, 1.4f + pulse, FIELD_L * 0.5f,
                  n, Vec3(1.0f, 0.95f, 0.4f));
}



void Game::addPauseBanner(Scene& scene) const {
    if (state_ != GameState::Pause) {
        return;
    }
    float pulse = 0.55f + 0.45f * std::sin(animTime_ * 3.5f);
    float cell = 0.13f;
    float gap = 0.16f;
    const char* word = "PAUSE";
    float width = 5 * (5 * cell + gap) - gap;
    float startX = -width * 0.5f;
    Vec3 col(0.95f, 0.9f * pulse, 0.35f + 0.25f * pulse);
    glyphs::addWord(scene, word, startX, 1.8f, FIELD_L * 0.35f, cell, gap, col, 0.5f);
}

void Game::addAttractHint(Scene& scene) const {
    if (state_ != GameState::Attract) {
        return;
    }
    float pulse = 0.5f + 0.5f * std::sin(attractTime_ * 3.0f);
    if (pulse < 0.28f) {
        return;
    }
    int phase = static_cast<int>(attractTime_ / 2.6f) % 4;
    const char* word = (phase == 0) ? "SPACE" : (phase == 1) ? "PONG" : (phase == 2) ? "OPTS" : "GO";
    float cell = (phase == 3) ? 0.14f : 0.09f;
    float gap = (phase == 3) ? 0.18f : 0.12f;
    int len = 0;
    for (const char* q = word; *q; ++q) {
        ++len;
    }
    float width = len * (5 * cell + gap) - gap;
    float startX = -width * 0.5f;
    Vec3 col = (phase == 1) ? Vec3(0.55f + 0.3f * pulse, 0.85f, 1.0f)
             : (phase == 2) ? Vec3(0.75f, 0.7f + 0.2f * pulse, 1.0f)
             : (phase == 3) ? Vec3(0.5f + 0.4f * pulse, 1.0f, 0.45f)
                            : Vec3(0.85f + 0.15f * pulse, 0.9f, 0.55f + 0.3f * pulse);
    glyphs::addWord(scene, word, startX, 1.15f, 2.4f, cell, gap, col, 0.4f);
}

void Game::addTitleGeometry(Scene& scene) const {
    float cell = 0.14f;
    float gap = 0.22f;
    const char* word = "KUGEL";
    float width = 5 * (5 * cell + gap) - gap;
    float startX = -width * 0.5f;
    glyphs::addWord(scene, word, startX, 2.0f, 3.5f, cell, gap,
                    Vec3(0.95f, 0.75f, 0.25f), 0.55f);
}

void Game::addOptionsGeometry(Scene& scene) const {
    // Three difficulty pillars along the near-left wall (visual options readout)
    const float baseZ = 2.2f;
    const float baseX = -FIELD_W * 0.5f + 0.55f;
    const float heights[3] = {0.7f, 1.2f, 1.85f};
    const Vec3 colors[3] = {
        Vec3(0.4f, 0.85f, 0.5f),
        Vec3(0.9f, 0.85f, 0.35f),
        Vec3(0.95f, 0.35f, 0.3f),
    };
    int sel = static_cast<int>(difficulty_);
    for (int i = 0; i < 3; ++i) {
        float h = heights[i];
        bool on = (i == sel);
        Vec3 c = colors[i] * (on ? 1.15f : 0.35f);
        float ref = on ? 0.55f : 0.15f;
        pushBox(scene,
                Vec3(baseX - 0.18f, 0.02f, baseZ + i * 0.55f - 0.15f),
                Vec3(baseX + 0.18f, h, baseZ + i * 0.55f + 0.15f),
                c, ref);
        if (on) {
            // Selection orb on top
            Sphere s;
            s.center = Vec3(baseX, h + 0.22f, baseZ + i * 0.55f);
            s.radius = 0.18f;
            s.color = colors[i];
            s.reflectivity = 0.8f;
            scene.spheres.push_back(s);
        }
    }

    // Camera-mode markers near near-right wall: three small pads
    const float cx = FIELD_W * 0.5f - 0.55f;
    const float cz = 2.0f;
    int cam = static_cast<int>(cameraMode_);
    for (int i = 0; i < 3; ++i) {
        bool on = (i == cam);
        Vec3 c = on ? Vec3(0.6f, 0.85f, 1.0f) : Vec3(0.15f, 0.18f, 0.22f);
        pushBox(scene,
                Vec3(cx - 0.2f, 0.02f, cz + i * 0.5f - 0.12f),
                Vec3(cx + 0.2f, on ? 0.35f : 0.12f, cz + i * 0.5f + 0.12f),
                c, on ? 0.5f : 0.1f);
    }

    // 1P / 2P marker spheres floating mid-near
    {
        Sphere s;
        s.center = Vec3(0.0f, 1.6f, 1.2f);
        s.radius = twoPlayer_ ? 0.28f : 0.2f;
        s.color = twoPlayer_ ? Vec3(1.0f, 0.7f, 0.3f) : Vec3(0.5f, 0.55f, 0.7f);
        s.reflectivity = 0.75f;
        scene.spheres.push_back(s);
    }

    // Points-to-win as a small digit on the floor front (always visible)
    glyphs::addDigit7(scene, 0.0f, 0.55f, 1.0f, pointsToWin_ >= 10 ? pointsToWin_ / 10 : 0,
                  Vec3(0.9f, 0.9f, 0.5f));
    glyphs::addDigit7(scene, 0.55f, 0.55f, 1.0f, pointsToWin_ % 10, Vec3(0.9f, 0.9f, 0.5f));
}

void Game::buildScene() {
    scene_.clear();

    Vec3 colP, colF;
    paddleColors(colP, colF);
    Vec3 themeMix = (colP + colF) * 0.5f;

    // Checker floor shifts with theme (light tile blended toward theme, dark stays cool)
    Plane floor;
    floor.point = Vec3(0, 0, 0);
    floor.normal = Vec3(0, 1, 0);
    floor.checker = true;
    floor.colorA = Vec3(0.82f, 0.82f, 0.80f) * 0.55f + themeMix * 0.45f;
    floor.colorB = Vec3(0.12f, 0.12f, 0.14f) * 0.7f + themeMix * 0.15f;
    floor.scale = 1.2f;
    floor.reflectivity = 0.15f;
    scene_.planes.push_back(floor);

    Plane ceil;
    ceil.point = Vec3(0, WALL_H, 0);
    ceil.normal = Vec3(0, -1, 0);
    ceil.colorA = Vec3(0.08f, 0.08f, 0.1f) * 0.85f + themeMix * 0.08f;
    ceil.reflectivity = 0.05f;
    scene_.planes.push_back(ceil);

    Plane back;
    back.point = Vec3(0, 0, FIELD_L + 0.5f);
    back.normal = Vec3(0, 0, -1);
    back.checker = true;
    back.colorA = Vec3(0.35f, 0.18f, 0.22f) * 0.5f + colF * 0.5f;
    back.colorB = Vec3(0.2f, 0.1f, 0.12f) * 0.55f + colF * 0.25f;
    back.scale = 0.8f;
    back.reflectivity = 0.1f;
    scene_.planes.push_back(back);

    Plane left;
    left.point = Vec3(-FIELD_W * 0.5f - 0.1f, 0, 0);
    left.normal = Vec3(1, 0, 0);
    left.checker = true;
    left.colorA = Vec3(0.18f, 0.28f, 0.4f) * 0.45f + colP * 0.55f;
    left.colorB = Vec3(0.1f, 0.15f, 0.22f) * 0.55f + colP * 0.2f;
    left.scale = 0.9f;
    left.reflectivity = 0.08f;
    scene_.planes.push_back(left);

    Plane right = left;
    right.point = Vec3(FIELD_W * 0.5f + 0.1f, 0, 0);
    right.normal = Vec3(-1, 0, 0);
    right.colorA = Vec3(0.18f, 0.28f, 0.4f) * 0.45f + colF * 0.55f;
    right.colorB = Vec3(0.1f, 0.15f, 0.22f) * 0.55f + colF * 0.2f;
    scene_.planes.push_back(right);

    // colP / colF already set for theme floor/walls above
    paddleColors(colP, colF);

    Box playerPad;
    playerPad.minb = Vec3(playerX_ - PADDLE_W * 0.5f, 0.05f, 0.25f);
    playerPad.maxb = Vec3(playerX_ + PADDLE_W * 0.5f, 0.05f + PADDLE_H, 0.25f + PADDLE_D);
    playerPad.color = colP;
    playerPad.reflectivity = 0.35f;
    scene_.boxes.push_back(playerPad);

    Box aiPad;
    aiPad.minb = Vec3(player2X_ - PADDLE_W * 0.5f, 0.05f, FIELD_L - 0.25f - PADDLE_D);
    aiPad.maxb = Vec3(player2X_ + PADDLE_W * 0.5f, 0.05f + PADDLE_H, FIELD_L - 0.25f);
    aiPad.color = colF;
    aiPad.reflectivity = 0.3f;
    scene_.boxes.push_back(aiPad);

    addScoreboard(scene_);
    // Options markers live on the near floor — hide them during play so the
    // court stays clear (still visible in attract / pause for settings).
    if (state_ == GameState::Attract || state_ == GameState::Pause) {
        addOptionsGeometry(scene_);
    }
    if (state_ == GameState::Attract || state_ == GameState::GameOver) {
        addTitleGeometry(scene_);
    }
    addAttractHint(scene_);
    addPauseBanner(scene_);
    addServeCountdown(scene_);
    addMatchPointBanner(scene_);
    addGameOverBanner(scene_);

    Sphere ball;
    ball.center = Vec3(ballX_, BALL_R + 0.02f, ballZ_);
    ball.radius = BALL_R;
    if (ballFlash_ > 0.0f) {
        float k = std::min(1.0f, ballFlash_ / 0.2f);
        ball.color = Vec3(1.0f, 1.0f, 1.0f) * (0.95f + 0.8f * k);
        ball.reflectivity = 0.85f - 0.25f * k;
    } else {
        ball.color = Vec3(0.95f, 0.95f, 1.0f);
        ball.reflectivity = 0.85f;
    }
    scene_.spheres.push_back(ball);

    const bool showTrail =
        (state_ == GameState::Play && serveTimer_ <= 0.0f && replayTimer_ <= 0.0f) ||
        (state_ == GameState::Attract && demoActive_);
    if (showTrail) {
        for (int i = 1; i < TRAIL_LEN; ++i) {
            Sphere g;
            float fade = 1.0f - static_cast<float>(i) / static_cast<float>(TRAIL_LEN);
            g.center = Vec3(trailX_[i], BALL_R * (0.7f + 0.2f * fade), trailZ_[i]);
            g.radius = BALL_R * (0.55f + 0.25f * fade);
            g.color = Vec3(0.7f, 0.8f, 1.0f) * (0.35f * fade);
            g.reflectivity = 0.35f * fade;
            scene_.spheres.push_back(g);
        }
    }

    // Ceiling-mounted decorative orbs (theme-tinted).
    // Kept off walls and floor so the playfield stays clear for play.
    // Y is set just under the ceiling plane; XY spread avoids the scoreboard.
    struct DecoSpec {
        Vec3 center; // y overwritten to sit under the ceiling
        float radius;
        Vec3 color;
        float reflectivity;
    };
    DecoSpec decos[] = {
        {Vec3(-2.6f, 0.0f, 2.5f), 0.30f, Vec3(0.9f, 0.55f, 0.2f), 0.65f},
        {Vec3(2.5f, 0.0f, 3.2f), 0.26f, Vec3(0.3f, 0.7f, 0.9f), 0.65f},
        {Vec3(-1.8f, 0.0f, 5.5f), 0.28f, Vec3(0.85f, 0.3f, 0.45f), 0.7f},
        {Vec3(2.2f, 0.0f, 6.0f), 0.24f, Vec3(0.95f, 0.85f, 0.35f), 0.55f},
        {Vec3(-2.4f, 0.0f, 10.5f), 0.32f, Vec3(0.4f, 0.75f, 0.95f), 0.6f},
        {Vec3(2.3f, 0.0f, 11.2f), 0.28f, Vec3(0.7f, 0.4f, 0.85f), 0.7f},
        {Vec3(-1.5f, 0.0f, 14.0f), 0.30f, Vec3(0.95f, 0.55f, 0.4f), 0.65f},
        {Vec3(1.6f, 0.0f, 13.5f), 0.26f, Vec3(0.55f, 0.9f, 0.7f), 0.6f},
    };
    for (auto& d : decos) {
        d.center.y = WALL_H - d.radius - 0.08f;
        // Blend base deco color toward theme palette
        d.color = d.color * 0.35f + themeMix * 0.4f + colP * 0.25f;
        Sphere s;
        s.center = d.center;
        s.radius = d.radius;
        s.color = d.color;
        s.reflectivity = d.reflectivity;
        scene_.spheres.push_back(s);
    }

    scene_.lightPos = Vec3(0.0f, WALL_H - 0.5f, FIELD_L * 0.45f);
    scene_.maxBounces = maxBounces_;
    scene_.shadowSamples = shadowSamples_;
    scene_.exposure = exposure_;

    if (state_ == GameState::Play && isMatchPoint()) {
        // Warm, pulsing key light on match point
        float phase = animTime_ * 5.5f;
        float pulse = 0.55f + 0.45f * (0.5f + 0.5f * std::sin(phase));
        scene_.lightColor = Vec3(1.1f + 0.6f * pulse, 0.8f + 0.2f * pulse, 0.55f + 0.15f * pulse);
        scene_.ambient = Vec3(0.07f + 0.05f * pulse, 0.05f, 0.04f);
        scene_.lightPos = Vec3(0.0f, WALL_H - 0.25f, FIELD_L * 0.5f);
    } else if (state_ == GameState::GameOver) {
        float pulse = 0.5f + 0.5f * std::sin(gameOverTime_ * 4.0f);
        if (playerWon_) {
            scene_.lightColor = Vec3(0.85f + 0.4f * pulse, 1.15f + 0.25f * pulse, 0.9f);
            scene_.exposure = exposure_ * (1.05f + 0.15f * pulse);
        } else {
            scene_.lightColor = Vec3(1.25f, 0.4f + 0.25f * pulse, 0.35f);
            scene_.exposure = exposure_ * (0.9f + 0.1f * pulse);
        }
        scene_.ambient = Vec3(0.1f, 0.1f, 0.12f);
    } else if (state_ == GameState::Pause) {
        scene_.lightColor = Vec3(0.7f, 0.7f, 0.75f);
        scene_.ambient = Vec3(0.08f, 0.08f, 0.1f);
        scene_.exposure = exposure_ * 0.85f;
    } else {
        switch (theme_) {
        case Theme::Neon:
            scene_.lightColor = Vec3(1.05f, 1.2f, 1.25f);
            scene_.ambient = Vec3(0.08f, 0.12f, 0.16f);
            break;
        case Theme::Ice:
            scene_.lightColor = Vec3(1.15f, 1.2f, 1.35f);
            scene_.ambient = Vec3(0.12f, 0.14f, 0.18f);
            break;
        case Theme::Ember:
            scene_.lightColor = Vec3(1.35f, 1.05f, 0.85f);
            scene_.ambient = Vec3(0.14f, 0.1f, 0.08f);
            break;
        default:
            scene_.lightColor = Vec3(1.2f, 1.15f, 1.05f);
            scene_.ambient = Vec3(0.12f, 0.12f, 0.15f);
            break;
        }
    }
}

void Game::updateCamera(float dt) {
    (void)dt;
    const Vec3 up(0, 1, 0);

    auto orbitCam = [&](float time) {
        float ang = time * 0.35f;
        float rad = 11.0f;
        float height = 4.5f + 0.4f * std::sin(time * 0.5f);
        Vec3 pos(std::sin(ang) * rad, height, FIELD_L * 0.5f + std::cos(ang) * rad * 0.85f);
        Vec3 look(0.0f, 0.8f, FIELD_L * 0.5f);
        camera_.set(pos, look, up, 55.0f);
    };

    auto paddleCam = [&](float sx, float sy) {
        Vec3 camPos(playerX_ + sx, 1.1f + sy, -0.6f);
        Vec3 lookAt(playerX_ * 0.3f + sx * 0.3f, 0.6f + sy * 0.2f, FIELD_L * 0.55f);
        camera_.set(camPos, lookAt, up, 70.0f);
    };

    if (state_ == GameState::Attract) {
        orbitCam(attractTime_);
        return;
    }

    if (state_ == GameState::GameOver) {
        // Victory/defeat beat: faster higher orbit, slight bob toward scoreboard
        float t = gameOverTime_;
        float ang = t * 0.85f;
        float rad = 9.0f + 1.5f * std::sin(t * 1.2f);
        float height = 5.0f + 0.8f * std::sin(t * 2.0f);
        Vec3 pos(std::sin(ang) * rad, height, FIELD_L * 0.5f + std::cos(ang) * rad * 0.7f);
        Vec3 look(0.0f, WALL_H - 1.0f, FIELD_L * 0.5f); // look at scoreboard
        camera_.set(pos, look, up, 52.0f);
        return;
    }

    if (state_ == GameState::Intro) {
        // Smoothstep fly-in from orbit pose to paddle cam
        float t = introT_;
        t = t * t * (3.0f - 2.0f * t);
        float ang = attractTime_ * 0.35f;
        float rad = 11.0f;
        Vec3 orbitPos(std::sin(ang) * rad, 4.5f, FIELD_L * 0.5f + std::cos(ang) * rad * 0.85f);
        Vec3 orbitLook(0.0f, 0.8f, FIELD_L * 0.5f);
        Vec3 padPos(playerX_, 1.1f, -0.6f);
        Vec3 padLook(playerX_ * 0.3f, 0.6f, FIELD_L * 0.55f);
        Vec3 pos = orbitPos * (1.0f - t) + padPos * t;
        Vec3 look = orbitLook * (1.0f - t) + padLook * t;
        float fov = 55.0f * (1.0f - t) + 70.0f * t;
        camera_.set(pos, look, up, fov);
        return;
    }

    if (state_ == GameState::Play && replayTimer_ > 0.0f) {
        float dur = std::max(0.2f, replayDuration_);
        float t = 1.0f - (replayTimer_ / dur);
        t = t * t * (3.0f - 2.0f * t);
        float goalZ = replayTowardPlayer_ ? 0.5f : FIELD_L - 0.5f;
        float spin = slowmoReplay_ ? 0.35f : 0.85f;
        float ang = animTime_ * spin;
        float rad = slowmoReplay_ ? 5.2f : 4.5f;
        Vec3 pos(std::sin(ang) * rad, 2.6f + 0.5f * t, goalZ + (replayTowardPlayer_ ? 3.8f : -3.8f));
        Vec3 look(ballX_ * 0.3f, BALL_R + 0.2f, ballZ_);
        camera_.set(pos, look, up, slowmoReplay_ ? 48.0f : 55.0f);
        return;
    }

    // Play / Pause: selected camera mode + shake
    float sx = shakeOffsetX_ * shake_;
    float sy = shakeOffsetY_ * shake_;
    if (shake_ > 0.001f && cameraMode_ == CameraMode::Paddle) {
        sx += ((std::rand() % 100) / 100.0f - 0.5f) * shake_ * 0.35f;
        sy += ((std::rand() % 100) / 100.0f - 0.5f) * shake_ * 0.25f;
    }

    if (cameraMode_ == CameraMode::High) {
        Vec3 camPos(playerX_ * 0.4f + sx * 0.3f, 3.2f + sy * 0.2f, -2.5f);
        Vec3 lookAt(0.0f, 0.4f, FIELD_L * 0.45f);
        camera_.set(camPos, lookAt, up, 58.0f);
    } else if (cameraMode_ == CameraMode::Sideline) {
        Vec3 camPos(FIELD_W * 0.5f + 3.5f, 2.4f, FIELD_L * 0.5f);
        Vec3 lookAt(0.0f, 0.5f, FIELD_L * 0.5f);
        camera_.set(camPos, lookAt, up, 50.0f);
    } else {
        paddleCam(sx, sy);
    }
}

void Game::presentCpu() {
    SDL_UpdateTexture(texture_, nullptr, framebuffer_,
                      fbW_ * static_cast<int>(sizeof(uint32_t)));
    SDL_RenderClear(sdlRenderer_);
    SDL_RenderCopy(sdlRenderer_, texture_, nullptr, nullptr);
    SDL_RenderPresent(sdlRenderer_);
}

void Game::updateHud() {
    const char* mode = backend_ == RenderBackend::Gpu ? "GPU" : "CPU";
    const char* mute = audio_.muted() ? " MUTE" : "";
    const char* st = "PLAY";
    switch (state_) {
    case GameState::Attract:
        st = "ATTRACT — SPACE start";
        break;
    case GameState::Intro:
        st = "INTRO";
        break;
    case GameState::Pause:
        st = "PAUSE";
        break;
    case GameState::GameOver:
        if (playerScore_ >= pointsToWin_) {
            st = "YOU WIN — SPACE";
        } else if (twoPlayer_) {
            st = "P2 WINS — SPACE";
        } else {
            st = "AI WINS — SPACE";
        }
        break;
    default:
        break;
    }
    const char* diff = "NML";
    switch (difficulty_) {
    case Difficulty::Easy: diff = "EASY"; break;
    case Difficulty::Hard: diff = "HARD"; break;
    default: break;
    }
    const char* cam = "PAD";
    switch (cameraMode_) {
    case CameraMode::High: cam = "HIGH"; break;
    case CameraMode::Sideline: cam = "SIDE"; break;
    default: break;
    }
    char buf[320];
    std::snprintf(buf, sizeof(buf),
                  "KugelMatch " KUGELMATCH_VERSION_STRING " [%s]%s | %s | %s %s | to%d | %s | %s refl%d sh%d sc%.2f exp%.2f %s | %.0fFPS",
                  mode, mute, st, diff, twoPlayer_ ? "2P" : "1P", pointsToWin_, cam,
                  qualityLabel(),
                  maxBounces_, shadowSamples_, cpuScale_, exposure_,
                  vsync_ ? "VSYNC" : "FREE", fpsSmooth_);
    SDL_SetWindowTitle(window_, buf);
}

void Game::toggleFullscreen() {
    Uint32 flags = SDL_GetWindowFlags(window_);
    if (flags & (SDL_WINDOW_FULLSCREEN | SDL_WINDOW_FULLSCREEN_DESKTOP)) {
        SDL_SetWindowFullscreen(window_, 0);
    } else {
        SDL_SetWindowFullscreen(window_, SDL_WINDOW_FULLSCREEN_DESKTOP);
    }
    int w = 0, h = 0;
    SDL_GetWindowSize(window_, &w, &h);
    if (backend_ == RenderBackend::Gpu) {
        gpuRt_.onResize(w, h);
    } else {
        ensureCpuFramebuffer(std::max(1, w), std::max(1, h));
    }
}

void Game::run() {
    Uint64 freq = SDL_GetPerformanceFrequency();
    Uint64 last = SDL_GetPerformanceCounter();

    while (running_) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                running_ = false;
            }
            if (e.type == SDL_WINDOWEVENT &&
                (e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED ||
                 e.window.event == SDL_WINDOWEVENT_RESIZED)) {
                int w = e.window.data1;
                int h = e.window.data2;
                if (backend_ == RenderBackend::Gpu) {
                    gpuRt_.onResize(w, h);
                } else {
                    ensureCpuFramebuffer(std::max(1, w), std::max(1, h));
                }
            }
            if (e.type == SDL_KEYUP) {
                switch (e.key.keysym.sym) {
                case SDLK_8:
                case SDLK_9:
                case SDLK_LEFTBRACKET:
                case SDLK_RIGHTBRACKET:
                case SDLK_EQUALS:
                case SDLK_KP_PLUS:
                case SDLK_MINUS:
                case SDLK_KP_MINUS:
                    persistConfig();
                    break;
                default:
                    break;
                }
            }
            if (e.type == SDL_KEYDOWN) {
                const SDL_Keymod mods = SDL_GetModState();
                switch (e.key.keysym.sym) {
                case SDLK_ESCAPE:
                    if (state_ == GameState::Play) {
                        state_ = GameState::Pause;
                    } else if (state_ == GameState::Pause) {
                        enterAttract();
                    } else if (state_ == GameState::Attract) {
                        running_ = false;
                    } else if (state_ == GameState::GameOver) {
                        enterAttract();
                    } else if (state_ == GameState::Intro) {
                        state_ = GameState::Play;
                        introT_ = 1.0f;
                    }
                    break;
                case SDLK_p:
                    if (state_ == GameState::Play) {
                        state_ = GameState::Pause;
                    } else if (state_ == GameState::Pause) {
                        state_ = GameState::Play;
                    }
                    break;
                case SDLK_m:
                    audio_.toggleMute();
                    persistConfig();
                    break;
                case SDLK_F11:
                    toggleFullscreen();
                    break;
                case SDLK_F8:
                    if (!switchBackend(backend_ == RenderBackend::Gpu ? RenderBackend::Cpu
                                                                      : RenderBackend::Gpu)) {
                        std::fprintf(stderr, "Backend switch failed.\n");
                    } else {
                        persistConfig();
                    }
                    break;
                case SDLK_RETURN:
                case SDLK_KP_ENTER:
                    if (mods & KMOD_ALT) {
                        toggleFullscreen();
                    } else if (state_ == GameState::Attract || state_ == GameState::GameOver) {
                        startMatch();
                    } else if (state_ == GameState::Intro) {
                        state_ = GameState::Play;
                        introT_ = 1.0f;
                    }
                    break;
                case SDLK_SPACE:
                    if (state_ == GameState::Attract || state_ == GameState::GameOver) {
                        startMatch();
                    } else if (state_ == GameState::Intro) {
                        state_ = GameState::Play;
                        introT_ = 1.0f;
                    } else if (state_ == GameState::Play && replayTimer_ > 0.0f) {
                        replayTimer_ = 0.0f;
                        queueServe(nextServeTowardPlayer_);
                        audio_.playSoftThud(1.0f, 0.2f);
                    }
                    break;
                case SDLK_r:
                    if (state_ == GameState::Play || state_ == GameState::Pause) {
                        startMatch();
                    }
                    break;
                case SDLK_1:
                    cycleDifficulty();
                    break;
                case SDLK_2:
                    cyclePointsToWin();
                    break;
                case SDLK_3:
                    cycleCameraMode();
                    break;
                case SDLK_4:
                    toggleTwoPlayer();
                    break;
                case SDLK_5:
                    cycleBounces();
                    break;
                case SDLK_7:
                    cycleShadowSamples();
                    break;
                case SDLK_0:
                    cycleTheme();
                    break;
                case SDLK_q:
                case SDLK_F10:
                    cycleQuality();
                    break;
                case SDLK_F9:
                    slowmoReplay_ = !slowmoReplay_;
                    audio_.playSoftThud(slowmoReplay_ ? 0.75f : 1.1f, 0.2f);
                    persistConfig();
                    break;
                case SDLK_6:
                    vsync_ = !vsync_;
                    applyVsync();
                    // CPU vsync requires renderer recreate
                    if (backend_ == RenderBackend::Cpu) {
                        int winW = 0, winH = 0;
                        SDL_GetWindowSize(window_, &winW, &winH);
                        if (texture_) {
                            SDL_DestroyTexture(texture_);
                            texture_ = nullptr;
                        }
                        if (sdlRenderer_) {
                            SDL_DestroyRenderer(sdlRenderer_);
                            sdlRenderer_ = nullptr;
                        }
                        sdlRenderer_ = SDL_CreateRenderer(
                            window_, -1,
                            SDL_RENDERER_ACCELERATED | (vsync_ ? SDL_RENDERER_PRESENTVSYNC : 0));
                        if (!sdlRenderer_) {
                            sdlRenderer_ = SDL_CreateRenderer(window_, -1, 0);
                        }
                        fbW_ = fbH_ = 0;
                        // Window size — ensureCpuFramebuffer applies scale itself
                        ensureCpuFramebuffer(std::max(1, winW), std::max(1, winH));
                    }
                    audio_.playSoftThud(vsync_ ? 1.0f : 0.7f, 0.2f);
                    persistConfig();
                    break;
                default:
                    break;
                }
            }
        }

        Uint64 now = SDL_GetPerformanceCounter();
        float dt = static_cast<float>(now - last) / static_cast<float>(freq);
        last = now;
        if (dt > 0.05f) {
            dt = 0.05f;
        }
        if (dt > 1e-6f) {
            float inst = 1.0f / dt;
            fpsSmooth_ = fpsSmooth_ > 1.0f ? (fpsSmooth_ * 0.9f + inst * 0.1f) : inst;
        }

        handleInput(dt);
        update(dt);
        buildScene();
        updateCamera(dt);

        if (backend_ == RenderBackend::Gpu) {
            gpuRt_.render(scene_, camera_);
            gpuRt_.present();
        } else {
            int winW = 0, winH = 0;
            SDL_GetWindowSize(window_, &winW, &winH);
            winW = std::max(1, winW);
            winH = std::max(1, winH);
            if (!ensureCpuFramebuffer(winW, winH)) {
                running_ = false;
                break;
            }
            // Framebuffer size may differ from window (cpuScale / max clamp)
            cpuRt_.render(scene_, camera_, framebuffer_, fbW_, fbH_);
            presentCpu();
        }
        updateHud();

        // Frame-time cap when vsync is off (optional target_fps)
        if (!vsync_ && targetFps_ > 0) {
            Uint64 after = SDL_GetPerformanceCounter();
            float elapsed = static_cast<float>(after - now) / static_cast<float>(freq);
            float target = 1.0f / static_cast<float>(targetFps_);
            if (elapsed < target) {
                Uint32 ms = static_cast<Uint32>((target - elapsed) * 1000.0f);
                if (ms > 0) {
                    SDL_Delay(ms);
                }
            }
        }
    }
}
