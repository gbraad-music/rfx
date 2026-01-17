/*
 * SID (MOS 6581/8580) Synthesizer Emulation
 * Simplified but authentic Commodore 64 sound chip emulation
 */

#include "synth_sid.h"
#include "synth_lfo.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define SID_VOICES 3

// ADSR envelope rates (exponential curves like real SID)
static const float ATTACK_RATES[16] = {
    0.002f, 0.008f, 0.016f, 0.024f, 0.038f, 0.056f, 0.068f, 0.080f,
    0.100f, 0.250f, 0.500f, 0.800f, 1.000f, 3.000f, 5.000f, 8.000f
};

static const float DECAY_RELEASE_RATES[16] = {
    0.006f, 0.024f, 0.048f, 0.072f, 0.114f, 0.168f, 0.204f, 0.240f,
    0.300f, 0.750f, 1.500f, 2.400f, 3.000f, 9.000f, 15.000f, 24.000f
};

typedef enum {
    ENV_ATTACK,
    ENV_DECAY,
    ENV_SUSTAIN,
    ENV_RELEASE,
    ENV_OFF
} EnvelopeState;

typedef struct {
    // Oscillator
    uint32_t phase;        // 24-bit phase accumulator (like real SID)
    uint32_t frequency;    // Frequency register value
    uint8_t waveform;      // Waveform select
    float pulse_width;     // Pulse width (0.0-1.0)

    // Envelope
    EnvelopeState env_state;
    float env_level;
    float attack;          // Normalized 0.0-1.0
    float decay;
    float sustain;
    float release;

    // Modulation
    int ring_mod;          // Ring modulation enable
    int sync;              // Hard sync enable
    int gate;              // Gate on/off
    int test;              // TEST bit (resets oscillator)
    float pitch_bend;      // Pitch bend amount (-1.0 to +1.0)

    // LFSR for noise (25-bit like real SID)
    uint32_t noise_lfsr;

    // Current note
    uint8_t note;
    uint8_t velocity;
} SIDVoice;

typedef struct {
    // Filter
    SIDFilterMode filter_mode;
    float filter_cutoff;      // 0.0-1.0
    float filter_resonance;   // 0.0-1.0
    int filter_voice[SID_VOICES]; // Which voices go through filter

    // Filter state (biquad - separate input/output history)
    float x1, x2;  // Input history
    float y1, y2;  // Output history
} SIDFilter;

struct SynthSID {
    SIDVoice voices[SID_VOICES];
    SIDFilter filter;
    float volume;
    int sample_rate;
    uint8_t registers[32];  // Shadow copy of hardware registers ($D400-$D41F)

    // LFO subsystem
    SynthLFO* lfo1;                  // Pitch modulation
    SynthLFO* lfo2;                  // Filter + PWM modulation
    float lfo1_to_pitch_depth;       // 0.0-1.0
    float lfo2_to_filter_depth;      // 0.0-1.0
    float lfo2_to_pw_depth;          // 0.0-1.0
    float mod_wheel_amount;          // CC 1 modulation wheel
};

// ============================================================================
// Helper Functions
// ============================================================================

static inline float note_to_frequency(uint8_t note) {
    return 440.0f * powf(2.0f, (note - 69) / 12.0f);
}

// Convert frequency to SID register value (PAL C64: Clock = 985248 Hz)
static inline uint32_t frequency_to_sid_register(float freq, int sample_rate) {
    // SID formula: Register = Frequency * 16777216 / Clock
    // Simplified for our sample rate
    return (uint32_t)(freq * 16777216.0f / sample_rate);
}

// Generate waveforms
static float generate_triangle(uint32_t phase) {
    // Triangle wave: linear ramp up then down
    if (phase < 0x800000) {
        return (phase / (float)0x800000) * 2.0f - 1.0f;
    } else {
        return 1.0f - ((phase - 0x800000) / (float)0x800000) * 2.0f;
    }
}

static float generate_sawtooth(uint32_t phase) {
    return (phase / (float)0x1000000) * 2.0f - 1.0f;
}

