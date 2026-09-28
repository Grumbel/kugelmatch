#include "game.hpp"
#include <cstdio>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <algorithm>
#include <string>

Game::Game() {
    std::srand(static_cast<unsigned>(std::time(nullptr)));
}

Game::~Game() {
    shutdown();
}

bool Game::init() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        std::fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return false;
    }

    window = SDL_CreateWindow(
        "KugelMatch - Checkerboard + Mirror Ball",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        Raytracer::WIDTH, Raytracer::HEIGHT,
        SDL_WINDOW_SHOWN
    );
    if (!window) {
        std::fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        return false;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        // Fallback without vsync / accelerated
        renderer = SDL_CreateRenderer(window, -1, 0);
    }
    if (!renderer) {
        std::fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
        return false;
    }

    texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        Raytracer::WIDTH, Raytracer::HEIGHT
    );
    if (!texture) {
        std::fprintf(stderr, "SDL_CreateTexture failed: %s\n", SDL_GetError());
        return false;
    }

    framebuffer = new uint32_t[Raytracer::WIDTH * Raytracer::HEIGHT];
    std::memset(framebuffer, 0, sizeof(uint32_t) * Raytracer::WIDTH * Raytracer::HEIGHT);

    // Initial game state
    playerX = 0.0f;
    aiX = 0.0f;
    resetBall(false); // serve toward AI

    scene.lightPos = Vec3(0.0f, 6.0f, 8.0f);
    scene.lightColor = Vec3(1.2f, 1.15f, 1.05f);
    scene.ambient = Vec3(0.12f, 0.12f, 0.15f);
    scene.skyColor = Vec3(0.02f, 0.02f, 0.05f);

    running = true;
    return true;
}

void Game::shutdown() {
    if (framebuffer) {
        delete[] framebuffer;
        framebuffer = nullptr;
    }
    if (texture) {
        SDL_DestroyTexture(texture);
        texture = nullptr;
    }
    if (renderer) {
        SDL_DestroyRenderer(renderer);
        renderer = nullptr;
    }
    if (window) {
        SDL_DestroyWindow(window);
        window = nullptr;
    }
    SDL_Quit();
}

void Game::resetBall(bool towardPlayer) {
    ballX = 0.0f;
    ballZ = FIELD_L * 0.5f;
    float speed = 6.5f + (std::rand() % 100) * 0.01f;
    ballVX = ((std::rand() % 200) - 100) * 0.02f;
    ballVZ = towardPlayer ? -speed : speed;
}

void Game::handleInput(float dt) {
    const Uint8* keys = SDL_GetKeyboardState(nullptr);
    float speed = 9.0f;

    if (keys[SDL_SCANCODE_LEFT] || keys[SDL_SCANCODE_A]) {
        playerX += speed * dt;
    }
    if (keys[SDL_SCANCODE_RIGHT] || keys[SDL_SCANCODE_D]) {
        playerX -= speed * dt;
    }
    // Clamp paddle
    float half = FIELD_W * 0.5f - PADDLE_W * 0.5f;
    playerX = std::max(-half, std::min(half, playerX));

    if (keys[SDL_SCANCODE_R]) {
        playerScore = 0;
        aiScore = 0;
        resetBall(false);
    }
}

