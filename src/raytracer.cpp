// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2026 Ingo Ruhnke <grumbel@gmail.com>
#include "raytracer.hpp"

#include <algorithm>
#include <cmath>

CpuRaytracer::CpuRaytracer() {
    numThreads_ = static_cast<int>(std::max(1u, std::thread::hardware_concurrency()));
    if (numThreads_ > 16) {
        numThreads_ = 16;
    }
}

CpuRaytracer::~CpuRaytracer() {
    for (auto& t : workers_) {
        if (t.joinable()) {
            t.join();
        }
    }
}

Hit CpuRaytracer::intersect(const Ray& ray, const Scene& scene) const {
    Hit best;

    for (const auto& s : scene.spheres) {
        Vec3 oc = ray.origin - s.center;
        float b = oc.dot(ray.dir);
        float c = oc.length2() - s.radius * s.radius;
        float disc = b * b - c;
        if (disc < 0.0f) {
            continue;
        }
        float sq = std::sqrt(disc);
        float t = -b - sq;
        if (t < 1e-4f) {
            t = -b + sq;
        }
        if (t > 1e-4f && t < best.t) {
            best.t = t;
            best.point = ray.origin + ray.dir * t;
            best.normal = (best.point - s.center).normalized();
            best.color = s.color;
            best.reflectivity = s.reflectivity;
            best.hit = true;
        }
    }

    for (const auto& box : scene.boxes) {
        Vec3 invDir(
            1.0f / ray.dir.x,
            1.0f / ray.dir.y,
            1.0f / ray.dir.z);
        float t1 = (box.minb.x - ray.origin.x) * invDir.x;
        float t2 = (box.maxb.x - ray.origin.x) * invDir.x;
        float t3 = (box.minb.y - ray.origin.y) * invDir.y;
        float t4 = (box.maxb.y - ray.origin.y) * invDir.y;
        float t5 = (box.minb.z - ray.origin.z) * invDir.z;
        float t6 = (box.maxb.z - ray.origin.z) * invDir.z;

        float tmin = std::max(std::max(std::min(t1, t2), std::min(t3, t4)), std::min(t5, t6));
        float tmax = std::min(std::min(std::max(t1, t2), std::max(t3, t4)), std::max(t5, t6));

        if (tmax < 0.0f || tmin > tmax) {
            continue;
        }
        float t = tmin > 1e-4f ? tmin : tmax;
        if (t < 1e-4f || t >= best.t) {
            continue;
        }

        best.t = t;
        best.point = ray.origin + ray.dir * t;
        Vec3 center = (box.minb + box.maxb) * 0.5f;
        Vec3 d = best.point - center;
        Vec3 halfExtent = (box.maxb - box.minb) * 0.5f;
        float bias = 1.0001f;
        best.normal = Vec3(
            static_cast<float>(static_cast<int>(d.x / std::abs(halfExtent.x) * bias)),
            static_cast<float>(static_cast<int>(d.y / std::abs(halfExtent.y) * bias)),
            static_cast<float>(static_cast<int>(d.z / std::abs(halfExtent.z) * bias))).normalized();
        best.color = box.color;
        best.reflectivity = box.reflectivity;
        best.hit = true;
    }

    for (const auto& p : scene.planes) {
        float denom = p.normal.dot(ray.dir);
        if (std::abs(denom) < 1e-6f) {
            continue;
        }
        float t = (p.point - ray.origin).dot(p.normal) / denom;
        if (t < 1e-4f || t >= best.t) {
            continue;
        }

        best.t = t;
        best.point = ray.origin + ray.dir * t;
        best.normal = denom < 0.0f ? p.normal : -p.normal;
        best.reflectivity = p.reflectivity;

        if (p.checker) {
            float u = 0.f, v = 0.f;
            if (std::abs(p.normal.y) > 0.9f) {
                u = best.point.x;
                v = best.point.z;
            } else if (std::abs(p.normal.x) > 0.9f) {
                u = best.point.z;
                v = best.point.y;
            } else {
                u = best.point.x;
                v = best.point.y;
            }
            int iu = static_cast<int>(std::floor(u * p.scale));
            int iv = static_cast<int>(std::floor(v * p.scale));
            best.color = ((iu + iv) & 1) ? p.colorA : p.colorB;
        } else {
            best.color = p.colorA;
        }
        best.hit = true;
    }

    return best;
}

