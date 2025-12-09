/*
 * Regroove Filter for logue SDK (v2 - uses modular effects)
 */

#include "userfx.h"

extern "C" {
    #include "../../effects/fx_filter.h"
}

static FXFilter* fx = NULL;

void FX_INIT(uint32_t platform, uint32_t api)
{
    fx = fx_filter_create();
    if (fx) {
        fx_filter_set_enabled(fx, 1);
        fx_filter_set_cutoff(fx, 0.8f);
        fx_filter_set_resonance(fx, 0.3f);
    }
}

void FX_PROCESS(float *xn, uint32_t frames)
{
    if (!fx) return;

    const uint32_t sample_rate = 48000;

    float * __restrict x = xn;
    const float * x_e = x + (frames << 1);

    for (; x != x_e; x += 2) {
        fx_filter_process_frame(fx, &x[0], &x[1], sample_rate);
    }
}

void FX_PARAM(uint8_t index, int32_t value)
{
    if (!fx) return;

    const float valf = param_val_to_f32(value);

    switch (index) {
    case 0: // Cutoff
        fx_filter_set_cutoff(fx, valf);
        break;
    case 1: // Resonance
        fx_filter_set_resonance(fx, valf);
        break;
    default:
        break;
    }
}