static float generate_pulse(uint32_t phase, float pulse_width) {
    // Clamp pulse width to prevent DC at extremes (0.5% - 99.5% duty cycle)
    // Real SID also produces no sound at 0% or 100%, but this is more musical
    if (pulse_width < 0.005f) pulse_width = 0.005f;
    if (pulse_width > 0.995f) pulse_width = 0.995f;

    uint32_t threshold = (uint32_t)(pulse_width * 0x1000000);
    return (phase < threshold) ? 1.0f : -1.0f;
}

static float generate_noise(SIDVoice* voice) {
    // 25-bit LFSR (like real SID 6581)
    // Taps at bits 22 and 17
    uint32_t bit0 = ((voice->noise_lfsr >> 22) ^ (voice->noise_lfsr >> 17)) & 1;
    voice->noise_lfsr = ((voice->noise_lfsr << 1) | bit0) & 0x1FFFFFF;

    // Use top 8 bits for output
    return ((voice->noise_lfsr >> 17) / 128.0f) - 1.0f;
}

// Process envelope
static void update_envelope(SIDVoice* voice, float delta_time) {
    switch (voice->env_state) {
        case ENV_ATTACK: {
            float attack_time = ATTACK_RATES[(int)(voice->attack * 15)];
            voice->env_level += delta_time / attack_time;
            if (voice->env_level >= 1.0f) {
                voice->env_level = 1.0f;
                voice->env_state = ENV_DECAY;
            }
            break;
        }

        case ENV_DECAY: {
            float decay_time = DECAY_RELEASE_RATES[(int)(voice->decay * 15)];
            voice->env_level -= delta_time / decay_time;
            if (voice->env_level <= voice->sustain) {
                voice->env_level = voice->sustain;
                voice->env_state = ENV_SUSTAIN;
            }
            break;
        }

        case ENV_SUSTAIN:
            // Hold at sustain level
            voice->env_level = voice->sustain;
            break;

        case ENV_RELEASE: {
            float release_time = DECAY_RELEASE_RATES[(int)(voice->release * 15)];
            voice->env_level -= delta_time / release_time;
            if (voice->env_level <= 0.0f) {
                voice->env_level = 0.0f;
                voice->env_state = ENV_OFF;
            }
            break;
        }

        case ENV_OFF:
            voice->env_level = 0.0f;
            break;
    }
}

// Simple biquad filter processing
static float process_filter(SIDFilter* filter, float input) {
    if (filter->filter_mode == SID_FILTER_OFF) {
        return input;
    }

    // Convert normalized cutoff (0-1) to frequency (30Hz - 12kHz like real SID)
    float cutoff_freq = 30.0f + filter->filter_cutoff * 11970.0f;
    float sample_rate = 44100.0f; // Assume standard rate
    float omega = 2.0f * M_PI * cutoff_freq / sample_rate;
    float cos_omega = cosf(omega);
    float sin_omega = sinf(omega);

    // Resonance (0-1) mapped to Q (0.5 - 10)
    float Q = 0.5f + filter->filter_resonance * 9.5f;
    float alpha = sin_omega / (2.0f * Q);

    // Biquad coefficients
    float b0, b1, b2, a0, a1, a2;

    switch (filter->filter_mode) {
        case SID_FILTER_LP:
            // Low pass
            b0 = (1.0f - cos_omega) / 2.0f;
            b1 = 1.0f - cos_omega;
            b2 = (1.0f - cos_omega) / 2.0f;
            a0 = 1.0f + alpha;
            a1 = -2.0f * cos_omega;
            a2 = 1.0f - alpha;
            break;

        case SID_FILTER_BP:
            // Band pass
            b0 = alpha;
            b1 = 0.0f;
            b2 = -alpha;
            a0 = 1.0f + alpha;
            a1 = -2.0f * cos_omega;
            a2 = 1.0f - alpha;
            break;

        case SID_FILTER_HP:
            // High pass
            b0 = (1.0f + cos_omega) / 2.0f;
            b1 = -(1.0f + cos_omega);
            b2 = (1.0f + cos_omega) / 2.0f;
            a0 = 1.0f + alpha;
            a1 = -2.0f * cos_omega;
            a2 = 1.0f - alpha;
            break;

        default:
            return input;
    }

    // Normalize coefficients
    b0 /= a0;
    b1 /= a0;
    b2 /= a0;
    a1 /= a0;
    a2 /= a0;

    // Apply biquad filter (Direct Form II Transposed)
    // y[n] = b0*x[n] + b1*x[n-1] + b2*x[n-2] - a1*y[n-1] - a2*y[n-2]
    float output = b0 * input + b1 * filter->x1 + b2 * filter->x2
                   - a1 * filter->y1 - a2 * filter->y2;

    // Update state (input and output history)
    filter->x2 = filter->x1;
    filter->x1 = input;
    filter->y2 = filter->y1;
    filter->y1 = output;

    return output;
}

