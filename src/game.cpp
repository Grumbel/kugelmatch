// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2026 Ingo Ruhnke <grumbel@gmail.com>
#include "game.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

namespace {

// 7-segment masks for digits 0-9 (bits A F B G E C D — standard-ish)
// Segment order: A(top) B(ur) C(lr) D(bot) E(ll) F(ul) G(mid)
constexpr int kSegMask[10] = {
    0b1110111, // 0 ABCDEF
    0b0010010, // 1 BC
    0b1011101, // 2 ABDEG
    0b1011011, // 3 ABCDG
    0b0111010, // 4 BCFG
    0b1101011, // 5 ACDFG
    0b1101111, // 6 ACDEFG
    0b1010010, // 7 ABC
    0b1111111, // 8
    0b1111011, // 9 ABCDFG
};

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
    } else {
        sdlRenderer_ = SDL_CreateRenderer(
            window_, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
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

    playerX_ = 0.0f;
    aiX_ = 0.0f;
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
}

void Game::startMatch() {
    playerScore_ = 0;
    aiScore_ = 0;
    playerX_ = 0.0f;
    aiX_ = 0.0f;
    state_ = GameState::Intro;
    introT_ = 0.0f;
    attractTime_ = 0.0f;
    queueServe(false);
}

void Game::handleInput(float dt) {
    const Uint8* keys = SDL_GetKeyboardState(nullptr);

    if (state_ == GameState::Attract) {
        // movement keys ignored; wait for start
        return;
    }

    if (state_ == GameState::Intro) {
        return;
    }

    if (state_ == GameState::GameOver) {
        return;
    }

    if (state_ == GameState::Pause) {
        return;
    }

    // Play
    float speed = 9.0f;
    if (keys[SDL_SCANCODE_LEFT] || keys[SDL_SCANCODE_A]) {
        playerX_ += speed * dt;
    }
    if (keys[SDL_SCANCODE_RIGHT] || keys[SDL_SCANCODE_D]) {
        playerX_ -= speed * dt;
    }
    float half = FIELD_W * 0.5f - PADDLE_W * 0.5f;
    playerX_ = std::max(-half, std::min(half, playerX_));

    if (keys[SDL_SCANCODE_EQUALS] || keys[SDL_SCANCODE_KP_PLUS]) {
        audio_.setMasterVolume(audio_.masterVolume() + 0.5f * dt);
    }
    if (keys[SDL_SCANCODE_MINUS] || keys[SDL_SCANCODE_KP_MINUS]) {
        audio_.setMasterVolume(audio_.masterVolume() - 0.5f * dt);
    }
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
    updateShake(dt);

    if (ballFlash_ > 0.0f) {
        ballFlash_ -= dt;
        if (ballFlash_ < 0.0f) {
            ballFlash_ = 0.0f;
        }
    }

    if (state_ == GameState::Attract) {
        attractTime_ += dt;
        // Gentle demo: AI-ish ball bounce without scoring pressure
        return;
    }

    if (state_ == GameState::Intro) {
        introT_ += dt / INTRO_DURATION;
        if (introT_ >= 1.0f) {
            introT_ = 1.0f;
            state_ = GameState::Play;
            audio_.playSoftThud(1.0f, 0.3f);
        }
        return;
    }

    if (state_ == GameState::Pause || state_ == GameState::GameOver) {
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
        if (aiX_ < -0.1f) {
            aiX_ += 4.0f * dt;
        } else if (aiX_ > 0.1f) {
            aiX_ -= 4.0f * dt;
        }
        aiX_ = std::max(-half, std::min(half, aiX_));
        return;
    }

    float target = ballX_;
    if (ballVZ_ > 0.0f) {
        float t = (FIELD_L - ballZ_) / std::max(0.1f, ballVZ_);
        target = ballX_ + ballVX_ * t * 0.7f;
    }
    float aiSpeed = 7.5f;
    if (aiX_ < target - 0.15f) {
        aiX_ += aiSpeed * dt;
    } else if (aiX_ > target + 0.15f) {
        aiX_ -= aiSpeed * dt;
    }
    float half = FIELD_W * 0.5f - PADDLE_W * 0.5f;
    aiX_ = std::max(-half, std::min(half, aiX_));

    ballX_ += ballVX_ * dt;
    ballZ_ += ballVZ_ * dt;

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
            float pitch = 0.85f + std::min(0.4f, sp * 0.03f) + std::abs(offset) * 0.15f;
            audio_.playClank(pitch, 0.75f);
            triggerShake(0.12f + std::min(0.08f, sp * 0.008f));
            ballFlash_ = 0.2f;
        }
    }

    float az = FIELD_L - 0.4f;
    if (ballZ_ + BALL_R > az - PADDLE_D * 0.5f && ballZ_ - BALL_R < az + PADDLE_D * 0.5f &&
        ballVZ_ > 0.0f) {
        if (ballX_ + BALL_R > aiX_ - PADDLE_W * 0.5f &&
            ballX_ - BALL_R < aiX_ + PADDLE_W * 0.5f) {
            ballZ_ = az - PADDLE_D * 0.5f - BALL_R;
            ballVZ_ = -ballVZ_ * 1.05f;
            float offset = (ballX_ - aiX_) / (PADDLE_W * 0.5f);
            ballVX_ += offset * 2.5f;
            float sp = std::sqrt(ballVX_ * ballVX_ + ballVZ_ * ballVZ_);
            if (sp > 14.0f) {
                ballVX_ *= 14.0f / sp;
                ballVZ_ *= 14.0f / sp;
            }
            audio_.playClank(0.75f + std::min(0.35f, sp * 0.025f), 0.55f);
            triggerShake(0.05f);
            ballFlash_ = 0.18f;
        }
    }

    if (ballZ_ < -1.0f) {
        aiScore_++;
        audio_.playSoftThud(0.6f, 0.35f);
        triggerShake(0.06f);
        if (aiScore_ >= pointsToWin_) {
            state_ = GameState::GameOver;
        } else {
            queueServe(false);
        }
    } else if (ballZ_ > FIELD_L + 1.0f) {
        playerScore_++;
        audio_.playSoftThud(0.7f, 0.35f);
        triggerShake(0.06f);
        if (playerScore_ >= pointsToWin_) {
            state_ = GameState::GameOver;
        } else {
            queueServe(true);
        }
    }
}

