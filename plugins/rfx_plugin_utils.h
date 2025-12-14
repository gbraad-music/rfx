/*
 * RFX Plugin Utilities - Shared code for all RFX plugins
 * Copyright (C) 2024
 * SPDX-License-Identifier: ISC
 */

#ifndef RFX_PLUGIN_UTILS_H
#define RFX_PLUGIN_UTILS_H

#include <cstring>

namespace RFX {

/**
 * Process stereo audio with an effect that expects interleaved float32 data
 * Handles interleaving/deinterleaving automatically
 */
template<typename EffectType, typename ProcessFunc>
inline void processStereo(
    const float** inputs,
    float** outputs,
    uint32_t frames,
    EffectType* effect,
    ProcessFunc processFunc,
    int sampleRate)
{
    if (!effect) {
        std::memcpy(outputs[0], inputs[0], sizeof(float) * frames);
        std::memcpy(outputs[1], inputs[1], sizeof(float) * frames);
        return;
    }

    // Allocate interleaved buffer
    float* buffer = new float[frames * 2];

    // Interleave: L,R,L,R,...
    for (uint32_t i = 0; i < frames; i++) {
        buffer[i * 2]     = inputs[0][i];
        buffer[i * 2 + 1] = inputs[1][i];
    }

    // Process
    processFunc(effect, buffer, frames, sampleRate);

    // Deinterleave: L,R,L,R,... -> L,L,L,... and R,R,R,...
    for (uint32_t i = 0; i < frames; i++) {
        outputs[0][i] = buffer[i * 2];
        outputs[1][i] = buffer[i * 2 + 1];
    }

    delete[] buffer;
}

} // namespace RFX

#endif // RFX_PLUGIN_UTILS_H
