<!--
SPDX-License-Identifier: GPL-3.0-or-later
SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
-->

# TODO / Handoff

## Current tip

Base of this work line: `a7bcc21` (Initial checkin)

**Tip:** see `git rev-parse --short HEAD` after pull — v1.2.3 GPU FBO render scale

## Done

- [x] v1.2.1 PAUSE / CPU scale
- [x] Quality presets Low/Medium/High/Ultra (**Q** / **F10**)
- [x] Version **1.2.2**
- [x] GPU-side resolution scale FBO (shared with quality / `cpu_scale`)
- [x] Version **1.2.3**

## Open / follow-ups

- [ ] Tag releases on upstream
- [ ] Consider renaming config `cpu_scale` → `render_scale` (keep alias)

## Bundle naming

Base short: `a7bcc21`
Previous tip commit before this work: `7a1210c`
This bundle: `kugelmatch-027.1-gpu-fbo-scale-a7bcc21.bundle`

## Notes

- Quality sets maxBounces, shadowSamples, render scale together
- GPU uses FBO when RT size differs from drawable (scale ≠ 1 or max clamp)
- `GpuRaytracer::setRenderScale` / `setMaxResolution`; `Game::syncGpuScale()`
