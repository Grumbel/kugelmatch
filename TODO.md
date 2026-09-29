<!--
SPDX-License-Identifier: GPL-3.0-or-later
SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
-->

# TODO / Handoff

## Current tip

Base of this work line: `a7bcc21` (Initial checkin)

**Tip:** `b053440` — attract hint, trail, CPU cap, Makefile

## Done

- [x] Theme floor/walls, skip replay, controls docs
- [x] Attract-mode blinking voxel **SPACE** hint
- [x] Raytraced ball ghost trail (3 fading spheres)
- [x] CPU resolution clamp (`cpu_max_width` / `cpu_max_height`, default 1280×720)
- [x] Top-level **Makefile** for non-Nix builds

## Open / follow-ups

- [ ] Optional CI workflow
- [ ] Further glyph words in attract (OPTS / PONG)

## Bundle naming

Base short: `a7bcc21`
Previous: `kugelmatch-020.1-theme-floor-skip-replay-a7bcc21.bundle`
Next: `kugelmatch-021.1-attract-hint-trail-cpu-cap-a7bcc21.bundle`

## Notes

- SPACE hint blinks (skipped half the time) to save box budget under MAX_BOXES=128
- CPU clamp scales up via SDL_RenderCopy when window is larger