// ============================================================================
// Lifecycle
// ============================================================================

SynthSID* synth_sid_create(int sample_rate) {
    SynthSID* sid = (SynthSID*)malloc(sizeof(SynthSID));
    if (!sid) return NULL;

    memset(sid, 0, sizeof(SynthSID));
    sid->sample_rate = sample_rate;
    sid->volume = 0.7f;

    // Initialize voices
    for (int i = 0; i < SID_VOICES; i++) {
        sid->voices[i].waveform = SID_WAVE_PULSE;
        sid->voices[i].pulse_width = 0.5f;
        sid->voices[i].attack = 0.0f;
        sid->voices[i].decay = 0.5f;
        sid->voices[i].sustain = 0.7f;
        sid->voices[i].release = 0.3f;
        sid->voices[i].env_state = ENV_OFF;
        sid->voices[i].noise_lfsr = 0x1FFFFFF; // Initialize LFSR
        sid->voices[i].pitch_bend = 0.0f; // No bend initially
        sid->voices[i].test = 0; // TEST bit off
    }

    // Initialize filter
    sid->filter.filter_mode = SID_FILTER_LP;
    sid->filter.filter_cutoff = 0.5f;
    sid->filter.filter_resonance = 0.0f;

    // Initialize LFOs
    sid->lfo1 = synth_lfo_create();
    sid->lfo2 = synth_lfo_create();

    if (!sid->lfo1 || !sid->lfo2) {
        synth_sid_destroy(sid);
        return NULL;
    }

    // Configure LFO defaults
    synth_lfo_set_frequency(sid->lfo1, 5.0f);         // 5 Hz vibrato
    synth_lfo_set_waveform(sid->lfo1, SYNTH_LFO_SINE);
    synth_lfo_set_frequency(sid->lfo2, 0.5f);         // 0.5 Hz filter sweep
    synth_lfo_set_waveform(sid->lfo2, SYNTH_LFO_TRIANGLE);

    // Initialize modulation depths to 0 (off by default)
    sid->lfo1_to_pitch_depth = 0.0f;
    sid->lfo2_to_filter_depth = 0.0f;
    sid->lfo2_to_pw_depth = 0.0f;
    sid->mod_wheel_amount = 0.0f;

    return sid;
}

void synth_sid_destroy(SynthSID* sid) {
    if (sid) {
        if (sid->lfo1) synth_lfo_destroy(sid->lfo1);
        if (sid->lfo2) synth_lfo_destroy(sid->lfo2);
        free(sid);
    }
}

void synth_sid_reset(SynthSID* sid) {
    if (!sid) return;

    for (int i = 0; i < SID_VOICES; i++) {
        sid->voices[i].phase = 0;
        sid->voices[i].gate = 0;
        sid->voices[i].env_state = ENV_OFF;
        sid->voices[i].env_level = 0.0f;
    }

    sid->filter.x1 = 0.0f;
    sid->filter.x2 = 0.0f;
    sid->filter.y1 = 0.0f;
    sid->filter.y2 = 0.0f;

    if (sid->lfo1) synth_lfo_reset(sid->lfo1);
    if (sid->lfo2) synth_lfo_reset(sid->lfo2);
}

