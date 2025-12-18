/*
 * Regroove Compressor for logue SDK
 * RMS compressor with soft knee
 */

#include "userfx.h"

extern "C" {
    #include "../../effects/fx_compressor.h"
}

static FXCompressor* fx = NULL;

void FX_INIT(uint32_t platform, uint32_t api)
{
    fx = fx_compressor_create();
    if (fx) {
        fx_compressor_set_enabled(fx, 1);
        fx_compressor_set_threshold(fx, 0.4f);
        fx_compressor_set_ratio(fx, 0.4f);
        fx_compressor_set_attack(fx, 0.05f);
        fx_compressor_set_release(fx, 0.5f);
        fx_compressor_set_makeup(fx, 0.65f);
    }
}

void FX_PROCESS(float *xn, uint32_t frames)
{
    if (!fx) return;

    const uint32_t sample_rate = 48000;

    float * __restrict x = xn;
    const float * x_e = x + (frames << 1);

    for (; x != x_e; x += 2) {
        fx_compressor_process_frame(fx, &x[0], &x[1], sample_rate);
    }
}

void FX_PARAM(uint8_t index, int32_t value)
{
    if (!fx) return;

    const float valf = param_val_to_f32(value);

    switch (index) {
    case 0: // Threshold
        fx_compressor_set_threshold(fx, valf);
        break;
    case 1: // Ratio
        fx_compressor_set_ratio(fx, valf);
        break;
    case 2: // Makeup gain
        fx_compressor_set_makeup(fx, valf);
        break;
    default:
        break;
    }
}
