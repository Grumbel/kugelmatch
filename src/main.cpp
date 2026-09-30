// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2026 Ingo Ruhnke <grumbel@gmail.com>
#include "config.hpp"
#include "game.hpp"
#include "version.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

namespace {

void printHelp(const char* argv0) {
    std::printf(
        "KugelMatch %s — raytraced Pong (CPU + GPU fragment-shader backends)\n"
        "\n"
        "Usage:\n"
        "  %s [options]\n"
        "\n"
        "Backend:\n"
        "  --cpu              Software multi-threaded raytracer (default unless config says gpu)\n"
        "  --gpu              OpenGL fragment-shader raytracer (full RT, no scene meshes)\n"
        "\n"
        "Display:\n"
        "  --fullscreen       Start fullscreen (desktop)\n"
        "  --windowed         Start windowed\n"
        "  --vsync            Enable vsync\n"
        "  --no-vsync         Disable vsync\n"
        "  --fps N            Target FPS when vsync is off (0 = uncapped, default from config)\n"
        "  --scale F          Render scale 0.25–2.0 (CPU framebuffer + GPU FBO)\n"
        "  --max-width N      Clamp render width (0 = unlimited)\n"
        "  --max-height N     Clamp render height (0 = unlimited)\n"
        "  --quality NAME     low | medium | high | ultra  (sets bounces, shadows, scale)\n"
        "  --bounces N        Reflection bounces 0–3\n"
        "  --shadows N        Soft-shadow samples 1, 2, 4, or 8\n"
        "  --exposure F       Exposure 0.1–3.0\n"
        "\n"
        "Game:\n"
        "  --1p / --2p        Single-player (AI) or two-player\n"
        "  --difficulty NAME  easy | normal | hard\n"
        "  --points N         Points to win: 7, 11, 15, or 21\n"
        "  --camera NAME      paddle | high | sideline\n"
        "  --theme NAME       classic | neon | ice | ember\n"
        "  --slowmo / --no-slowmo   Goal-replay slow motion\n"
        "\n"
        "Audio:\n"
        "  --volume F         Master volume 0.0–1.0\n"
        "  --mute / --no-mute\n"
        "\n"
        "Developer / testing (not saved to the config file):\n"
        "  --size WxH         Initial window size, e.g. 960x540\n"
        "  --start            Skip the attract screen and start a match immediately\n"
        "  --fixed-dt S       Deterministic simulation step in seconds (e.g. 0.0167)\n"
        "  --shot N:PATH      Save frame N as a BMP screenshot (repeatable)\n"
        "  --quit-after N     Exit after N frames\n"
        "\n"
        "Info:\n"
        "  --version          Print version and exit\n"
        "  -h, --help         Show this help\n"
        "\n"
        "Config file (loaded first, then overridden by options above):\n"
        "  ~/.config/kugelmatch/config.cfg\n"
        "\n"
        "Coordinates: right-handed, +Y up, +Z toward the far paddle.\n"
        "See docs/COORDINATES.md in the source tree.\n",
        KUGELMATCH_VERSION_STRING, argv0);
}

bool parseInt(const char* s, int& out) {
    if (!s || !*s) return false;
    char* end = nullptr;
    long v = std::strtol(s, &end, 10);
    if (end == s || *end != '\0') return false;
    out = static_cast<int>(v);
    return true;
}

bool parseFloat(const char* s, float& out) {
    if (!s || !*s) return false;
    char* end = nullptr;
    float v = std::strtof(s, &end);
    if (end == s || *end != '\0') return false;
    out = v;
    return true;
}

int eq(const char* a, const char* b) { return std::strcmp(a, b) == 0; }

} // namespace

