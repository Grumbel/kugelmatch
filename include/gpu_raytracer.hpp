// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2026 Ingo Ruhnke <grumbel@gmail.com>
#pragma once

#include "scene.hpp"
#include <SDL.h>

// OpenGL fragment-shader raytracer at the window's drawable resolution.
// Full analytic RT — no scene meshes.
class GpuRaytracer {
public:
    static constexpr int DEFAULT_WIDTH = 960;
    static constexpr int DEFAULT_HEIGHT = 540;

    // Must match shaders/raytrace.frag
    static constexpr int MAX_SPHERES = 16;
    static constexpr int MAX_BOXES = 64;
    static constexpr int MAX_PLANES = 12;

    GpuRaytracer() = default;
    ~GpuRaytracer();

    bool init(SDL_Window* window);
    void shutdown();
    void render(const Scene& scene, const Camera& cam);
    void present();
    void onResize(int windowW, int windowH);

    int width() const { return rtW_; }
    int height() const { return rtH_; }
    bool ready() const { return ready_; }

private:
    SDL_Window* window_ = nullptr;
    SDL_GLContext glctx_ = nullptr;
    unsigned program_ = 0;
    unsigned vao_ = 0;
    int rtW_ = DEFAULT_WIDTH;
    int rtH_ = DEFAULT_HEIGHT;
    bool ready_ = false;

    bool loadShaders();
    static unsigned compileShader(unsigned type, const char* source);
    void uploadScene(const Scene& scene, const Camera& cam) const;
    void syncDrawableSize();
};
