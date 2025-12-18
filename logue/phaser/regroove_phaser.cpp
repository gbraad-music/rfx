/*
 * Regroove Phaser for logue SDK
 * Cascaded allpass filters with LFO modulation
 */

#include "userfx.h"

extern "C" {
    #include "../../effects/fx_phaser.h"
}

static FXPhaser* fx = NULL;

void FX_INIT(uint32_t platform, uint32_t api)
{
    fx = fx_phaser_create();
    if (fx) {
        fx_phaser_set_enabled(fx, 1);
        fx_phaser_set_rate(fx, 0.5f);
        fx_phaser_set_depth(fx, 0.5f);
        fx_phaser_set_feedback(fx, 0.5f);
    }
}

void FX_PROCESS(float *xn, uint32_t frames)
{
    if (!fx) return;

    const uint32_t sample_rate = 48000;

    float * __restrict x = xn;
    const float * x_e = x + (frames << 1);

    for (; x != x_e; x += 2) {
        fx_phaser_process_frame(fx, &x[0], &x[1], sample_rate);
    }
}

void FX_PARAM(uint8_t index, int32_t value)
{
    if (!fx) return;

    const float valf = param_val_to_f32(value);

    switch (index) {
    case 0: // Rate
        fx_phaser_set_rate(fx, valf);
        break;
    case 1: // Depth
        fx_phaser_set_depth(fx, valf);
        break;
    case 2: // Feedback
        fx_phaser_set_feedback(fx, valf);
        break;
    default:
        break;
    }
}
