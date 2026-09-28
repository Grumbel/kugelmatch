<!--
SPDX-License-Identifier: GPL-3.0-or-later
SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
-->

# KugelMatch — Agent Notes

## Project

Raytraced Pong (KugelMatch). Classic 90s checkerboard + mirror-ball look.
Camera is attached to the player paddle and looks down the playfield.
CPU backend: 640×480. GPU backend: native window/drawable resolution.

Two render backends:

1. **CPU** — multi-threaded software raytracer (SDL2 texture upload)
2. **GPU** — full raytracing in an OpenGL fragment shader (fullscreen triangle, no scene meshes)

Both backends share the same scene description and game logic.

## Standing rules

- License: GPLv3+ (REUSE / SPDX headers on new files)
- Author for commits: `Ingo Ruhnke <grumbel@gmail.com>`
- Every commit must carry: `Co-authored-by: Grok <grok@x.ai>`
- Deliverables are **git bundles only** (see session instructions)
- Keep CPU and GPU paths honest raytracers — never fall back to rasterized 3D meshes of the playfield
- Prefer small, focused commits
- Document progress and the current tip in `TODO.md`

## Layout

```
include/          Public headers (vec3, scene, backends, game)
src/              Implementation
shaders/          GLSL sources for the GPU backend
flake.nix         Nix packaging + dev shell
CMakeLists.txt    Builds both backends into one binary (CLI select)
```

## Build

```bash
nix develop
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j
./kugelmatch          # CPU (default)
./kugelmatch --gpu    # GPU fragment-shader raytracer
```
