/*
 * Regroove Amiga Filter Effect
 * Authentic Amiga 500 and 1200 audio filters
 *
 * Based on OpenMPT's Paula emulation (BSD license)
 * Original authors: OpenMPT Devs, Antti S. Lankila
 *
 * Filter characteristics:
 * - A500: 4.9 kHz RC lowpass + optional 3275 Hz LED Butterworth
 * - A1200: 32 kHz RC lowpass + optional 3275 Hz LED Butterworth
 */

#ifndef FX_AMIGA_FILTER_H
#define FX_AMIGA_FILTER_H

#include "fx_common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct FXAmigaFilter FXAmigaFilter;

typedef enum {
    AMIGA_FILTER_OFF = 0,           // Bypass
    AMIGA_FILTER_A500_LED_OFF,      // A500 with 4.9kHz RC lowpass only
    AMIGA_FILTER_A500_LED_ON,       // A500 with 4.9kHz + 3275Hz Butterworth
    AMIGA_FILTER_A1200_LED_OFF,     // A1200 with 32kHz RC lowpass only
    AMIGA_FILTER_A1200_LED_ON,      // A1200 with 32kHz + 3275Hz Butterworth
    AMIGA_FILTER_UNFILTERED         // No filtering (for comparison)
} AmigaFilterType;

// Lifecycle
FXAmigaFilter* fx_amiga_filter_create(void);
void fx_amiga_filter_destroy(FXAmigaFilter* fx);
void fx_amiga_filter_reset(FXAmigaFilter* fx);

// Processing
void fx_amiga_filter_process_f32(FXAmigaFilter* fx, float* buffer, int frames, int sample_rate);
void fx_amiga_filter_process_i16(FXAmigaFilter* fx, int16_t* buffer, int frames, int sample_rate);
void fx_amiga_filter_process_frame(FXAmigaFilter* fx, float* left, float* right, int sample_rate);

// Parameters
void fx_amiga_filter_set_enabled(FXAmigaFilter* fx, int enabled);
void fx_amiga_filter_set_type(FXAmigaFilter* fx, AmigaFilterType type);
void fx_amiga_filter_set_mix(FXAmigaFilter* fx, float mix);  // Dry/wet 0.0-1.0

int fx_amiga_filter_get_enabled(FXAmigaFilter* fx);
AmigaFilterType fx_amiga_filter_get_type(FXAmigaFilter* fx);
float fx_amiga_filter_get_mix(FXAmigaFilter* fx);

// ============================================================================
// Generic Parameter Interface (for wrapper use)
// ============================================================================

int fx_amiga_filter_get_parameter_count(void);
float fx_amiga_filter_get_parameter_value(FXAmigaFilter* fx, int index);
void fx_amiga_filter_set_parameter_value(FXAmigaFilter* fx, int index, float value);
const char* fx_amiga_filter_get_parameter_name(int index);
const char* fx_amiga_filter_get_parameter_label(int index);
float fx_amiga_filter_get_parameter_default(int index);
float fx_amiga_filter_get_parameter_min(int index);
float fx_amiga_filter_get_parameter_max(int index);
int fx_amiga_filter_get_parameter_group(int index);
const char* fx_amiga_filter_get_group_name(int group);
int fx_amiga_filter_parameter_is_integer(int index);

#ifdef __cplusplus
}
#endif

#endif // FX_AMIGA_FILTER_H
