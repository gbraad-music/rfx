/*
 * Regroove Bass Station Synthesizer
 * Monophonic analog bass synthesizer inspired by Novation Bass Station
 *
 * Features:
 * - 2 DCOs (OSC1, OSC2) with 4 waveforms each (sine, saw, square, triangle)
 * - Sub-oscillator (OSC3) - 1 or 2 octaves below OSC1
 * - Dual filter modes: Classic (Moog ladder) and Acid (TB-303 style)
 * - 2 ADSR envelopes (amplitude and modulation)
 * - 2 LFOs for modulation
 * - Oscillator sync (OSC2 -> OSC1)
 * - Portamento/glide
 * - Analog distortion
 */

#ifndef BASS_STATION_H
#define BASS_STATION_H

#include "synth_common.h"

#ifdef __cplusplus
extern "C" {
#endif

// Forward declaration
typedef struct BassStation BassStation;

// Filter modes
typedef enum {
    BASS_STATION_FILTER_CLASSIC,  // Moog-style ladder filter
    BASS_STATION_FILTER_ACID      // TB-303 style filter
} BassStationFilterMode;

// Filter types (for Classic mode)
typedef enum {
    BASS_STATION_FILTER_LPF_12DB,
    BASS_STATION_FILTER_LPF_24DB,
    BASS_STATION_FILTER_HPF_12DB,
    BASS_STATION_FILTER_HPF_24DB,
    BASS_STATION_FILTER_BPF_12DB,
    BASS_STATION_FILTER_BPF_24DB
} BassStationFilterType;

// Sub-oscillator modes
typedef enum {
    BASS_STATION_SUB_OFF,
    BASS_STATION_SUB_MINUS_1_OCT,  // 1 octave below OSC1
    BASS_STATION_SUB_MINUS_2_OCT   // 2 octaves below OSC1
} BassStationSubMode;

// Sub-oscillator waveforms
typedef enum {
    BASS_STATION_SUB_SQUARE,
    BASS_STATION_SUB_SINE,
    BASS_STATION_SUB_PULSE
} BassStationSubWave;

// ============================================================================
// Lifecycle
// ============================================================================

/**
 * Create a new Bass Station synthesizer
 */
BassStation* bass_station_create(void);

/**
 * Destroy a Bass Station synthesizer
 */
void bass_station_destroy(BassStation* bs);

/**
 * Reset all synthesis components to initial state
 */
void bass_station_reset(BassStation* bs);

// ============================================================================
// MIDI Control
// ============================================================================

/**
 * Trigger a note
 * @param note MIDI note number (0-127)
 * @param velocity Note velocity (0-127)
 */
void bass_station_note_on(BassStation* bs, uint8_t note, uint8_t velocity);

/**
 * Release a note
 * @param note MIDI note number (0-127)
 */
void bass_station_note_off(BassStation* bs, uint8_t note);

/**
 * Check if the synth is currently active (playing or releasing)
 */
int bass_station_is_active(BassStation* bs);

// ============================================================================
// Oscillator Parameters
// ============================================================================

/**
 * Set OSC1 waveform (0.0-3.0: 0=Sine, 1=Saw, 2=Square, 3=Triangle)
 */
void bass_station_set_osc1_waveform(BassStation* bs, float waveform);

/**
 * Set OSC1 octave (-2 to +2)
 */
void bass_station_set_osc1_octave(BassStation* bs, int octave);

/**
 * Set OSC1 fine tune in semitones (-12.0 to +12.0)
 */
void bass_station_set_osc1_fine(BassStation* bs, float semitones);

/**
 * Set OSC1 pulse width (0.0-1.0, only affects square/pulse waveforms)
 */
void bass_station_set_osc1_pw(BassStation* bs, float pw);

/**
 * Set OSC2 waveform (0.0-3.0: 0=Sine, 1=Saw, 2=Square, 3=Triangle)
 */
void bass_station_set_osc2_waveform(BassStation* bs, float waveform);

/**
 * Set OSC2 octave (-2 to +2)
 */
void bass_station_set_osc2_octave(BassStation* bs, int octave);

/**
 * Set OSC2 fine tune in semitones (-12.0 to +12.0)
 */
void bass_station_set_osc2_fine(BassStation* bs, float semitones);

/**
 * Set OSC2 pulse width (0.0-1.0, only affects square/pulse waveforms)
 */
void bass_station_set_osc2_pw(BassStation* bs, float pw);

/**
 * Set oscillator mix (0.0 = OSC1 only, 0.5 = equal mix, 1.0 = OSC2 only)
 */
void bass_station_set_osc_mix(BassStation* bs, float mix);

/**
 * Enable/disable oscillator sync (OSC2 syncs to OSC1)
 */
void bass_station_set_osc_sync(BassStation* bs, int enable);

// ============================================================================
// Sub-Oscillator Parameters
// ============================================================================

/**
 * Set sub-oscillator mode (off, -1 octave, -2 octaves)
 */
void bass_station_set_sub_mode(BassStation* bs, BassStationSubMode mode);

/**
 * Set sub-oscillator waveform
 */
void bass_station_set_sub_wave(BassStation* bs, BassStationSubWave wave);

/**
 * Set sub-oscillator level (0.0-1.0)
 */
void bass_station_set_sub_level(BassStation* bs, float level);

// ============================================================================
// Filter Parameters
// ============================================================================

/**
 * Set filter mode (Classic or Acid)
 */
void bass_station_set_filter_mode(BassStation* bs, BassStationFilterMode mode);

/**
 * Set filter type (for Classic mode: LPF/HPF/BPF with 12dB/24dB slopes)
 */
void bass_station_set_filter_type(BassStation* bs, BassStationFilterType type);

/**
 * Set filter cutoff frequency (0.0-1.0)
 */
void bass_station_set_filter_cutoff(BassStation* bs, float cutoff);

/**
 * Set filter resonance (0.0-1.0)
 */
void bass_station_set_filter_resonance(BassStation* bs, float resonance);

/**
 * Set filter overdrive amount (0.0-1.0)
 */
void bass_station_set_filter_drive(BassStation* bs, float drive);

// ============================================================================
// Envelope Parameters
// ============================================================================

/**
 * Set amplitude envelope attack time (0.0-5.0 seconds)
 */
void bass_station_set_amp_attack(BassStation* bs, float attack);

/**
 * Set amplitude envelope decay time (0.0-5.0 seconds)
 */
void bass_station_set_amp_decay(BassStation* bs, float decay);

/**
 * Set amplitude envelope sustain level (0.0-1.0)
 */
void bass_station_set_amp_sustain(BassStation* bs, float sustain);

/**
 * Set amplitude envelope release time (0.0-5.0 seconds)
 */
void bass_station_set_amp_release(BassStation* bs, float release);

/**
 * Set modulation envelope attack time (0.0-5.0 seconds)
 */
void bass_station_set_mod_attack(BassStation* bs, float attack);

/**
 * Set modulation envelope decay time (0.0-5.0 seconds)
 */
void bass_station_set_mod_decay(BassStation* bs, float decay);

/**
 * Set modulation envelope sustain level (0.0-1.0)
 */
void bass_station_set_mod_sustain(BassStation* bs, float sustain);

/**
 * Set modulation envelope release time (0.0-5.0 seconds)
 */
void bass_station_set_mod_release(BassStation* bs, float release);

// ============================================================================
// Modulation Parameters
// ============================================================================

/**
 * Set mod envelope -> filter cutoff amount (-1.0 to +1.0)
 */
void bass_station_set_mod_env_to_filter(BassStation* bs, float amount);

/**
 * Set mod envelope -> pitch amount (-1.0 to +1.0, in octaves)
 */
void bass_station_set_mod_env_to_pitch(BassStation* bs, float amount);

/**
 * Set mod envelope -> pulse width amount (-1.0 to +1.0)
 */
void bass_station_set_mod_env_to_pw(BassStation* bs, float amount);

/**
 * Set LFO1 frequency (0.1-20.0 Hz)
 */
void bass_station_set_lfo1_rate(BassStation* bs, float rate);

/**
 * Set LFO1 waveform (0.0-5.0: 0=Sine, 1=Triangle, 2=Square, 3=SawUp, 4=SawDown, 5=Random)
 */
void bass_station_set_lfo1_waveform(BassStation* bs, float waveform);

/**
 * Set LFO1 -> pitch modulation amount (-1.0 to +1.0, in semitones)
 */
void bass_station_set_lfo1_to_pitch(BassStation* bs, float amount);

/**
 * Set LFO2 frequency (0.1-20.0 Hz)
 */
void bass_station_set_lfo2_rate(BassStation* bs, float rate);

/**
 * Set LFO2 waveform (0.0-5.0: 0=Sine, 1=Triangle, 2=Square, 3=SawUp, 4=SawDown, 5=Random)
 */
void bass_station_set_lfo2_waveform(BassStation* bs, float waveform);

/**
 * Set LFO2 -> pulse width modulation amount (-1.0 to +1.0)
 */
void bass_station_set_lfo2_to_pw(BassStation* bs, float amount);

/**
 * Set LFO2 -> filter cutoff modulation amount (-1.0 to +1.0)
 */
void bass_station_set_lfo2_to_filter(BassStation* bs, float amount);

// ============================================================================
// Performance Parameters
// ============================================================================

/**
 * Set portamento/glide time (0.0-1.0 seconds)
 */
void bass_station_set_portamento(BassStation* bs, float time);

/**
 * Set master volume (0.0-1.0)
 */
void bass_station_set_volume(BassStation* bs, float volume);

/**
 * Set distortion amount (0.0-1.0)
 */
void bass_station_set_distortion(BassStation* bs, float amount);

// ============================================================================
// Audio Processing
// ============================================================================

/**
 * Process a single sample
 * @param sample_rate Current sample rate
 * @return Rendered audio sample
 */
float bass_station_process(BassStation* bs, int sample_rate);

/**
 * Process a buffer of stereo samples
 * @param output Interleaved stereo output buffer (L, R, L, R, ...)
 * @param num_frames Number of frames to process
 * @param sample_rate Current sample rate
 */
void bass_station_process_stereo(BassStation* bs, float* output, int num_frames, int sample_rate);

#ifdef __cplusplus
}
#endif

#endif // BASS_STATION_H
