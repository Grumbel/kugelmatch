<!--
SPDX-License-Identifier: GPL-3.0-or-later
SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
-->

# TODO / Handoff

## Current tip

Base of this work line: `a7bcc21` (Initial checkin)

**Tip:** camera rig + animation polish + renderer fixes (see CHANGELOG)

## Done

- [x] Camera up basis (no upside-down world)
- [x] docs/COORDINATES.md
- [x] CLI overrides config file; detailed --help
- [x] VERSION-driven versioning (1.2.14-dev)
- [x] Camera rig (smoothed shots, impact feel), sideline/high/paddle recomposed
- [x] Animation polish (countdown, ball spawn-in, paddle recoil, score pop, goal roll-out)
- [x] Renderer: GPU/CPU last-bounce parity, filtered checkers, one-sided walls, end walls
- [x] F12 screenshots + headless dev flags (`--shot`, `--fixed-dt`, `--start`, ...)

## Open / follow-ups

- [ ] Tag releases on upstream
- [ ] Simulation is not reproducible run-to-run (the audio synth probably consumes `rand()`);
      give audio its own RNG so `--fixed-dt` scenario runs are fully deterministic
- [ ] Geometry edge anti-aliasing (silhouettes of the ball/paddles are still aliased)
- [ ] Verify the GPU backend on real hardware (only software GL was available while developing)
- [ ] Emissive material for the ceiling lamp (currently only a specular streak)

## Bundle naming

Base short: `a7bcc21`
Previous: `kugelmatch-039.1-coords-cli-a7bcc21.bundle`
This: `kugelmatch-040.1-camera-polish-a7bcc21.bundle`
