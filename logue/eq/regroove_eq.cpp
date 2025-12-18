/*
 * Regroove EQ for logue SDK
 * 3-band DJ-style kill EQ
 */

#include "userfx.h"

extern "C" {
    #include "../../effects/fx_eq.h"
}

static FXEqualizer* fx = NULL;

void FX_INIT(uint32_t platform, uint32_t api)
{
    fx = fx_eq_create();
    if (fx) {
        fx_eq_set_enabled(fx, 1);
        fx_eq_set_low(fx, 0.5f);   // Neutral
        fx_eq_set_mid(fx, 0.5f);   // Neutral
        fx_eq_set_high(fx, 0.5f);  // Neutral
    }
}

void FX_PROCESS(float *xn, uint32_t frames)
{
    if (!fx) return;

    const uint32_t sample_rate = 48000;

    float * __restrict x = xn;
    const float * x_e = x + (frames << 1);

    for (; x != x_e; x += 2) {
        fx_eq_process_frame(fx, &x[0], &x[1], sample_rate);
    }
}

void FX_PARAM(uint8_t index, int32_t value)
{
    if (!fx) return;

    const float valf = param_val_to_f32(value);

    switch (index) {
    case 0: // Low (0.0 = kill, 0.5 = neutral, 1.0 = boost)
        fx_eq_set_low(fx, valf);
        break;
    case 1: // Mid
        fx_eq_set_mid(fx, valf);
        break;
    case 2: // High
        fx_eq_set_high(fx, valf);
        break;
    default:
        break;
    }
}
