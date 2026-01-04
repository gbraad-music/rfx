#include "synth_modal_piano.h"
#include "synth_sample_player.h"
#include <stdlib.h>
#include <math.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

#define NUM_RESONATORS 6

// Biquad filter for modal resonators
typedef struct {
    float b0, b1, b2, a1, a2;  // Filter coefficients
    float x1, x2, y1, y2;       // State variables
} BiquadFilter;

struct ModalPiano {
    SynthSamplePlayer* sample_player;

    // Modal resonators (biquad bandpass filters)
    BiquadFilter resonators[NUM_RESONATORS];
    float resonator_gains[NUM_RESONATORS];
    float resonance_amount;

    // Filter envelope
    float filter_cutoff;          // Current cutoff frequency (Hz)
    float filter_envelope;        // Current envelope value (0-1)
    float filter_attack_time;     // Attack time in seconds
    float filter_decay_time;      // Decay time in seconds
    float filter_sustain;         // Sustain level (0-1)
    float filter_velocity_amt;    // Velocity sensitivity (0-1)
    float filter_peak;            // Peak brightness from velocity

    // Envelope state
    enum { ENV_ATTACK, ENV_DECAY, ENV_SUSTAIN, ENV_RELEASE, ENV_IDLE } env_state;
    float env_time;               // Time in current envelope stage

    // One-pole low-pass filter state
    float filter_prev_sample;

    // Note info
    float fundamental_freq;
    uint8_t velocity;
};

// Initialize biquad as bandpass filter with moderate Q
static void biquad_init_bandpass(BiquadFilter* filter, float freq, float sample_rate, float Q) {
    float w0 = 2.0f * M_PI * freq / sample_rate;
    float alpha = sinf(w0) / (2.0f * Q);
    float cos_w0 = cosf(w0);

    // Bandpass filter (constant 0 dB peak gain)
    filter->b0 = alpha;
    filter->b1 = 0.0f;
    filter->b2 = -alpha;
    filter->a1 = -2.0f * cos_w0;
    filter->a2 = 1.0f - alpha;

    // Normalize by a0
    float a0 = 1.0f + alpha;
    filter->b0 /= a0;
    filter->b1 /= a0;
    filter->b2 /= a0;
    filter->a1 /= a0;
    filter->a2 /= a0;

    filter->x1 = filter->x2 = 0.0f;
    filter->y1 = filter->y2 = 0.0f;
}

// Process one sample through biquad filter
static inline float biquad_process(BiquadFilter* filter, float input) {
    float output = filter->b0 * input + filter->b1 * filter->x1 + filter->b2 * filter->x2
                   - filter->a1 * filter->y1 - filter->a2 * filter->y2;

    // Clamp output to prevent extreme values
    if (output > 10.0f) output = 10.0f;
    if (output < -10.0f) output = -10.0f;
    // Check for NaN (NaN != NaN is true)
    if (output != output) output = 0.0f;

    filter->x2 = filter->x1;
    filter->x1 = input;
    filter->y2 = filter->y1;
    filter->y1 = output;

    return output;
}

// Reset biquad filter state
static void biquad_reset(BiquadFilter* filter) {
    filter->x1 = filter->x2 = 0.0f;
    filter->y1 = filter->y2 = 0.0f;
}

// Convert MIDI note to frequency
static float midi_to_freq(uint8_t note) {
    return 440.0f * powf(2.0f, (note - 69) / 12.0f);
}

ModalPiano* modal_piano_create(void) {
    ModalPiano* piano = (ModalPiano*)malloc(sizeof(ModalPiano));
    if (!piano) return NULL;

    memset(piano, 0, sizeof(ModalPiano));

    // Create internal sample player
    piano->sample_player = synth_sample_player_create();
    if (!piano->sample_player) {
        free(piano);
        return NULL;
    }

    // Default parameters
    piano->resonance_amount = 0.0f;      // Start with resonators off for safety
    piano->filter_attack_time = 0.01f;   // 10ms attack (not used - we skip to decay)
    piano->filter_decay_time = 0.3f;     // 300ms decay
    piano->filter_sustain = 0.6f;        // Sustain at 60% brightness
    piano->filter_velocity_amt = 0.8f;   // 80% velocity sensitivity
    piano->env_state = ENV_IDLE;

    return piano;
}

