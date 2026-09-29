<!--
SPDX-License-Identifier: GPL-3.0-or-later
SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
-->

# TODO / Handoff

## Current tip

Base of this work line: `a7bcc21` (Initial checkin)

**Tip:** `59b0eae` — v1.2.1 PAUSE, CPU scale

## Done

- [x] v1.2.0 alphabet / CHANGELOG
- [x] Voxel **PAUSE** banner + dimmed pause lighting
- [x] CPU `cpu_scale` (supersample/undersample before max clamp)
- [x] Version **1.2.1**

## Open / follow-ups

- [ ] Tag releases on upstream
- [ ] GPU render scale / quality preset bundle key

## Bundle naming

Base short: `a7bcc21`
Previous: `kugelmatch-024.1-v1.2.0-alphabet-a7bcc21.bundle`
Next: `kugelmatch-025.1-pause-cpu-scale-a7bcc21.bundle`

## Notes

- `cpu_scale` default 1.0; useful values 0.5 (faster) or 1.5 (sharper, costlier)
