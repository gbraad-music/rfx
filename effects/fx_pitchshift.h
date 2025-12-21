/*
 * Regroove Pitch Shifter Effect
 * Real-time pitch shifting using time-domain overlap-add method
 */

#ifndef FX_PITCHSHIFT_H
#define FX_PITCHSHIFT_H

#include "fx_common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct FXPitchShift FXPitchShift;

// Lifecycle
FXPitchShift* fx_pitchshift_create(void);
void fx_pitchshift_destroy(FXPitchShift* fx);
void fx_pitchshift_reset(FXPitchShift* fx);

// Processing
void fx_pitchshift_process_f32(FXPitchShift* fx, float* buffer, int frames, int sample_rate);
void fx_pitchshift_process_i16(FXPitchShift* fx, int16_t* buffer, int frames, int sample_rate);
void fx_pitchshift_process_frame(FXPitchShift* fx, float* left, float* right, int sample_rate);

// Parameters (0.0 - 1.0)
void fx_pitchshift_set_enabled(FXPitchShift* fx, int enabled);
void fx_pitchshift_set_pitch(FXPitchShift* fx, float pitch);       // 0.0-1.0 maps to -12 to +12 semitones
void fx_pitchshift_set_mix(FXPitchShift* fx, float mix);           // 0.0-1.0 maps to 0% to 100% wet
void fx_pitchshift_set_formant(FXPitchShift* fx, float formant);   // 0.0-1.0, 0.5=preserve formants

int fx_pitchshift_get_enabled(FXPitchShift* fx);
float fx_pitchshift_get_pitch(FXPitchShift* fx);
float fx_pitchshift_get_mix(FXPitchShift* fx);
float fx_pitchshift_get_formant(FXPitchShift* fx);

#ifdef __cplusplus
}
#endif

#endif // FX_PITCHSHIFT_H
