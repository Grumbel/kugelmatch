// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2024–2026 Ingo Ruhnke <grumbel@gmail.com>
#include "audio.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

Audio::~Audio() {
    shutdown();
}

bool Audio::init() {
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) {
        std::fprintf(stderr, "SDL_INIT_AUDIO failed: %s\n", SDL_GetError());
        return false;
    }

    SDL_AudioSpec want{};
    want.freq = 44100;
    want.format = AUDIO_F32SYS;
    want.channels = 1;
    want.samples = 512;
    want.callback = callback;
    want.userdata = this;

    device_ = SDL_OpenAudioDevice(nullptr, 0, &want, &spec_, 0);
    if (!device_) {
        std::fprintf(stderr, "SDL_OpenAudioDevice failed: %s\n", SDL_GetError());
        return false;
    }

    voices_.resize(MAX_VOICES);
    SDL_PauseAudioDevice(device_, 0);
    ready_ = true;
    return true;
}

void Audio::shutdown() {
    if (device_) {
        SDL_CloseAudioDevice(device_);
        device_ = 0;
    }
    ready_ = false;
}

void Audio::setMasterVolume(float v) {
    master_.store(std::max(0.0f, std::min(1.0f, v)));
}

void Audio::playClank(float pitch, float volume) {
    if (!ready_ || muted_.load()) {
        return;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    int best = -1;
    float bestEnv = 1e9f;
    for (int i = 0; i < MAX_VOICES; ++i) {
        if (voices_[static_cast<size_t>(i)].samplesLeft <= 0) {
            best = i;
            break;
        }
        if (voices_[static_cast<size_t>(i)].env < bestEnv) {
            bestEnv = voices_[static_cast<size_t>(i)].env;
            best = i;
        }
    }
    if (best < 0) {
        return;
    }
    Voice& v = voices_[static_cast<size_t>(best)];
    v.phase = 0.0;
    v.phase2 = 0.0;
    v.env = 1.0f;
    v.pitch = std::max(0.4f, std::min(2.0f, pitch));
    v.volume = std::max(0.0f, std::min(1.0f, volume));
    v.noiseState = 0.5f;
    v.samplesLeft = static_cast<int>(spec_.freq * 0.18f);
    v.soft = false;
}

void Audio::playSoftThud(float pitch, float volume) {
    if (!ready_ || muted_.load()) {
        return;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    int best = -1;
    for (int i = 0; i < MAX_VOICES; ++i) {
        if (voices_[static_cast<size_t>(i)].samplesLeft <= 0) {
            best = i;
            break;
        }
    }
    if (best < 0) {
        best = 0;
    }
    Voice& v = voices_[static_cast<size_t>(best)];
    v.phase = 0.0;
    v.phase2 = 0.0;
    v.env = 1.0f;
    v.pitch = std::max(0.4f, std::min(2.0f, pitch));
    v.volume = std::max(0.0f, std::min(1.0f, volume));
    v.noiseState = 0.3f;
    v.samplesLeft = static_cast<int>(spec_.freq * 0.12f);
    v.soft = true;
}

void SDLCALL Audio::callback(void* userdata, Uint8* stream, int len) {
    auto* self = static_cast<Audio*>(userdata);
    auto* out = reinterpret_cast<float*>(stream);
    int frames = len / static_cast<int>(sizeof(float));
    std::memset(stream, 0, static_cast<size_t>(len));
    if (self->muted_.load()) {
        return;
    }
    self->mix(out, frames);
}

void Audio::mix(float* out, int frames) {
    std::lock_guard<std::mutex> lock(mutex_);
    const float invFreq = 1.0f / static_cast<float>(spec_.freq);
    const float master = master_.load();

    for (int i = 0; i < frames; ++i) {
        float sample = 0.0f;
        for (auto& v : voices_) {
            if (v.samplesLeft <= 0) {
                continue;
            }

            float f1 = (v.soft ? 180.0f : 420.0f) * v.pitch;
            float f2 = (v.soft ? 90.0f : 980.0f) * v.pitch;
            float f3 = (v.soft ? 60.0f : 1550.0f) * v.pitch;

            v.phase += 2.0 * 3.141592653589793 * static_cast<double>(f1) * invFreq;
            v.phase2 += 2.0 * 3.141592653589793 * static_cast<double>(f2) * invFreq;
            if (v.phase > 2.0 * 3.141592653589793) {
                v.phase -= 2.0 * 3.141592653589793;
            }
            if (v.phase2 > 2.0 * 3.141592653589793) {
                v.phase2 -= 2.0 * 3.141592653589793;
            }

            v.noiseState = v.noiseState * 1.0003f + 0.13f;
            if (v.noiseState > 1.0f) {
                v.noiseState -= 1.0f + static_cast<float>(static_cast<int>(v.noiseState));
            }
            float noise = v.noiseState * 2.0f - 1.0f;

            float tone = static_cast<float>(
                0.55 * std::sin(v.phase) +
                0.30 * std::sin(v.phase2) +
                0.15 * std::sin(2.0 * 3.141592653589793 * static_cast<double>(f3) *
                                static_cast<double>(v.samplesLeft) * invFreq));

            float metallic = tone * (0.7f + 0.3f * noise) + noise * (v.soft ? 0.05f : 0.18f);

            float decay = v.soft ? 0.9992f : 0.9985f;
            v.env *= decay;

            sample += metallic * v.env * v.volume * 0.35f * master;
            --v.samplesLeft;
        }

        if (sample > 1.0f) {
            sample = 1.0f;
        } else if (sample < -1.0f) {
            sample = -1.0f;
        }
        out[i] = sample;
    }
}
