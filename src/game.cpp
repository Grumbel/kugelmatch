// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2024–2026 Ingo Ruhnke <grumbel@gmail.com>
#include "game.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

Game::Game(RenderBackend backend) : backend_(backend) {
    std::srand(static_cast<unsigned>(std::time(nullptr)));
}

Game::~Game() {
    shutdown();
}

bool Game::init() {
    Uint32 sdlFlags = SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO;
    if (SDL_Init(sdlFlags) != 0) {
        std::fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return false;
    }

    Uint32 winFlags = SDL_WINDOW_SHOWN;
    if (backend_ == RenderBackend::Gpu) {
        winFlags |= SDL_WINDOW_OPENGL;
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    }

    window_ = SDL_CreateWindow(
        "KugelMatch",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        CpuRaytracer::WIDTH, CpuRaytracer::HEIGHT,
        winFlags);
    if (!window_) {
        std::fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        return false;
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
        texture_ = SDL_CreateTexture(
            sdlRenderer_,
            SDL_PIXELFORMAT_ARGB8888,
            SDL_TEXTUREACCESS_STREAMING,
            CpuRaytracer::WIDTH, CpuRaytracer::HEIGHT);
        if (!texture_) {
            std::fprintf(stderr, "SDL_CreateTexture failed: %s\n", SDL_GetError());
            return false;
        }
        framebuffer_ = new uint32_t[CpuRaytracer::WIDTH * CpuRaytracer::HEIGHT];
        std::memset(framebuffer_, 0, sizeof(uint32_t) * CpuRaytracer::WIDTH * CpuRaytracer::HEIGHT);
    }

    // Audio is optional — game still runs if device open fails
    if (!audio_.init()) {
        std::fprintf(stderr, "Warning: audio init failed; continuing without sound.\n");
    }

    playerX_ = 0.0f;
    aiX_ = 0.0f;
    resetBall(false);

    scene_.lightPos = Vec3(0.0f, 6.0f, 8.0f);
    scene_.lightColor = Vec3(1.2f, 1.15f, 1.05f);
    scene_.ambient = Vec3(0.12f, 0.12f, 0.15f);
    scene_.skyColor = Vec3(0.02f, 0.02f, 0.05f);

    running_ = true;
    return true;
}

void Game::shutdown() {
    audio_.shutdown();
    if (framebuffer_) {
        delete[] framebuffer_;
        framebuffer_ = nullptr;
    }
    if (texture_) {
        SDL_DestroyTexture(texture_);
        texture_ = nullptr;
    }
    if (sdlRenderer_) {
        SDL_DestroyRenderer(sdlRenderer_);
        sdlRenderer_ = nullptr;
    }
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
}

void Game::handleInput(float dt) {
    const Uint8* keys = SDL_GetKeyboardState(nullptr);
    float speed = 9.0f;

    if (keys[SDL_SCANCODE_LEFT] || keys[SDL_SCANCODE_A]) {
        playerX_ -= speed * dt;
    }
    if (keys[SDL_SCANCODE_RIGHT] || keys[SDL_SCANCODE_D]) {
        playerX_ += speed * dt;
    }
    float half = FIELD_W * 0.5f - PADDLE_W * 0.5f;
    playerX_ = std::max(-half, std::min(half, playerX_));

    if (keys[SDL_SCANCODE_R]) {
        playerScore_ = 0;
        aiScore_ = 0;
        resetBall(false);
    }
}

void Game::triggerShake(float amount) {
    // Keep the stronger of current and new impulse
    if (amount > shake_) {
        shake_ = amount;
    }
    shakeTime_ = 0.0f;
    // Random direction in X/Y (view space-ish)
    float angle = (static_cast<float>(std::rand() % 1000) / 1000.0f) * 6.2831853f;
    shakeOffsetX_ = std::cos(angle);
    shakeOffsetY_ = std::sin(angle) * 0.6f; // slightly less vertical
}

void Game::updateShake(float dt) {
    if (shake_ <= 0.001f) {
        shake_ = 0.0f;
        return;
    }
    shakeTime_ += dt;
    // Exponential decay ~120 ms half-life feel
    shake_ *= std::exp(-dt * 8.0f);
    if (shake_ < 0.001f) {
        shake_ = 0.0f;
    }
}

