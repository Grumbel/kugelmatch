<!--
SPDX-License-Identifier: GPL-3.0-or-later
SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
-->

# TODO / Handoff

## Current tip

Base of this work line: `a7bcc21` (Initial checkin)

**Tip:** (pending) — 3D scoreboard, attract/intro, native CPU res, F8 backend switch

## Done

- [x] Dual CPU/GPU pure raytracers (no scene meshes)
- [x] Procedural audio, shake, vignette, REUSE
- [x] Wall-mounted decorative spheres
- [x] Ceiling-hung **raytraced scoreboard** (7-segment boxes)
- [x] Game states: Attract (orbit) → Intro (fly-in) → Play → Pause → GameOver
- [x] CPU + GPU both render at window resolution (no fixed 640×480)
- [x] Runtime CPU ↔ GPU switch (F8)

## Open / follow-ups

- [ ] Options “menu” as raytraced geometry (difficulty, points-to-win)
- [ ] Alternate camera modes selectable in attract
- [ ] Local 2-player
- [ ] Optional quality toggles (reflections) for slow GPUs

## Bundle naming

Base short: `a7bcc21`
Next: `kugelmatch-012.1-scoreboard-states-a7bcc21.bundle`

## Notes

- Score digits are boxes; GPU `MAX_BOXES=64` must stay in sync with the shader.
- Attract/intro are pure camera paths — no bitmap overlays.
- Prefer raytraced/procedural UI over fonts or sample files.
