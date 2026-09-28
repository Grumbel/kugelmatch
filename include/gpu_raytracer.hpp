// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2024–2026 Ingo Ruhnke <grumbel@gmail.com>
#pragma once

#include "scene.hpp"
#include <SDL.h>

// OpenGL fragment-shader raytracer. Renders into a fixed 640×480 FBO, then
// letterboxes to the window. Full analytic RT — no scene meshes.
class GpuRaytracer {
public:
    static constexpr int WIDTH = 640;
    static constexpr int HEIGHT = 480;

    static constexpr int MAX_SPHERES = 8;
    static constexpr int MAX_BOXES = 4;
    static constexpr int MAX_PLANES = 8;

    GpuRaytracer() = default;
    ~GpuRaytracer();

    bool init(SDL_Window* window);
    void render(const Scene& scene, const Camera& cam);
    void present(); // letterbox blit + swap
    void onResize(int windowW, int windowH);

    bool ready() const { return ready_; }

private:
    SDL_Window* window_ = nullptr;
    SDL_GLContext glctx_ = nullptr;
    unsigned program_ = 0;
    unsigned vao_ = 0;
    unsigned fbo_ = 0;
    unsigned colorTex_ = 0;
    int winW_ = WIDTH;
    int winH_ = HEIGHT;
    bool ready_ = false;

    bool loadShaders();
    bool createFbo();
    static unsigned compileShader(unsigned type, const char* source);
    void uploadScene(const Scene& scene, const Camera& cam) const;
    void letterboxDst(int& dx, int& dy, int& dw, int& dh) const;
};
