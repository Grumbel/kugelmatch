<!--
SPDX-License-Identifier: GPL-3.0-or-later
SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
-->

# Changelog

## 1.2.2

- Quality presets Low/Medium/High/Ultra (**Q** / **F10**): bounces, soft-shadow samples, CPU scale

## 1.2.1


- Voxel **PAUSE** banner and dimmed lighting while paused
- CPU render scale (`cpu_scale` in config, 0.25–2.0) before resolution clamp

## 1.2.0


- Shared `glyphs` module (full A–Z 5×7 + 7-segment digits)
- Attract hints cycle SPACE → PONG → OPTS → GO
- Themes, ceiling scoreboard, match-point / WIN-LOSE banners
- Soft shadows, reflection quality, exposure, vsync, CPU res cap
- Goal replay (optional slow-mo), ball trail, procedural audio
- Config file under `~/.config/kugelmatch/config.cfg`
- CI: Ubuntu CMake + Nix; Makefile for local builds
- Dual CPU / GPU pure raytracers (no scene meshes)

## 1.1.0

- GPU fragment-shader raytracer and CPU multi-thread path
- Initial raytraced Pong playfield
