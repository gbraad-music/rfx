/*
 * Regroove Model1 Trim for logue SDK
 * Soft saturation and gain control inspired by Model 1
 */

#include "userfx.h"

extern "C" {
    #include "../../effects/fx_model1_trim.h"
}

static FXModel1Trim* fx = NULL;

void FX_INIT(uint32_t platform, uint32_t api)
{
    fx = fx_model1_trim_create();
    if (fx) {
        fx_model1_trim_set_enabled(fx, 1);
        fx_model1_trim_set_drive(fx, 0.5f);
        fx_model1_trim_set_level(fx, 0.5f);
    }
}

void FX_PROCESS(float *xn, uint32_t frames)
{
    if (!fx) return;

    const uint32_t sample_rate = 48000;

    float * __restrict x = xn;
    const float * x_e = x + (frames << 1);

    for (; x != x_e; x += 2) {
        fx_model1_trim_process_frame(fx, &x[0], &x[1], sample_rate);
    }
}

void FX_PARAM(uint8_t index, int32_t value)
{
    if (!fx) return;

    const float valf = param_val_to_f32(value);

    switch (index) {
    case 0: // Drive
        fx_model1_trim_set_drive(fx, valf);
        break;
    case 1: // Level
        fx_model1_trim_set_level(fx, valf);
        break;
    default:
        break;
    }
}
