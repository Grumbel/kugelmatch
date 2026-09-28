# KugelMatch

Classic Pong reimagined as a **90s-style raytracer**.

Two backends, same scene and rules:

| Flag | Backend |
|------|---------|
| `--cpu` (default) | Multi-threaded software raytracer (SDL2 texture) |
| `--gpu` | Full raytracing in an OpenGL **fragment shader** (fullscreen triangle, **no scene meshes**) |

Shared look: checkerboard floor + walls, reflective **mirror ball**, camera locked to the player paddle looking down the playfield. Resolution **640×480**.

## Controls

| Key | Action |
|-----|--------|
| ← / → or A / D | Move paddle |
| R | Reset scores + ball |
| P | Pause |
| M | Mute / unmute |
| + / − | Volume up / down |
| F11 or Alt+Enter | Toggle fullscreen |
| ESC | Quit |

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
- Resizable window; internal RT stays **640×480**, letterboxed/scaled to the window
- Desktop fullscreen via F11 / Alt+Enter

License: GPLv3+
