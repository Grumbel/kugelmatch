#!/bin/bash
# SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
# SPDX-License-Identifier: GPL-3.0-or-later
cd "$(dirname "$0")"
export LD_LIBRARY_PATH="$PWD/lib:${LD_LIBRARY_PATH:-}"
exec ./kugelmatch --gpu "$@"
