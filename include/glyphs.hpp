// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2026 Ingo Ruhnke <grumbel@gmail.com>
#pragma once

#include "scene.hpp"

// Shared 5×7 voxel letterforms and 7-segment digits for in-scene text.
namespace glyphs {

// Returns 7 row bitmasks (5 bits wide, bit 4 = leftmost) for A–Z / 0–9.
// Unknown characters map to a solid block.
const int* rowsFor(char ch);

// Emit a word as reflective axis-aligned boxes into the scene.
void addWord(Scene& scene, const char* word, float originX, float originY,
             float originZ, float cell, float gap, const Vec3& color,
             float reflectivity = 0.45f);

// Seven-segment digit at (ox, oy, oz) center.
void addDigit7(Scene& scene, float ox, float oy, float oz, int digit,
               const Vec3& color, float reflectivity = 0.45f);

} // namespace glyphs
