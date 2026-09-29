<!--
SPDX-License-Identifier: GPL-3.0-or-later
SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
-->

# TODO / Handoff

## Current tip

Base of this work line: `a7bcc21` (Initial checkin)

**Tip:** (pending) — v1.2.4 CPU scale buffer fix

## Done

- [x] v1.2.1 PAUSE / CPU scale
- [x] Quality presets Low/Medium/High/Ultra (**Q** / **F10**)
- [x] Version **1.2.2**
- [x] GPU-side resolution scale FBO (shared with quality / `cpu_scale`)
- [x] Version **1.2.3**
- [x] Fix CPU framebuffer size vs window size when scaled
- [x] Fix vsync recreate double-scale
- [x] `render_scale` config alias
- [x] Version **1.2.4**

## Open / follow-ups

- [ ] Tag releases on upstream

## Bundle naming

Base short: `a7bcc21`
Previous: `kugelmatch-027.1-gpu-fbo-scale-a7bcc21.bundle`
This: `kugelmatch-028.1-cpu-scale-fix-a7bcc21.bundle`

## Notes

- CPU render must use fbW_/fbH_ after ensureCpuFramebuffer (scale/clamp)
- Vsync CPU recreate must pass *window* size into ensureCpuFramebuffer
