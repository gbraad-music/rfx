/*
 * Regroove 3-Band DJ EQ Implementation
 */

#include "fx_eq.h"
#include <stdlib.h>
#include <string.h>
#include "windows_compat.h"
#include <math.h>

struct FXEqualizer {
    // Parameters
    int enabled;
    float low;     // 0.0 - 1.0 (0.5 = neutral)
    float mid;     // 0.0 - 1.0 (0.5 = neutral)
    float high;    // 0.0 - 1.0 (0.5 = neutral)

    // Filter state (stereo)
    float lp1[2];  // Low band filter (250Hz)
    float lp2[2];  // Mid+High band filter (6kHz)
};

FXEqualizer* fx_eq_create(void)
{
    FXEqualizer* fx = (FXEqualizer*)malloc(sizeof(FXEqualizer));
    if (!fx) return NULL;

    fx->enabled = 0;
    fx->low = 0.5f;
    fx->mid = 0.5f;
    fx->high = 0.5f;

    fx_eq_reset(fx);
    return fx;
}

void fx_eq_destroy(FXEqualizer* fx)
{
    if (fx) free(fx);
}

void fx_eq_reset(FXEqualizer* fx)
{
    if (!fx) return;

    for (int i = 0; i < 2; i++) {
        fx->lp1[i] = 0.0f;
        fx->lp2[i] = 0.0f;
    }
}

void fx_eq_process_frame(FXEqualizer* fx, float* left, float* right, int sample_rate)
{
    if (!fx || !fx->enabled) return;

    // DJ kill gain curve:
    // 0.0 to 0.5: linear kill (0.0x to 1.0x) - allows total frequency elimination
    // 0.5 to 1.0: exponential boost (1.0x to 4.0x) - +12dB max boost
    float low_mult, mid_mult, high_mult;

    if (fx->low < 0.5f) {
        low_mult = fx->low * 2.0f;  // 0.0 to 1.0 (kill zone)
    } else {
        low_mult = powf(4.0f, (fx->low - 0.5f) * 2.0f);  // 1.0 to 4.0 (boost zone)
    }

    if (fx->mid < 0.5f) {
        mid_mult = fx->mid * 2.0f;
    } else {
        mid_mult = powf(4.0f, (fx->mid - 0.5f) * 2.0f);
    }

    if (fx->high < 0.5f) {
        high_mult = fx->high * 2.0f;
    } else {
        high_mult = powf(4.0f, (fx->high - 0.5f) * 2.0f);
    }

    // Process left channel
    float sample_l = *left;

    // Low band: lowpass at 250Hz (bass)
    float low_freq = 250.0f / sample_rate;
    float low_alpha = 1.0f - expf(-2.0f * 3.14159f * low_freq);
    fx->lp1[0] += low_alpha * (sample_l - fx->lp1[0]);
    float low_band_l = fx->lp1[0];

    // Mid+High band: lowpass at 6kHz
    float mid_freq = 6000.0f / sample_rate;
    float mid_alpha = 1.0f - expf(-2.0f * 3.14159f * mid_freq);
    fx->lp2[0] += mid_alpha * (sample_l - fx->lp2[0]);

    // Extract mid band (250Hz to 6kHz)
    float mid_band_l = fx->lp2[0] - fx->lp1[0];

    // Extract high band (above 6kHz)
    float high_band_l = sample_l - fx->lp2[0];

    // Apply gains
    *left = low_band_l * low_mult + mid_band_l * mid_mult + high_band_l * high_mult;

    // Process right channel
    float sample_r = *right;

    fx->lp1[1] += low_alpha * (sample_r - fx->lp1[1]);
    float low_band_r = fx->lp1[1];

    fx->lp2[1] += mid_alpha * (sample_r - fx->lp2[1]);
    float mid_band_r = fx->lp2[1] - fx->lp1[1];
    float high_band_r = sample_r - fx->lp2[1];

    *right = low_band_r * low_mult + mid_band_r * mid_mult + high_band_r * high_mult;
}

void fx_eq_process_f32(FXEqualizer* fx, float* buffer, int frames, int sample_rate)
{
    if (!fx || !fx->enabled) return;

    for (int i = 0; i < frames; i++) {
        fx_eq_process_frame(fx, &buffer[i * 2], &buffer[i * 2 + 1], sample_rate);
    }
}

void fx_eq_process_i16(FXEqualizer* fx, int16_t* buffer, int frames, int sample_rate)
{
    if (!fx || !fx->enabled) return;

    for (int i = 0; i < frames; i++) {
        float left = buffer[i * 2] / 32768.0f;
        float right = buffer[i * 2 + 1] / 32768.0f;

        fx_eq_process_frame(fx, &left, &right, sample_rate);

        buffer[i * 2] = (int16_t)(left * 32767.0f);
        buffer[i * 2 + 1] = (int16_t)(right * 32767.0f);
    }
}

void fx_eq_set_enabled(FXEqualizer* fx, int enabled)
{
    if (fx) fx->enabled = enabled;
}

void fx_eq_set_low(FXEqualizer* fx, float gain)
{
    if (fx) fx->low = gain < 0.0f ? 0.0f : (gain > 1.0f ? 1.0f : gain);
}

void fx_eq_set_mid(FXEqualizer* fx, float gain)
{
    if (fx) fx->mid = gain < 0.0f ? 0.0f : (gain > 1.0f ? 1.0f : gain);
}

void fx_eq_set_high(FXEqualizer* fx, float gain)
{
    if (fx) fx->high = gain < 0.0f ? 0.0f : (gain > 1.0f ? 1.0f : gain);
}

int fx_eq_get_enabled(FXEqualizer* fx)
{
    return fx ? fx->enabled : 0;
}

float fx_eq_get_low(FXEqualizer* fx)
{
    return fx ? fx->low : 0.5f;
}

float fx_eq_get_mid(FXEqualizer* fx)
{
    return fx ? fx->mid : 0.5f;
}

float fx_eq_get_high(FXEqualizer* fx)
{
    return fx ? fx->high : 0.5f;
}
