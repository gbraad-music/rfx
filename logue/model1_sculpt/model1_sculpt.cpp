/*
 * MODEL 1 Sculpt for logue SDK
 * Semi-Parametric EQ: 70 Hz to 7 kHz, -20 dB to +8 dB
 */

#include "userfx.h"

extern "C" {
    #include "../../effects/fx_model1_sculpt.h"
}

static FXModel1Sculpt* fx = NULL;

void FX_INIT(uint32_t platform, uint32_t api)
{
    fx = fx_model1_sculpt_create();
    if (fx) {
        fx_model1_sculpt_set_enabled(fx, 1);
        fx_model1_sculpt_set_frequency(fx, 0.5f);  // Default to ~500Hz (mid-point)
        fx_model1_sculpt_set_gain(fx, 0.5f);       // Default to 0dB (neutral)
    }
}

void FX_PROCESS(float *xn, uint32_t frames)
{
    if (!fx) return;

    const uint32_t sample_rate = 48000;

    float * __restrict x = xn;
    const float * x_e = x + (frames << 1);

    for (; x != x_e; x += 2) {
        fx_model1_sculpt_process_frame(fx, &x[0], &x[1], sample_rate);
    }
}

void FX_PARAM(uint8_t index, int32_t value)
{
    if (!fx) return;

    // Generic parameter interface - no switch needed!
    fx_model1_sculpt_set_parameter_value(fx, index, param_val_to_f32(value));
}
}

void FX_RESUME(void)
{
    if (fx) fx_model1_sculpt_reset(fx);
}

void FX_SUSPEND(void)
{
