// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2026 Ingo Ruhnke <grumbel@gmail.com>
#pragma once

#include "scene.hpp"
#include <SDL.h>

#include <cstdint>
#include <string>
#include <vector>

// OpenGL ES 2.0 fragment-shader raytracer. Full analytic RT — no scene meshes.
// Optional render scale via an FBO sized to the raytrace resolution (under- or
// supersample), then a textured fullscreen triangle blit to the window drawable
// (GLES2 has no glBlitFramebuffer). Scale is shared with the CPU path quality
// presets.
class GpuRaytracer {
public:
    static constexpr int DEFAULT_WIDTH = 960;
    static constexpr int DEFAULT_HEIGHT = 540;

    // Must match shaders/raytrace.frag (128 boxes keeps uniform pressure
    // within practical GLES2 / WebGL1 limits for typical scenes).
    static constexpr int MAX_SPHERES = 16;
#if defined(KUGELMATCH_USE_OPENGLES2)
    static constexpr int MAX_BOXES = 128;
#else
    static constexpr int MAX_BOXES = 512;
#endif
    static constexpr int MAX_PLANES = 12;

    GpuRaytracer() = default;
    ~GpuRaytracer();

    bool init(SDL_Window* window);
    void shutdown();
    void render(const Scene& scene, const Camera& cam);
    void present();
    /** Block until GPU work is done (pace CPU to GPU when swap does not). */
    void finish();
    /** Read back the window's back buffer (call after render(), before present()).
     *  Output is 0xAARRGGBB, top row first. */
    bool readPixels(std::vector<uint32_t>& argb, int& w, int& h) const;
    void onResize(int windowW, int windowH);

    // Render resolution = drawable * scale, clamped to maxW/maxH when > 0.
    void setRenderScale(float scale);
    void setMaxResolution(int maxW, int maxH);
    float renderScale() const { return renderScale_; }

    int width() const { return rtW_; }
    int height() const { return rtH_; }
    bool ready() const { return ready_; }

private:
    SDL_Window* window_ = nullptr;
    SDL_GLContext glctx_ = nullptr;
    unsigned program_ = 0;      // raytrace
    unsigned blitProgram_ = 0;  // FBO → window
    unsigned vbo_ = 0;          // fullscreen triangle (a_pos)
    unsigned vao_ = 0;          // required for OpenGL 3.3 core; unused on GLES2
    int aPosLoc_ = -1;
    int aPosBlitLoc_ = -1;
    int uTexLoc_ = -1;
    int uUvScaleLoc_ = -1;

    // FBO for scaled raytrace target (0 when rendering directly to default FB)
    unsigned fbo_ = 0;
    unsigned fboTex_ = 0;
    int fboW_ = 0;
    int fboH_ = 0;

    int drawableW_ = DEFAULT_WIDTH;
    int drawableH_ = DEFAULT_HEIGHT;
    int rtW_ = DEFAULT_WIDTH;
    int rtH_ = DEFAULT_HEIGHT;
    float renderScale_ = 1.0f;
    int maxW_ = 0; // 0 = no clamp
    int maxH_ = 0;
    // Raytrace program uniform locations (filled once after link).
    struct Uniforms {
        int camPos = -1, camForward = -1, camRight = -1, camUp = -1;
        int fovScale = -1, aspect = -1, pixelAngle = -1;
        int lightPos = -1, lightColor = -1, ambient = -1, skyColor = -1;
        int maxBounces = -1, shadowSamples = -1, exposure = -1;
        int numSpheres = -1, sphereCenter = -1, sphereRadius = -1;
        int sphereColor = -1, sphereReflect = -1;
        int numBoxes = -1, boxMin = -1, boxMax = -1, boxColor = -1, boxReflect = -1;
        int numPlanes = -1, planePoint = -1, planeNormal = -1;
        int planeColorA = -1, planeColorB = -1, planeScale = -1, planeReflect = -1;
        int planeChecker = -1, planeOneSided = -1;
    } u_;
    // Scratch for bulk uniform uploads (avoid per-frame heap alloc).
    mutable std::vector<float> uploadF_;
    mutable std::vector<int> uploadI_;
    bool ready_ = false;
    bool useFbo_ = false;

    bool loadShaders();
    void cacheUniformLocations();
    static unsigned compileShader(unsigned type, const char* source);
    bool linkProgram(unsigned& outProg, unsigned vs, unsigned fs, const char* label);
    void uploadScene(const Scene& scene, const Camera& cam) const;
    void syncDrawableSize();
    void recomputeRtSize();
    bool ensureFbo(int w, int h);
    void destroyFbo();
    void drawFullscreenTriangle(int aPosLoc) const;
};
