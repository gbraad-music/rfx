/*
 * Regroove Stereo Widening for logue SDK
 * Mid-side stereo width control
 */

#include "userfx.h"

extern "C" {
    #include "../../effects/fx_stereo_widen.h"
}

static FXStereoWiden* fx = NULL;

void FX_INIT(uint32_t platform, uint32_t api)
{
    fx = fx_stereo_widen_create();
    if (fx) {
        fx_stereo_widen_set_enabled(fx, 1);
        fx_stereo_widen_set_width(fx, 0.5f);
    }
}

void FX_PROCESS(float *xn, uint32_t frames)
{
    if (!fx) return;

    const uint32_t sample_rate = 48000;

    float * __restrict x = xn;
    const float * x_e = x + (frames << 1);

    for (; x != x_e; x += 2) {
        fx_stereo_widen_process_frame(fx, &x[0], &x[1], sample_rate);
    }
}

void FX_PARAM(uint8_t index, int32_t value)
{
    if (!fx) return;

    const float valf = param_val_to_f32(value);

    switch (index) {
    case 0: // Width
        fx_stereo_widen_set_width(fx, valf);
        break;
    default:
        break;
    }
}
