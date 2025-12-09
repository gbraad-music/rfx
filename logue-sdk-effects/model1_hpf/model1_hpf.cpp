/*
 * MODEL 1 Contour HPF for logue SDK
 * High Pass Filter: FLAT to 1 kHz
 */

#include "userfx.h"

extern "C" {
    #include "../../effects/fx_model1_hpf.h"
}

static FXModel1HPF* fx = NULL;

void FX_INIT(uint32_t platform, uint32_t api)
{
    fx = fx_model1_hpf_create();
    if (fx) {
        fx_model1_hpf_set_enabled(fx, 1);
        fx_model1_hpf_set_cutoff(fx, 0.0f);  // Default to FLAT
    }
}

void FX_PROCESS(float *xn, uint32_t frames)
{
    if (!fx) return;

    const uint32_t sample_rate = 48000;

    float * __restrict x = xn;
    const float * x_e = x + (frames << 1);

    for (; x != x_e; x += 2) {
        fx_model1_hpf_process_frame(fx, &x[0], &x[1], sample_rate);
    }
}

void FX_PARAM(uint8_t index, int32_t value)
{
    if (!fx) return;

    const float valf = param_val_to_f32(value);

    switch (index) {
    case 0: // Cutoff (0.0 = FLAT/20Hz, 1.0 = 1kHz)
        fx_model1_hpf_set_cutoff(fx, valf);
        break;
    default:
        break;
    }
}

void FX_RESUME(void)
{
    if (fx) fx_model1_hpf_reset(fx);
}

void FX_SUSPEND(void)
{
}
