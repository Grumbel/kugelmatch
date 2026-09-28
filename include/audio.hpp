// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2024–2026 Ingo Ruhnke <grumbel@gmail.com>
#pragma once

#include <SDL.h>
#include <cstdint>
#include <mutex>
#include <vector>

// Procedural metallic clanks via SDL audio (no external sample files).
class Audio {
public:
    Audio() = default;
    ~Audio();

    bool init();
    void shutdown();

    // pitch: 0.7–1.4 typical; volume: 0–1
    void playClank(float pitch = 1.0f, float volume = 0.7f);
    void playSoftThud(float pitch = 1.0f, float volume = 0.4f);

    bool ready() const { return ready_; }

private:
    static void SDLCALL callback(void* userdata, Uint8* stream, int len);
    void mix(float* out, int frames);

    SDL_AudioDeviceID device_ = 0;
    SDL_AudioSpec spec_{};
    bool ready_ = false;

    struct Voice {
        double phase = 0.0;
        double phase2 = 0.0;
        float env = 0.0f;
        float pitch = 1.0f;
        float volume = 0.0f;
        float noiseState = 0.0f;
        int samplesLeft = 0;
        bool soft = false;
    };

    std::mutex mutex_;
    std::vector<Voice> voices_;
    static constexpr int MAX_VOICES = 8;
};
