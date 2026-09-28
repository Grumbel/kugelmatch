// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2026 Ingo Ruhnke <grumbel@gmail.com>
#include "game.hpp"

#include <cstdio>
#include <cstring>

static void printUsage(const char* argv0) {
    std::fprintf(stderr,
                 "Usage: %s [--cpu|--gpu]\n"
                 "  --cpu   Multi-threaded software raytracer (default)\n"
                 "  --gpu   OpenGL fragment-shader raytracer (full RT, no meshes)\n",
                 argv0);
}

int main(int argc, char** argv) {
    RenderBackend backend = RenderBackend::Cpu;

    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--gpu") == 0) {
            backend = RenderBackend::Gpu;
        } else if (std::strcmp(argv[i], "--cpu") == 0) {
            backend = RenderBackend::Cpu;
        } else if (std::strcmp(argv[i], "-h") == 0 || std::strcmp(argv[i], "--help") == 0) {
            printUsage(argv[0]);
            return 0;
        } else {
            std::fprintf(stderr, "Unknown argument: %s\n", argv[i]);
            printUsage(argv[0]);
            return 1;
        }
    }

    Game game(backend);
    if (!game.init()) {
        std::fprintf(stderr, "Failed to initialize KugelMatch.\n");
        return 1;
    }
    game.run();
    game.shutdown();
    return 0;
}