int main(int argc, char** argv) {
    AppConfig cli{};
    unsigned mask = CliOverride::None;
    // Default backend for construction before config; may be overridden by --cpu/--gpu
    // or by config file (applied inside Game::init).
    bool backendFromCli = false;
    RenderBackend backend = RenderBackend::Cpu;

    DevOptions dev;

    for (int i = 1; i < argc; ++i) {
        const char* a = argv[i];
        auto need = [&](const char* name) -> const char* {
            if (i + 1 >= argc) {
                std::fprintf(stderr, "Missing value for %s\n", name);
                std::exit(2);
            }
            return argv[++i];
        };

        if (eq(a, "--cpu")) {
            backend = RenderBackend::Cpu;
            cli.useGpu = false;
            mask |= CliOverride::Backend;
            backendFromCli = true;
        } else if (eq(a, "--gpu")) {
            backend = RenderBackend::Gpu;
            cli.useGpu = true;
            mask |= CliOverride::Backend;
            backendFromCli = true;
        } else if (eq(a, "--fullscreen")) {
            cli.fullscreen = true;
            mask |= CliOverride::Fullscreen;
        } else if (eq(a, "--windowed")) {
            cli.fullscreen = false;
            mask |= CliOverride::Fullscreen;
        } else if (eq(a, "--vsync")) {
            cli.vsync = true;
            mask |= CliOverride::Vsync;
        } else if (eq(a, "--no-vsync")) {
            cli.vsync = false;
            mask |= CliOverride::Vsync;
        } else if (eq(a, "--fps")) {
            if (!parseInt(need("--fps"), cli.targetFps)) {
                std::fprintf(stderr, "Invalid --fps value\n");
                return 2;
            }
            mask |= CliOverride::TargetFps;
        } else if (eq(a, "--scale")) {
            if (!parseFloat(need("--scale"), cli.cpuScale)) {
                std::fprintf(stderr, "Invalid --scale value\n");
                return 2;
            }
            mask |= CliOverride::CpuScale;
        } else if (eq(a, "--max-width")) {
            if (!parseInt(need("--max-width"), cli.cpuMaxWidth)) {
                std::fprintf(stderr, "Invalid --max-width value\n");
                return 2;
            }
            mask |= CliOverride::CpuMaxWidth;
        } else if (eq(a, "--max-height")) {
            if (!parseInt(need("--max-height"), cli.cpuMaxHeight)) {
                std::fprintf(stderr, "Invalid --max-height value\n");
                return 2;
            }
            mask |= CliOverride::CpuMaxHeight;
        } else if (eq(a, "--quality")) {
            const char* v = need("--quality");
            if (eq(v, "low")) cli.quality = 0;
            else if (eq(v, "medium") || eq(v, "med")) cli.quality = 1;
            else if (eq(v, "high")) cli.quality = 2;
            else if (eq(v, "ultra")) cli.quality = 3;
            else {
                std::fprintf(stderr, "Unknown quality '%s' (low|medium|high|ultra)\n", v);
                return 2;
            }
            mask |= CliOverride::QualityId;
        } else if (eq(a, "--bounces")) {
            if (!parseInt(need("--bounces"), cli.maxBounces)) {
                std::fprintf(stderr, "Invalid --bounces value\n");
                return 2;
            }
            mask |= CliOverride::MaxBounces;
        } else if (eq(a, "--shadows")) {
            if (!parseInt(need("--shadows"), cli.shadowSamples)) {
                std::fprintf(stderr, "Invalid --shadows value\n");
                return 2;
            }
            mask |= CliOverride::ShadowSamples;
        } else if (eq(a, "--exposure")) {
            if (!parseFloat(need("--exposure"), cli.exposure)) {
                std::fprintf(stderr, "Invalid --exposure value\n");
                return 2;
            }
            mask |= CliOverride::Exposure;
        } else if (eq(a, "--1p")) {
            cli.twoPlayer = false;
            mask |= CliOverride::TwoPlayer;
        } else if (eq(a, "--2p")) {
            cli.twoPlayer = true;
            mask |= CliOverride::TwoPlayer;
        } else if (eq(a, "--difficulty")) {
            const char* v = need("--difficulty");
            if (eq(v, "easy")) cli.difficulty = 0;
            else if (eq(v, "normal") || eq(v, "nml")) cli.difficulty = 1;
            else if (eq(v, "hard")) cli.difficulty = 2;
            else {
                std::fprintf(stderr, "Unknown difficulty '%s' (easy|normal|hard)\n", v);
                return 2;
            }
            mask |= CliOverride::Diff;
        } else if (eq(a, "--points")) {
            if (!parseInt(need("--points"), cli.pointsToWin)) {
                std::fprintf(stderr, "Invalid --points value\n");
                return 2;
            }
            mask |= CliOverride::PointsToWin;
        } else if (eq(a, "--camera")) {
            const char* v = need("--camera");
            if (eq(v, "paddle") || eq(v, "pad")) cli.cameraMode = 0;
            else if (eq(v, "high")) cli.cameraMode = 1;
            else if (eq(v, "sideline") || eq(v, "side")) cli.cameraMode = 2;
            else {
                std::fprintf(stderr, "Unknown camera '%s' (paddle|high|sideline)\n", v);
                return 2;
            }
            mask |= CliOverride::Cam;
        } else if (eq(a, "--theme")) {
            const char* v = need("--theme");
            if (eq(v, "classic")) cli.theme = 0;
            else if (eq(v, "neon")) cli.theme = 1;
            else if (eq(v, "ice")) cli.theme = 2;
            else if (eq(v, "ember")) cli.theme = 3;
            else {
                std::fprintf(stderr, "Unknown theme '%s' (classic|neon|ice|ember)\n", v);
                return 2;
            }
            mask |= CliOverride::ThemeId;
        } else if (eq(a, "--slowmo")) {
            cli.slowmoReplay = true;
            mask |= CliOverride::SlowmoReplay;
        } else if (eq(a, "--no-slowmo")) {
            cli.slowmoReplay = false;
            mask |= CliOverride::SlowmoReplay;
        } else if (eq(a, "--volume")) {
            if (!parseFloat(need("--volume"), cli.volume)) {
                std::fprintf(stderr, "Invalid --volume value\n");
                return 2;
            }
            mask |= CliOverride::Volume;
        } else if (eq(a, "--mute")) {
            cli.mute = true;
            mask |= CliOverride::Mute;
        } else if (eq(a, "--no-mute")) {
            cli.mute = false;
            mask |= CliOverride::Mute;
        } else if (eq(a, "--size")) {
            const char* v = need("--size");
            if (std::sscanf(v, "%dx%d", &dev.windowW, &dev.windowH) != 2 ||
                dev.windowW < 64 || dev.windowH < 64) {
                std::fprintf(stderr, "Invalid --size value '%s' (expected WxH)\n", v);
                return 2;
            }
        } else if (eq(a, "--start")) {
            dev.autoStart = true;
        } else if (eq(a, "--fixed-dt")) {
            if (!parseFloat(need("--fixed-dt"), dev.fixedDt) || dev.fixedDt <= 0.0f) {
                std::fprintf(stderr, "Invalid --fixed-dt value\n");
                return 2;
            }
        } else if (eq(a, "--quit-after")) {
            if (!parseInt(need("--quit-after"), dev.quitAfterFrames)) {
                std::fprintf(stderr, "Invalid --quit-after value\n");
                return 2;
            }
        } else if (eq(a, "--shot")) {
            const char* v = need("--shot");
            const char* colon = std::strchr(v, ':');
            int frame = 0;
            if (!colon || colon == v || !parseInt(std::string(v, colon).c_str(), frame) ||
                frame < 0 || !colon[1]) {
                std::fprintf(stderr, "Invalid --shot value '%s' (expected N:PATH)\n", v);
                return 2;
            }
            dev.shots.emplace_back(frame, std::string(colon + 1));
        } else if (eq(a, "--version")) {
            std::printf("kugelmatch %s\n", KUGELMATCH_VERSION_STRING);
            return 0;
        } else if (eq(a, "-h") || eq(a, "--help")) {
            printHelp(argv[0]);
            return 0;
        } else {
            std::fprintf(stderr, "Unknown argument: %s\n", a);
            printHelp(argv[0]);
            return 1;
        }
    }

    (void)backendFromCli;
    Game game(backend);
    game.setDevOptions(dev);
    if (!game.init((mask != CliOverride::None) ? &cli : nullptr, mask)) {
        std::fprintf(stderr, "Failed to initialize KugelMatch.\n");
        return 1;
    }
    game.run();
    game.shutdown();
    return 0;
}
