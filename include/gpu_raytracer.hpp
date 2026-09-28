// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2024–2026 Ingo Ruhnke <grumbel@gmail.com>
#pragma once

#include "scene.hpp"
#include <SDL.h>
#include <string>

// OpenGL fragment-shader raytracer. Renders a fullscreen triangle;
// every pixel runs the full raytracing pipeline in GLSL. No scene meshes.
class GpuRaytracer {
public:
    static constexpr int WIDTH = 640;
    static constexpr int HEIGHT = 480;

    // Limits must match the shader (#define MAX_*)
    static constexpr int MAX_SPHERES = 8;
    static constexpr int MAX_BOXES = 4;
    static constexpr int MAX_PLANES = 8;

    GpuRaytracer() = default;
    ~GpuRaytracer();

    // Create GL context on an existing SDL window (must be created with
    // SDL_WINDOW_OPENGL). Returns false on failure.
    bool init(SDL_Window* window);

    void render(const Scene& scene, const Camera& cam);
    void present(); // swap buffers

    bool ready() const { return ready_; }

private:
    SDL_Window* window_ = nullptr;
    SDL_GLContext glctx_ = nullptr;
    unsigned program_ = 0;
    unsigned vao_ = 0;
    bool ready_ = false;

    bool loadShaders();
    static unsigned compileShader(unsigned type, const char* source);
    void uploadScene(const Scene& scene, const Camera& cam) const;
};
