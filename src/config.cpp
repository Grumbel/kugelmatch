// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2026 Ingo Ruhnke <grumbel@gmail.com>
#include "config.hpp"

#include <SDL.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <sys/stat.h>
#ifdef _WIN32
#include <direct.h>
#endif

namespace {

bool dirExists(const std::string& path) {
    struct stat st {};
    return ::stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode);
}

bool ensureDir(const std::string& path) {
    if (path.empty() || dirExists(path)) {
        return true;
    }
    // mkdir -p: create missing parents first (e.g. ~/.config on a fresh account).
    const auto slash = path.find_last_of("/\\");
    if (slash != std::string::npos && slash > 0) {
        ensureDir(path.substr(0, slash));
    }
#if defined(_WIN32)
    return _mkdir(path.c_str()) == 0 || dirExists(path);
#else
    return ::mkdir(path.c_str(), 0755) == 0 || dirExists(path);
#endif
}

} // namespace

std::string defaultConfigPath() {
#if defined(_WIN32) || defined(__ANDROID__) || defined(__EMSCRIPTEN__)
    // SDL owns the directory layout on these platforms (and IDBFS on the web).
    char* pref = SDL_GetPrefPath("grumbel", "kugelmatch");
    if (pref && pref[0]) {
        std::string base(pref);
        SDL_free(pref);
        return base + "config.cfg";
    }
    if (pref) {
        SDL_free(pref);
    }
    return "kugelmatch.cfg";
#else
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
#endif
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
        } else if (key == "slowmo_replay" || key == "slowmo") {
            cfg.slowmoReplay = (val == "1" || val == "true");
        } else if (key == "cpu_max_width") {
            cfg.cpuMaxWidth = std::atoi(val.c_str());
        } else if (key == "cpu_max_height") {
            cfg.cpuMaxHeight = std::atoi(val.c_str());
        } else if (key == "cpu_scale" || key == "render_scale") {
            cfg.cpuScale = std::strtof(val.c_str(), nullptr);
        } else if (key == "quality") {
            cfg.quality = std::atoi(val.c_str());
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
    out << "slowmo_replay=" << (cfg.slowmoReplay ? 1 : 0) << "\n";
    out << "cpu_max_width=" << cfg.cpuMaxWidth << "\n";
    out << "cpu_max_height=" << cfg.cpuMaxHeight << "\n";
    out << "render_scale=" << cfg.cpuScale << "\n";
    out << "cpu_scale=" << cfg.cpuScale << "\n"; // alias
    out << "quality=" << cfg.quality << "\n";
    return true;
}
