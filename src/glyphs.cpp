// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2026 Ingo Ruhnke <grumbel@gmail.com>
#include "glyphs.hpp"

#include <algorithm>
#include <cctype>

namespace glyphs {
namespace {

void pushBox(Scene& scene, const Vec3& minb, const Vec3& maxb, const Vec3& color,
             float reflectivity) {
    Box b;
    b.minb = minb;
    b.maxb = maxb;
    b.color = color;
    b.reflectivity = reflectivity;
    scene.boxes.push_back(b);
}

constexpr int kSegMask[10] = {
    0b1110111, 0b0010010, 0b1011101, 0b1011011, 0b0111010,
    0b1101011, 0b1101111, 0b1010010, 0b1111111, 0b1111011,
};

} // namespace

const int* rowsFor(char ch) {
    static const int blk[7] = {0b11111, 0b11111, 0b11111, 0b11111, 0b11111, 0b11111, 0b11111};
    static const int A[7] = {0b01110, 0b10001, 0b10001, 0b11111, 0b10001, 0b10001, 0b10001};
    static const int C[7] = {0b01110, 0b10001, 0b10000, 0b10000, 0b10000, 0b10001, 0b01110};
    static const int E[7] = {0b11111, 0b10000, 0b10000, 0b11110, 0b10000, 0b10000, 0b11111};
    static const int G[7] = {0b01110, 0b10001, 0b10000, 0b10111, 0b10001, 0b10001, 0b01110};
    static const int H[7] = {0b10001, 0b10001, 0b10001, 0b11111, 0b10001, 0b10001, 0b10001};
    static const int I[7] = {0b01110, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b01110};
    static const int K[7] = {0b10001, 0b10010, 0b10100, 0b11000, 0b10100, 0b10010, 0b10001};
    static const int L[7] = {0b10000, 0b10000, 0b10000, 0b10000, 0b10000, 0b10000, 0b11111};
    static const int M[7] = {0b10001, 0b11011, 0b10101, 0b10001, 0b10001, 0b10001, 0b10001};
    static const int N[7] = {0b10001, 0b11001, 0b10101, 0b10011, 0b10001, 0b10001, 0b10001};
    static const int O[7] = {0b01110, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b01110};
    static const int P[7] = {0b11110, 0b10001, 0b10001, 0b11110, 0b10000, 0b10000, 0b10000};
    static const int S[7] = {0b01111, 0b10000, 0b10000, 0b01110, 0b00001, 0b00001, 0b11110};
    static const int T[7] = {0b11111, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100};
    static const int U[7] = {0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b01110};
    static const int W[7] = {0b10001, 0b10001, 0b10001, 0b10101, 0b10101, 0b11011, 0b10001};

    ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
    switch (ch) {
    case 'A': return A;
    case 'C': return C;
    case 'E': return E;
    case 'G': return G;
    case 'H': return H;
    case 'I': return I;
    case 'K': return K;
    case 'L': return L;
    case 'M': return M;
    case 'N': return N;
    case 'O': return O;
    case 'P': return P;
    case 'S': return S;
    case 'T': return T;
    case 'U': return U;
    case 'W': return W;
    default: return blk;
    }
}

void addWord(Scene& scene, const char* word, float originX, float originY,
             float originZ, float cell, float gap, const Vec3& color,
             float reflectivity) {
    if (!word) {
        return;
    }
    for (int ci = 0; word[ci]; ++ci) {
        const int* rows = rowsFor(word[ci]);
        float ox = originX + ci * (5 * cell + gap);
        for (int r = 0; r < 7; ++r) {
            int bits = rows[r];
            for (int c = 0; c < 5; ++c) {
                if (bits & (1 << (4 - c))) {
                    float x0 = ox + c * cell;
                    float y0 = originY + (6 - r) * cell;
                    pushBox(scene, Vec3(x0, y0, originZ),
                            Vec3(x0 + cell * 0.85f, y0 + cell * 0.85f, originZ + cell * 0.7f),
                            color, reflectivity);
                }
            }
        }
    }
}

void addDigit7(Scene& scene, float ox, float oy, float oz, int digit,
               const Vec3& color, float reflectivity) {
    digit = std::max(0, std::min(9, digit));
    const int mask = kSegMask[digit];
    const float t = 0.07f;
    const float w = 0.38f;
    const float h = 0.55f;
    const float d = 0.08f;

    auto seg = [&](int bit, float x0, float y0, float x1, float y1) {
        if ((mask & bit) == 0) {
            return;
        }
        pushBox(scene, Vec3(ox + x0, oy + y0, oz - d * 0.5f),
                Vec3(ox + x1, oy + y1, oz + d * 0.5f), color, reflectivity);
    };

    seg(0b1000000, -w * 0.5f, h - t, w * 0.5f, h);
    seg(0b0000010, w * 0.5f - t, 0.0f, w * 0.5f, h - t);
    seg(0b0000100, w * 0.5f - t, -h, w * 0.5f, 0.0f);
    seg(0b0000001, -w * 0.5f, -h, w * 0.5f, -h + t);
    seg(0b0001000, -w * 0.5f, -h, -w * 0.5f + t, 0.0f);
    seg(0b0100000, -w * 0.5f, 0.0f, -w * 0.5f + t, h - t);
    seg(0b0010000, -w * 0.5f, -t * 0.5f, w * 0.5f, t * 0.5f);
}

} // namespace glyphs