// ============================================================================
// MIDI Event Handling
// ============================================================================

void synth_sid_note_on(SynthSID* sid, uint8_t voice, uint8_t note, uint8_t velocity) {
    if (!sid || voice >= SID_VOICES) return;

    SIDVoice* v = &sid->voices[voice];

    v->note = note;
    v->velocity = velocity;
    v->frequency = frequency_to_sid_register(note_to_frequency(note), sid->sample_rate);
    v->gate = 1;
    v->env_state = ENV_ATTACK;
    v->env_level = 0.0f;  // Reset envelope to start attack from zero
}

void synth_sid_note_off(SynthSID* sid, uint8_t voice) {
    if (!sid || voice >= SID_VOICES) return;

    sid->voices[voice].gate = 0;
    sid->voices[voice].env_state = ENV_RELEASE;
}

void synth_sid_all_notes_off(SynthSID* sid) {
    if (!sid) return;

    for (int i = 0; i < SID_VOICES; i++) {
        synth_sid_note_off(sid, i);
    }
}

// ============================================================================
// Audio Processing
// ============================================================================

void synth_sid_process_f32(SynthSID* sid, float* buffer, int frames, int sample_rate) {
    if (!sid || !buffer) return;

    float delta_time = 1.0f / sample_rate;

    for (int frame = 0; frame < frames; frame++) {
        // Process LFOs once per sample
        float lfo1_value = sid->lfo1 ? synth_lfo_process(sid->lfo1, sample_rate) : 0.0f;
        float lfo2_value = sid->lfo2 ? synth_lfo_process(sid->lfo2, sample_rate) : 0.0f;

        // Calculate modulation amounts
        float pitch_mod = sid->lfo1_to_pitch_depth * sid->mod_wheel_amount * lfo1_value;
        float filter_mod = sid->lfo2_to_filter_depth * lfo2_value * 0.3f;
        float pw_mod = sid->lfo2_to_pw_depth * lfo2_value * 0.3f;

        // Apply filter modulation (global)
        float modulated_cutoff = sid->filter.filter_cutoff + filter_mod;
        modulated_cutoff = fminf(fmaxf(modulated_cutoff, 0.0f), 1.0f);
        float original_cutoff = sid->filter.filter_cutoff;
        sid->filter.filter_cutoff = modulated_cutoff;

        float filtered_mix = 0.0f;
        float unfiltered_mix = 0.0f;

        // Process each voice
        for (int v = 0; v < SID_VOICES; v++) {
            SIDVoice* voice = &sid->voices[v];

            // TEST bit: reset oscillator and silence voice
            if (voice->test) {
                voice->phase = 0;
                voice->noise_lfsr = 0x1FFFFFF; // Reset LFSR to initial state
                continue; // Skip processing (voice is silenced)
            }

            // Update envelope
            update_envelope(voice, delta_time);

            if (voice->env_level <= 0.0f) {
                continue; // Silent voice
            }

            // Advance phase with pitch bend + LFO pitch modulation applied
            // Pitch bend: -1.0 to +1.0 maps to ±12 semitones (1 octave)
            // LFO pitch: pitch_mod in semitones (scaled by depth and mod wheel)
            float pitch_multiplier = powf(2.0f, voice->pitch_bend) * powf(2.0f, pitch_mod / 12.0f);
            uint32_t bent_frequency = (uint32_t)(voice->frequency * pitch_multiplier);
            voice->phase += bent_frequency;
            voice->phase &= 0xFFFFFF; // 24-bit wraparound

            // Apply hard sync (voice 0->1, 1->2, 2->0)
            if (voice->sync) {
                int target = (v + 1) % SID_VOICES;
                if (voice->phase < voice->frequency) { // Wrapped
                    sid->voices[target].phase = 0;
                }
            }

            // Generate waveform
            float sample = 0.0f;
            uint32_t phase_to_use = voice->phase;

            // Ring modulation (circular chain like real SID)
            // Voice 1 modded by Voice 3, Voice 2 by Voice 1, Voice 3 by Voice 2
            if (voice->ring_mod) {
                int src_voice = (v + 2) % SID_VOICES;  // Previous voice in circular chain
                uint32_t src_phase = sid->voices[src_voice].phase;
                // XOR phase with source MSB to create ring mod effect
                phase_to_use = voice->phase ^ (src_phase & 0x800000);
            }

            // Generate combined waveform (SID allows multiple waveforms simultaneously)
            if (voice->waveform & SID_WAVE_TRIANGLE) {
                sample += generate_triangle(phase_to_use);
            }
            if (voice->waveform & SID_WAVE_SAWTOOTH) {
                sample += generate_sawtooth(phase_to_use);
            }
            if (voice->waveform & SID_WAVE_PULSE) {
                // Apply LFO pulse width modulation
                float modulated_pw = voice->pulse_width + pw_mod;
                modulated_pw = fminf(fmaxf(modulated_pw, 0.005f), 0.995f);
                sample += generate_pulse(phase_to_use, modulated_pw);
            }
            if (voice->waveform & SID_WAVE_NOISE) {
                sample += generate_noise(voice);
            }

            // Normalize if multiple waveforms
            int waveform_count = 0;
            for (int w = 0; w < 4; w++) {
                if (voice->waveform & (1 << w)) waveform_count++;
            }
            if (waveform_count > 1) {
                sample /= waveform_count;
            }

            // Apply envelope and velocity
            sample *= voice->env_level * (voice->velocity / 127.0f);

            // Route to filter or direct mix based on voice routing
            if (sid->filter.filter_voice[v]) {
                filtered_mix += sample;
            } else {
                unfiltered_mix += sample;
            }
        }

        // Apply filter to filtered voices
        if (filtered_mix != 0.0f) {
            filtered_mix = process_filter(&sid->filter, filtered_mix);
        }

        // Combine filtered and unfiltered signals
        float mix = (filtered_mix + unfiltered_mix) * 0.33f;

        // Apply volume
        mix *= sid->volume;

        // Soft clipping
        if (mix > 1.0f) mix = 1.0f;
        if (mix < -1.0f) mix = -1.0f;

        // Stereo output (mono source)
        buffer[frame * 2] = mix;
        buffer[frame * 2 + 1] = mix;

        // Restore original cutoff (modulation is per-sample)
        sid->filter.filter_cutoff = original_cutoff;
    }
}