void Game::update(float dt) {
    if (paused) return;

    // AI: simple proportional + a bit of prediction
    float target = ballX;
    if (ballVZ > 0.0f) {
        // Predict roughly when it reaches far end
        float t = (FIELD_L - ballZ) / std::max(0.1f, ballVZ);
        target = ballX + ballVX * t * 0.7f;
    }
    float aiSpeed = 7.5f;
    if (aiX < target - 0.15f) aiX += aiSpeed * dt;
    else if (aiX > target + 0.15f) aiX -= aiSpeed * dt;
    float half = FIELD_W * 0.5f - PADDLE_W * 0.5f;
    aiX = std::max(-half, std::min(half, aiX));

    // Ball motion
    ballX += ballVX * dt;
    ballZ += ballVZ * dt;

    // Side walls
    float wall = FIELD_W * 0.5f - BALL_R;
    if (ballX < -wall) {
        ballX = -wall;
        ballVX = -ballVX;
    } else if (ballX > wall) {
        ballX = wall;
        ballVX = -ballVX;
    }

    // Player paddle (near, z ≈ 0)
    float pz = 0.4f;
    if (ballZ - BALL_R < pz + PADDLE_D * 0.5f && ballZ + BALL_R > pz - PADDLE_D * 0.5f &&
        ballVZ < 0.0f) {
        if (ballX + BALL_R > playerX - PADDLE_W * 0.5f &&
            ballX - BALL_R < playerX + PADDLE_W * 0.5f) {
            ballZ = pz + PADDLE_D * 0.5f + BALL_R;
            ballVZ = -ballVZ * 1.05f; // slight speed up
            // Spin based on hit position
            float offset = (ballX - playerX) / (PADDLE_W * 0.5f);
            ballVX += offset * 2.5f;
            // Clamp speed
            float sp = std::sqrt(ballVX * ballVX + ballVZ * ballVZ);
            if (sp > 14.0f) {
                ballVX *= 14.0f / sp;
                ballVZ *= 14.0f / sp;
            }
        }
    }

    // AI paddle (far)
    float az = FIELD_L - 0.4f;
    if (ballZ + BALL_R > az - PADDLE_D * 0.5f && ballZ - BALL_R < az + PADDLE_D * 0.5f &&
        ballVZ > 0.0f) {
        if (ballX + BALL_R > aiX - PADDLE_W * 0.5f &&
            ballX - BALL_R < aiX + PADDLE_W * 0.5f) {
            ballZ = az - PADDLE_D * 0.5f - BALL_R;
            ballVZ = -ballVZ * 1.05f;
            float offset = (ballX - aiX) / (PADDLE_W * 0.5f);
            ballVX += offset * 2.5f;
            float sp = std::sqrt(ballVX * ballVX + ballVZ * ballVZ);
            if (sp > 14.0f) {
                ballVX *= 14.0f / sp;
                ballVZ *= 14.0f / sp;
            }
        }
    }

    // Scoring
    if (ballZ < -1.0f) {
        aiScore++;
        resetBall(false);
    } else if (ballZ > FIELD_L + 1.0f) {
        playerScore++;
        resetBall(true);
    }
}

