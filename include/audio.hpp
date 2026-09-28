// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2024–2026 Ingo Ruhnke <grumbel@gmail.com>
#pragma once

#include <SDL.h>
#include <atomic>
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

    // pitch: 0.7–1.4 typical; volume: 0–1 (scaled by master)
    void playClank(float pitch = 1.0f, float volume = 0.7f);
    void playSoftThud(float pitch = 1.0f, float volume = 0.4f);

    void setMuted(bool muted) { muted_.store(muted); }
    bool muted() const { return muted_.load(); }
    void toggleMute() { muted_.store(!muted_.load()); }

    // 0–1 master gain
    void setMasterVolume(float v);
    float masterVolume() const { return master_.load(); }

    bool ready() const { return ready_; }

private:
    static void SDLCALL callback(void* userdata, Uint8* stream, int len);
    void mix(float* out, int frames);

    SDL_AudioDeviceID device_ = 0;
    SDL_AudioSpec spec_{};
    bool ready_ = false;

    std::atomic<bool> muted_{false};
    std::atomic<float> master_{0.85f};

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
