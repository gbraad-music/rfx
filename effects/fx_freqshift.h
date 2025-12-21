/*
 * Regroove Frequency Shifter Effect
 * Bode-style frequency shifter using Hilbert transform + quadrature oscillator
 */

#ifndef FX_FREQSHIFT_H
#define FX_FREQSHIFT_H

#include "fx_common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct FXFreqShift FXFreqShift;

// Lifecycle
FXFreqShift* fx_freqshift_create(void);
void fx_freqshift_destroy(FXFreqShift* fx);
void fx_freqshift_reset(FXFreqShift* fx);

// Processing
void fx_freqshift_process_f32(FXFreqShift* fx, float* buffer, int frames, int sample_rate);
void fx_freqshift_process_i16(FXFreqShift* fx, int16_t* buffer, int frames, int sample_rate);
void fx_freqshift_process_frame(FXFreqShift* fx, float* left, float* right, int sample_rate);

// Parameters (0.0 - 1.0)
void fx_freqshift_set_enabled(FXFreqShift* fx, int enabled);
void fx_freqshift_set_freq(FXFreqShift* fx, float freq);     // 0.0-1.0 maps to -500Hz to +500Hz
void fx_freqshift_set_mix(FXFreqShift* fx, float mix);       // 0.0-1.0 maps to 0% to 100% wet

int fx_freqshift_get_enabled(FXFreqShift* fx);
float fx_freqshift_get_freq(FXFreqShift* fx);
float fx_freqshift_get_mix(FXFreqShift* fx);

#ifdef __cplusplus
}
#endif

#endif // FX_FREQSHIFT_H
