#pragma once
/**
 * Modal Piano Synthesizer - Piano-specific sample-based synthesis
 *
 * Features:
 * - Modal resonator bank for sympathetic resonance
 * - Filter envelope for dynamic brightness (velocity → brightness)
 * - Per-partial decay for realistic piano timbre
 * - Built on top of generic sample player
 */

#include <stdint.h>
#include <stdbool.h>
#include "synth_sample_player.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ModalPiano ModalPiano;

/**
 * Create a new modal piano instance
 * Returns NULL on allocation failure
 */
ModalPiano* modal_piano_create(void);

/**
 * Destroy a modal piano instance
 */
void modal_piano_destroy(ModalPiano* piano);

/**
 * Load sample data into the piano
 * The sample data must remain valid for the lifetime of playback
 */
void modal_piano_load_sample(ModalPiano* piano, const SampleData* sample_data);

/**
 * Trigger a note
 * note: MIDI note number (0-127)
 * velocity: MIDI velocity (0-127)
 */
void modal_piano_trigger(ModalPiano* piano, uint8_t note, uint8_t velocity);

/**
 * Release the note (start release phase)
 */
void modal_piano_release(ModalPiano* piano);

/**
 * Set decay time for the sample loop
 * decay_time: Decay time in seconds (e.g., 0.5 - 8.0)
 */
void modal_piano_set_decay(ModalPiano* piano, float decay_time);

/**
 * Set resonance amount (dry/wet mix of modal resonators)
 * amount: 0.0 (no resonance) to 1.0 (full resonance)
 */
void modal_piano_set_resonance(ModalPiano* piano, float amount);

/**
 * Set filter envelope parameters
 * attack_time: Attack time in seconds
 * decay_time: Decay time in seconds
 * sustain_level: Sustain brightness level (0.0-1.0)
 */
void modal_piano_set_filter_envelope(ModalPiano* piano, float attack_time, float decay_time, float sustain_level);

/**
 * Set velocity sensitivity for filter brightness
 * amount: 0.0 (no velocity sensitivity) to 1.0 (full sensitivity)
 */
void modal_piano_set_velocity_sensitivity(ModalPiano* piano, float amount);

/**
 * Set LFO parameters for tremolo effect
 * rate: LFO frequency in Hz (e.g., 0.5 - 8.0)
 * depth: Modulation depth 0.0 (off) to 1.0 (maximum)
 */
void modal_piano_set_lfo(ModalPiano* piano, float rate, float depth);

/**
 * Check if the piano is currently active
 */
bool modal_piano_is_active(const ModalPiano* piano);

/**
 * Process one sample and return the output
 * output_sample_rate: The target sample rate (e.g., 48000)
 * Returns the processed sample value (-1.0 to 1.0)
 */
float modal_piano_process(ModalPiano* piano, uint32_t output_sample_rate);

/**
 * Reset the piano to initial state
 */
void modal_piano_reset(ModalPiano* piano);

#ifdef __cplusplus
}
#endif
