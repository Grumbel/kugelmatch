// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2024–2026 Ingo Ruhnke <grumbel@gmail.com>
#pragma once

#include "vec3.hpp"
#include <cmath>
#include <vector>

struct Ray {
    Vec3 origin;
    Vec3 dir; // normalized
};

struct Hit {
    float t = 1e30f;
    Vec3 point;
    Vec3 normal;
    Vec3 color;
    float reflectivity = 0.0f;
    bool hit = false;
};

struct Sphere {
    Vec3 center;
    float radius = 1.0f;
    Vec3 color;
    float reflectivity = 0.0f;
};

struct Box {
    Vec3 minb;
    Vec3 maxb;
    Vec3 color;
    float reflectivity = 0.0f;
};

struct Plane {
    Vec3 point;
    Vec3 normal;
    bool checker = false;
    Vec3 colorA;
    Vec3 colorB;
    float scale = 1.0f;
    float reflectivity = 0.0f;
};

struct Scene {
    std::vector<Sphere> spheres;
    std::vector<Box> boxes;
    std::vector<Plane> planes;
    Vec3 lightPos{0.f, 5.f, 5.f};
    Vec3 lightColor{1.2f, 1.15f, 1.05f};
    Vec3 ambient{0.12f, 0.12f, 0.15f};
    Vec3 skyColor{0.02f, 0.02f, 0.05f};

    void clear() {
        spheres.clear();
        boxes.clear();
        planes.clear();
    }
};

// Shared camera state used by both backends
struct Camera {
    Vec3 pos;
    Vec3 forward;
    Vec3 right;
    Vec3 up;
    float fovScale = 1.0f;

    void set(const Vec3& position, const Vec3& lookAt, const Vec3& worldUp, float fovDeg) {
        pos = position;
        forward = (lookAt - position).normalized();
        right = forward.cross(worldUp).normalized();
        up = right.cross(forward).normalized();
        float fovRad = fovDeg * 3.14159265f / 180.0f;
        fovScale = std::tan(fovRad * 0.5f);
    }
};
