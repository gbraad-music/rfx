/*
 * Regroove Distortion for logue SDK (v2 - uses modular effects)
 *
 * This version directly uses the fx_distortion module with minimal wrapping
 */

#include "userfx.h"

// Include the core distortion module
extern "C" {
    #include "../../effects/fx_distortion.h"
}

static FXDistortion* fx = NULL;

void FX_INIT(uint32_t platform, uint32_t api)
{
    fx = fx_distortion_create();
    if (fx) {
        fx_distortion_set_enabled(fx, 1);
        fx_distortion_set_drive(fx, 0.5f);
        fx_distortion_set_mix(fx, 0.5f);
    }
}

void FX_PROCESS(float *xn, uint32_t frames)
{
    if (!fx) return;

    const uint32_t sample_rate = 48000;

    // Process using the module's frame-by-frame function
    // which is optimized for real-time processing
    float * __restrict x = xn;
    const float * x_e = x + (frames << 1);

    for (; x != x_e; x += 2) {
        fx_distortion_process_frame(fx, &x[0], &x[1], sample_rate);
    }
}

void FX_PARAM(uint8_t index, int32_t value)
{
    if (!fx) return;

    // Generic parameter interface - no switch needed!
    fx_distortion_set_parameter_value(fx, index, param_val_to_f32(value));
}
