<!--
SPDX-License-Identifier: GPL-3.0-or-later
SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
-->

# KugelMatch

Classic Pong reimagined as a **90s-style raytracer**.

Two backends, same scene and rules:

| Flag | Backend |
|------|---------|
| `--cpu` (default) | Multi-threaded software raytracer (SDL2 texture) |
| `--gpu` | Full raytracing in an OpenGL **fragment shader** (fullscreen triangle, **no scene meshes**) |

Shared look: checkerboard floor + walls, reflective **mirror ball**, camera locked to the player paddle looking down the playfield. Both backends render at **window resolution** (resizable).

## Controls

### Match flow
| Key | Action |
|-----|--------|
| Space / Enter | Start match; skip intro; **skip goal replay**; rematch after game over |
| P | Pause / resume |
| R | Restart match |
| ESC | Pause → attract → quit |
| F8 | Switch CPU ↔ GPU |
| F9 | Toggle slow-mo goal replay |
| F11 / Alt+Enter | Fullscreen |

### Play
| Key | Action |
|-----|--------|
| A / D | Move player paddle (also arrows in 1P) |
| Arrows | Player 2 paddle in 2P mode |

### Options (in-world + keys)
| Key | Action |
|-----|--------|
| 1 | Difficulty Easy → Normal → Hard |
| 2 | Points to win 7 / 11 / 15 / 21 |
| 3 | Camera Paddle / High / Sideline |
| 4 | 1P / 2P |
| 5 | Reflection bounces 0–3 |
| 6 | Vsync on/off |
| 7 | Soft-shadow samples 1 / 2 / 4 / 8 |
| 8 / 9 or [ ] | Hold: exposure down / up |
| 0 | Theme Classic / Neon / Ice / Ember |
| M | Mute |
| + / − | Volume |

Settings persist in `~/.config/kugelmatch/config.cfg`.

## Build with Nix

```bash
nix develop
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j
./kugelmatch          # CPU
./kugelmatch --gpu    # GPU
```

Or:

```bash
nix build
./result/bin/kugelmatch --gpu
```

## Build without Nix

Needs: CMake ≥ 3.16, SDL2, OpenGL, C++17, pthread.

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j
./kugelmatch [--cpu|--gpu]
```

## Technical notes

- Analytic ray–sphere, ray–AABB, ray–plane intersections on both backends
- Up to 3 reflection bounces; mirror ball is highly reflective
- Soft shadow term + Blinn-style specular
- CPU: `std::thread` + atomic row queue
- GPU: GLSL 330 core fragment shader; scene uploaded as uniform arrays (no meshes)
- Shaders live in `shaders/`; an embedded copy is compiled in as fallback
- Procedural **metallic clank** sounds on paddle/wall hits (SDL audio, no sample files)
- Short **camera shake** on collisions (stronger for player paddle hits)
- Mute (M), master volume (+/−), serve delay after points, ball impact flash
- Soft vignette on both backends; FPS in window title
- Resizable window; CPU and GPU both raytrace at window/drawable size
- **3D scoreboard** hanging from the ceiling (7-segment digits as reflective boxes)
- Attract-mode orbit camera + fly-in intro before play; first to 11 points
- Runtime backend switch (F8)
- Desktop fullscreen via F11 / Alt+Enter

License: GPLv3+
