<!--
SPDX-License-Identifier: GPL-3.0-or-later
SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
-->

# TODO / Handoff

## Current tip

Base of this work line: `a7bcc21` (Initial checkin)

**Tip:**  — copyright year 2026

## Done

- [x] Dual backends, audio, polish, resize/fullscreen, GLSL fixes
- [x] Wall decos, swapped controls
- [x] GPU raytraces at drawable/window resolution (CPU stays 640×480)

## Open / follow-ups

- [ ] Optional: compute-shader path
- [ ] Optional: multi-sample soft shadows on GPU
- [ ] Score overlay in-framebuffer (bitmap font)
- [ ] Windows / macOS build notes

## Bundle naming for this stack

Base short: `a7bcc21`
Previous: `kugelmatch-008.1-fix-glsl-recursion-a7bcc21.bundle`
Previous: 
Next: 

## Notes for next agent

- GPU uses `SDL_GL_GetDrawableSize` each frame; aspect is `rtW_/rtH_`.
- CPU still letterboxes via `SDL_RenderSetLogicalSize(640,480)`.
- After a new tip bundle, delete superseded intermediate bundles for this project.