Vec3 CpuRaytracer::shade(const Ray& ray, const Scene& scene, int depth) const {
    if (depth > scene.maxBounces) {
        return scene.skyColor;
    }

    Hit h = intersect(ray, scene);
    if (!h.hit) {
        return scene.skyColor;
    }

    Vec3 col = h.color * scene.ambient;

    Vec3 toLight = (scene.lightPos - h.point).normalized();
    float ndotl = std::max(0.0f, h.normal.dot(toLight));

    // Soft shadow: up to 8 disk samples (scene.shadowSamples)
    float shadowFactor = 0.0f;
    const Vec3 offsets[8] = {
        Vec3(0.35f, 0.0f, 0.15f), Vec3(-0.25f, 0.1f, -0.30f),
        Vec3(0.10f, 0.0f, -0.35f), Vec3(-0.15f, 0.05f, 0.40f),
        Vec3(0.40f, 0.05f, -0.10f), Vec3(-0.40f, 0.0f, 0.20f),
        Vec3(0.05f, 0.1f, 0.45f), Vec3(-0.05f, 0.0f, -0.45f),
    };
    const float lightRadius = 0.55f;
    int samples = std::max(1, std::min(8, scene.shadowSamples));
    for (int i = 0; i < samples; ++i) {
        Vec3 lp = scene.lightPos + offsets[i] * lightRadius;
        Vec3 toL = lp - h.point;
        float dist = toL.length();
        toL = toL * (1.0f / std::max(dist, 1e-4f));
        Ray shadow;
        shadow.origin = h.point + h.normal * 1e-3f;
        shadow.dir = toL;
        Hit sh = intersect(shadow, scene);
        if (!(sh.hit && sh.t < dist)) {
            shadowFactor += 1.0f;
        }
    }
    shadowFactor = 0.22f + 0.78f * (shadowFactor / static_cast<float>(samples));

    col += h.color * scene.lightColor * ndotl * shadowFactor;

    Vec3 viewDir = -ray.dir;
    Vec3 halfV = (toLight + viewDir).normalized();
    float spec = std::pow(std::max(0.0f, h.normal.dot(halfV)), 32.0f);
    col += scene.lightColor * (spec * 0.4f * shadowFactor);

    if (h.reflectivity > 0.01f && depth < scene.maxBounces) {
        Ray refl;
        refl.origin = h.point + h.normal * 1e-3f;
        refl.dir = ray.dir.reflect(h.normal).normalized();
        Vec3 rcol = shade(refl, scene, depth + 1);
        col = col * (1.0f - h.reflectivity) + rcol * h.reflectivity;
    }

    return col.clamp01();
}

void CpuRaytracer::renderRow(const Scene& scene, const Camera& cam, uint32_t* fb,
                               int y, int width, int height) {
    const float aspect = static_cast<float>(width) / static_cast<float>(height);
    for (int x = 0; x < width; ++x) {
        float u = (2.0f * (x + 0.5f) / width - 1.0f) * aspect * cam.fovScale;
        float v = (1.0f - 2.0f * (y + 0.5f) / height) * cam.fovScale;

        Ray ray;
        ray.origin = cam.pos;
        ray.dir = (cam.forward + cam.right * u + cam.up * v).normalized();

        Vec3 col = shade(ray, scene, 0);
        col = col * std::max(0.1f, scene.exposure);

        // Soft 90s-style vignette
        float nx = (x + 0.5f) / width * 2.0f - 1.0f;
        float ny = (y + 0.5f) / height * 2.0f - 1.0f;
        float vig = 1.0f - 0.35f * (nx * nx + ny * ny);
        if (vig < 0.0f) vig = 0.0f;
        col *= vig;

        int r = static_cast<int>(std::sqrt(col.x) * 255.0f + 0.5f);
        int g = static_cast<int>(std::sqrt(col.y) * 255.0f + 0.5f);
        int b = static_cast<int>(std::sqrt(col.z) * 255.0f + 0.5f);
        r = std::min(255, std::max(0, r));
        g = std::min(255, std::max(0, g));
        b = std::min(255, std::max(0, b));
        fb[y * width + x] = (0xFFu << 24) | (static_cast<uint32_t>(r) << 16) |
                            (static_cast<uint32_t>(g) << 8) | static_cast<uint32_t>(b);
    }
}

void CpuRaytracer::render(const Scene& scene, const Camera& cam, uint32_t* framebuffer,
                          int width, int height) {
    if (width <= 0 || height <= 0 || !framebuffer) {
        return;
    }
    nextRow_.store(0);
    workers_.clear();
    workers_.reserve(static_cast<size_t>(numThreads_));

    auto worker = [this, &scene, &cam, framebuffer, width, height]() {
        while (true) {
            int y = nextRow_.fetch_add(1);
            if (y >= height) {
                break;
            }
            renderRow(scene, cam, framebuffer, y, width, height);
        }
    };

    for (int i = 0; i < numThreads_; ++i) {
        workers_.emplace_back(worker);
    }
    for (auto& t : workers_) {
        t.join();
    }
    workers_.clear();
}
