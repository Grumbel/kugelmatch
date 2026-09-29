// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2026 Ingo Ruhnke <grumbel@gmail.com>
#pragma once

#include "scene.hpp"
#include <atomic>
#include <cstdint>
#include <thread>
#include <vector>

class CpuRaytracer {
public:
    CpuRaytracer();
    ~CpuRaytracer();

    // Renders into framebuffer of size width*height (ARGB8888).
    void render(const Scene& scene, const Camera& cam, uint32_t* framebuffer,
                int width, int height);

private:
    int numThreads_ = 1;
    std::vector<std::thread> workers_;
    std::atomic<int> nextRow_{0};

    Hit intersect(const Ray& ray, const Scene& scene) const;
    Vec3 shade(const Ray& ray, const Scene& scene, int depth) const;
    void renderRow(const Scene& scene, const Camera& cam, uint32_t* fb,
                   int y, int width, int height);
};