void Game::buildScene() {
    scene.clear();

    // Classic 90s checkerboard floor
    Plane floor;
    floor.point = Vec3(0, 0, 0);
    floor.normal = Vec3(0, 1, 0);
    floor.checker = true;
    floor.colorA = Vec3(0.85f, 0.85f, 0.82f);
    floor.colorB = Vec3(0.15f, 0.15f, 0.18f);
    floor.scale = 1.2f;
    floor.reflectivity = 0.15f;
    scene.planes.push_back(floor);

    // Ceiling (dark)
    Plane ceil;
    ceil.point = Vec3(0, WALL_H, 0);
    ceil.normal = Vec3(0, -1, 0);
    ceil.checker = false;
    ceil.colorA = Vec3(0.08f, 0.08f, 0.1f);
    ceil.reflectivity = 0.05f;
    scene.planes.push_back(ceil);

    // Back wall (far)
    Plane back;
    back.point = Vec3(0, 0, FIELD_L + 0.5f);
    back.normal = Vec3(0, 0, -1);
    back.checker = true;
    back.colorA = Vec3(0.4f, 0.2f, 0.25f);
    back.colorB = Vec3(0.25f, 0.12f, 0.15f);
    back.scale = 0.8f;
    back.reflectivity = 0.1f;
    scene.planes.push_back(back);

    // Left / right walls
    Plane left;
    left.point = Vec3(-FIELD_W * 0.5f - 0.1f, 0, 0);
    left.normal = Vec3(1, 0, 0);
    left.checker = true;
    left.colorA = Vec3(0.2f, 0.3f, 0.45f);
    left.colorB = Vec3(0.12f, 0.18f, 0.28f);
    left.scale = 0.9f;
    left.reflectivity = 0.08f;
    scene.planes.push_back(left);

    Plane right = left;
    right.point = Vec3(FIELD_W * 0.5f + 0.1f, 0, 0);
    right.normal = Vec3(-1, 0, 0);
    scene.planes.push_back(right);

    // Player paddle (near) - metallic
    Box playerPad;
    playerPad.minb = Vec3(playerX - PADDLE_W * 0.5f, 0.05f, 0.25f);
    playerPad.maxb = Vec3(playerX + PADDLE_W * 0.5f, 0.05f + PADDLE_H, 0.25f + PADDLE_D);
    playerPad.color = Vec3(0.7f, 0.75f, 0.9f);
    playerPad.reflectivity = 0.35f;
    scene.boxes.push_back(playerPad);

    // AI paddle
    Box aiPad;
    aiPad.minb = Vec3(aiX - PADDLE_W * 0.5f, 0.05f, FIELD_L - 0.25f - PADDLE_D);
    aiPad.maxb = Vec3(aiX + PADDLE_W * 0.5f, 0.05f + PADDLE_H, FIELD_L - 0.25f);
    aiPad.color = Vec3(0.9f, 0.4f, 0.35f);
    aiPad.reflectivity = 0.3f;
    scene.boxes.push_back(aiPad);

    // The mirror ball (the Pong ball)
    Sphere ball;
    ball.center = Vec3(ballX, BALL_R + 0.02f, ballZ);
    ball.radius = BALL_R;
    ball.color = Vec3(0.95f, 0.95f, 1.0f);
    ball.reflectivity = 0.85f; // strong mirror
    scene.spheres.push_back(ball);

    // A couple of decorative smaller spheres for classic look (optional fixed)
    Sphere deco1;
    deco1.center = Vec3(-3.2f, 0.5f, 4.0f);
    deco1.radius = 0.5f;
    deco1.color = Vec3(0.9f, 0.6f, 0.2f);
    deco1.reflectivity = 0.6f;
    scene.spheres.push_back(deco1);

    Sphere deco2;
    deco2.center = Vec3(3.0f, 0.4f, 12.0f);
    deco2.radius = 0.4f;
    deco2.color = Vec3(0.3f, 0.7f, 0.9f);
    deco2.reflectivity = 0.55f;
    scene.spheres.push_back(deco2);

    // Light position slightly above center
    scene.lightPos = Vec3(0.0f, WALL_H - 0.5f, FIELD_L * 0.45f);
}

void Game::present() {
    SDL_UpdateTexture(texture, nullptr, framebuffer, Raytracer::WIDTH * sizeof(uint32_t));
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, nullptr, nullptr);
    SDL_RenderPresent(renderer);
}

void Game::drawHUD() {
    // Title / scores via window title (simple, no TTF dependency)
    char buf[128];
    std::snprintf(buf, sizeof(buf),
                  "KugelMatch  |  Player %d  -  %d AI  |  Arrows/A D move  |  R reset  |  ESC quit",
                  playerScore, aiScore);
    SDL_SetWindowTitle(window, buf);
}

void Game::run() {
    Uint64 freq = SDL_GetPerformanceFrequency();
    Uint64 last = SDL_GetPerformanceCounter();

    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = false;
            if (e.type == SDL_KEYDOWN) {
                if (e.key.keysym.sym == SDLK_ESCAPE) running = false;
                if (e.key.keysym.sym == SDLK_p) paused = !paused;
            }
        }

        Uint64 now = SDL_GetPerformanceCounter();
        float dt = static_cast<float>(now - last) / static_cast<float>(freq);
        last = now;
        // Cap dt to avoid spiral
        if (dt > 0.05f) dt = 0.05f;

        handleInput(dt);
        update(dt);
        buildScene();

        // Camera attached to player paddle, looking down the playfield
        // Slightly above and behind the paddle, looking toward +Z (AI)
        Vec3 camPos(playerX, 1.1f, -0.6f);
        Vec3 lookAt(playerX * 0.3f, 0.6f, FIELD_L * 0.55f); // slight look bias toward center
        Vec3 up(0, 1, 0);
        rt.setCamera(camPos, lookAt, up, 70.0f);

        rt.render(scene, framebuffer);
        drawHUD();
        present();
    }
}