void Game::update(float dt) {
    updateShake(dt);
    if (paused_) {
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

    // Side walls — metallic clank + light shake
    float wall = FIELD_W * 0.5f - BALL_R;
    if (ballX_ < -wall) {
        ballX_ = -wall;
        ballVX_ = -ballVX_;
        float speed = std::sqrt(ballVX_ * ballVX_ + ballVZ_ * ballVZ_);
        float pitch = 0.95f + std::min(0.25f, speed * 0.02f);
        audio_.playClank(pitch, 0.45f);
        triggerShake(0.04f);
    } else if (ballX_ > wall) {
        ballX_ = wall;
        ballVX_ = -ballVX_;
        float speed = std::sqrt(ballVX_ * ballVX_ + ballVZ_ * ballVZ_);
        float pitch = 0.95f + std::min(0.25f, speed * 0.02f);
        audio_.playClank(pitch, 0.45f);
        triggerShake(0.04f);
    }

    // Player paddle
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
            // Stronger clank + shake for player hits (camera is on this paddle)
            float pitch = 0.85f + std::min(0.4f, sp * 0.03f) + std::abs(offset) * 0.15f;
            audio_.playClank(pitch, 0.75f);
            triggerShake(0.12f + std::min(0.08f, sp * 0.008f));
        }
    }

    // AI paddle
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
            float pitch = 0.75f + std::min(0.35f, sp * 0.025f);
            audio_.playClank(pitch, 0.55f);
            triggerShake(0.05f);
        }
    }

    // Scoring — soft thud, no big shake
    if (ballZ_ < -1.0f) {
        aiScore_++;
        audio_.playSoftThud(0.6f, 0.35f);
        triggerShake(0.06f);
        resetBall(false);
    } else if (ballZ_ > FIELD_L + 1.0f) {
        playerScore_++;
        audio_.playSoftThud(0.7f, 0.35f);
        triggerShake(0.06f);
        resetBall(true);
    }
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

    Sphere ball;
    ball.center = Vec3(ballX_, BALL_R + 0.02f, ballZ_);
    ball.radius = BALL_R;
    ball.color = Vec3(0.95f, 0.95f, 1.0f);
    ball.reflectivity = 0.85f;
    scene_.spheres.push_back(ball);

    Sphere deco1;
    deco1.center = Vec3(-3.2f, 0.5f, 4.0f);
    deco1.radius = 0.5f;
    deco1.color = Vec3(0.9f, 0.6f, 0.2f);
    deco1.reflectivity = 0.6f;
    scene_.spheres.push_back(deco1);

    Sphere deco2;
    deco2.center = Vec3(3.0f, 0.4f, 12.0f);
    deco2.radius = 0.4f;
    deco2.color = Vec3(0.3f, 0.7f, 0.9f);
    deco2.reflectivity = 0.55f;
    scene_.spheres.push_back(deco2);

    scene_.lightPos = Vec3(0.0f, WALL_H - 0.5f, FIELD_L * 0.45f);
}

void Game::presentCpu() {
    SDL_UpdateTexture(texture_, nullptr, framebuffer_,
                      CpuRaytracer::WIDTH * static_cast<int>(sizeof(uint32_t)));
    SDL_RenderClear(sdlRenderer_);
    SDL_RenderCopy(sdlRenderer_, texture_, nullptr, nullptr);
    SDL_RenderPresent(sdlRenderer_);
}

void Game::updateHud() {
    const char* mode = backend_ == RenderBackend::Gpu ? "GPU" : "CPU";
    char buf[160];
    std::snprintf(buf, sizeof(buf),
                  "KugelMatch [%s]  |  Player %d  -  %d AI  |  Arrows/A D  |  R reset  |  ESC quit",
                  mode, playerScore_, aiScore_);
    SDL_SetWindowTitle(window_, buf);
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
            if (e.type == SDL_KEYDOWN) {
                if (e.key.keysym.sym == SDLK_ESCAPE) {
                    running_ = false;
                }
                if (e.key.keysym.sym == SDLK_p) {
                    paused_ = !paused_;
                }
            }
        }

        Uint64 now = SDL_GetPerformanceCounter();
        float dt = static_cast<float>(now - last) / static_cast<float>(freq);
        last = now;
        if (dt > 0.05f) {
            dt = 0.05f;
        }

        handleInput(dt);
        update(dt);
        buildScene();

        // Camera attached to player paddle + shake offset
        float sx = shakeOffsetX_ * shake_;
        float sy = shakeOffsetY_ * shake_;
        // Slight high-frequency jitter while shake is active
        if (shake_ > 0.001f) {
            sx += ((std::rand() % 100) / 100.0f - 0.5f) * shake_ * 0.35f;
            sy += ((std::rand() % 100) / 100.0f - 0.5f) * shake_ * 0.25f;
        }

        Vec3 camPos(playerX_ + sx, 1.1f + sy, -0.6f);
        Vec3 lookAt(playerX_ * 0.3f + sx * 0.3f, 0.6f + sy * 0.2f, FIELD_L * 0.55f);
        camera_.set(camPos, lookAt, Vec3(0, 1, 0), 70.0f);

        if (backend_ == RenderBackend::Gpu) {
            gpuRt_.render(scene_, camera_);
            gpuRt_.present();
        } else {
            cpuRt_.render(scene_, camera_, framebuffer_);
            presentCpu();
        }
        updateHud();
    }
}
