// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2026 Ingo Ruhnke <grumbel@gmail.com>
#include "config.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <sys/stat.h>

namespace {

bool dirExists(const std::string& path) {
    struct stat st {};
    return ::stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode);
}

bool ensureDir(const std::string& path) {
    if (dirExists(path)) {
        return true;
    }
    return ::mkdir(path.c_str(), 0755) == 0;
}

} // namespace

std::string defaultConfigPath() {
    const char* xdg = std::getenv("XDG_CONFIG_HOME");
    std::string base;
    if (xdg && xdg[0]) {
        base = std::string(xdg) + "/kugelmatch";
    } else {
        const char* home = std::getenv("HOME");
        if (home && home[0]) {
            base = std::string(home) + "/.config/kugelmatch";
        } else {
            return "kugelmatch.cfg";
        }
    }
    ensureDir(base);
    return base + "/config.cfg";
}

bool loadConfig(AppConfig& cfg, std::string* loadedFrom) {
    std::string path = defaultConfigPath();
    std::ifstream in(path);
    if (!in) {
        // fallback next to cwd
        in.open("kugelmatch.cfg");
        if (!in) {
            return false;
        }
        path = "kugelmatch.cfg";
    }
    if (loadedFrom) {
        *loadedFrom = path;
    }
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#' || line[0] == ';') {
            continue;
        }
        auto eq = line.find('=');
        if (eq == std::string::npos) {
            continue;
        }
        std::string key = line.substr(0, eq);
        std::string val = line.substr(eq + 1);
        auto trim = [](std::string& s) {
            while (!s.empty() && (s.back() == '\r' || s.back() == ' ')) {
                s.pop_back();
            }
            size_t i = 0;
            while (i < s.size() && s[i] == ' ') {
                ++i;
            }
            s = s.substr(i);
        };
        trim(key);
        trim(val);
        if (key == "backend") {
            cfg.useGpu = (val == "gpu" || val == "GPU" || val == "1");
        } else if (key == "volume") {
            cfg.volume = std::strtof(val.c_str(), nullptr);
        } else if (key == "mute") {
            cfg.mute = (val == "1" || val == "true");
        } else if (key == "difficulty") {
            cfg.difficulty = std::atoi(val.c_str());
        } else if (key == "points") {
            cfg.pointsToWin = std::atoi(val.c_str());
        } else if (key == "camera") {
            cfg.cameraMode = std::atoi(val.c_str());
        } else if (key == "twoplayer") {
            cfg.twoPlayer = (val == "1" || val == "true");
        } else if (key == "bounces") {
            cfg.maxBounces = std::atoi(val.c_str());
        } else if (key == "fullscreen") {
            cfg.fullscreen = (val == "1" || val == "true");
        } else if (key == "vsync") {
            cfg.vsync = (val == "1" || val == "true");
        } else if (key == "target_fps" || key == "fps") {
            cfg.targetFps = std::atoi(val.c_str());
        } else if (key == "shadow_samples" || key == "shadows") {
            cfg.shadowSamples = std::atoi(val.c_str());
        } else if (key == "exposure") {
            cfg.exposure = std::strtof(val.c_str(), nullptr);
        } else if (key == "theme") {
            cfg.theme = std::atoi(val.c_str());
        }
    }
    if (cfg.maxBounces < 0) {
        cfg.maxBounces = 0;
    }
    if (cfg.maxBounces > 3) {
        cfg.maxBounces = 3;
    }
    if (cfg.difficulty < 0) {
        cfg.difficulty = 0;
    }
    if (cfg.difficulty > 2) {
        cfg.difficulty = 2;
    }
    return true;
}

bool saveConfig(const AppConfig& cfg, std::string* savedTo) {
    std::string path = defaultConfigPath();
    std::ofstream out(path);
    if (!out) {
        path = "kugelmatch.cfg";
        out.open(path);
        if (!out) {
            return false;
        }
    }
    if (savedTo) {
        *savedTo = path;
    }
    out << "# KugelMatch config\n";
    out << "backend=" << (cfg.useGpu ? "gpu" : "cpu") << "\n";
    out << "volume=" << cfg.volume << "\n";
    out << "mute=" << (cfg.mute ? 1 : 0) << "\n";
    out << "difficulty=" << cfg.difficulty << "\n";
    out << "points=" << cfg.pointsToWin << "\n";
    out << "camera=" << cfg.cameraMode << "\n";
    out << "twoplayer=" << (cfg.twoPlayer ? 1 : 0) << "\n";
    out << "bounces=" << cfg.maxBounces << "\n";
    out << "fullscreen=" << (cfg.fullscreen ? 1 : 0) << "\n";
    out << "vsync=" << (cfg.vsync ? 1 : 0) << "\n";
    out << "target_fps=" << cfg.targetFps << "\n";
    out << "shadow_samples=" << cfg.shadowSamples << "\n";
    out << "exposure=" << cfg.exposure << "\n";
    out << "theme=" << cfg.theme << "\n";
    return true;
}
