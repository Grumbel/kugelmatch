// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2026 Ingo Ruhnke <grumbel@gmail.com>
#pragma once

#include <string>

struct AppConfig {
    bool useGpu = true;  // GLES2 fragment-shader RT (default)
    float volume = 0.85f;
    bool mute = false;
    int difficulty = 1;   // 0 easy 1 normal 2 hard
    int pointsToWin = 11;
    int cameraMode = 0;   // 0 paddle 1 high 2 side
    bool twoPlayer = false;
    int maxBounces = 3;   // 0..3 reflection quality
    bool fullscreen = false;
    bool vsync = true;
    int targetFps = 60; // 0 = uncapped when vsync off
    int shadowSamples = 4; // 1..8
    float exposure = 1.0f;
    int theme = 0; // 0 classic 1 neon 2 ice 3 ember
    bool slowmoReplay = true;
    int cpuMaxWidth = 1280;  // clamp CPU RT resolution (0 = unlimited)
    int cpuMaxHeight = 720;
    float cpuScale = 1.0f; // render scale (cpu_scale / render_scale); applies to CPU + GPU
    int quality = 2; // 0 low 1 medium 2 high 3 ultra
};

// Load from path (or default locations). Returns true if a file was read.
bool loadConfig(AppConfig& cfg, std::string* loadedFrom = nullptr);
bool saveConfig(const AppConfig& cfg, std::string* savedTo = nullptr);
std::string defaultConfigPath();

// Bitmask for which AppConfig fields were set on the command line (override file).
namespace CliOverride {
enum : unsigned {
    None         = 0,
    Backend      = 1u << 0,  // useGpu
    Volume       = 1u << 1,
    Mute         = 1u << 2,
    Diff         = 1u << 3,
    PointsToWin  = 1u << 4,
    Cam          = 1u << 5,
    TwoPlayer    = 1u << 6,
    MaxBounces   = 1u << 7,
    Fullscreen   = 1u << 8,
    Vsync        = 1u << 9,
    TargetFps    = 1u << 10,
    ShadowSamples= 1u << 11,
    Exposure     = 1u << 12,
    ThemeId      = 1u << 13,
    SlowmoReplay = 1u << 14,
    CpuMaxWidth  = 1u << 15,
    CpuMaxHeight = 1u << 16,
    CpuScale     = 1u << 17,
    QualityId    = 1u << 18,
};
} // namespace CliOverride
