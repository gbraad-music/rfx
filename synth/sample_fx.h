/**
 * Sample Effects Module
 * Granular pitch shifting and time-stretching for sample playback
 * Shares core algorithm with fx_pitchshift.c
 */

#ifndef SAMPLE_FX_H
#define SAMPLE_FX_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct SampleFX SampleFX;

/**
 * Create sample effects processor
 * sample_rate: Target sample rate in Hz
 */
SampleFX* sample_fx_create(uint32_t sample_rate);

/**
 * Destroy sample effects processor
 */
void sample_fx_destroy(SampleFX* fx);

/**
 * Reset processor state (clear grains, buffers)
 */
void sample_fx_reset(SampleFX* fx);

/**
 * Set pitch shift in semitones
 * semitones: -12.0 to +12.0 (0.0 = no pitch shift)
 */
void sample_fx_set_pitch(SampleFX* fx, float semitones);

/**
 * Set time-stretch ratio
 * ratio: 0.5 to 2.0 (1.0 = normal speed, 0.5 = half speed, 2.0 = double speed)
 */
void sample_fx_set_time_stretch(SampleFX* fx, float ratio);

/**
 * Set formant preservation (future use)
 * preserve: 0.0 to 1.0 (0.5 = preserve formants)
 */
void sample_fx_set_formant(SampleFX* fx, float preserve);

/**
 * Get current pitch shift in semitones
 */
float sample_fx_get_pitch(SampleFX* fx);

/**
 * Get current time-stretch ratio
 */
float sample_fx_get_time_stretch(SampleFX* fx);

/**
 * Process a single mono sample (int16)
 * input: Input sample (-32768 to 32767)
 * Returns: Processed sample
 */
int16_t sample_fx_process_sample(SampleFX* fx, int16_t input);

/**
 * Process a buffer of mono samples (int16)
 * buffer: Input/output buffer
 * num_samples: Number of samples to process
 */
void sample_fx_process_buffer(SampleFX* fx, int16_t* buffer, uint32_t num_samples);

/**
 * Check if effects are active (pitch != 0 or time_stretch != 1.0)
 */
bool sample_fx_is_active(SampleFX* fx);

#ifdef __cplusplus
}
#endif

#endif // SAMPLE_FX_H
