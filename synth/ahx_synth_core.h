/*
 * AHX Synthesis Core
 *
 * Shared synthesis engine using authentic AHX algorithms
 * Extracted from ahx_player.c for use by:
 * - ahx_player (module playback)
 * - ahx_instrument (synth plugin)
 */

#ifndef AHX_SYNTH_CORE_H
#define AHX_SYNTH_CORE_H

#include <stdint.h>
#include <stdbool.h>
#include "../players/tracker_voice.h"
#include "../players/tracker_modulator.h"

#ifdef __cplusplus
extern "C" {
#endif

// AHX envelope definition (from .ahx file format)
typedef struct {
    int aFrames, aVolume;      // Attack: frames and target volume (0-64)
    int dFrames, dVolume;      // Decay: frames and target volume (0-64)
    int sFrames;               // Sustain: frames to hold (0 = infinite)
    int rFrames, rVolume;      // Release: frames and target volume (0-64)
} AhxCoreEnvelope;

// AHX instrument definition (from .ahx file format)
typedef struct {
    int Waveform;                              // Waveform type (0=triangle, 1=sawtooth, 2=square, 3=noise)
    int Volume;                                // Instrument volume (0-64)
    int WaveLength;                            // Waveform harmonic length (0-7)
    AhxCoreEnvelope Envelope;                  // ADSR envelope
    int FilterLowerLimit, FilterUpperLimit;    // Filter modulation range (0-63)
    int FilterSpeed;                           // Filter modulation speed (0-63)
    int SquareLowerLimit, SquareUpperLimit;    // PWM range (0-255)
    int SquareSpeed;                           // PWM speed (0-255)
    int VibratoDelay;                          // Vibrato delay frames (0-255)
    int VibratoDepth;                          // Vibrato depth (0-15)
    int VibratoSpeed;                          // Vibrato speed (0-255)
    int HardCutRelease;                        // Hard cut release enabled (0/1)
    int HardCutReleaseFrames;                  // Hard cut release frames (0-7)
} AhxCoreInstrument;

// AHX synthesis voice state (runtime)
typedef struct {
    // Generic tracker components
    TrackerModulator filter_mod;
    TrackerModulator square_mod;
    TrackerVoice voice_playback;

    // Instrument reference
    const AhxCoreInstrument* Instrument;

    // ADSR state (using authentic AHX algorithm)
    int ADSRVolume;                // Current ADSR volume (8-bit fixed point: value << 8)
    AhxCoreEnvelope ADSR;          // Runtime ADSR deltas (calculated from instrument)

    // Voice state
    int NoteMaxVolume;             // Note volume (0-64)
    int PerfSubVolume;             // Performance sub-volume (0-64)
    int TrackMasterVolume;         // Track master volume (0-64)
    int VoiceVolume;               // Final output volume
    int VoicePeriod;               // Current period (Amiga-style)
    int InstrPeriod;               // Base instrument period
    int VibratoPeriod;             // Vibrato offset
    int Waveform;                  // Current waveform (0-3)
    int WaveLength;                // Wave length (0-7)
    int NewWaveform;               // Waveform change pending
    int IgnoreFilter;              // Ignore next filter command
    int IgnoreSquare;              // Ignore next square command
    int FilterPos;                 // Current filter position
    int SquarePos;                 // Current square position
    int PlantPeriod;               // Period update pending
    int FixedNote;                 // Fixed note (no transpose)

    // Vibrato state
    int VibratoDelay;              // Frames until vibrato starts
    int VibratoCurrent;            // Current vibrato phase
    int VibratoDepth;              // Vibrato depth
    int VibratoSpeed;              // Vibrato speed

    // Hard cut release
    int HardCutRelease;            // Hard cut enabled
    int HardCutReleaseF;           // Hard cut release frames
    int NoteCutOn;                 // Note cut active
    int NoteCutWait;               // Note cut wait frames

    // Active state
    bool TrackOn;                  // Voice active
    bool Released;                 // Note released

    // Wavetable for synthesis
    int16_t VoiceBuffer[0x281];    // 16-bit wavetable

    // Per-voice white noise random state
    int WNRandom;

    // Frame counter for sample-accurate timing
    uint32_t samples_per_frame;
    uint32_t samples_in_frame;
} AhxSynthVoice;

/**
 * Initialize synthesis voice
 */
void ahx_synth_voice_init(AhxSynthVoice* voice);

/**
 * Calculate ADSR deltas from instrument (authentic AHX algorithm)
 * Must be called before note_on or when instrument changes
 */
void ahx_synth_voice_calc_adsr(AhxSynthVoice* voice, const AhxCoreInstrument* instrument);

/**
 * Trigger note on
 * @param voice Voice to trigger
 * @param note MIDI note (0-127)
 * @param velocity MIDI velocity (0-127)
 * @param sample_rate Output sample rate
 */
void ahx_synth_voice_note_on(AhxSynthVoice* voice, uint8_t note, uint8_t velocity, uint32_t sample_rate);

/**
 * Trigger note off
 */
void ahx_synth_voice_note_off(AhxSynthVoice* voice);

/**
 * Process one AHX frame (50Hz timing)
 * Uses authentic AHX ADSR and modulation algorithms
 */
void ahx_synth_voice_process_frame(AhxSynthVoice* voice);

/**
 * Process audio samples
 * @param voice Voice to process
 * @param output Output buffer (mono)
 * @param num_samples Number of samples to generate
 * @param sample_rate Output sample rate
 * @return Number of samples actually generated
 */
uint32_t ahx_synth_voice_process(AhxSynthVoice* voice, float* output, uint32_t num_samples, uint32_t sample_rate);

/**
 * Check if voice is active
 */
bool ahx_synth_voice_is_active(const AhxSynthVoice* voice);

/**
 * Reset voice to initial state
 */
void ahx_synth_voice_reset(AhxSynthVoice* voice);

/**
 * Convert MIDI note to Amiga period
 */
int ahx_synth_note_to_period(uint8_t note);

#ifdef __cplusplus
}
#endif

#endif // AHX_SYNTH_CORE_H
