/*
 * Regroove Delay for logue SDK (v2 - uses modular effects)
 */

#include "userfx.h"

extern "C" {
    #include "../../effects/fx_delay.h"
}

static FXDelay* fx = NULL;

void FX_INIT(uint32_t platform, uint32_t api)
{
    fx = fx_delay_create();
    if (fx) {
        fx_delay_set_enabled(fx, 1);
        fx_delay_set_time(fx, 0.5f);
        fx_delay_set_feedback(fx, 0.4f);
        fx_delay_set_mix(fx, 0.3f);
    }
}

void FX_PROCESS(float *xn, uint32_t frames)
{
    if (!fx) return;

    const uint32_t sample_rate = 48000;

    float * __restrict x = xn;
    const float * x_e = x + (frames << 1);

    for (; x != x_e; x += 2) {
        fx_delay_process_frame(fx, &x[0], &x[1], sample_rate);
    }
}

void FX_PARAM(uint8_t index, int32_t value)
{
    if (!fx) return;

    const float valf = param_val_to_f32(value);

    switch (index) {
    case 0: // Time
        fx_delay_set_time(fx, valf);
        break;
    case 1: // Feedback
        fx_delay_set_feedback(fx, valf);
        break;
    case 2: // Mix
        fx_delay_set_mix(fx, valf);
        break;
    default:
        break;
    }
}
