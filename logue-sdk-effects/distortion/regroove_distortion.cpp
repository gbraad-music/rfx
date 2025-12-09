/*
 * Regroove Distortion for logue SDK
 *
 * Analog-style distortion effect
 * Compatible with: minilogue xd, prologue, NTS-1
 */

#include "userfx.h"

// Extract only the distortion-related state from regroove_effects
typedef struct {
    float drive;              // 0.0 - 1.0
    float mix;                // 0.0 - 1.0 (dry/wet)

    // Internal state
    float hp[2];              // Pre-emphasis highpass state (L, R)
    float bp_lp[2];           // Bandpass lowpass state
    float bp_bp[2];           // Bandpass state
    float env[2];             // Envelope follower state
    float lp[2];              // Post-filter state
} RegrooveDistortion;

static RegrooveDistortion fx_state;

// Forward declarations of processing functions
static inline float distortion_process_sample(float input, float *hp, float *bp_lp,
                                             float *bp_bp, float *env, float *lp,
                                             float drive, int sample_rate);

void FX_INIT(uint32_t platform, uint32_t api)
{
    // Initialize state
    fx_state.drive = 0.5f;
    fx_state.mix = 0.5f;

    for (int i = 0; i < 2; i++) {
        fx_state.hp[i] = 0.0f;
        fx_state.bp_lp[i] = 0.0f;
        fx_state.bp_bp[i] = 0.0f;
        fx_state.env[i] = 0.0f;
        fx_state.lp[i] = 0.0f;
    }
}

void FX_PROCESS(float *xn, uint32_t frames)
{
    const float drive = fx_state.drive;
    const float mix = fx_state.mix;
    const uint32_t sample_rate = 48000; // logue runs at 48kHz

    // Process stereo interleaved samples
    float * __restrict x = xn;
    const float * x_e = x + (frames << 1); // frames * 2

    for (; x != x_e; ) {
        // Left channel
        float dry_l = *x;
        float wet_l = distortion_process_sample(dry_l, &fx_state.hp[0],
                                               &fx_state.bp_lp[0], &fx_state.bp_bp[0],
                                               &fx_state.env[0], &fx_state.lp[0],
                                               drive, sample_rate);
        *x++ = dry_l + mix * (wet_l - dry_l);

        // Right channel
        float dry_r = *x;
        float wet_r = distortion_process_sample(dry_r, &fx_state.hp[1],
                                               &fx_state.bp_lp[1], &fx_state.bp_bp[1],
                                               &fx_state.env[1], &fx_state.lp[1],
                                               drive, sample_rate);
        *x++ = dry_r + mix * (wet_r - dry_r);
    }
}

void FX_PARAM(uint8_t index, int32_t value)
{
    // logue parameters are 0-1023 (10-bit)
    const float valf = param_val_to_f32(value); // Converts to 0.0-1.0

    switch (index) {
    case 0: // Drive
        fx_state.drive = valf;
        break;
    case 1: // Mix (dry/wet)
        fx_state.mix = valf;
        break;
    default:
        break;
    }
}

// Distortion processing function (extracted from regroove_effects.c)
static inline float distortion_process_sample(float input, float *hp, float *bp_lp,
                                             float *bp_bp, float *env, float *lp,
                                             float drive, int sample_rate)
{
    // Pre-emphasis high-pass (reduces mud)
    const float hp_coeff = 0.999f;
    float hp_out = input - *hp;
    *hp = input - hp_coeff * hp_out;

    // Bandpass filter for dynamic processing (simulates tube saturation)
    const float bp_freq = 1000.0f / sample_rate;
    const float bp_q = 0.707f;
    float bp_in = hp_out;
    float bp_out = bp_in - *bp_lp;
    *bp_bp = *bp_bp * (1.0f - bp_q * bp_freq) + bp_out * bp_freq;
    *bp_lp = *bp_lp + *bp_bp * bp_freq;

    // Envelope follower for dynamic gain
    float bp_abs = bp_out < 0.0f ? -bp_out : bp_out;
    const float env_coeff = 0.01f;
    *env = *env + env_coeff * (bp_abs - *env);

    // Waveshaping with dynamic gain
    float gain = 1.0f + drive * 50.0f * (1.0f + *env);
    float shaped = bp_out * gain;

    // Soft clipping (tanh approximation)
    if (shaped > 1.0f) shaped = 1.0f;
    else if (shaped < -1.0f) shaped = -1.0f;
    else {
        // Fast tanh approximation: x - x^3/3
        float x2 = shaped * shaped;
        shaped = shaped * (1.0f - x2 * 0.333f);
    }

    // Post low-pass filter (smooth harsh edges)
    const float lp_coeff = 0.3f;
    *lp = *lp + lp_coeff * (shaped - *lp);

    return *lp;
}
