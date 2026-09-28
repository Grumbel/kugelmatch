# KugelMatch

Classic Pong reimagined as a **90s-style software raytracer**.

- **SDL2** window + software framebuffer (no GPU)
- Pure CPU raytracer with multi-threading
- Checkerboard floor + walls, reflective **mirror ball**
- Camera locked to the player paddle, looking down the playfield
- Resolution: **640×480**
- Build system: **Nix flake** + **CMake**

## Controls

| Key            | Action              |
|----------------|---------------------|
| ← / → or A / D | Move paddle         |
| R              | Reset scores + ball |
| P              | Pause               |
| ESC            | Quit                |

## Build with Nix (recommended)

```bash
# Enter dev shell
nix develop

# Configure & build
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j

# Run
./kugelmatch
```

Or build the package directly:

```bash
nix build
./result/bin/kugelmatch
```

## Build without Nix

Requirements: CMake ≥ 3.16, SDL2, C++17 compiler, pthread.

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j
./kugelmatch
```

## Technical notes

- Ray-sphere, ray-AABB and ray-plane intersections
- Up to 3 levels of reflection (mirror ball is highly reflective)
- Soft shadows + Blinn-style specular
- Multi-threaded scanline rendering (`std::thread` + atomic work queue)
- Gamma-corrected output into ARGB8888 SDL texture

Enjoy the classic reflective sphere bouncing across the checkerboard.
