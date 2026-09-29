<!--
SPDX-License-Identifier: GPL-3.0-or-later
SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
-->

# TODO / Handoff

## Current tip

Base of this work line: `a7bcc21` (Initial checkin)

**Tip:** `49e68bd` — theme floor, skip replay, docs

## Done

- [x] Theme-tinted wall decos, hold exposure, slow-mo replay
- [x] Checker floor + side/back walls shift with theme
- [x] **Space** skips goal replay → immediate serve
- [x] README controls rewritten (full key map)

## Open / follow-ups

- [ ] Attract-mode input legend as voxel glyphs (optional)
- [ ] CI / packaging beyond Nix flake

## Bundle naming

Base short: `a7bcc21`
Previous: `kugelmatch-019.1-theme-decos-slowmo-a7bcc21.bundle`
Next: `kugelmatch-020.1-theme-floor-skip-replay-a7bcc21.bundle`

## Notes

- Theme drives paddles, lights, decos, floor, and walls
- Space during replay ends it and calls queueServe
