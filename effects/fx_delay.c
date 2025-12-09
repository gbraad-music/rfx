/*
 * Regroove Delay Implementation
 */

#include "fx_delay.h"
#include <stdlib.h>
#include <string.h>

#define MAX_DELAY_SAMPLES 48000  // 1 second at 48kHz

struct FXDelay {
    // Parameters
    int enabled;
    float time;        // 0.0 - 1.0 (maps to 10ms-1000ms)
    float feedback;    // 0.0 - 1.0
    float mix;         // 0.0 - 1.0

    // Delay buffers (stereo)
    float* buffer_l;
    float* buffer_r;
    int write_pos;
};

FXDelay* fx_delay_create(void)
{
    FXDelay* fx = (FXDelay*)malloc(sizeof(FXDelay));
    if (!fx) return NULL;

    fx->buffer_l = (float*)malloc(MAX_DELAY_SAMPLES * sizeof(float));
    fx->buffer_r = (float*)malloc(MAX_DELAY_SAMPLES * sizeof(float));

    if (!fx->buffer_l || !fx->buffer_r) {
        fx_delay_destroy(fx);
        return NULL;
    }

    fx->enabled = 0;
    fx->time = 0.5f;
    fx->feedback = 0.4f;
    fx->mix = 0.3f;

    fx_delay_reset(fx);
    return fx;
}

void fx_delay_destroy(FXDelay* fx)
{
    if (!fx) return;
    if (fx->buffer_l) free(fx->buffer_l);
    if (fx->buffer_r) free(fx->buffer_r);
    free(fx);
}

void fx_delay_reset(FXDelay* fx)
{
    if (!fx) return;

    fx->write_pos = 0;

    if (fx->buffer_l) {
        memset(fx->buffer_l, 0, MAX_DELAY_SAMPLES * sizeof(float));
    }
    if (fx->buffer_r) {
        memset(fx->buffer_r, 0, MAX_DELAY_SAMPLES * sizeof(float));
    }
}

void fx_delay_process_frame(FXDelay* fx, float* left, float* right, int sample_rate)
{
    if (!fx || !fx->enabled) return;

    // Map time to delay samples (10ms - 1000ms)
    const int min_delay = sample_rate / 100;  // 10ms
    const int max_delay = sample_rate;         // 1000ms
    const int delay_samples = min_delay + (int)(fx->time * (max_delay - min_delay));

    // Calculate read position
    int read_pos = fx->write_pos - delay_samples;
    if (read_pos < 0) read_pos += MAX_DELAY_SAMPLES;

    // Process left
    float dry_l = *left;
    float delayed_l = fx->buffer_l[read_pos];
    *left = dry_l + fx->mix * (delayed_l - dry_l);
    fx->buffer_l[fx->write_pos] = dry_l + delayed_l * fx->feedback;

    // Process right
    float dry_r = *right;
    float delayed_r = fx->buffer_r[read_pos];
    *right = dry_r + fx->mix * (delayed_r - dry_r);
    fx->buffer_r[fx->write_pos] = dry_r + delayed_r * fx->feedback;

    // Advance write position
    fx->write_pos++;
    if (fx->write_pos >= MAX_DELAY_SAMPLES) fx->write_pos = 0;
}

void fx_delay_process_f32(FXDelay* fx, float* buffer, int frames, int sample_rate)
{
    if (!fx || !fx->enabled) return;

    for (int i = 0; i < frames; i++) {
        fx_delay_process_frame(fx, &buffer[i * 2], &buffer[i * 2 + 1], sample_rate);
    }
}

void fx_delay_process_i16(FXDelay* fx, int16_t* buffer, int frames, int sample_rate)
{
    if (!fx || !fx->enabled) return;

    for (int i = 0; i < frames; i++) {
        float left = buffer[i * 2] / 32768.0f;
        float right = buffer[i * 2 + 1] / 32768.0f;

        fx_delay_process_frame(fx, &left, &right, sample_rate);

        buffer[i * 2] = (int16_t)(left * 32767.0f);
        buffer[i * 2 + 1] = (int16_t)(right * 32767.0f);
    }
}

void fx_delay_set_enabled(FXDelay* fx, int enabled)
{
    if (fx) fx->enabled = enabled;
}

void fx_delay_set_time(FXDelay* fx, float time)
{
    if (fx) fx->time = time < 0.0f ? 0.0f : (time > 1.0f ? 1.0f : time);
}

void fx_delay_set_feedback(FXDelay* fx, float feedback)
{
    if (fx) fx->feedback = feedback < 0.0f ? 0.0f : (feedback > 1.0f ? 1.0f : feedback);
}

void fx_delay_set_mix(FXDelay* fx, float mix)
{
    if (fx) fx->mix = mix < 0.0f ? 0.0f : (mix > 1.0f ? 1.0f : mix);
}

int fx_delay_get_enabled(FXDelay* fx)
{
    return fx ? fx->enabled : 0;
}

float fx_delay_get_time(FXDelay* fx)
{
    return fx ? fx->time : 0.0f;
}

float fx_delay_get_feedback(FXDelay* fx)
{
    return fx ? fx->feedback : 0.0f;
}

float fx_delay_get_mix(FXDelay* fx)
{
    return fx ? fx->mix : 0.0f;
}
