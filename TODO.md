<!--
SPDX-License-Identifier: GPL-3.0-or-later
SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
-->

# TODO / Handoff

## Current tip

Base of this work line: `a7bcc21` (Initial checkin)

**Tip:** (pending) — difficulty, cameras, 2P, options geometry, attract demo

## Done

- [x] Raytraced scoreboard, states, native res, F8 backend switch
- [x] Attract demo AI-vs-AI rally under orbit camera
- [x] Difficulty Easy/Normal/Hard (key **1**) — AI speed + prediction
- [x] Points to win 7/11/15/21 (key **2**) — floor digits + match end
- [x] Camera modes Paddle/High/Sideline (key **3**)
- [x] Local 2-player (key **4**): P1 A/D, P2 arrows
- [x] Raytraced options readout: difficulty pillars, cam pads, 1P/2P orb

## Open / follow-ups

- [ ] Config file persistence
- [ ] Reflections quality toggle
- [ ] Block-letter title in attract (box glyphs)
- [ ] Winner celebration camera beat

## Bundle naming

Base short: `a7bcc21`
Previous: `kugelmatch-012.1-scoreboard-states-a7bcc21.bundle`
Next: `kugelmatch-013.1-options-2p-cameras-a7bcc21.bundle`

## Notes

- GPU limits: MAX_SPHERES=16, MAX_BOXES=64 — keep shader + header + embedded in sync.
- Options are scene geometry, not overlays.
