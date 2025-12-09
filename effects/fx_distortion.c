/*
 * Regroove Distortion Effect Implementation
 */

#include "fx_distortion.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

struct FXDistortion {
    // Parameters
    int enabled;
    float drive;    // 0.0 - 1.0
    float mix;      // 0.0 - 1.0 (dry/wet)

    // Internal state (stereo)
    float hp[2];        // Pre-emphasis highpass
    float bp_lp[2];     // Bandpass lowpass
    float bp_bp[2];     // Bandpass state
    float env[2];       // Envelope follower
    float lp[2];        // Post-filter lowpass
};

// Lifecycle
FXDistortion* fx_distortion_create(void)
{
    FXDistortion* fx = (FXDistortion*)malloc(sizeof(FXDistortion));
    if (!fx) return NULL;

    fx->enabled = 0;
    fx->drive = 0.5f;
    fx->mix = 0.5f;

    fx_distortion_reset(fx);
    return fx;
}

void fx_distortion_destroy(FXDistortion* fx)
{
    if (fx) free(fx);
}

void fx_distortion_reset(FXDistortion* fx)
{
    if (!fx) return;

    for (int i = 0; i < 2; i++) {
        fx->hp[i] = 0.0f;
        fx->bp_lp[i] = 0.0f;
        fx->bp_bp[i] = 0.0f;
        fx->env[i] = 0.0f;
        fx->lp[i] = 0.0f;
    }
}

// Internal processing function
static inline float process_sample(FXDistortion* fx, float input, int channel, int sample_rate)
{
    // Pre-emphasis high-pass (reduces mud)
    const float hp_coeff = 0.999f;
    float hp_out = input - fx->hp[channel];
    fx->hp[channel] = input - hp_coeff * hp_out;

    // Bandpass filter for dynamic processing
    const float bp_freq = 1000.0f / sample_rate;
    const float bp_q = 0.707f;
    float bp_in = hp_out;
    float bp_out = bp_in - fx->bp_lp[channel];
    fx->bp_bp[channel] = fx->bp_bp[channel] * (1.0f - bp_q * bp_freq) + bp_out * bp_freq;
    fx->bp_lp[channel] = fx->bp_lp[channel] + fx->bp_bp[channel] * bp_freq;

    // Envelope follower for dynamic gain
    float bp_abs = bp_out < 0.0f ? -bp_out : bp_out;
    const float env_coeff = 0.01f;
    fx->env[channel] = fx->env[channel] + env_coeff * (bp_abs - fx->env[channel]);

    // Waveshaping with dynamic gain
    float gain = 1.0f + fx->drive * 50.0f * (1.0f + fx->env[channel]);
    float shaped = bp_out * gain;

    // Soft clipping (fast tanh approximation)
    if (shaped > 1.0f) shaped = 1.0f;
    else if (shaped < -1.0f) shaped = -1.0f;
    else {
        float x2 = shaped * shaped;
        shaped = shaped * (1.0f - x2 * 0.333f);
    }

    // Post low-pass filter
    const float lp_coeff = 0.3f;
    fx->lp[channel] = fx->lp[channel] + lp_coeff * (shaped - fx->lp[channel]);

    return fx->lp[channel];
}

// Process single stereo frame
void fx_distortion_process_frame(FXDistortion* fx, float* left, float* right, int sample_rate)
{
    if (!fx || !fx->enabled) return;

    float dry_l = *left;
    float dry_r = *right;

    float wet_l = process_sample(fx, dry_l, 0, sample_rate);
    float wet_r = process_sample(fx, dry_r, 1, sample_rate);

    *left = dry_l + fx->mix * (wet_l - dry_l);
    *right = dry_r + fx->mix * (wet_r - dry_r);
}

// Process float32 buffer (interleaved stereo)
void fx_distortion_process_f32(FXDistortion* fx, float* buffer, int frames, int sample_rate)
{
    if (!fx || !fx->enabled) return;

    for (int i = 0; i < frames; i++) {
        fx_distortion_process_frame(fx, &buffer[i * 2], &buffer[i * 2 + 1], sample_rate);
    }
}

// Process int16 buffer (interleaved stereo)
void fx_distortion_process_i16(FXDistortion* fx, int16_t* buffer, int frames, int sample_rate)
{
    if (!fx || !fx->enabled) return;

    for (int i = 0; i < frames; i++) {
        // Convert to float
        float left = buffer[i * 2] / 32768.0f;
        float right = buffer[i * 2 + 1] / 32768.0f;

        // Process
        fx_distortion_process_frame(fx, &left, &right, sample_rate);

        // Convert back to int16
        buffer[i * 2] = (int16_t)(left * 32767.0f);
        buffer[i * 2 + 1] = (int16_t)(right * 32767.0f);
    }
}

// Parameters
void fx_distortion_set_enabled(FXDistortion* fx, int enabled)
{
    if (fx) fx->enabled = enabled;
}

void fx_distortion_set_drive(FXDistortion* fx, float drive)
{
    if (fx) fx->drive = drive < 0.0f ? 0.0f : (drive > 1.0f ? 1.0f : drive);
}

void fx_distortion_set_mix(FXDistortion* fx, float mix)
{
    if (fx) fx->mix = mix < 0.0f ? 0.0f : (mix > 1.0f ? 1.0f : mix);
}

int fx_distortion_get_enabled(FXDistortion* fx)
{
    return fx ? fx->enabled : 0;
}

float fx_distortion_get_drive(FXDistortion* fx)
{
    return fx ? fx->drive : 0.0f;
}

float fx_distortion_get_mix(FXDistortion* fx)
{
    return fx ? fx->mix : 0.0f;
}
