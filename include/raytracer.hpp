#pragma once
#include "vec3.hpp"
#include <vector>
#include <cstdint>
#include <thread>
#include <atomic>

struct Ray {
    Vec3 origin;
    Vec3 dir; // must be normalized
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
    float radius;
    Vec3 color;
    float reflectivity;
};

struct Box {
    Vec3 minb, maxb;
    Vec3 color;
    float reflectivity;
};

struct Plane {
    Vec3 point;
    Vec3 normal;
    // checkerboard params
    bool checker = false;
    Vec3 colorA, colorB;
    float scale = 1.0f;
    float reflectivity = 0.0f;
};

class Scene {
public:
    std::vector<Sphere> spheres;
    std::vector<Box> boxes;
    std::vector<Plane> planes;
    Vec3 lightPos;
    Vec3 lightColor;
    Vec3 ambient;
    Vec3 skyColor;

    void clear() {
        spheres.clear();
        boxes.clear();
        planes.clear();
    }
};

class Raytracer {
public:
    static constexpr int WIDTH = 640;
    static constexpr int HEIGHT = 480;

    Raytracer();
    ~Raytracer();

    void setCamera(const Vec3& pos, const Vec3& lookAt, const Vec3& up, float fovDeg);
    void render(const Scene& scene, uint32_t* framebuffer);

private:
    Vec3 camPos, camForward, camRight, camUp;
    float fovScale;

    int numThreads;
    std::vector<std::thread> workers;
    std::atomic<int> nextRow{0};

    Hit intersect(const Ray& ray, const Scene& scene) const;
    Vec3 shade(const Ray& ray, const Scene& scene, int depth) const;
    void renderTile(const Scene& scene, uint32_t* fb, int yStart, int yEnd);
};