void Game::addDigitBoxes(Scene& scene, float ox, float oy, float oz, int digit,
                         const Vec3& color) const {
    digit = std::max(0, std::min(9, digit));
    const int mask = kSegMask[digit];
    const float t = 0.07f;  // segment thickness
    const float w = 0.38f;  // horizontal span
    const float h = 0.55f;  // vertical half
    const float d = 0.08f;  // depth into room

    auto seg = [&](int bit, float x0, float y0, float x1, float y1) {
        if ((mask & bit) == 0) {
            return;
        }
        pushBox(scene,
                Vec3(ox + x0, oy + y0, oz - d * 0.5f),
                Vec3(ox + x1, oy + y1, oz + d * 0.5f),
                color, 0.45f);
    };

    // A top, B upper-right, C lower-right, D bottom, E lower-left, F upper-left, G mid
    seg(0b1000000, -w * 0.5f, h - t, w * 0.5f, h);           // A
    seg(0b0000010, w * 0.5f - t, 0.0f, w * 0.5f, h - t);     // B
    seg(0b0000100, w * 0.5f - t, -h, w * 0.5f, 0.0f);        // C
    seg(0b0000001, -w * 0.5f, -h, w * 0.5f, -h + t);         // D
    seg(0b0001000, -w * 0.5f, -h, -w * 0.5f + t, 0.0f);      // E
    seg(0b0100000, -w * 0.5f, 0.0f, -w * 0.5f + t, h - t);   // F
    seg(0b0010000, -w * 0.5f, -t * 0.5f, w * 0.5f, t * 0.5f); // G
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
    const Vec3 colPlayer(0.35f, 0.85f, 1.0f);
    const Vec3 colAi(1.0f, 0.45f, 0.35f);

    int pTens = (playerScore_ / 10) % 10;
    int pOnes = playerScore_ % 10;
    int aTens = (aiScore_ / 10) % 10;
    int aOnes = aiScore_ % 10;

    // Always show ones; show tens if >= 10 or always for classic look
    addDigitBoxes(scene, -1.05f, digitY, digitZ, pTens, colPlayer);
    addDigitBoxes(scene, -0.45f, digitY, digitZ, pOnes, colPlayer);
    addDigitBoxes(scene, 0.45f, digitY, digitZ, aTens, colAi);
    addDigitBoxes(scene, 1.05f, digitY, digitZ, aOnes, colAi);
}

