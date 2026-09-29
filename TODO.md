<!--
SPDX-License-Identifier: GPL-3.0-or-later
SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
-->

# TODO / Handoff

## Current tip

Base of this work line: `a7bcc21` (Initial checkin)

**Tip:** `99b0ccc` — soft shadows, MATCH banner, frame cap

## Done

- [x] Match-point lighting, serve countdown, vsync
- [x] Soft shadows (4-sample disk) on **CPU and GPU**
- [x] Block-letter **MATCH** flash on match point
- [x] Frame-time sleep when vsync off (`target_fps` in config, default 60; 0 = uncapped)

## Open / follow-ups

- [ ] Optional higher soft-shadow sample count via config
- [ ] WIN/LOSE voxel banners on game over
- [ ] Subtle post-tone (exposure) on GPU only

## Bundle naming

Base short: `a7bcc21`
Previous: `kugelmatch-015.1-matchpoint-countdown-vsync-a7bcc21.bundle`
Next: `kugelmatch-016.1-softshadow-match-banner-a7bcc21.bundle`

## Notes

- GPU: MAX_SPHERES=16, MAX_BOXES=128
- Config: vsync, target_fps (also alias `fps`)
