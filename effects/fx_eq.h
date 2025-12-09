/*
 * Regroove 3-Band DJ EQ Effect
 * DJ-style kill EQ with low/mid/high bands
 */

#ifndef FX_EQ_H
#define FX_EQ_H

#include "fx_common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct FXEqualizer FXEqualizer;

// Lifecycle
FXEqualizer* fx_eq_create(void);
void fx_eq_destroy(FXEqualizer* fx);
void fx_eq_reset(FXEqualizer* fx);

// Processing
void fx_eq_process_f32(FXEqualizer* fx, float* buffer, int frames, int sample_rate);
void fx_eq_process_i16(FXEqualizer* fx, int16_t* buffer, int frames, int sample_rate);
void fx_eq_process_frame(FXEqualizer* fx, float* left, float* right, int sample_rate);

// Parameters (0.0 - 1.0, where 0.5 = neutral)
// 0.0 = KILL, 0.5 = neutral, 1.0 = boost
void fx_eq_set_enabled(FXEqualizer* fx, int enabled);
void fx_eq_set_low(FXEqualizer* fx, float gain);
void fx_eq_set_mid(FXEqualizer* fx, float gain);
void fx_eq_set_high(FXEqualizer* fx, float gain);

int fx_eq_get_enabled(FXEqualizer* fx);
float fx_eq_get_low(FXEqualizer* fx);
float fx_eq_get_mid(FXEqualizer* fx);
float fx_eq_get_high(FXEqualizer* fx);

#ifdef __cplusplus
}
#endif

#endif // FX_EQ_H