void Game::buildScene() {
    scene_.clear();

    Plane floor;
    floor.point = Vec3(0, 0, 0);
    floor.normal = Vec3(0, 1, 0);
    floor.checker = true;
    floor.colorA = Vec3(0.85f, 0.85f, 0.82f);
    floor.colorB = Vec3(0.15f, 0.15f, 0.18f);
    floor.scale = 1.2f;
    floor.reflectivity = 0.15f;
    scene_.planes.push_back(floor);

    Plane ceil;
    ceil.point = Vec3(0, WALL_H, 0);
    ceil.normal = Vec3(0, -1, 0);
    ceil.colorA = Vec3(0.08f, 0.08f, 0.1f);
    ceil.reflectivity = 0.05f;
    scene_.planes.push_back(ceil);

    Plane back;
    back.point = Vec3(0, 0, FIELD_L + 0.5f);
    back.normal = Vec3(0, 0, -1);
    back.checker = true;
    back.colorA = Vec3(0.4f, 0.2f, 0.25f);
    back.colorB = Vec3(0.25f, 0.12f, 0.15f);
    back.scale = 0.8f;
    back.reflectivity = 0.1f;
    scene_.planes.push_back(back);

    Plane left;
    left.point = Vec3(-FIELD_W * 0.5f - 0.1f, 0, 0);
    left.normal = Vec3(1, 0, 0);
    left.checker = true;
    left.colorA = Vec3(0.2f, 0.3f, 0.45f);
    left.colorB = Vec3(0.12f, 0.18f, 0.28f);
    left.scale = 0.9f;
    left.reflectivity = 0.08f;
    scene_.planes.push_back(left);

    Plane right = left;
    right.point = Vec3(FIELD_W * 0.5f + 0.1f, 0, 0);
    right.normal = Vec3(-1, 0, 0);
    scene_.planes.push_back(right);

    Box playerPad;
    playerPad.minb = Vec3(playerX_ - PADDLE_W * 0.5f, 0.05f, 0.25f);
    playerPad.maxb = Vec3(playerX_ + PADDLE_W * 0.5f, 0.05f + PADDLE_H, 0.25f + PADDLE_D);
    playerPad.color = Vec3(0.7f, 0.75f, 0.9f);
    playerPad.reflectivity = 0.35f;
    scene_.boxes.push_back(playerPad);

    Box aiPad;
    aiPad.minb = Vec3(aiX_ - PADDLE_W * 0.5f, 0.05f, FIELD_L - 0.25f - PADDLE_D);
    aiPad.maxb = Vec3(aiX_ + PADDLE_W * 0.5f, 0.05f + PADDLE_H, FIELD_L - 0.25f);
    aiPad.color = Vec3(0.9f, 0.4f, 0.35f);
    aiPad.reflectivity = 0.3f;
    scene_.boxes.push_back(aiPad);

    addScoreboard(scene_);

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

    // Wall-mounted decos
    const float wallL = -FIELD_W * 0.5f;
    const float wallR = FIELD_W * 0.5f;
    const float wallB = FIELD_L + 0.5f;
    struct DecoSpec {
        Vec3 center;
        float radius;
        Vec3 color;
        float reflectivity;
    };
    const DecoSpec decos[] = {
        {Vec3(wallL + 0.28f, 1.6f, 3.5f), 0.35f, Vec3(0.9f, 0.55f, 0.2f), 0.65f},
        {Vec3(wallL + 0.22f, 2.2f, 8.0f), 0.28f, Vec3(0.85f, 0.3f, 0.45f), 0.7f},
        {Vec3(wallL + 0.32f, 1.1f, 13.0f), 0.4f, Vec3(0.4f, 0.75f, 0.95f), 0.6f},
        {Vec3(wallR - 0.28f, 1.8f, 5.0f), 0.32f, Vec3(0.3f, 0.7f, 0.9f), 0.65f},
        {Vec3(wallR - 0.25f, 2.4f, 10.5f), 0.3f, Vec3(0.95f, 0.85f, 0.35f), 0.55f},
        {Vec3(wallR - 0.35f, 1.3f, 14.2f), 0.38f, Vec3(0.7f, 0.4f, 0.85f), 0.7f},
        {Vec3(0.0f, 2.15f, wallB - 0.3f), 0.4f, Vec3(0.95f, 0.55f, 0.4f), 0.65f},
    };
    for (const auto& d : decos) {
        Sphere s;
        s.center = d.center;
        s.radius = d.radius;
        s.color = d.color;
        s.reflectivity = d.reflectivity;
        scene_.spheres.push_back(s);
    }

    scene_.lightPos = Vec3(0.0f, WALL_H - 0.5f, FIELD_L * 0.45f);
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

    if (state_ == GameState::Attract || state_ == GameState::GameOver) {
        orbitCam(attractTime_);
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

    // Play / Pause: paddle cam + shake
    float sx = shakeOffsetX_ * shake_;
    float sy = shakeOffsetY_ * shake_;
    if (shake_ > 0.001f) {
        sx += ((std::rand() % 100) / 100.0f - 0.5f) * shake_ * 0.35f;
        sy += ((std::rand() % 100) / 100.0f - 0.5f) * shake_ * 0.25f;
    }
    paddleCam(sx, sy);
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
        st = playerScore_ >= pointsToWin_ ? "YOU WIN — SPACE" : "AI WINS — SPACE";
        break;
    default:
        break;
    }
    char buf[256];
    std::snprintf(buf, sizeof(buf),
                  "KugelMatch [%s]%s  |  %s  |  %.0f FPS  |  F8 backend  |  ESC",
                  mode, mute, st, fpsSmooth_);
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
            if (e.type == SDL_KEYDOWN) {
                const SDL_Keymod mods = SDL_GetModState();
                switch (e.key.keysym.sym) {
                case SDLK_ESCAPE:
                    if (state_ == GameState::Play) {
                        state_ = GameState::Pause;
                    } else if (state_ == GameState::Pause) {
                        state_ = GameState::Attract;
                        attractTime_ = 0.0f;
                    } else if (state_ == GameState::Attract) {
                        running_ = false;
                    } else if (state_ == GameState::GameOver) {
                        state_ = GameState::Attract;
                        attractTime_ = 0.0f;
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
                    break;
                case SDLK_F11:
                    toggleFullscreen();
                    break;
                case SDLK_F8:
                    if (!switchBackend(backend_ == RenderBackend::Gpu ? RenderBackend::Cpu
                                                                      : RenderBackend::Gpu)) {
                        std::fprintf(stderr, "Backend switch failed.\n");
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
                    }
                    break;
                case SDLK_r:
                    if (state_ == GameState::Play || state_ == GameState::Pause) {
                        startMatch();
                    }
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
            int w = 0, h = 0;
            SDL_GetWindowSize(window_, &w, &h);
            w = std::max(1, w);
            h = std::max(1, h);
            if (!ensureCpuFramebuffer(w, h)) {
                running_ = false;
                break;
            }
            cpuRt_.render(scene_, camera_, framebuffer_, w, h);
            presentCpu();
        }
        updateHud();
    }
}
