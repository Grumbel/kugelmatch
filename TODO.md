<!--
SPDX-License-Identifier: GPL-3.0-or-later
SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
-->

# TODO / Handoff

## Current tip

Base of this work line: `a7bcc21` (Initial checkin)

**Tip:** `bf3b4c1` — WIN/LOSE, shadow samples, exposure

## Done

- [x] Soft shadows, MATCH banner, frame cap
- [x] Configurable soft-shadow samples 1/2/4/8 (key **7**, config `shadow_samples`)
- [x] Voxel **WIN** / **LOSE** banners on game over
- [x] Exposure on CPU + GPU (`u_exposure` / scene.exposure); slight pulse on game over

## Open / follow-ups

- [ ] Manual exposure key adjust
- [ ] AI personality / paddle color themes
- [ ] Replay last point camera

## Bundle naming

Base short: `a7bcc21`
Previous: `kugelmatch-016.1-softshadow-match-banner-a7bcc21.bundle`
Next: `kugelmatch-017.1-winlose-shadow-samples-a7bcc21.bundle`

## Notes

- GPU: MAX_SPHERES=16, MAX_BOXES=128
- Soft-shadow loop is fixed at 8 with early break on sample count (GLSL-friendly)
