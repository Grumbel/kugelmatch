<!--
SPDX-License-Identifier: GPL-3.0-or-later
SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
-->

# TODO / Handoff

## Current tip

Base of this work line: `a7bcc21` (Initial checkin)

**Tip:** (pending) — v1.2.11 resize debounce

## Done

- [x] v1.2.1–1.2.10
- [x] Debounced window resize (avoid FB/FBO thrash while dragging)
- [x] Version **1.2.11**

## Open / follow-ups

- [ ] Tag releases on upstream
- [ ] Camera orbit / scoreboard face / 7-seg / mirrored text (reported, not all landed)

## Bundle naming

Base short: `a7bcc21`
Previous: `kugelmatch-034.1-controls-angle-a7bcc21.bundle`
This: `kugelmatch-035.1-resize-debounce-a7bcc21.bundle`

## Notes

- `noteWindowSize` / `flushPendingResize` / `applyWindowSize`; settle = 100ms
- During cooldown, keep rendering at `appliedWinW_/H_`
