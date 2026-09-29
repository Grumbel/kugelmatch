<!--
SPDX-License-Identifier: GPL-3.0-or-later
SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
-->

# TODO / Handoff

## Current tip

Base of this work line: `a7bcc21` (Initial checkin)

**Tip:** (pending) — shared glyphs module, Nix CI job

## Done

- [x] CI Ubuntu + **Nix** build jobs
- [x] Shared `glyphs` module (`addWord`, `addDigit7`) — banners/title/hints DRY
- [x] Attract SPACE/PONG/GO, trails, themes, scoreboard, etc.

## Open / follow-ups

- [ ] Expand glyph alphabet as needed
- [ ] Version bump / release tag notes

## Bundle naming

Base short: `a7bcc21`
Previous: `kugelmatch-022.1-ci-attract-cycle-a7bcc21.bundle`
Next: `kugelmatch-023.1-glyphs-nix-ci-a7bcc21.bundle`

## Notes

- New files: `include/glyphs.hpp`, `src/glyphs.cpp`
- Banners call `glyphs::addWord` / `glyphs::addDigit7`
