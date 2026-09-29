<!--
SPDX-License-Identifier: GPL-3.0-or-later
SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
-->

# Changelog

## 1.2.6

- ESC from Pause/GameOver enters a clean attract (clears scores, banners, replay)
- Persist volume when releasing +/- volume keys
- Quiet clanks during attract demo rallies



- Physics substeps to prevent paddle tunneling at high ball speed
- Quality HUD shows CUST when bounces/shadows/scale diverge from preset
- Intro camera keeps orbit angle advancing for a smoother fly-in



- Fix CPU path writing past framebuffer when render scale ≠ 1 (use fbW/fbH)
- Fix vsync toggle double-applying cpu_scale on CPU backend
- Safe AABB invDir / thin-box normals in CPU raytracer
- Config accepts `render_scale` (written alongside `cpu_scale` alias)

## 1.2.3

- GPU render scale via FBO (quality presets / `cpu_scale` apply to both backends)
- Optional GPU max resolution clamp (shared with CPU caps)
- HUD shows render scale

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
