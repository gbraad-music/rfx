/*
 * Regroove Paula BLEP Effect
 * Amiga Paula chip emulation using Band-Limited Steps
 *
 * Based on OpenMPT Paula emulation (BSD license)
 * Original authors: OpenMPT Devs, Antti S. Lankila
 *
 * This effect emulates the Amiga Paula DAC behavior, which outputs
 * discrete voltage steps rather than continuous interpolation.
 * BLEP (Band-Limited stEP) synthesis prevents aliasing.
 */

#ifndef FX_PAULA_BLEP_H
#define FX_PAULA_BLEP_H

#include "fx_common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct FXPaulaBlep FXPaulaBlep;

typedef enum {
    PAULA_A500_OFF = 0,       // A500: 4.9kHz RC filter, no LED
    PAULA_A500_ON,            // A500: 4.9kHz RC + 3275Hz LED Butterworth
    PAULA_A1200_OFF,          // A1200: 32kHz leakage, no LED
    PAULA_A1200_ON,           // A1200: 32kHz leakage + 3275Hz LED
    PAULA_UNFILTERED,         // No filtering (pure BLEP)
    PAULA_NUM_MODES
} PaulaMode;

// Lifecycle
FXPaulaBlep* fx_paula_blep_create(void);
void fx_paula_blep_destroy(FXPaulaBlep* fx);
void fx_paula_blep_reset(FXPaulaBlep* fx);

// Processing
void fx_paula_blep_process_f32(FXPaulaBlep* fx, const float* input, float* output,
                                int input_frames, int output_frames, int sample_rate);

// Input sample (int16 format, as Paula uses 8-bit DAC)
void fx_paula_blep_input_sample(FXPaulaBlep* fx, int16_t sample);

// Output sample (returns accumulated BLEP output)
int fx_paula_blep_output_sample(FXPaulaBlep* fx);

// Advance clock by cycles
void fx_paula_blep_clock(FXPaulaBlep* fx, int cycles);

// Parameters
void fx_paula_blep_set_enabled(FXPaulaBlep* fx, int enabled);
void fx_paula_blep_set_mode(FXPaulaBlep* fx, PaulaMode mode);
void fx_paula_blep_set_mix(FXPaulaBlep* fx, float mix);  // 0.0 = dry, 1.0 = wet

int fx_paula_blep_get_enabled(FXPaulaBlep* fx);
PaulaMode fx_paula_blep_get_mode(FXPaulaBlep* fx);
float fx_paula_blep_get_mix(FXPaulaBlep* fx);

// Utility
const char* fx_paula_blep_get_mode_name(PaulaMode mode);

// ============================================================================
// Generic Parameter Interface (for wrapper use)
// ============================================================================

int fx_paula_blep_get_parameter_count(void);
float fx_paula_blep_get_parameter_value(FXPaulaBlep* fx, int index);
void fx_paula_blep_set_parameter_value(FXPaulaBlep* fx, int index, float value);
const char* fx_paula_blep_get_parameter_name(int index);
const char* fx_paula_blep_get_parameter_label(int index);
float fx_paula_blep_get_parameter_default(int index);
float fx_paula_blep_get_parameter_min(int index);
float fx_paula_blep_get_parameter_max(int index);
int fx_paula_blep_get_parameter_group(int index);
const char* fx_paula_blep_get_group_name(int group);
int fx_paula_blep_parameter_is_integer(int index);

#ifdef __cplusplus
}
#endif

#endif // FX_PAULA_BLEP_H
