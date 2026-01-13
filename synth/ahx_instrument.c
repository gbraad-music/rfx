/*
 * AHX Instrument Synth - Implementation
 *
 * Extracts synthesis logic from AHX player for standalone instrument use.
 */

#include "ahx_instrument.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

// AHX timing: 50Hz frame rate (PAL)
#define AHX_FRAME_RATE 50

// Waveform generation (extracted from ahx_player.c)
static void generate_triangle(int16_t* buffer, int len) {
    for (int i = 0; i < len; i++) {
        int phase = (i * 256) / len;
        if (phase < 128) {
            buffer[i] = (phase * 256) - 16384;
        } else {
            buffer[i] = 16384 - ((phase - 128) * 256);
        }
    }
}

static void generate_sawtooth(int16_t* buffer, int len) {
    for (int i = 0; i < len; i++) {
        int phase = (i * 256) / len;
        buffer[i] = (phase * 128) - 16384;
    }
}

static void generate_square(int16_t* buffer, int len, int width) {
    int threshold = (len * width) / 255;
    for (int i = 0; i < len; i++) {
        buffer[i] = (i < threshold) ? 16383 : -16384;
    }
}

static void generate_noise(int16_t* buffer, int len) {
    uint32_t seed = 0x12345678;
    for (int i = 0; i < len; i++) {
        seed = (seed * 1103515245 + 12345) & 0x7FFFFFFF;
        buffer[i] = ((int16_t)(seed >> 16)) ;
    }
}

// Apply filter to waveform
static void apply_filter(int16_t* buffer, int len, int cutoff) {
    if (cutoff >= 63) return;  // No filtering

    float rc = 1.0f / (2.0f * 3.14159f * (63 - cutoff) * 100.0f);
    float dt = 1.0f / 3546895.0f;  // Paula clock
    float alpha = dt / (rc + dt);

    float prev = 0.0f;
    for (int i = 0; i < len; i++) {
        prev = prev + alpha * ((float)buffer[i] - prev);
        buffer[i] = (int16_t)prev;
    }
}

// Initialize instrument
void ahx_instrument_init(AhxInstrument* inst) {
    if (!inst) return;

    memset(inst, 0, sizeof(AhxInstrument));

    // Initialize tracker components
    tracker_voice_init(&inst->voice);
    tracker_modulator_init(&inst->filter_mod);
    tracker_modulator_init(&inst->square_mod);

    // Set default parameters
    inst->params = ahx_instrument_default_params();
    inst->env_state = ENV_OFF;
}

// Set parameters
void ahx_instrument_set_params(AhxInstrument* inst, const AhxInstrumentParams* params) {
    if (!inst || !params) return;

    inst->params = *params;

    // Configure modulators
    if (params->filter_enabled) {
        tracker_modulator_set_limits(&inst->filter_mod, params->filter_lower, params->filter_upper);
        tracker_modulator_set_speed(&inst->filter_mod, params->filter_speed);
        tracker_modulator_set_active(&inst->filter_mod, true);
    } else {
        tracker_modulator_set_active(&inst->filter_mod, false);
    }

    if (params->square_enabled) {
        tracker_modulator_set_limits(&inst->square_mod, params->square_lower, params->square_upper);
        tracker_modulator_set_speed(&inst->square_mod, params->square_speed);
        tracker_modulator_set_active(&inst->square_mod, true);
    } else {
        tracker_modulator_set_active(&inst->square_mod, false);
    }
}

// Get parameters
void ahx_instrument_get_params(const AhxInstrument* inst, AhxInstrumentParams* params) {
    if (!inst || !params) return;
    *params = inst->params;
}

