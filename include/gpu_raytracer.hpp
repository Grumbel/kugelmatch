// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2026 Ingo Ruhnke <grumbel@gmail.com>
#pragma once

#include "scene.hpp"
#include <SDL.h>

// OpenGL fragment-shader raytracer at the window's drawable resolution.
// Full analytic RT — no scene meshes.
class GpuRaytracer {
public:
    // Default window size (also used before the first resize query)
    static constexpr int DEFAULT_WIDTH = 640;
    static constexpr int DEFAULT_HEIGHT = 480;

    static constexpr int MAX_SPHERES = 8;
    static constexpr int MAX_BOXES = 4;
    static constexpr int MAX_PLANES = 8;

    GpuRaytracer() = default;
    ~GpuRaytracer();

    bool init(SDL_Window* window);
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