void modal_piano_destroy(ModalPiano* piano) {
    if (!piano) return;

    if (piano->sample_player) {
        synth_sample_player_destroy(piano->sample_player);
    }

    free(piano);
}

void modal_piano_load_sample(ModalPiano* piano, const SampleData* sample_data) {
    if (!piano || !piano->sample_player) return;

    synth_sample_player_load_sample(piano->sample_player, sample_data);
}

void modal_piano_trigger(ModalPiano* piano, uint8_t note, uint8_t velocity) {
    if (!piano || !piano->sample_player) return;

    piano->velocity = velocity;
    piano->fundamental_freq = midi_to_freq(note);

    // Trigger internal sample player
    synth_sample_player_trigger(piano->sample_player, note, velocity);

    // Initialize modal resonators tuned to harmonics
    float sample_rate = 48000.0f; // TODO: make configurable

    for (int i = 0; i < NUM_RESONATORS; i++) {
        // Tune to harmonic (1x, 2x, 3x, 4x, 5x, 6x fundamental)
        float harmonic = (i + 1);
        float freq = piano->fundamental_freq * harmonic;

        // Clamp to Nyquist
        if (freq > sample_rate * 0.45f) {
            freq = sample_rate * 0.45f;
        }

        // Higher harmonics have lower Q (decay faster)
        // Q: 12 (fundamental) down to 7 (6th harmonic) - moderate Q for stability
        float Q = 12.0f - (harmonic - 1) * 0.8f;

        biquad_init_bandpass(&piano->resonators[i], freq, sample_rate, Q);

        // Explicitly reset filter state for safety
        biquad_reset(&piano->resonators[i]);

        // Higher harmonics have lower gain
        piano->resonator_gains[i] = 1.0f / harmonic;
    }

    // Calculate peak brightness based on velocity
    float vel_norm = velocity / 127.0f;
    piano->filter_peak = piano->filter_sustain +
                        (1.0f - piano->filter_sustain) * vel_norm * piano->filter_velocity_amt;

    // Start filter envelope at PEAK (bright) - piano starts bright!
    piano->filter_envelope = 1.0f;  // Start at full brightness
    piano->env_state = ENV_DECAY;   // Skip attack, go straight to decay
    piano->env_time = 0.0f;
}

void modal_piano_release(ModalPiano* piano) {
    if (!piano) return;

    if (piano->sample_player) {
        synth_sample_player_release(piano->sample_player);
    }

    piano->env_state = ENV_RELEASE;
    piano->env_time = 0.0f;
}

void modal_piano_set_decay(ModalPiano* piano, float decay_time) {
    if (!piano || !piano->sample_player) return;

    synth_sample_player_set_loop_decay(piano->sample_player, decay_time);
}

void modal_piano_set_resonance(ModalPiano* piano, float amount) {
    if (!piano) return;

    piano->resonance_amount = amount;
    if (piano->resonance_amount < 0.0f) piano->resonance_amount = 0.0f;
    if (piano->resonance_amount > 1.0f) piano->resonance_amount = 1.0f;
}

void modal_piano_set_filter_envelope(ModalPiano* piano, float attack_time, float decay_time, float sustain_level) {
    if (!piano) return;

    piano->filter_attack_time = attack_time;
    piano->filter_decay_time = decay_time;
    piano->filter_sustain = sustain_level;
}

void modal_piano_set_velocity_sensitivity(ModalPiano* piano, float amount) {
    if (!piano) return;

    piano->filter_velocity_amt = amount;
    if (piano->filter_velocity_amt < 0.0f) piano->filter_velocity_amt = 0.0f;
    if (piano->filter_velocity_amt > 1.0f) piano->filter_velocity_amt = 1.0f;
}

void modal_piano_set_lfo(ModalPiano* piano, float rate, float depth) {
    if (!piano || !piano->sample_player) return;

    synth_sample_player_set_lfo(piano->sample_player, rate, depth);
}