// ============================================================================
// Parameter Setters/Getters
// ============================================================================

void synth_sid_set_waveform(SynthSID* sid, uint8_t voice, uint8_t waveform) {
    if (!sid || voice >= SID_VOICES) return;
    sid->voices[voice].waveform = waveform;
}

uint8_t synth_sid_get_waveform(SynthSID* sid, uint8_t voice) {
    if (!sid || voice >= SID_VOICES) return 0;
    return sid->voices[voice].waveform;
}

void synth_sid_set_pulse_width(SynthSID* sid, uint8_t voice, float width) {
    if (!sid || voice >= SID_VOICES) return;
    sid->voices[voice].pulse_width = width;
}

float synth_sid_get_pulse_width(SynthSID* sid, uint8_t voice) {
    if (!sid || voice >= SID_VOICES) return 0.5f;
    return sid->voices[voice].pulse_width;
}

void synth_sid_set_attack(SynthSID* sid, uint8_t voice, float attack) {
    if (!sid || voice >= SID_VOICES) return;
    sid->voices[voice].attack = attack;
}

void synth_sid_set_decay(SynthSID* sid, uint8_t voice, float decay) {
    if (!sid || voice >= SID_VOICES) return;
    sid->voices[voice].decay = decay;
}

void synth_sid_set_sustain(SynthSID* sid, uint8_t voice, float sustain) {
    if (!sid || voice >= SID_VOICES) return;
    sid->voices[voice].sustain = sustain;
}

