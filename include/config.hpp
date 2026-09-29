// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2026 Ingo Ruhnke <grumbel@gmail.com>
#pragma once

#include <string>

struct AppConfig {
    bool useGpu = false;
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
};

// Load from path (or default locations). Returns true if a file was read.
bool loadConfig(AppConfig& cfg, std::string* loadedFrom = nullptr);
bool saveConfig(const AppConfig& cfg, std::string* savedTo = nullptr);
std::string defaultConfigPath();
