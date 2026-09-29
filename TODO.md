<!--
SPDX-License-Identifier: GPL-3.0-or-later
SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
-->

# TODO / Handoff

## Current tip

Base of this work line: `a7bcc21` (Initial checkin)

**Tip:** `828b2d0` — VERSION-driven versioning + --version

## Done

- [x] v1.2.1–1.2.13
- [x] Top-level `VERSION` is sole source of truth (`1.2.14-dev`)
- [x] CMake: `PROJECT_VERSION_FULL`, generated `version.hpp`, optional local git metadata
- [x] Flake: `-dev` → `.{revCount}+g{shortRev}`; pass `-DPROJECT_VERSION_FULL`
- [x] CLI `--version`
- [x] Changelog section headers restored

## Open / follow-ups

- [ ] Tag releases on upstream (set VERSION without -dev, tag vX.Y.Z, then bump -dev)

## Bundle naming

Base short: `a7bcc21`
Previous: `kugelmatch-037.1-visual-camera-glyphs-a7bcc21.bundle`
This: `kugelmatch-038.1-version-from-file-a7bcc21.bundle`

## Notes

- Release: VERSION=`1.2.14` → commit → tag `v1.2.14` → bump to `1.2.15-dev`
