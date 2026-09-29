<!--
SPDX-License-Identifier: GPL-3.0-or-later
SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
-->

# Changelog

## 1.2.13

- Fix mirrored view/text: camera right = worldUp × forward
- Attract/intro/game-over/replay cameras stay inside the room and orbit the ball or scoreboard
- Scoreboard digits on the player-facing face
- Correct 7-segment bit→geometry mapping; denser letter voxels
- Raise GPU MAX_BOXES to 512 so banner text is not truncated
- Align paddle controls with corrected camera axes



- Linux packaging: man page, CMake install, .desktop, SVG icon, AppStream metainfo
- flake apps (`nix run`, `nix run .#kugelmatch-gpu`) and dev-shell helpers
  `kugelmatch-configure` / `kugelmatch-run` (build in `/tmp/kugelmatch-build`)
- Portable Release flags by default; `-DKUGELMATCH_NATIVE=ON` for local builds



- Debounce window resize (~100ms settle) so CPU textures / GPU FBOs are not
  rebuilt on every drag sample (fixes multi-second hitch while resizing)



- Fix A/D and arrow paddle controls (screen-left = -X with paddle camera)
- Soft-limit ball angle after paddle hits (min |VZ|, max |VX|/|VZ|)



- 2P game-over HUD/banner: P2 WINS / "P2" instead of AI/LOSE
- Attract demo shows ball trail
- AI prediction target clamped to court width
- Re-sync embedded GPU fragment shader with shaders/raytrace.frag



- Hide options geometry during play (attract/pause only) for a clear court
- Persist mute on M
- Config load keeps explicit bounces/shadow samples after quality preset



- Move decorative spheres from walls onto the ceiling (clear playfield)



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