// Note on
void ahx_instrument_note_on(AhxInstrument* inst, uint8_t note, uint8_t velocity, uint32_t sample_rate) {
    if (!inst) return;

    inst->note = note;
    inst->velocity = velocity;
    inst->active = true;
    inst->released = false;

    // Reset envelope
    inst->env_state = ENV_ATTACK;
    inst->env_frames = 0;
    inst->env_volume = 0;

    // Reset vibrato
    inst->vibrato_frames = 0;
    inst->vibrato_phase = 0.0f;

    // Reset modulators
    tracker_modulator_init(&inst->filter_mod);
    tracker_modulator_init(&inst->square_mod);
    ahx_instrument_set_params(inst, &inst->params);

    // Calculate samples per frame (for 50Hz AHX timing)
    inst->samples_per_frame = sample_rate / AHX_FRAME_RATE;
    inst->frame_counter = 0;

    // Generate initial waveform
    int wave_len = 4 << inst->params.wave_length;  // 4, 8, 16, ..., 512

    switch (inst->params.waveform) {
        case AHX_WAVE_TRIANGLE:
            generate_triangle(inst->waveform_buffer, wave_len);
            break;
        case AHX_WAVE_SAWTOOTH:
            generate_sawtooth(inst->waveform_buffer, wave_len);
            break;
        case AHX_WAVE_SQUARE:
            generate_square(inst->waveform_buffer, wave_len, 128);
            break;
        case AHX_WAVE_NOISE:
            generate_noise(inst->waveform_buffer, wave_len);
            break;
    }

    // Apply initial filter if enabled
    if (inst->params.filter_enabled) {
        int cutoff = tracker_modulator_get_position(&inst->filter_mod);
        apply_filter(inst->waveform_buffer, wave_len, cutoff);
    }

    // Set waveform in voice
    tracker_voice_set_waveform_16bit(&inst->voice, inst->waveform_buffer, wave_len);
    tracker_voice_set_loop(&inst->voice, 0, wave_len * 2, 2);  // Loop full waveform

    // Set note frequency
    // AHX uses period-based playback
    uint32_t period = 3546895 / (uint32_t)(440.0f * powf(2.0f, (note - 69) / 12.0f));
    tracker_voice_set_period(&inst->voice, period, 3546895, sample_rate);

    // Set volume based on velocity and instrument volume
    int volume = (inst->params.volume * velocity) / 127;
    tracker_voice_set_volume(&inst->voice, volume);

    // Reset position
    tracker_voice_reset_position(&inst->voice);
}

// Note off
void ahx_instrument_note_off(AhxInstrument* inst) {
    if (!inst) return;

    inst->released = true;

    if (inst->params.hard_cut_release) {
        // Hard cut: short fade out
        inst->env_state = ENV_RELEASE;
        inst->env_frames = 0;
    } else {
        // Normal release
        if (inst->env_state != ENV_RELEASE && inst->env_state != ENV_OFF) {
            inst->env_state = ENV_RELEASE;
            inst->env_frames = 0;
        }
    }
}

// Process one frame (50Hz)
void ahx_instrument_process_frame(AhxInstrument* inst) {
    if (!inst || !inst->active) return;

    // Update envelope
    const AhxEnvelope* env = &inst->params.envelope;

    switch (inst->env_state) {
        case ENV_ATTACK:
            if (inst->env_frames >= env->attack_frames) {
                inst->env_state = ENV_DECAY;
                inst->env_frames = 0;
                inst->env_volume = env->attack_volume;
            } else {
                // Linear interpolation
                inst->env_volume = (env->attack_volume * inst->env_frames) / (env->attack_frames ? env->attack_frames : 1);
                inst->env_frames++;
            }
            break;

        case ENV_DECAY:
            if (inst->env_frames >= env->decay_frames) {
                inst->env_state = ENV_SUSTAIN;
                inst->env_frames = 0;
                inst->env_volume = env->decay_volume;
            } else {
                // Linear interpolation from attack to decay volume
                int range = env->attack_volume - env->decay_volume;
                inst->env_volume = env->attack_volume - (range * inst->env_frames) / (env->decay_frames ? env->decay_frames : 1);
                inst->env_frames++;
            }
            break;

        case ENV_SUSTAIN:
            inst->env_volume = env->decay_volume;
            if (env->sustain_frames > 0) {
                inst->env_frames++;
                if (inst->env_frames >= env->sustain_frames) {
                    // Sustain timeout - go to release
                    inst->env_state = ENV_RELEASE;
                    inst->env_frames = 0;
                }
            }
            break;

        case ENV_RELEASE:
            if (inst->params.hard_cut_release) {
                // Hard cut: quick fade
                uint32_t cut_frames = inst->params.hard_cut_frames ? inst->params.hard_cut_frames : 1;
                if (inst->env_frames >= cut_frames) {
                    inst->env_state = ENV_OFF;
                    inst->active = false;
                    inst->env_volume = 0;
                } else {
                    inst->env_volume = (env->decay_volume * (cut_frames - inst->env_frames)) / cut_frames;
                    inst->env_frames++;
                }
            } else {
                // Normal release
                if (inst->env_frames >= env->release_frames) {
                    inst->env_state = ENV_OFF;
                    inst->active = false;
                    inst->env_volume = 0;
                } else {
                    inst->env_volume = env->decay_volume - ((env->decay_volume - env->release_volume) * inst->env_frames) / (env->release_frames ? env->release_frames : 1);
                    inst->env_frames++;
                }
            }
            break;

        case ENV_OFF:
            inst->active = false;
            inst->env_volume = 0;
            break;
    }

    // Update modulators
    if (tracker_modulator_is_active(&inst->filter_mod)) {
        tracker_modulator_update(&inst->filter_mod);
    }

    if (tracker_modulator_is_active(&inst->square_mod)) {
        tracker_modulator_update(&inst->square_mod);

        // Regenerate square wave with new width
        if (inst->params.waveform == AHX_WAVE_SQUARE) {
            int wave_len = 4 << inst->params.wave_length;
            int width = tracker_modulator_get_position(&inst->square_mod);
            generate_square(inst->waveform_buffer, wave_len, width);
            tracker_voice_set_waveform_16bit(&inst->voice, inst->waveform_buffer, wave_len);
        }
    }

    // Update vibrato
    inst->vibrato_frames++;
}