bool modal_piano_is_active(const ModalPiano* piano) {
    if (!piano || !piano->sample_player) return false;

    return synth_sample_player_is_active(piano->sample_player);
}

void modal_piano_reset(ModalPiano* piano) {
    if (!piano) return;

    if (piano->sample_player) {
        synth_sample_player_reset(piano->sample_player);
    }

    // Reset resonator state
    for (int i = 0; i < NUM_RESONATORS; i++) {
        biquad_reset(&piano->resonators[i]);
    }

    piano->filter_prev_sample = 0.0f;
    piano->filter_envelope = 0.0f;
    piano->env_state = ENV_IDLE;
}

float modal_piano_process(ModalPiano* piano, uint32_t output_sample_rate) {
    if (!piano || !piano->sample_player) return 0.0f;

    // Get sample from sample player
    float sample = synth_sample_player_process(piano->sample_player, output_sample_rate);

    // Update filter envelope
    float dt = 1.0f / output_sample_rate;

    switch (piano->env_state) {
        case ENV_ATTACK:
            if (piano->filter_attack_time > 0.0f) {
                piano->env_time += dt;
                piano->filter_envelope = piano->env_time / piano->filter_attack_time;

                if (piano->filter_envelope >= 1.0f) {
                    piano->filter_envelope = 1.0f;
                    piano->env_state = ENV_DECAY;
                    piano->env_time = 0.0f;
                }
            } else {
                piano->filter_envelope = 1.0f;
                piano->env_state = ENV_DECAY;
            }
            break;

        case ENV_DECAY:
            if (piano->filter_decay_time > 0.0f) {
                piano->env_time += dt;
                float decay_progress = piano->env_time / piano->filter_decay_time;

                if (decay_progress >= 1.0f) {
                    piano->filter_envelope = piano->filter_sustain / piano->filter_peak;
                    piano->env_state = ENV_SUSTAIN;
                } else {
                    // Exponential decay from 1.0 to sustain level
                    piano->filter_envelope = 1.0f - decay_progress * (1.0f - piano->filter_sustain / piano->filter_peak);
                }
            } else {
                piano->filter_envelope = piano->filter_sustain / piano->filter_peak;
                piano->env_state = ENV_SUSTAIN;
            }
            break;

        case ENV_SUSTAIN:
            piano->filter_envelope = piano->filter_sustain / piano->filter_peak;
            break;

        case ENV_RELEASE:
            piano->env_time += dt;
            // Quick release (100ms)
            if (piano->env_time > 0.1f) {
                piano->env_state = ENV_IDLE;
                piano->filter_envelope = 0.0f;
            } else {
                piano->filter_envelope *= 0.999f; // Exponential decay
            }
            break;

        case ENV_IDLE:
            piano->filter_envelope = 0.0f;
            break;
    }

    // Apply modal resonators (ADD sympathetic resonance to signal)
    if (piano->resonance_amount > 0.0f) {
        float resonant_sample = 0.0f;

        for (int i = 0; i < NUM_RESONATORS; i++) {
            float resonator_out = biquad_process(&piano->resonators[i], sample);
            resonant_sample += resonator_out * piano->resonator_gains[i];
        }

        // Normalize resonator sum
        // Bandpass filters with Q=7-12 need moderate scaling
        resonant_sample *= 1.5f;

        // ADD to dry signal (preserves volume)
        sample = sample + resonant_sample * piano->resonance_amount;

        // Soft clip the result to prevent harsh peaks from high resonance
        if (sample > 0.95f) {
            sample = 0.95f + 0.05f * tanhf((sample - 0.95f) / 0.05f);
        } else if (sample < -0.95f) {
            sample = -0.95f + 0.05f * tanhf((sample + 0.95f) / 0.05f);
        }
    }

    // Apply filter envelope (one-pole low-pass with time-varying cutoff)
    // Map envelope to cutoff: 0.2 (dark) to 1.0 (bright)
    float brightness = piano->filter_envelope * piano->filter_peak;
    float cutoff = 0.2f + brightness * 0.8f;

    sample = piano->filter_prev_sample + cutoff * (sample - piano->filter_prev_sample);
    piano->filter_prev_sample = sample;

    return sample;
}
