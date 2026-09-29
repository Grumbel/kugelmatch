<!--
SPDX-License-Identifier: GPL-3.0-or-later
SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
-->

# TODO / Handoff

## Current tip

Base of this work line: `a7bcc21` (Initial checkin)

**Tip:** `97294e6` — title, config, celebration, quality

## Done

- [x] Scoreboard, states, options geometry, 2P, cameras, demo
- [x] Block-letter **KUGEL** title (box voxels) in attract / game over
- [x] Config persistence (`~/.config/kugelmatch/config.cfg`)
- [x] Reflection quality 0–3 (key **5**); CPU + GPU respect `scene.maxBounces`
- [x] Winner celebration camera (faster orbit, look at scoreboard)

## Open / follow-ups

- [ ] More glyph words / localized title
- [ ] Match-point dramatic lighting pulse
- [ ] Optional vsync / FPS cap in config

## Bundle naming

Base short: `a7bcc21`
Previous: `kugelmatch-013.1-options-2p-cameras-a7bcc21.bundle`
Next: `kugelmatch-014.1-title-config-quality-a7bcc21.bundle`

## Notes

- GPU: MAX_SPHERES=16, MAX_BOXES=128 — sync shader + header + embedded.
- Config keys: backend, volume, mute, difficulty, points, camera, twoplayer, bounces, fullscreen.