void synth_sid_set_release(SynthSID* sid, uint8_t voice, float release) {
    if (!sid || voice >= SID_VOICES) return;
    sid->voices[voice].release = release;
}

void synth_sid_set_ring_mod(SynthSID* sid, uint8_t voice, int enabled) {
    if (!sid || voice >= SID_VOICES) return;
    sid->voices[voice].ring_mod = enabled;
}

int synth_sid_get_ring_mod(SynthSID* sid, uint8_t voice) {
    if (!sid || voice >= SID_VOICES) return 0;
    return sid->voices[voice].ring_mod;
}

void synth_sid_set_sync(SynthSID* sid, uint8_t voice, int enabled) {
    if (!sid || voice >= SID_VOICES) return;
    sid->voices[voice].sync = enabled;
}

int synth_sid_get_sync(SynthSID* sid, uint8_t voice) {
    if (!sid || voice >= SID_VOICES) return 0;
    return sid->voices[voice].sync;
}

void synth_sid_set_filter_mode(SynthSID* sid, SIDFilterMode mode) {
    if (!sid) return;
    sid->filter.filter_mode = mode;
}

SIDFilterMode synth_sid_get_filter_mode(SynthSID* sid) {
    if (!sid) return SID_FILTER_OFF;
    return sid->filter.filter_mode;
}

void synth_sid_set_filter_cutoff(SynthSID* sid, float cutoff) {
    if (!sid) return;
    sid->filter.filter_cutoff = cutoff;
}

float synth_sid_get_filter_cutoff(SynthSID* sid) {
    if (!sid) return 0.5f;
    return sid->filter.filter_cutoff;
}

void synth_sid_set_filter_resonance(SynthSID* sid, float resonance) {
    if (!sid) return;
    sid->filter.filter_resonance = resonance;
}

float synth_sid_get_filter_resonance(SynthSID* sid) {
    if (!sid) return 0.0f;
    return sid->filter.filter_resonance;
}

void synth_sid_set_filter_voice(SynthSID* sid, uint8_t voice, int enabled) {
    if (!sid || voice >= SID_VOICES) return;
    sid->filter.filter_voice[voice] = enabled;
}

int synth_sid_get_filter_voice(SynthSID* sid, uint8_t voice) {
    if (!sid || voice >= SID_VOICES) return 0;
    return sid->filter.filter_voice[voice];
}

void synth_sid_set_volume(SynthSID* sid, float volume) {
    if (!sid) return;
    sid->volume = volume;
}

float synth_sid_get_volume(SynthSID* sid) {
    if (!sid) return 0.7f;
    return sid->volume;
}

void synth_sid_set_pitch_bend(SynthSID* sid, uint8_t voice, float bend) {
    if (!sid || voice >= SID_VOICES) return;
    // Clamp to ±1.0 (±12 semitones / 1 octave)
    if (bend < -1.0f) bend = -1.0f;
    if (bend > 1.0f) bend = 1.0f;
    sid->voices[voice].pitch_bend = bend;
}

float synth_sid_get_pitch_bend(SynthSID* sid, uint8_t voice) {
    if (!sid || voice >= SID_VOICES) return 0.0f;
    return sid->voices[voice].pitch_bend;
}

void synth_sid_set_test(SynthSID* sid, uint8_t voice, int enabled) {
    if (!sid || voice >= SID_VOICES) return;
    sid->voices[voice].test = enabled;
}

int synth_sid_get_test(SynthSID* sid, uint8_t voice) {
    if (!sid || voice >= SID_VOICES) return 0;
    return sid->voices[voice].test;
}

// ============================================================================
// Hardware Register Interface
// ============================================================================

