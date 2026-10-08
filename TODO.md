<!--
SPDX-License-Identifier: GPL-3.0-or-later
SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
-->

# TODO / Handoff

## Current tip

Base of this work line: `a7bcc21` (Initial checkin)

**Tip:** GLES2 default renderer + multi-platform packaging (Win32/64, WASM, Android, R36S)

## Done

- [x] Camera up basis (no upside-down world)
- [x] docs/COORDINATES.md
- [x] CLI overrides config file; detailed --help
- [x] VERSION-driven versioning (1.2.14-dev)
- [x] Camera rig (smoothed shots, impact feel), sideline/high/paddle recomposed
- [x] Animation polish (countdown, ball spawn-in, paddle recoil, score pop, goal roll-out)
- [x] Renderer: GPU/CPU last-bounce parity, filtered checkers, one-sided walls, end walls
- [x] F12 screenshots + headless dev flags (`--shot`, `--fixed-dt`, `--start`, ...)
- [x] GPU backend: OpenGL ES 2.0 (GLSL ES 1.00, VBO, textured FBO blit)
- [x] GLES2 is the default backend (`--gpu`; `--cpu` still available)
- [x] Platform packaging skeleton
- [x] R36S: GLSL ES integer clamp fix + PortMaster launcher: Win32/64, WASM, Android, R36S (kurvenrausch-style)

## Open / follow-ups

- [ ] Tag releases on upstream
- [ ] Simulation is not reproducible run-to-run (the audio synth probably consumes `rand()`);
      give audio its own RNG so `--fixed-dt` scenario runs are fully deterministic
- [ ] Geometry edge anti-aliasing (silhouettes of the ball/paddles are still aliased)
- [ ] Verify the GPU backend on real hardware (only software GL was available while developing)
- [ ] Emissive material for the ceiling lamp (currently only a specular streak)
- [ ] End-to-end `nix build` for each port on a machine with network + unfree Android SDK
- [ ] Touch / gamepad on-screen controls for Android & R36S
- [ ] Windows `.ico` is currently a placeholder copied from another project — replace with KugelMatch art

## Bundle naming

Base short: `a7bcc21`
Previous: `kugelmatch-041.3-gles2-ctor-fix-a7bcc21.bundle`
Previous: `kugelmatch-042.1-gles2-default-ports-a7bcc21.bundle`
This: `kugelmatch-043.9-autoscale-target-fps-a7bcc21.bundle`
