#!/bin/bash
# SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
# SPDX-License-Identifier: GPL-3.0-or-later
#
# KugelMatch: PortMaster launcher for the R36S / ArkOS (and alike).

XDG_DATA_HOME=${XDG_DATA_HOME:-$HOME/.local/share}
if [ -d "/opt/system/Tools/PortMaster/" ]; then
  controlfolder="/opt/system/Tools/PortMaster"
elif [ -d "/opt/tools/PortMaster/" ]; then
  controlfolder="/opt/tools/PortMaster"
elif [ -d "$XDG_DATA_HOME/PortMaster/" ]; then
  controlfolder="$XDG_DATA_HOME/PortMaster"
else
  controlfolder="/roms/ports/PortMaster"
fi
source "$controlfolder/control.txt"
[ -f "${controlfolder}/mod_${CFW_NAME}.txt" ] && source "${controlfolder}/mod_${CFW_NAME}.txt"
get_controls

GAMEDIR="/$directory/ports/kugelmatch"
cd "$GAMEDIR" || exit 1
> "$GAMEDIR/log.txt" && exec > >(tee "$GAMEDIR/log.txt") 2>&1

# Settings stay with the port (SD card), not under ~/.config.
mkdir -p "$GAMEDIR/conf"
export XDG_CONFIG_HOME="$GAMEDIR/conf"
export XDG_STATE_HOME="$GAMEDIR/conf"
# PortMaster pad layout for SDL's game controller API.
export SDL_GAMECONTROLLERCONFIG="$sdl_controllerconfig"

# gptokeyb: Select+Start quits.
$GPTOKEYB "kugelmatch" &
pm_platform_helper "$GAMEDIR/kugelmatch"
./kugelmatch --gpu --fullscreen
pm_finish