void synth_sid_write_register(SynthSID* sid, uint8_t reg, uint8_t value) {
    if (!sid || reg > 0x18) return;  // Only registers 0x00-0x18 are valid

    // Update shadow register
    sid->registers[reg] = value;

    // Voice registers (each voice uses 7 registers)
    if (reg < 0x15) {
        uint8_t voice = reg / 7;  // Voice 0, 1, or 2
        uint8_t voice_reg = reg % 7;

        switch (voice_reg) {
            case 0: // Frequency Lo
                // Frequency is 16-bit: combine with Hi byte
                // Will be applied when Hi byte is written
                break;

            case 1: { // Frequency Hi
                // Combine with Lo byte and calculate frequency
                uint16_t freq_reg = (value << 8) | sid->registers[voice * 7 + 0];
                // SID frequency = freq_reg * clock / 16777216
                // For PAL: clock = 985248 Hz
                // freq_hz = freq_reg * 985248 / 16777216 = freq_reg * 0.0587
                float freq_hz = freq_reg * 0.0587f;

                // Update voice frequency register (will be used in process)
                sid->voices[voice].frequency = (uint32_t)((freq_hz * 16777216.0f) / sid->sample_rate);
                break;
            }

            case 2: // Pulse Width Lo
                // Will be combined when Hi byte is written
                break;

            case 3: { // Pulse Width Hi (bits 0-3 only, 12-bit value)
                uint16_t pw_reg = ((value & 0x0F) << 8) | sid->registers[voice * 7 + 2];
                // Pulse width: 0-4095 maps to 0.0-1.0
                float pw = pw_reg / 4095.0f;
                synth_sid_set_pulse_width(sid, voice, pw);
                break;
            }

            case 4: { // Control Register
                uint8_t waveform = 0;
                if (value & 0x10) waveform |= SID_WAVE_TRIANGLE;
                if (value & 0x20) waveform |= SID_WAVE_SAWTOOTH;
                if (value & 0x40) waveform |= SID_WAVE_PULSE;
                if (value & 0x80) waveform |= SID_WAVE_NOISE;

                synth_sid_set_waveform(sid, voice, waveform);
                synth_sid_set_test(sid, voice, (value & 0x08) ? 1 : 0);
                synth_sid_set_ring_mod(sid, voice, (value & 0x04) ? 1 : 0);
                synth_sid_set_sync(sid, voice, (value & 0x02) ? 1 : 0);

                // GATE bit
                int gate = (value & 0x01) ? 1 : 0;
                if (gate && !sid->voices[voice].gate) {
                    // Gate transitioned 0→1: trigger note
                    // Use last frequency to determine MIDI note (approximate)
                    uint16_t freq_reg = (sid->registers[voice * 7 + 1] << 8) |
                                       sid->registers[voice * 7 + 0];
                    float freq_hz = freq_reg * 0.0587f;
                    uint8_t note = (uint8_t)(69 + 12 * log2f(freq_hz / 440.0f));
                    synth_sid_note_on(sid, voice, note, 100);
                } else if (!gate && sid->voices[voice].gate) {
                    // Gate transitioned 1→0: release note
                    synth_sid_note_off(sid, voice);
                }
                sid->voices[voice].gate = gate;
                break;
            }

            case 5: { // Attack/Decay
                float attack = ((value >> 4) & 0x0F) / 15.0f;
                float decay = (value & 0x0F) / 15.0f;
                synth_sid_set_attack(sid, voice, attack);
                synth_sid_set_decay(sid, voice, decay);
                break;
            }

            case 6: { // Sustain/Release
                float sustain = ((value >> 4) & 0x0F) / 15.0f;
                float release = (value & 0x0F) / 15.0f;
                synth_sid_set_sustain(sid, voice, sustain);
                synth_sid_set_release(sid, voice, release);
                break;
            }
        }
    }
    // Filter registers
    else if (reg == 0x15) { // Filter Cutoff Lo
        // Will be combined when Hi byte is written
    }
    else if (reg == 0x16) { // Filter Cutoff Hi (bits 0-2 only, 11-bit value)
        uint16_t fc_reg = ((value & 0x07) << 8) | sid->registers[0x15];
        float cutoff = fc_reg / 2047.0f;  // 11-bit value
        synth_sid_set_filter_cutoff(sid, cutoff);
    }
    else if (reg == 0x17) { // Resonance + Filter Routing
        float resonance = ((value >> 4) & 0x0F) / 15.0f;
        synth_sid_set_filter_resonance(sid, resonance);

        // Filter voice routing (bits 0-2)
        synth_sid_set_filter_voice(sid, 0, (value & 0x01) ? 1 : 0);
        synth_sid_set_filter_voice(sid, 1, (value & 0x02) ? 1 : 0);
        synth_sid_set_filter_voice(sid, 2, (value & 0x04) ? 1 : 0);
    }
    else if (reg == 0x18) { // Filter Mode + Volume
        // Filter mode (bits 4-6)
        SIDFilterMode mode = SID_FILTER_OFF;
        if (value & 0x10) mode = SID_FILTER_LP;   // Low-pass
        if (value & 0x20) mode = SID_FILTER_BP;   // Band-pass
        if (value & 0x40) mode = SID_FILTER_HP;   // High-pass
        synth_sid_set_filter_mode(sid, mode);

        // Volume (bits 0-3)
        float volume = (value & 0x0F) / 15.0f;
        synth_sid_set_volume(sid, volume);
    }
    // Registers 0x19-0x1C are read-only (paddles, OSC3, ENV3) - ignored on write
}

