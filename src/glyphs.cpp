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
    // bit6=a top, 5=b UR, 4=c LR, 3=d bot, 2=e LL, 1=f UL, 0=g mid
    0b1111110, 0b0110000, 0b1101101, 0b1111001, 0b0110011,
    0b1011011, 0b1011111, 0b1110000, 0b1111111, 0b1111011,
};

// 5×7 bitmaps — row 0 is top; bit 4 is leftmost
constexpr int A_[7] = {0b01110, 0b10001, 0b10001, 0b11111, 0b10001, 0b10001, 0b10001};
constexpr int B_[7] = {0b11110, 0b10001, 0b10001, 0b11110, 0b10001, 0b10001, 0b11110};
constexpr int C_[7] = {0b01110, 0b10001, 0b10000, 0b10000, 0b10000, 0b10001, 0b01110};
constexpr int D_[7] = {0b11110, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b11110};
constexpr int E_[7] = {0b11111, 0b10000, 0b10000, 0b11110, 0b10000, 0b10000, 0b11111};
constexpr int F_[7] = {0b11111, 0b10000, 0b10000, 0b11110, 0b10000, 0b10000, 0b10000};
constexpr int G_[7] = {0b01110, 0b10001, 0b10000, 0b10111, 0b10001, 0b10001, 0b01110};
constexpr int H_[7] = {0b10001, 0b10001, 0b10001, 0b11111, 0b10001, 0b10001, 0b10001};
constexpr int I_[7] = {0b01110, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b01110};
constexpr int J_[7] = {0b00111, 0b00010, 0b00010, 0b00010, 0b00010, 0b10010, 0b01100};
constexpr int K_[7] = {0b10001, 0b10010, 0b10100, 0b11000, 0b10100, 0b10010, 0b10001};
constexpr int L_[7] = {0b10000, 0b10000, 0b10000, 0b10000, 0b10000, 0b10000, 0b11111};
constexpr int M_[7] = {0b10001, 0b11011, 0b10101, 0b10001, 0b10001, 0b10001, 0b10001};
constexpr int N_[7] = {0b10001, 0b11001, 0b10101, 0b10011, 0b10001, 0b10001, 0b10001};
constexpr int O_[7] = {0b01110, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b01110};
constexpr int P_[7] = {0b11110, 0b10001, 0b10001, 0b11110, 0b10000, 0b10000, 0b10000};
constexpr int Q_[7] = {0b01110, 0b10001, 0b10001, 0b10001, 0b10101, 0b10010, 0b01101};
constexpr int R_[7] = {0b11110, 0b10001, 0b10001, 0b11110, 0b10100, 0b10010, 0b10001};
constexpr int S_[7] = {0b01111, 0b10000, 0b10000, 0b01110, 0b00001, 0b00001, 0b11110};
constexpr int T_[7] = {0b11111, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100};
constexpr int U_[7] = {0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b01110};
constexpr int V_[7] = {0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b01010, 0b00100};
constexpr int W_[7] = {0b10001, 0b10001, 0b10001, 0b10101, 0b10101, 0b11011, 0b10001};
constexpr int X_[7] = {0b10001, 0b10001, 0b01010, 0b00100, 0b01010, 0b10001, 0b10001};
constexpr int Y_[7] = {0b10001, 0b10001, 0b01010, 0b00100, 0b00100, 0b00100, 0b00100};
constexpr int Z_[7] = {0b11111, 0b00001, 0b00010, 0b00100, 0b01000, 0b10000, 0b11111};
constexpr int blk[7] = {0b11111, 0b11111, 0b11111, 0b11111, 0b11111, 0b11111, 0b11111};
constexpr int spc[7] = {0, 0, 0, 0, 0, 0, 0};

} // namespace

const int* rowsFor(char ch) {
    if (ch == ' ' || ch == '_') {
        return spc;
    }
    ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
    switch (ch) {
    case 'A': return A_;
    case 'B': return B_;
    case 'C': return C_;
    case 'D': return D_;
    case 'E': return E_;
    case 'F': return F_;
    case 'G': return G_;
    case 'H': return H_;
    case 'I': return I_;
    case 'J': return J_;
    case 'K': return K_;
    case 'L': return L_;
    case 'M': return M_;
    case 'N': return N_;
    case 'O': return O_;
    case 'P': return P_;
    case 'Q': return Q_;
    case 'R': return R_;
    case 'S': return S_;
    case 'T': return T_;
    case 'U': return U_;
    case 'V': return V_;
    case 'W': return W_;
    case 'X': return X_;
    case 'Y': return Y_;
    case 'Z': return Z_;
    default: return blk;
    }
}

void addWord(Scene& scene, const char* word, float originX, float originY,
             float originZ, float cell, float gap, const Vec3& color,
             float reflectivity) {
    if (!word) {
        return;
    }
    // Merge consecutive set pixels in each row into one box (same look, far fewer
    // AABBs for the GPU/CPU raytracers — title/banner text is the main cost).
    for (int ci = 0; word[ci]; ++ci) {
        const int* rows = rowsFor(word[ci]);
        float ox = originX + ci * (5 * cell + gap);
        for (int r = 0; r < 7; ++r) {
            int bits = rows[r];
            int c = 0;
            while (c < 5) {
                if ((bits & (1 << (4 - c))) == 0) {
                    ++c;
                    continue;
                }
                const int c0 = c;
                while (c < 5 && (bits & (1 << (4 - c))) != 0) {
                    ++c;
                }
                const float x0 = ox + c0 * cell;
                const float x1 = ox + c * cell - cell * 0.05f;
                const float y0 = originY + (6 - r) * cell;
                pushBox(scene, Vec3(x0, y0, originZ),
                        Vec3(x1, y0 + cell * 0.95f, originZ + cell * 0.85f),
                        color, reflectivity);
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

    // a top, b upper-right, c lower-right, d bottom, e lower-left, f upper-left, g middle
    seg(0b1000000, -w * 0.5f, h - t, w * 0.5f, h);
    seg(0b0100000, w * 0.5f - t, 0.0f, w * 0.5f, h - t);
    seg(0b0010000, w * 0.5f - t, -h, w * 0.5f, 0.0f);
    seg(0b0001000, -w * 0.5f, -h, w * 0.5f, -h + t);
    seg(0b0000100, -w * 0.5f, -h, -w * 0.5f + t, 0.0f);
    seg(0b0000010, -w * 0.5f, 0.0f, -w * 0.5f + t, h - t);
    seg(0b0000001, -w * 0.5f, -t * 0.5f, w * 0.5f, t * 0.5f);
}

} // namespace glyphs
