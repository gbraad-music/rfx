/*
 * Regroove Reverb for logue SDK
 * Algorithmic reverb with room size, damping, and mix controls
 */

#include "userfx.h"

extern "C" {
    #include "../../effects/fx_reverb.h"
}

static FXReverb* fx = NULL;

void FX_INIT(uint32_t platform, uint32_t api)
{
    fx = fx_reverb_create();
    if (fx) {
        fx_reverb_set_enabled(fx, 1);
        fx_reverb_set_size(fx, 0.5f);
        fx_reverb_set_damping(fx, 0.5f);
        fx_reverb_set_mix(fx, 0.3f);
    }
}

void FX_PROCESS(float *xn, uint32_t frames)
{
    if (!fx) return;

    const uint32_t sample_rate = 48000;

    float * __restrict x = xn;
    const float * x_e = x + (frames << 1);

    for (; x != x_e; x += 2) {
        fx_reverb_process_frame(fx, &x[0], &x[1], sample_rate);
    }
}

void FX_PARAM(uint8_t index, int32_t value)
{
    if (!fx) return;

    // Generic parameter interface - no switch needed!
    fx_reverb_set_parameter_value(fx, index, param_val_to_f32(value));
}
