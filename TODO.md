<!--
SPDX-License-Identifier: GPL-3.0-or-later
SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
-->

# TODO / Handoff

## Current tip

Base of this work line: `a7bcc21` (Initial checkin)

**Tip:** (pending) — v1.2.5 physics substeps + quality label

## Done

- [x] v1.2.1–1.2.4 (pause, quality, GPU FBO scale, CPU scale buffer fix)
- [x] Physics substeps (anti-tunnel)
- [x] Quality HUD CUST when settings diverge
- [x] Intro orbit continuity
- [x] Version **1.2.5**

## Open / follow-ups

- [ ] Tag releases on upstream

## Bundle naming

Base short: `a7bcc21`
Previous: `kugelmatch-028.1-cpu-scale-fix-a7bcc21.bundle`
This: `kugelmatch-029.1-physics-quality-label-a7bcc21.bundle`

## Notes

- Substep count capped at 8; step distance ~0.45 * paddle depth
- `qualityLabel()` matches preset table or returns CUST
