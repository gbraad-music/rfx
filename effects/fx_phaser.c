/*
 * Regroove Phaser Implementation
 * Based on cascaded allpass filters with LFO modulation
 */

#include "fx_phaser.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define NUM_STAGES 4

typedef struct {
    float zm1;  // Single sample delay
    float a1;   // Allpass coefficient
} AllpassStage;

struct FXPhaser {
    // Parameters
    int enabled;
    float rate;      // 0.0 - 1.0
    float depth;     // 0.0 - 1.0
    float feedback;  // 0.0 - 1.0

    // Processing state
    AllpassStage stages_l[NUM_STAGES];
    AllpassStage stages_r[NUM_STAGES];
    float lfo_phase;
    float zm1;  // Feedback delay
};

static void allpass_init(AllpassStage* stage)
{
    stage->zm1 = 0.0f;
    stage->a1 = 0.0f;
}

// Proper first-order allpass filter
// H(z) = (a1 + z^-1) / (1 + a1*z^-1)
static float allpass_process(AllpassStage* stage, float input)
{
    float output = stage->a1 * input + stage->zm1;
    stage->zm1 = input - stage->a1 * output;
    return output;
}

FXPhaser* fx_phaser_create(void)
{
    FXPhaser* fx = (FXPhaser*)malloc(sizeof(FXPhaser));
    if (!fx) return NULL;

    fx->enabled = 0;
    fx->rate = 0.5f;
    fx->depth = 0.5f;
    fx->feedback = 0.5f;
    fx->lfo_phase = 0.0f;
    fx->zm1 = 0.0f;

    for (int i = 0; i < NUM_STAGES; i++) {
        allpass_init(&fx->stages_l[i]);
        allpass_init(&fx->stages_r[i]);
    }

    return fx;
}

void fx_phaser_destroy(FXPhaser* fx)
{
    if (fx) free(fx);
}

void fx_phaser_reset(FXPhaser* fx)
{
    if (!fx) return;

    fx->lfo_phase = 0.0f;
    fx->zm1 = 0.0f;

    for (int i = 0; i < NUM_STAGES; i++) {
        allpass_init(&fx->stages_l[i]);
        allpass_init(&fx->stages_r[i]);
    }
}

void fx_phaser_process_frame(FXPhaser* fx, float* left, float* right, int sample_rate)
{
    if (!fx || !fx->enabled) return;

    // LFO: 0.1 Hz to 10 Hz
    float lfo_freq = 0.1f + fx->rate * 9.9f;
    float lfo_inc = 2.0f * M_PI * lfo_freq / sample_rate;
    
    fx->lfo_phase += lfo_inc;
    if (fx->lfo_phase >= 2.0f * M_PI) {
        fx->lfo_phase -= 2.0f * M_PI;
    }
    
    float lfo = sinf(fx->lfo_phase);
    
    // Map LFO to allpass coefficient (0.3 to 0.95 range)
    float min_freq = 200.0f;
    float max_freq = 2000.0f;
    float freq = min_freq + (max_freq - min_freq) * (0.5f + 0.5f * lfo * fx->depth);
    float damp = (2.0f * M_PI * freq) / sample_rate;
    float a1 = (1.0f - damp) / (1.0f + damp);
    
    // Update allpass coefficients
    for (int i = 0; i < NUM_STAGES; i++) {
        fx->stages_l[i].a1 = a1;
        fx->stages_r[i].a1 = a1;
    }
    
    float input_l = *left;
    float input_r = *right;

    // Apply feedback with limiting to prevent blowup
    float fb_scaled = fx->feedback * 0.7f;  // Scale down to prevent instability
    float y_l = input_l + fx->zm1 * fb_scaled;
    float y_r = input_r + fx->zm1 * fb_scaled;

    // Limit input to allpass chain
    y_l = fmaxf(-2.0f, fminf(2.0f, y_l));
    y_r = fmaxf(-2.0f, fminf(2.0f, y_r));

    // Process through allpass stages
    for (int i = 0; i < NUM_STAGES; i++) {
        y_l = allpass_process(&fx->stages_l[i], y_l);
        y_r = allpass_process(&fx->stages_r[i], y_r);
    }

    // Store feedback signal with soft limiting
    float fb_sig = (y_l + y_r) * 0.5f;
    fx->zm1 = fmaxf(-1.0f, fminf(1.0f, fb_sig));

    // Mix with dry signal (50/50)
    *left = (input_l + y_l) * 0.5f;
    *right = (input_r + y_r) * 0.5f;
}

void fx_phaser_process_f32(FXPhaser* fx, float* buffer, int frames, int sample_rate)
{
    if (!fx || !fx->enabled) return;

    for (int i = 0; i < frames; i++) {
        fx_phaser_process_frame(fx, &buffer[i * 2], &buffer[i * 2 + 1], sample_rate);
    }
}

void fx_phaser_process_i16(FXPhaser* fx, int16_t* buffer, int frames, int sample_rate)
{
    if (!fx || !fx->enabled) return;

    for (int i = 0; i < frames; i++) {
        float left = buffer[i * 2] / 32768.0f;
        float right = buffer[i * 2 + 1] / 32768.0f;

        fx_phaser_process_frame(fx, &left, &right, sample_rate);

        buffer[i * 2] = (int16_t)(left * 32767.0f);
        buffer[i * 2 + 1] = (int16_t)(right * 32767.0f);
    }
}

void fx_phaser_set_enabled(FXPhaser* fx, int enabled)
{
    if (fx) fx->enabled = enabled;
}

void fx_phaser_set_rate(FXPhaser* fx, float rate)
{
    if (fx) fx->rate = rate < 0.0f ? 0.0f : (rate > 1.0f ? 1.0f : rate);
}

void fx_phaser_set_depth(FXPhaser* fx, float depth)
{
    if (fx) fx->depth = depth < 0.0f ? 0.0f : (depth > 1.0f ? 1.0f : depth);
}

void fx_phaser_set_feedback(FXPhaser* fx, float feedback)
{
    if (fx) fx->feedback = feedback < 0.0f ? 0.0f : (feedback > 1.0f ? 1.0f : feedback);
}

int fx_phaser_get_enabled(FXPhaser* fx)
{
    return fx ? fx->enabled : 0;
}

float fx_phaser_get_rate(FXPhaser* fx)
{
    return fx ? fx->rate : 0.0f;
}

float fx_phaser_get_depth(FXPhaser* fx)
{
    return fx ? fx->depth : 0.0f;
}

float fx_phaser_get_feedback(FXPhaser* fx)
{
    return fx ? fx->feedback : 0.0f;
}
