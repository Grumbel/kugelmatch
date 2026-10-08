<!--
SPDX-License-Identifier: GPL-3.0-or-later
SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
-->

# KugelMatch — Agent Notes

## Project

Raytraced Pong (KugelMatch). Classic 90s checkerboard + mirror-ball look.
Camera is attached to the player paddle and looks down the playfield.
Both backends: window/drawable resolution (resizable).

Two render backends:

1. **CPU** — multi-threaded software raytracer (SDL2 texture upload)
2. **GPU** — full raytracing in an **OpenGL ES 2.0** fragment shader (fullscreen triangle VBO, no scene meshes; WebGL1-ready)

Both backends share the same scene description and game logic.

## Standing rules

- License: GPLv3+ (REUSE / SPDX headers on new files)
- Author for commits: `Ingo Ruhnke <grumbel@gmail.com>`
- Every commit must carry: `Co-authored-by: Grok <grok@x.ai>`
- Deliverables are **git bundles only** (see session instructions)
- Keep CPU and GPU paths honest raytracers — never fall back to rasterized 3D meshes of the playfield
- Prefer small, focused commits
- Document progress and the current tip in `TODO.md`

## Coordinates

Right-handed: **+Y up**, **+Z** toward the far paddle, **+X** to the right when looking down-field.
Details: `docs/COORDINATES.md`.

## Layout

```
include/          Public headers (vec3, scene, backends, game)
src/              Implementation
shaders/          GLSL sources for the GPU backend
flake.nix         Nix packaging + dev shell
CMakeLists.txt    Builds both backends into one binary (CLI select)
```

## Ports

| Package | Command |
|---------|---------|
| Linux | `nix build` / `nix run` |
| Win64 zip | `nix build .#kugelmatch-win64-zip` |
| WASM site | `nix build .#kugelmatch-wasm` / `nix run .#kugelmatch-wasm` (serve + open) |
| R36S PortMaster | `nix build .#kugelmatch-r36s` |
| Android APK | `nix build .#kugelmatch-android` (unfree SDK) |

Default in-game backend is **GPU (GLES2)**. Use `--cpu` for the software raytracer.

## Build

```bash
nix develop
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j
./kugelmatch          # CPU (default)
./kugelmatch --gpu    # GPU fragment-shader raytracer
```
