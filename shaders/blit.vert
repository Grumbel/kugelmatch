// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
#version 100
// Fullscreen triangle for FBO → window scale blit.
// u_uvScale maps the raytraced sub-rectangle (lower-left) into texture UVs.
attribute vec2 a_pos;
uniform vec2 u_uvScale;
varying vec2 v_uv;
void main() {
    vec2 base = a_pos * 0.5 + 0.5;
    v_uv = base * u_uvScale;
    gl_Position = vec4(a_pos, 0.0, 1.0);
}
