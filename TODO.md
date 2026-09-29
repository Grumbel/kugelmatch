<!--
SPDX-License-Identifier: GPL-3.0-or-later
SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
-->

# TODO / Handoff

## Current tip

Base of this work line: `a7bcc21` (Initial checkin)

**Tip:** (pending) — theme decos, hold exposure, slow-mo replay

## Done

- [x] Themes, exposure keys, goal replay
- [x] Wall decos tint with active theme
- [x] Hold-to-ramp exposure (8/9/[ ]); persist on key-up
- [x] Slow-mo goal replay (default on; **F9** toggles; config `slowmo_replay`)

## Open / follow-ups

- [ ] Checker floor color shift per theme
- [ ] Replay skip with Space
- [ ] Package/release notes for keys

## Bundle naming

Base short: `a7bcc21`
Previous: `kugelmatch-018.1-themes-exposure-replay-a7bcc21.bundle`
Next: `kugelmatch-019.1-theme-decos-slowmo-a7bcc21.bundle`

## Notes

- Exposure no longer writes config every frame; KEYUP persists
- Slow-mo slows animTime during replay and widens orbit
