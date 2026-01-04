#include "synth_modal_piano.h"
#include "synth_sample_player.h"
#include <stdlib.h>
#include <math.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

#define NUM_RESONATORS 6
#define MAX_COMB_DELAY 256  // Max delay samples (covers down to ~188Hz at 48kHz)

// Simple comb filter for modal resonators (much more stable than biquad)
typedef struct {
    float buffer[MAX_COMB_DELAY];
    int delay_samples;
    int write_pos;
    float feedback;
} CombFilter;

struct ModalPiano {
    SynthSamplePlayer* sample_player;

    // Modal resonators (comb filters)
    CombFilter resonators[NUM_RESONATORS];
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

// Initialize comb filter for resonance at given frequency
static void comb_init(CombFilter* comb, float freq, float sample_rate, float decay_time) {
    // Calculate delay for this frequency
    // delay = sample_rate / freq
    int delay = (int)(sample_rate / freq + 0.5f);

    // Clamp to valid range
    if (delay < 1) delay = 1;
    if (delay >= MAX_COMB_DELAY) delay = MAX_COMB_DELAY - 1;

    comb->delay_samples = delay;
    comb->write_pos = 0;

    // Calculate feedback for desired decay time
    // feedback = pow(0.001, 1.0 / (decay_time * sample_rate / delay))
    // Simpler: use fixed feedback based on harmonic number (higher = less feedback)
    comb->feedback = decay_time;  // Will be set per-harmonic (0.95-0.99)

    // Clear buffer
    for (int i = 0; i < MAX_COMB_DELAY; i++) {
        comb->buffer[i] = 0.0f;
    }
}

// Process one sample through comb filter
static inline float comb_process(CombFilter* comb, float input) {
    // Read delayed sample
    int read_pos = (comb->write_pos - comb->delay_samples + MAX_COMB_DELAY) % MAX_COMB_DELAY;
    float delayed = comb->buffer[read_pos];

    // Comb filter: output = input + feedback * delayed
    float output = input + comb->feedback * delayed;

    // Soft clip to prevent runaway feedback
    if (output > 1.0f) output = 1.0f;
    if (output < -1.0f) output = -1.0f;

    // Write to buffer
    comb->buffer[comb->write_pos] = output;

    // Advance write position
    comb->write_pos = (comb->write_pos + 1) % MAX_COMB_DELAY;

    return output;
}

// Reset comb filter state
static void comb_reset(CombFilter* comb) {
    for (int i = 0; i < MAX_COMB_DELAY; i++) {
        comb->buffer[i] = 0.0f;
    }
    comb->write_pos = 0;
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

        // Higher harmonics decay faster (lower feedback)
        // Feedback: 0.5 (subtle ring) to 0.3 (quick decay) - much lower to avoid distortion
        float feedback = 0.5f - (harmonic - 1) * 0.04f;

        comb_init(&piano->resonators[i], freq, sample_rate, feedback);

        // Explicitly reset comb filter state for safety
        comb_reset(&piano->resonators[i]);

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
        comb_reset(&piano->resonators[i]);
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
            float resonator_out = comb_process(&piano->resonators[i], sample);
            resonant_sample += resonator_out * piano->resonator_gains[i];
        }

        // Normalize resonator sum and scale down heavily to avoid distortion
        // Comb filters build up amplitude, so we need very low scaling
        resonant_sample *= 0.05f;

        // ADD to dry signal (preserves volume)
        sample = sample + resonant_sample * piano->resonance_amount;
    }

    // Apply filter envelope (one-pole low-pass with time-varying cutoff)
    // Map envelope to cutoff: 0.2 (dark) to 1.0 (bright)
    float brightness = piano->filter_envelope * piano->filter_peak;
    float cutoff = 0.2f + brightness * 0.8f;

    sample = piano->filter_prev_sample + cutoff * (sample - piano->filter_prev_sample);
    piano->filter_prev_sample = sample;

    return sample;
}
