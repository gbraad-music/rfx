#pragma once
/**
 * Generic Sample Player - Reusable sample playback with looping support
 *
 * Features:
 * - Playback of 16-bit PCM samples
 * - Pitch shifting via playback rate adjustment
 * - Support for attack (one-shot) and loop (sustain) regions
 * - Automatic crossfade between attack and loop
 * - Amplitude envelope for volume decay during loop
 */

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct SynthSamplePlayer SynthSamplePlayer;

/**
 * Sample data structure
 * Stores pointers to sample data and loop points
 */
typedef struct {
    const int16_t* attack_data;   // Attack/onset sample data
    uint32_t attack_length;       // Attack sample length in samples

    const int16_t* loop_data;     // Loop/tail sample data (can be NULL for one-shot)
    uint32_t loop_length;         // Loop sample length in samples

    uint32_t sample_rate;         // Original sample rate (e.g., 22050)
    uint8_t root_note;            // MIDI note of the sample (for pitch calculation)
} SampleData;

/**
 * Create a new sample player instance
 * Returns NULL on allocation failure
 */
SynthSamplePlayer* synth_sample_player_create(void);

/**
 * Destroy a sample player instance
 */
void synth_sample_player_destroy(SynthSamplePlayer* player);

/**
 * Load sample data into the player
 * The sample data must remain valid for the lifetime of playback
 */
void synth_sample_player_load_sample(SynthSamplePlayer* player, const SampleData* sample);

/**
 * Trigger sample playback
 * note: MIDI note number (0-127)
 * velocity: MIDI velocity (0-127)
 */
void synth_sample_player_trigger(SynthSamplePlayer* player, uint8_t note, uint8_t velocity);

/**
 * Release the note (start decay if looping)
 */
void synth_sample_player_release(SynthSamplePlayer* player);

/**
 * Set loop decay time in seconds
 * Controls how quickly the volume fades during the loop
 */
void synth_sample_player_set_loop_decay(SynthSamplePlayer* player, float decay_time);

/**
 * Set LFO (Low Frequency Oscillator) parameters for tremolo effect
 * rate: LFO frequency in Hz (e.g., 0.5 - 8.0)
 * depth: Modulation depth 0.0 (off) to 1.0 (maximum)
 */
void synth_sample_player_set_lfo(SynthSamplePlayer* player, float rate, float depth);

/**
 * Check if the sample player is currently active
 */
bool synth_sample_player_is_active(const SynthSamplePlayer* player);

/**
 * Process one sample and return the output
 * output_sample_rate: The target sample rate (e.g., 48000)
 * Returns the processed sample value (-1.0 to 1.0)
 */
float synth_sample_player_process(SynthSamplePlayer* player, uint32_t output_sample_rate);

/**
 * Reset the sample player to initial state
 */
void synth_sample_player_reset(SynthSamplePlayer* player);

#ifdef __cplusplus
}
#endif
