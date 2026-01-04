#include "synth_sample_player.h"
#include <stdlib.h>
#include <math.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

struct SynthSamplePlayer {
    const SampleData* sample;

    // Playback state
    float playback_position;      // Current position in samples (fractional)
    float playback_rate;          // Playback rate multiplier (1.0 = original pitch)
    bool in_attack;               // True if playing attack, false if in loop
    bool is_active;               // True if currently playing
    bool is_released;             // True if note has been released

    // Envelope for loop decay
    float loop_decay_time;        // Decay time in seconds
    float loop_amplitude;         // Current amplitude during loop (0.0-1.0)
    float velocity_scale;         // Velocity scaling (0.0-1.0)

    // LFO (Low Frequency Oscillator) for tremolo effect
    float lfo_rate;               // LFO frequency in Hz
    float lfo_depth;              // LFO modulation depth (0.0-1.0)
    float lfo_phase;              // Current LFO phase (0 to 2*PI)
};

SynthSamplePlayer* synth_sample_player_create(void) {
    SynthSamplePlayer* player = (SynthSamplePlayer*)malloc(sizeof(SynthSamplePlayer));
    if (!player) return NULL;

    memset(player, 0, sizeof(SynthSamplePlayer));
    player->loop_decay_time = 2.0f;  // Default 2 second decay
    player->loop_amplitude = 1.0f;

    return player;
}

void synth_sample_player_destroy(SynthSamplePlayer* player) {
    if (player) {
        free(player);
    }
}

void synth_sample_player_load_sample(SynthSamplePlayer* player, const SampleData* sample) {
    if (!player) return;
    player->sample = sample;
}

void synth_sample_player_trigger(SynthSamplePlayer* player, uint8_t note, uint8_t velocity) {
    if (!player || !player->sample) return;

    // Calculate playback rate based on pitch difference
    // playback_rate = 2^((note - root_note) / 12)
    float pitch_diff = (float)(note - player->sample->root_note);
    player->playback_rate = powf(2.0f, pitch_diff / 12.0f);

    // Also account for sample rate difference
    // If sample is 22050 and output is 48000, we consume samples slower
    // to maintain original pitch: sample_rate / output_rate
    player->playback_rate *= (float)player->sample->sample_rate / 48000.0f;

    // Initialize playback state
    player->playback_position = 0.0f;
    player->in_attack = true;
    player->is_active = true;
    player->is_released = false;
    player->loop_amplitude = 1.0f;
    player->velocity_scale = velocity / 127.0f;

    // Reset LFO phase for each note
    player->lfo_phase = 0.0f;
}

void synth_sample_player_release(SynthSamplePlayer* player) {
    if (!player) return;
    player->is_released = true;
}

void synth_sample_player_set_loop_decay(SynthSamplePlayer* player, float decay_time) {
    if (!player) return;
    player->loop_decay_time = decay_time;
}

void synth_sample_player_set_lfo(SynthSamplePlayer* player, float rate, float depth) {
    if (!player) return;
    player->lfo_rate = rate;
    player->lfo_depth = depth;
}

bool synth_sample_player_is_active(const SynthSamplePlayer* player) {
    return player ? player->is_active : false;
}

void synth_sample_player_reset(SynthSamplePlayer* player) {
    if (!player) return;

    player->playback_position = 0.0f;
    player->is_active = false;
    player->is_released = false;
    player->in_attack = true;
    player->loop_amplitude = 1.0f;
}

// Linear interpolation helper
static inline float lerp(float a, float b, float t) {
    return a + (b - a) * t;
}

// Get interpolated sample from buffer
static float get_sample_interpolated(const int16_t* data, uint32_t length, float position) {
    if (!data || length == 0) return 0.0f;

    uint32_t index = (uint32_t)position;
    float frac = position - (float)index;

    if (index >= length - 1) {
        // At or past end, wrap or return last sample
        if (length > 0) {
            return (float)data[length - 1] / 32768.0f;
        }
        return 0.0f;
    }

    // Linear interpolation between current and next sample
    float sample1 = (float)data[index] / 32768.0f;
    float sample2 = (float)data[index + 1] / 32768.0f;

    return lerp(sample1, sample2, frac);
}

float synth_sample_player_process(SynthSamplePlayer* player, uint32_t output_sample_rate) {
    if (!player || !player->is_active || !player->sample) {
        return 0.0f;
    }

    float output = 0.0f;

    if (player->in_attack) {
        // Playing attack portion
        if (!player->sample->attack_data || player->sample->attack_length == 0) {
            player->is_active = false;
            return 0.0f;
        }

        output = get_sample_interpolated(
            player->sample->attack_data,
            player->sample->attack_length,
            player->playback_position
        );

        // Advance playback position
        player->playback_position += player->playback_rate;

        // Check if we've reached the end of attack
        if (player->playback_position >= (float)player->sample->attack_length) {
            if (player->sample->loop_data && player->sample->loop_length > 0 && !player->is_released) {
                // Transition to loop
                player->in_attack = false;
                player->playback_position = 0.0f;
                player->loop_amplitude = 1.0f;
            } else {
                // No loop or already released, stop playback
                player->is_active = false;
                return output * player->velocity_scale;
            }
        }

    } else {
        // Playing loop portion
        if (!player->sample->loop_data || player->sample->loop_length == 0) {
            player->is_active = false;
            return 0.0f;
        }

        output = get_sample_interpolated(
            player->sample->loop_data,
            player->sample->loop_length,
            player->playback_position
        );

        // Apply LFO modulation (tremolo effect) if enabled
        if (player->lfo_depth > 0.0f && player->lfo_rate > 0.0f) {
            // Update LFO phase
            player->lfo_phase += (2.0f * M_PI * player->lfo_rate) / output_sample_rate;
            if (player->lfo_phase >= 2.0f * M_PI) {
                player->lfo_phase -= 2.0f * M_PI;
            }

            // Calculate LFO modulation
            float lfo_value = sinf(player->lfo_phase);
            // LFO creates tremolo: depth 0 = no effect, depth 1 = full tremolo
            // Scale depth to prevent complete silence
            float lfo_mod = 1.0f - (player->lfo_depth * 0.3f * (1.0f - lfo_value));
            output *= lfo_mod;
        }

        // Apply decay envelope
        output *= player->loop_amplitude;

        // Advance playback position
        player->playback_position += player->playback_rate;

        // Wrap loop position
        if (player->playback_position >= (float)player->sample->loop_length) {
            player->playback_position = fmodf(player->playback_position, (float)player->sample->loop_length);
        }

        // Update decay envelope
        if (player->loop_decay_time > 0.0f) {
            float decay_rate = 1.0f / (player->loop_decay_time * output_sample_rate);
            player->loop_amplitude -= decay_rate;

            if (player->loop_amplitude <= 0.0f) {
                player->loop_amplitude = 0.0f;
                player->is_active = false;
            }
        }

        // If released, speed up decay
        if (player->is_released) {
            float release_rate = 1.0f / (0.1f * output_sample_rate);  // 100ms release
            player->loop_amplitude -= release_rate;

            if (player->loop_amplitude <= 0.0f) {
                player->loop_amplitude = 0.0f;
                player->is_active = false;
            }
        }
    }

    return output * player->velocity_scale;
}