// Process samples
uint32_t ahx_instrument_process(AhxInstrument* inst, float* output, uint32_t num_samples, uint32_t sample_rate) {
    (void)sample_rate;  // Reserved for future use

    if (!inst || !output || !inst->active) {
        if (output) {
            memset(output, 0, num_samples * sizeof(float));
        }
        return 0;
    }

    for (uint32_t i = 0; i < num_samples; i++) {
        // Process frames at 50Hz
        inst->frame_counter++;
        if (inst->frame_counter >= inst->samples_per_frame) {
            inst->frame_counter = 0;
            ahx_instrument_process_frame(inst);
        }

        // Get sample from voice
        int32_t left, right;
        tracker_voice_get_stereo_sample(&inst->voice, &left, &right);

        // Apply envelope
        int32_t sample = (left * inst->env_volume) / 64;

        // Convert to float (-1.0 to 1.0)
        output[i] = (float)sample / 32768.0f;

        // Check if still active
        if (!inst->active) {
            // Fill remaining with silence
            for (uint32_t j = i + 1; j < num_samples; j++) {
                output[j] = 0.0f;
            }
            return i + 1;
        }
    }

    return num_samples;
}

// Check if active
bool ahx_instrument_is_active(const AhxInstrument* inst) {
    return inst && inst->active;
}

// Reset
void ahx_instrument_reset(AhxInstrument* inst) {
    if (!inst) return;

    inst->active = false;
    inst->released = false;
    inst->env_state = ENV_OFF;
    inst->env_frames = 0;
    inst->env_volume = 0;
    inst->vibrato_frames = 0;
    inst->vibrato_phase = 0.0f;
    inst->frame_counter = 0;

    tracker_voice_init(&inst->voice);
    tracker_modulator_init(&inst->filter_mod);
    tracker_modulator_init(&inst->square_mod);
}

// Default parameters
AhxInstrumentParams ahx_instrument_default_params(void) {
    AhxInstrumentParams params = {0};

    // Oscillator
    params.waveform = AHX_WAVE_SAWTOOTH;
    params.wave_length = 3;  // 32 samples
    params.volume = 64;

    // Envelope (classic ADSR)
    params.envelope.attack_frames = 1;
    params.envelope.attack_volume = 64;
    params.envelope.decay_frames = 10;
    params.envelope.decay_volume = 48;
    params.envelope.sustain_frames = 0;  // Infinite
    params.envelope.release_frames = 20;
    params.envelope.release_volume = 0;

    // Filter (disabled by default)
    params.filter_lower = 0;
    params.filter_upper = 63;
    params.filter_speed = 4;
    params.filter_enabled = false;

    // Square modulation (disabled by default)
    params.square_lower = 64;
    params.square_upper = 192;
    params.square_speed = 4;
    params.square_enabled = false;

    // Vibrato (disabled by default)
    params.vibrato_delay = 0;
    params.vibrato_depth = 0;
    params.vibrato_speed = 0;

    // Release
    params.hard_cut_release = false;
    params.hard_cut_frames = 2;

    // No PList by default
    params.plist = NULL;

    return params;
}
