/*
 * Regroove Compressor Implementation
 */

#include "fx_compressor.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

struct FXCompressor {
    // Parameters
    int enabled;
    float threshold;  // 0.0 - 1.0
    float ratio;      // 0.0 - 1.0
    float attack;     // 0.0 - 1.0
    float release;    // 0.0 - 1.0
    float makeup;     // 0.0 - 1.0

    // Internal state (stereo)
    float envelope[2];  // Envelope follower
    float rms[2];       // RMS state
};

FXCompressor* fx_compressor_create(void)
{
    FXCompressor* fx = (FXCompressor*)malloc(sizeof(FXCompressor));
    if (!fx) return NULL;

    fx->enabled = 0;
    fx->threshold = 0.4f;
    fx->ratio = 0.4f;
    fx->attack = 0.05f;
    fx->release = 0.5f;
    fx->makeup = 0.65f;

    fx_compressor_reset(fx);
    return fx;
}

void fx_compressor_destroy(FXCompressor* fx)
{
    if (fx) free(fx);
}

void fx_compressor_reset(FXCompressor* fx)
{
    if (!fx) return;

    for (int i = 0; i < 2; i++) {
        fx->envelope[i] = 0.0f;
        fx->rms[i] = 0.0f;
    }
}

static inline float process_channel(FXCompressor* fx, float input, int channel, int sample_rate)
{
    // 1. Compute RMS level
    float squared = input * input;
    float rms_alpha = 0.01f;
    fx->rms[channel] += rms_alpha * (squared - fx->rms[channel]);
    float rms_level = sqrtf(fmaxf(fx->rms[channel], 0.0f));

    // 2. Attack/release envelope follower
    // Attack: 0.5ms to 50ms
    // Release: 10ms to 500ms
    float attack_time = 0.0005f + fx->attack * 0.0495f;
    float release_time = 0.01f + fx->release * 0.49f;
    float attack_coeff = 1.0f - expf(-1.0f / (sample_rate * attack_time));
    float release_coeff = 1.0f - expf(-1.0f / (sample_rate * release_time));

    if (rms_level > fx->envelope[channel]) {
        fx->envelope[channel] += attack_coeff * (rms_level - fx->envelope[channel]);
    } else {
        fx->envelope[channel] += release_coeff * (rms_level - fx->envelope[channel]);
    }

    // 3. Threshold (0.0-1.0 maps to -40dB to -6dB, linear domain: 0.01 to 0.5)
    float threshold = 0.01f + fx->threshold * 0.49f;

    // 4. Ratio (0.0-1.0 maps to 1:1 to 20:1)
    float ratio = 1.0f + fx->ratio * 19.0f;

    // 5. Soft knee compression
    float knee_width = 0.1f;
    float gain = 1.0f;
    float envelope = fx->envelope[channel];

    if (envelope > threshold) {
        float delta = envelope - threshold;
        float knee_range = threshold * knee_width;

        if (delta < knee_range) {
            // Soft knee: smooth polynomial transition
            float x = delta / knee_range;  // 0.0 to 1.0
            float curve = x * x * (3.0f - 2.0f * x);  // Smoothstep
            float hard_gain = (threshold + delta / ratio) / envelope;
            gain = 1.0f - curve * (1.0f - hard_gain);
        } else {
            // Hard compression above knee
            gain = (threshold + delta / ratio) / envelope;
        }
    }

    // 6. Makeup gain (0.0-1.0 maps to 1x to 8x)
    // At 0.5, makeup is 1x. At 1.0, makeup is 8x.
    float makeup = powf(8.0f, (fx->makeup - 0.5f) * 2.0f);

    // 7. Apply compression and makeup
    return input * gain * makeup;
}

void fx_compressor_process_frame(FXCompressor* fx, float* left, float* right, int sample_rate)
{
    if (!fx || !fx->enabled) return;

    *left = process_channel(fx, *left, 0, sample_rate);
    *right = process_channel(fx, *right, 1, sample_rate);
}

void fx_compressor_process_f32(FXCompressor* fx, float* buffer, int frames, int sample_rate)
{
    if (!fx || !fx->enabled) return;

    for (int i = 0; i < frames; i++) {
        fx_compressor_process_frame(fx, &buffer[i * 2], &buffer[i * 2 + 1], sample_rate);
    }
}

void fx_compressor_process_i16(FXCompressor* fx, int16_t* buffer, int frames, int sample_rate)
{
    if (!fx || !fx->enabled) return;

    for (int i = 0; i < frames; i++) {
        float left = buffer[i * 2] / 32768.0f;
        float right = buffer[i * 2 + 1] / 32768.0f;

        fx_compressor_process_frame(fx, &left, &right, sample_rate);

        buffer[i * 2] = (int16_t)(left * 32767.0f);
        buffer[i * 2 + 1] = (int16_t)(right * 32767.0f);
    }
}

void fx_compressor_set_enabled(FXCompressor* fx, int enabled)
{
    if (fx) fx->enabled = enabled;
}

void fx_compressor_set_threshold(FXCompressor* fx, float threshold)
{
    if (fx) fx->threshold = threshold < 0.0f ? 0.0f : (threshold > 1.0f ? 1.0f : threshold);
}

void fx_compressor_set_ratio(FXCompressor* fx, float ratio)
{
    if (fx) fx->ratio = ratio < 0.0f ? 0.0f : (ratio > 1.0f ? 1.0f : ratio);
}

void fx_compressor_set_attack(FXCompressor* fx, float attack)
{
    if (fx) fx->attack = attack < 0.0f ? 0.0f : (attack > 1.0f ? 1.0f : attack);
}

void fx_compressor_set_release(FXCompressor* fx, float release)
{
    if (fx) fx->release = release < 0.0f ? 0.0f : (release > 1.0f ? 1.0f : release);
}

void fx_compressor_set_makeup(FXCompressor* fx, float makeup)
{
    if (fx) fx->makeup = makeup < 0.0f ? 0.0f : (makeup > 1.0f ? 1.0f : makeup);
}

int fx_compressor_get_enabled(FXCompressor* fx)
{
    return fx ? fx->enabled : 0;
}

float fx_compressor_get_threshold(FXCompressor* fx)
{
    return fx ? fx->threshold : 0.5f;
}

float fx_compressor_get_ratio(FXCompressor* fx)
{
    return fx ? fx->ratio : 0.5f;
}

float fx_compressor_get_attack(FXCompressor* fx)
{
    return fx ? fx->attack : 0.5f;
}

float fx_compressor_get_release(FXCompressor* fx)
{
    return fx ? fx->release : 0.5f;
}

float fx_compressor_get_makeup(FXCompressor* fx)
{
    return fx ? fx->makeup : 0.5f;
}
