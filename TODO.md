<!--
SPDX-License-Identifier: GPL-3.0-or-later
SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
-->

# TODO / Handoff

## Current tip

Base of this work line: `a7bcc21` (Initial checkin)

**Tip:** `b820f7c` — CI, attract SPACE/PONG/GO

## Done

- [x] Attract SPACE hint, ball trail, CPU cap, Makefile
- [x] Attract hint cycles **SPACE → PONG → GO**
- [x] GitHub Actions CI (Ubuntu CMake build + dummy SDL smoke)
- [x] Trail positions reset on serve

## Open / follow-ups

- [ ] Nix CI job (optional)
- [ ] Shared glyph atlas helper to DRY banner code

## Bundle naming

Base short: `a7bcc21`
Previous: `kugelmatch-021.1-attract-hint-trail-cpu-cap-a7bcc21.bundle`
Next: `kugelmatch-022.1-ci-attract-cycle-a7bcc21.bundle`

## Notes

- CI uses SDL_VIDEODRIVER=dummy; timeout treats 124 as success
