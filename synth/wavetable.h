#ifndef WAVETABLE_H
#define WAVETABLE_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Wavetable Oscillator
 *
 * A simple, efficient wavetable oscillator with linear interpolation.
 * Supports arbitrary waveform data and can be used for both 8-bit samples
 * (like OctaMED) and floating-point tables (like plugin synths).
 */

typedef struct {
    float phase;           // Current phase (0.0 to 1.0, normalized)
    float phase_increment; // Phase increment per sample
} WavetableOscillator;

/**
 * Initialize a wavetable oscillator
 * @param osc Oscillator to initialize
 */
static inline void wavetable_init(WavetableOscillator* osc) {
    osc->phase = 0.0f;
    osc->phase_increment = 0.0f;
}

/**
 * Set oscillator frequency
 * @param osc Oscillator
 * @param frequency Frequency in Hz
 * @param sample_rate Sample rate in Hz
 */
static inline void wavetable_set_frequency(WavetableOscillator* osc, float frequency, float sample_rate) {
    osc->phase_increment = frequency / sample_rate;
}

/**
 * Process one sample from an 8-bit signed wavetable (OctaMED-style)
 * @param osc Oscillator
 * @param wavetable Pointer to 8-bit signed waveform data
 * @param length Length of waveform in samples
 * @return Sample value in range [-1.0, 1.0]
 */
static inline float wavetable_process_int8(WavetableOscillator* osc, const int8_t* wavetable, uint16_t length) {
    // Calculate position in wavetable
    float phase_pos = osc->phase * length;
    int pos1 = (int)phase_pos % length;
    int pos2 = (pos1 + 1) % length;
    float frac = phase_pos - (int)phase_pos;

    // Linear interpolation
    float samp1 = wavetable[pos1] / 128.0f;
    float samp2 = wavetable[pos2] / 128.0f;
    float sample = samp1 + (samp2 - samp1) * frac;

    // Advance phase
    osc->phase += osc->phase_increment;
    if (osc->phase >= 1.0f) {
        osc->phase -= (int)osc->phase;  // Keep fractional part
    }

    return sample;
}

/**
 * Process one sample from a float wavetable (plugin-style)
 * @param osc Oscillator
 * @param wavetable Pointer to float waveform data
 * @param length Length of waveform in samples
 * @return Sample value (range depends on wavetable data)
 */
static inline float wavetable_process_float(WavetableOscillator* osc, const float* wavetable, uint16_t length) {
    // Calculate position in wavetable
    float phase_pos = osc->phase * length;
    int pos1 = (int)phase_pos % length;
    int pos2 = (pos1 + 1) % length;
    float frac = phase_pos - (int)phase_pos;

    // Linear interpolation
    float sample = wavetable[pos1] * (1.0f - frac) + wavetable[pos2] * frac;

    // Advance phase
    osc->phase += osc->phase_increment;
    if (osc->phase >= 1.0f) {
        osc->phase -= (int)osc->phase;  // Keep fractional part
    }

    return sample;
}

/**
 * Reset oscillator phase to start
 * @param osc Oscillator
 * @param phase Initial phase (0.0 to 1.0)
 */
static inline void wavetable_reset_phase(WavetableOscillator* osc, float phase) {
    osc->phase = phase;
    // Normalize to [0.0, 1.0)
    if (osc->phase >= 1.0f) {
        osc->phase -= (int)osc->phase;
    }
}

/**
 * Get current phase
 * @param osc Oscillator
 * @return Phase value (0.0 to 1.0)
 */
static inline float wavetable_get_phase(const WavetableOscillator* osc) {
    return osc->phase;
}

#ifdef __cplusplus
}
#endif

#endif // WAVETABLE_H
