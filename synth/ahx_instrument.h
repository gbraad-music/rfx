/*
 * AHX Instrument Synth
 *
 * Synthesizer based on AHX/HVL instrument architecture.
 * Extracts the synthesis engine from the AHX player for use as a
 * standalone, MIDI-controllable polyphonic synthesizer.
 *
 * Features:
 * - Authentic AHX waveform synthesis (triangle, saw, square, noise)
 * - ADSR envelope with per-stage volume control
 * - Filter modulation (sweep between limits)
 * - Square wave modulation (PWM effect)
 * - Vibrato with delay
 * - PList sequencing (optional)
 * - Hard-cut release
 *
 * Uses reusable tracker components:
 * - TrackerVoice for wavetable playback
 * - TrackerModulator for filter/PWM sweeps
 * - TrackerSequence for PList automation
 */

#ifndef AHX_INSTRUMENT_H
#define AHX_INSTRUMENT_H

#include <stdint.h>
#include <stdbool.h>
#include "ahx_synth_core.h"

#ifdef __cplusplus
extern "C" {
#endif

// PList (Performance List) - sequence of commands executed per note
typedef struct {
    int16_t note;           // Note to play (0 = no change, 1-60 = note)
    uint8_t fixed;          // If true, note doesn't transpose
    uint8_t waveform;       // Waveform (0 = no change, 1-4 = triangle/saw/square/noise)
    uint8_t fx[2];          // Two effect commands (0-7)
    uint8_t fx_param[2];    // Parameters for the two effects
} AhxPListEntry;

typedef struct {
    uint8_t speed;          // Frames per entry (ticks between steps)
    uint8_t length;         // Number of entries
    AhxPListEntry* entries; // Array of entries [length]
} AhxPList;

// AHX waveform types
typedef enum {
    AHX_WAVE_TRIANGLE = 0,
    AHX_WAVE_SAWTOOTH = 1,
    AHX_WAVE_SQUARE = 2,
    AHX_WAVE_NOISE = 3
} AhxWaveform;

// ADSR envelope (AHX-style: frames + volumes)
typedef struct {
    uint8_t attack_frames;      // 0-255
    uint8_t attack_volume;      // 0-64
    uint8_t decay_frames;       // 0-255
    uint8_t decay_volume;       // 0-64
    uint8_t sustain_frames;     // 0-255 (0 = infinite)
    uint8_t release_frames;     // 0-255
    uint8_t release_volume;     // 0-64
} AhxEnvelope;

// AHX Instrument parameters
typedef struct {
    // Oscillator
    AhxWaveform waveform;       // Base waveform
    uint8_t wave_length;        // 0-7 (affects harmonics)
    uint8_t volume;             // 0-64

    // Envelope
    AhxEnvelope envelope;

    // Filter modulation
    uint8_t filter_lower;       // 0-63
    uint8_t filter_upper;       // 0-63
    uint8_t filter_speed;       // 0-63
    bool filter_enabled;

    // Square modulation (PWM)
    uint8_t square_lower;       // 0-255
    uint8_t square_upper;       // 0-255
    uint8_t square_speed;       // 0-255
    bool square_enabled;

    // Vibrato
    uint8_t vibrato_delay;      // Frames before vibrato starts
    uint8_t vibrato_depth;      // 0-15
    uint8_t vibrato_speed;      // 0-255

    // Release
    bool hard_cut_release;      // If true, ignore release envelope
    uint8_t hard_cut_frames;    // Frames for hard cut (0-7)

    // PList (optional - set to NULL if not used)
    AhxPList* plist;            // Optional PList sequence
} AhxInstrumentParams;

// AHX Instrument voice state (uses authentic AHX synthesis core)
typedef struct {
    // Plugin parameters (external control)
    AhxInstrumentParams params;

    // Authentic AHX synthesis core
    AhxSynthVoice voice;
    AhxCoreInstrument core_inst;

    // Note info (for plugin reference)
    uint8_t note;               // MIDI note
    uint8_t velocity;           // MIDI velocity (0-127)
    bool active;                // Is voice active?
    bool released;              // Has note been released?

    // PList execution state
    uint8_t perf_current;       // Current PList position
    uint8_t perf_speed;         // Current speed (frames per entry)
    uint8_t perf_wait;          // Frames until next entry
    int16_t perf_sub_volume;    // PList sub-volume (0-64)
    int16_t period_perf_slide_speed;   // Portamento speed from PList
    int16_t period_perf_slide_period;  // Portamento accumulator
    bool period_perf_slide_on;         // Portamento active flag
} AhxInstrument;

/**
 * Initialize AHX instrument with default parameters
 */
void ahx_instrument_init(AhxInstrument* inst);

/**
 * Set instrument parameters
 * Makes a copy of params, so caller can free/modify after this call
 */
void ahx_instrument_set_params(AhxInstrument* inst, const AhxInstrumentParams* params);

/**
 * Get current instrument parameters
 */
void ahx_instrument_get_params(const AhxInstrument* inst, AhxInstrumentParams* params);

/**
 * Trigger note (Note On)
 * @param inst Instrument instance
 * @param note MIDI note (0-127)
 * @param velocity MIDI velocity (0-127)
 * @param sample_rate Output sample rate
 */
void ahx_instrument_note_on(AhxInstrument* inst, uint8_t note, uint8_t velocity, uint32_t sample_rate);

/**
 * Release note (Note Off)
 * Begins release phase of envelope
 */
void ahx_instrument_note_off(AhxInstrument* inst);

/**
 * Process one sample frame (50Hz for AHX timing)
 * Internal use - called by ahx_instrument_process
 */
void ahx_instrument_process_frame(AhxInstrument* inst);

/**
 * Generate audio samples
 * @param inst Instrument instance
 * @param output Output buffer (mono)
 * @param num_samples Number of samples to generate
 * @param sample_rate Sample rate (e.g., 48000)
 * @return Number of samples actually generated
 */
uint32_t ahx_instrument_process(AhxInstrument* inst, float* output, uint32_t num_samples, uint32_t sample_rate);

/**
 * Check if instrument is active (still producing sound)
 */
bool ahx_instrument_is_active(const AhxInstrument* inst);

/**
 * Reset instrument to initial state
 */
void ahx_instrument_reset(AhxInstrument* inst);

/**
 * Create default instrument parameters
 * Returns a basic preset suitable for testing
 */
AhxInstrumentParams ahx_instrument_default_params(void);

#ifdef __cplusplus
}
#endif

#endif // AHX_INSTRUMENT_H
