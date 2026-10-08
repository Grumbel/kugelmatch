<!--
SPDX-License-Identifier: GPL-3.0-or-later
SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
-->

# Changelog

## Unreleased (1.2.14-dev)

### Rendering
- GPU raytracer: any-hit occlusion for soft shadows (skip closest-hit work on
  shadow rays), test planes before boxes so `best.t` prunes glyph geometry, and
  compute box `invDir` once per ray
- Desktop OpenGL 3.3 core: create and bind a VAO (required for attribute draws;
  without it the GPU path rendered a black screen). GLES2 unchanged.
- GPU FBO is sized to the raytrace resolution (not the drawable); viewport is
  always the full FBO and the blit samples the whole texture. Fixes broken
  supersampling (scale > 1) and wrong viewport when max-resolution clamp made
  `rt != drawable`
- Max-resolution clamp (CPU + GPU) uses a uniform fit so aspect ratio is preserved
- `--scale auto` uses `glFinish` so FPS tracks the GPU (not the CPU queue);
  starts at scale 0.25 and steps every ~0.75s
- `--scale auto`: adapt render resolution to target ~60 fps

### Platforms
- R36S launcher defaults: `--scale 0.25 --bounces 1 --shadows 1` (handheld-friendly)
- Render scale minimum lowered from 0.25 to **0.0625**

- Default renderer is **GPU (OpenGL ES 2.0)**; `--cpu` selects the software path
- Multi-platform packaging (kurvenrausch-style): Windows MinGW zips, Emscripten WASM site,
  Android APK, R36S/ArkOS PortMaster tree (`nix/`, `mk/`)
- Emscripten main loop; config dir via `SDL_GetPrefPath` on Windows/Android/Web

### Rendering
- GPU backend targets **OpenGL ES 2.0** (GLSL ES 1.00): attribute VBO fullscreen
  triangle, no VAO / `gl_VertexID`, FBO scale via textured blit (no `glBlitFramebuffer`)
- Box uniform capacity 512 → 128 (practical GLES2 / WebGL1 uniform budget)
- CMake links GLESv2/EGL when present; desktop Mesa ES context works with `--gpu`

### Camera
- Camera rig: every shot defines an ideal pose and the view follows it through critically damped
  smoothing, so replay start/end, camera-mode changes and the intro hand-over glide instead of cutting
- Paddle camera raised and re-aimed so the whole paddle stays in frame; leans into strafing,
  widens slightly with ball speed, FOV punch and dolly recoil on paddle hits
- Sideline camera is now a working broadcast view (it previously looked at the back of a wall);
  high camera re-aimed so the paddle is visible
- Screen shake is a deterministic, time-based damped oscillation (was per-frame `rand()` noise)
- Replay orbit starts on the side facing the action and pushes in

### Animation
- Serve countdown 0.75 s → 1.35 s with a drop-in per digit; ball grows in while counting down
- Paddles recoil and flash on hits; score digits pop when the score changes
- After a goal the ball keeps rolling and thuds into the end wall instead of freezing
- Ghost trail sampled at a fixed time step (frame-rate independent)

### Rendering
- Fix GPU last-bounce shading (mirror surfaces were darker on GPU than CPU at bounces 0–1)
- Analytic anti-aliased checkerboard (ray-footprint filtering through mirror bounces)
- Robust box normals (dominant axis); one-sided wall planes; real near/far end walls
  (the ball no longer passes through the back wall before a goal)
- GPU uniforms: cached locations and bulk array uploads
- Embedded fallback shader is generated from `shaders/raytrace.frag` at configure time

### Tooling / fixes
- F12 screenshots; developer flags `--size`, `--start`, `--fixed-dt`, `--shot N:PATH`, `--quit-after`
- Create `~/.config/kugelmatch/` recursively on first run (settings used to land in the cwd)

### Earlier in this cycle
- Fix upside-down view: camera `up = forward × right` (was inverted)
- Document world axes in `docs/COORDINATES.md`
- Expanded CLI (`--help`): display, quality, match, audio; options override config file
- Version from top-level `VERSION` file (CMake + Nix; `--version` prints full string)
- Development builds append `.{revCount}+g{shortRev}` when `VERSION` contains `-dev`

## 1.2.13

- Fix mirrored view/text: camera right = worldUp × forward
- Attract/intro/game-over/replay cameras stay inside the room and orbit the ball or scoreboard
- Scoreboard digits on the player-facing face
- Correct 7-segment bit→geometry mapping; denser letter voxels
- Raise GPU MAX_BOXES to 512 so banner text is not truncated
- Align paddle controls with corrected camera axes

## 1.2.12

- Linux packaging: man page, CMake install, .desktop, SVG icon, AppStream metainfo
- flake apps (`nix run`, `nix run .#kugelmatch-gpu`) and dev-shell helpers
  `kugelmatch-configure` / `kugelmatch-run` (build in `/tmp/kugelmatch-build`)
- Portable Release flags by default; `-DKUGELMATCH_NATIVE=ON` for local builds

## 1.2.11

- Debounce window resize (~100ms settle) so CPU textures / GPU FBOs are not
  rebuilt on every drag sample (fixes multi-second hitch while resizing)

## 1.2.10

- Fix A/D and arrow paddle controls (screen-left = -X with paddle camera)
- Soft-limit ball angle after paddle hits (min |VZ|, max |VX|/|VZ|)

## 1.2.9

- 2P game-over HUD/banner: P2 WINS / "P2" instead of AI/LOSE
- Attract demo shows ball trail
- AI prediction target clamped to court width
- Re-sync embedded GPU fragment shader with shaders/raytrace.frag

## 1.2.8

- Hide options geometry during play (attract/pause only) for a clear court
- Persist mute on M
- Config load keeps explicit bounces/shadow samples after quality preset

## 1.2.7

- Move decorative spheres from walls onto the ceiling (clear playfield)

## 1.2.6

- ESC from Pause/GameOver enters a clean attract (clears scores, banners, replay)
- Persist volume when releasing +/- volume keys
- Quiet clanks during attract demo rallies

## 1.2.5

- Physics substeps to prevent paddle tunneling at high ball speed
- Quality HUD shows CUST when bounces/shadows/scale diverge from preset
- Intro camera keeps orbit angle advancing for a smoother fly-in

## 1.2.4

- Fix CPU path writing past framebuffer when render scale ≠ 1 (use fbW/fbH)
- Fix vsync toggle double-applying cpu_scale on CPU backend
- Safe AABB invDir / thin-box normals in CPU raytracer
- Config accepts `render_scale` (written alongside `cpu_scale` alias)

## 1.2.3

- GPU render scale via FBO (quality presets / `cpu_scale` apply to both backends)
- Optional GPU max resolution clamp (shared with CPU caps)
- HUD shows render scale

## 1.2.2

- Quality presets Low/Medium/High/Ultra (**Q** / **F10**): bounces, soft-shadow samples, scale

## 1.2.1

- Voxel **PAUSE** banner and dimmed lighting while paused
- CPU render scale (`cpu_scale` in config, 0.25–2.0) before resolution clamp

## 1.2.0

- Voxel **PAUSE** banner and dimmed lighting while paused
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