uint8_t synth_sid_read_register(SynthSID* sid, uint8_t reg) {
    if (!sid || reg > 0x1F) return 0;

    // Most registers are write-only and return the written value
    if (reg < 0x19) {
        return sid->registers[reg];
    }

    // Read-only registers (0x19-0x1C) would return hardware state
    // Not implemented in this synth (paddles, OSC3, ENV3)
    return 0;
}

// ============================================================================
// LFO Parameters
// ============================================================================

void synth_sid_set_lfo_frequency(SynthSID* sid, int lfo_num, float freq_hz) {
    if (!sid) return;
    freq_hz = fminf(fmaxf(freq_hz, 0.01f), 20.0f);
    if (lfo_num == 0 && sid->lfo1) synth_lfo_set_frequency(sid->lfo1, freq_hz);
    else if (lfo_num == 1 && sid->lfo2) synth_lfo_set_frequency(sid->lfo2, freq_hz);
}

void synth_sid_set_lfo_waveform(SynthSID* sid, int lfo_num, int waveform) {
    if (!sid) return;
    if (lfo_num == 0 && sid->lfo1) synth_lfo_set_waveform(sid->lfo1, (SynthLFOWaveform)waveform);
    else if (lfo_num == 1 && sid->lfo2) synth_lfo_set_waveform(sid->lfo2, (SynthLFOWaveform)waveform);
}

void synth_sid_set_lfo1_to_pitch(SynthSID* sid, float depth) {
    if (sid) sid->lfo1_to_pitch_depth = fminf(fmaxf(depth, 0.0f), 1.0f);
}

float synth_sid_get_lfo1_to_pitch(SynthSID* sid) {
    return sid ? sid->lfo1_to_pitch_depth : 0.0f;
}

void synth_sid_set_lfo2_to_filter(SynthSID* sid, float depth) {
    if (sid) sid->lfo2_to_filter_depth = fminf(fmaxf(depth, 0.0f), 1.0f);
}

float synth_sid_get_lfo2_to_filter(SynthSID* sid) {
    return sid ? sid->lfo2_to_filter_depth : 0.0f;
}

void synth_sid_set_lfo2_to_pw(SynthSID* sid, float depth) {
    if (sid) sid->lfo2_to_pw_depth = fminf(fmaxf(depth, 0.0f), 1.0f);
}

float synth_sid_get_lfo2_to_pw(SynthSID* sid) {
    return sid ? sid->lfo2_to_pw_depth : 0.0f;
}

void synth_sid_set_mod_wheel(SynthSID* sid, float amount) {
    if (sid) sid->mod_wheel_amount = fminf(fmaxf(amount, 0.0f), 1.0f);
}

float synth_sid_get_mod_wheel(SynthSID* sid) {
    return sid ? sid->mod_wheel_amount : 0.0f;
}
