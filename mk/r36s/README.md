<!--
SPDX-License-Identifier: GPL-3.0-or-later
SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
-->
# KugelMatch on R36S (ArkOS)

## Install

1. Copy the **contents** of the PortMaster package so you have:
   - `/roms/ports/KugelMatch.sh` (launcher)
   - `/roms/ports/kugelmatch/kugelmatch` (binary)
2. Launch **KugelMatch** from PortMaster / Ports.

Do **not** run `./KugelMatch.sh` from inside `/roms/ports/kugelmatch` — the
script expects to live one level up and `cd` into the game directory.

## Controls

| Key / pad | Action |
|-----------|--------|
| D-pad / stick | Move paddle |
| A / South | Start match / confirm |
| Start | Pause |
| Select+Start | Quit (gptokeyb) |

Default backend is the GLES2 raytracer (`--gpu`). For the software path, edit
the launcher line to `./kugelmatch --cpu --fullscreen`.
