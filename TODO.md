<!--
SPDX-License-Identifier: GPL-3.0-or-later
SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
-->

# TODO / Handoff

## Current tip

Base of this work line: `a7bcc21` (Initial checkin)

**Tip:** (pending) — match-point light, serve countdown, vsync

## Done

- [x] Title, config, reflection quality, win camera
- [x] Match-point warm lighting pulse + scoreboard digit glow
- [x] Raytraced serve countdown (3-2-1 boxes above midfield)
- [x] Vsync toggle (key **6**) + config `vsync=`

## Open / follow-ups

- [ ] Soft shadow samples on GPU
- [ ] Block-letter "MATCH" flash on match point
- [ ] Optional frame-time sleep cap when vsync off

## Bundle naming

Base short: `a7bcc21`
Previous: `kugelmatch-014.1-title-config-quality-a7bcc21.bundle`
Next: `kugelmatch-015.1-matchpoint-countdown-vsync-a7bcc21.bundle`

## Notes

- GPU limits: MAX_SPHERES=16, MAX_BOXES=128
- Config keys include: vsync
