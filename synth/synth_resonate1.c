/*
 * RGResonate1 - Polyphonic Subtractive Synthesizer Implementation
 */

#include "synth_resonate1.h"
#include "synth_voice_manager.h"
#include "synth_oscillator.h"
#include "synth_envelope.h"
#include "synth_filter_ladder.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define MAX_VOICES 16
#define DEFAULT_VOICES 8

// Voice structure - one instance per polyphonic voice
typedef struct {
    SynthOscillator* osc;
    SynthEnvelope* amp_env;
    SynthEnvelope* filter_env;
    SynthFilterLadder* filter;
    int active;
} Resonate1Voice;

// Main synthesizer structure
struct SynthResonate1 {
    int sample_rate;

    // Voice management
    SynthVoiceManager* voice_manager;
    Resonate1Voice voices[MAX_VOICES];
    int num_voices;

    // Global parameters
    Resonate1Waveform waveform;

    // Filter parameters
    float filter_cutoff;
    float filter_resonance;
    float filter_env_amount;

    // Amplitude envelope parameters (normalized 0-1)
    float amp_attack;
    float amp_decay;
    float amp_sustain;
    float amp_release;

    // Filter envelope parameters (normalized 0-1)
    float filter_attack;
    float filter_decay;
    float filter_sustain;
    float filter_release;
};

// Helper: Map normalized parameter (0-1) to time in seconds
static inline float param_to_time(float param, float min_time, float max_time) {
    // Exponential mapping for more musical feel
    return min_time + (max_time - min_time) * param * param;
}

// ============================================================================
// Lifecycle
// ============================================================================

SynthResonate1* synth_resonate1_create(int sample_rate) {
    SynthResonate1* synth = (SynthResonate1*)calloc(1, sizeof(SynthResonate1));
    if (!synth) return NULL;

    synth->sample_rate = sample_rate;
    synth->num_voices = DEFAULT_VOICES;

    // Create voice manager
    synth->voice_manager = synth_voice_manager_create(DEFAULT_VOICES);
    if (!synth->voice_manager) {
        free(synth);
        return NULL;
    }

    // Create voice components
    for (int i = 0; i < MAX_VOICES; i++) {
        synth->voices[i].osc = synth_oscillator_create();
        synth->voices[i].amp_env = synth_envelope_create();
        synth->voices[i].filter_env = synth_envelope_create();
        synth->voices[i].filter = synth_filter_ladder_create();
        synth->voices[i].active = 0;

        if (!synth->voices[i].osc || !synth->voices[i].amp_env ||
            !synth->voices[i].filter_env || !synth->voices[i].filter) {
            synth_resonate1_destroy(synth);
            return NULL;
        }

        // Set default waveform
        synth_oscillator_set_waveform(synth->voices[i].osc, SYNTH_OSC_SAW);
    }

    // Default parameters
    synth->waveform = RESONATE1_WAVE_SAW;
    synth->filter_cutoff = 0.8f;
    synth->filter_resonance = 0.3f;
    synth->filter_env_amount = 0.5f;

    // Default amp envelope (quick attack, medium decay/release)
    synth->amp_attack = 0.01f;   // ~10ms
    synth->amp_decay = 0.3f;     // ~300ms
    synth->amp_sustain = 0.7f;   // 70% level
    synth->amp_release = 0.2f;   // ~400ms

    // Default filter envelope
    synth->filter_attack = 0.05f;
    synth->filter_decay = 0.3f;
    synth->filter_sustain = 0.5f;
    synth->filter_release = 0.2f;

    // Apply default envelope settings to all voices
    for (int i = 0; i < MAX_VOICES; i++) {
        float attack_time = param_to_time(synth->amp_attack, 0.001f, 2.0f);
        float decay_time = param_to_time(synth->amp_decay, 0.001f, 2.0f);
        float release_time = param_to_time(synth->amp_release, 0.001f, 4.0f);

        synth_envelope_set_attack(synth->voices[i].amp_env, attack_time);
        synth_envelope_set_decay(synth->voices[i].amp_env, decay_time);
        synth_envelope_set_sustain(synth->voices[i].amp_env, synth->amp_sustain);
        synth_envelope_set_release(synth->voices[i].amp_env, release_time);

        float filt_attack = param_to_time(synth->filter_attack, 0.001f, 2.0f);
        float filt_decay = param_to_time(synth->filter_decay, 0.001f, 2.0f);
        float filt_release = param_to_time(synth->filter_release, 0.001f, 4.0f);

        synth_envelope_set_attack(synth->voices[i].filter_env, filt_attack);
        synth_envelope_set_decay(synth->voices[i].filter_env, filt_decay);
        synth_envelope_set_sustain(synth->voices[i].filter_env, synth->filter_sustain);
        synth_envelope_set_release(synth->voices[i].filter_env, filt_release);
    }

    return synth;
}

void synth_resonate1_destroy(SynthResonate1* synth) {
    if (!synth) return;

    // Destroy voice components
    for (int i = 0; i < MAX_VOICES; i++) {
        if (synth->voices[i].osc) synth_oscillator_destroy(synth->voices[i].osc);
        if (synth->voices[i].amp_env) synth_envelope_destroy(synth->voices[i].amp_env);
        if (synth->voices[i].filter_env) synth_envelope_destroy(synth->voices[i].filter_env);
        if (synth->voices[i].filter) synth_filter_ladder_destroy(synth->voices[i].filter);
    }

    if (synth->voice_manager) synth_voice_manager_destroy(synth->voice_manager);

    free(synth);
}

void synth_resonate1_reset(SynthResonate1* synth) {
    if (!synth) return;

    synth_voice_manager_reset(synth->voice_manager);

    for (int i = 0; i < MAX_VOICES; i++) {
        synth_oscillator_reset(synth->voices[i].osc);
        synth_envelope_reset(synth->voices[i].amp_env);
        synth_envelope_reset(synth->voices[i].filter_env);
        synth_filter_ladder_reset(synth->voices[i].filter);
        synth->voices[i].active = 0;
    }
}

// ============================================================================
// MIDI Event Handling
// ============================================================================

void synth_resonate1_note_on(SynthResonate1* synth, uint8_t note, uint8_t velocity) {
    if (!synth) return;

    // Allocate a voice
    int voice_index = synth_voice_manager_allocate(synth->voice_manager, note, velocity);
    if (voice_index < 0) return;  // No voice available

    Resonate1Voice* voice = &synth->voices[voice_index];

    // Set oscillator frequency
    float freq = synth_midi_to_freq(note);
    synth_oscillator_set_frequency(voice->osc, freq);

    // Trigger envelopes
    synth_envelope_trigger(voice->amp_env);
    synth_envelope_trigger(voice->filter_env);

    voice->active = 1;
}

void synth_resonate1_note_off(SynthResonate1* synth, uint8_t note) {
    if (!synth) return;

    // Release the voice
    int voice_index = synth_voice_manager_release(synth->voice_manager, note);
    if (voice_index < 0) return;  // Note not found

    Resonate1Voice* voice = &synth->voices[voice_index];

    // Release envelopes
    synth_envelope_release(voice->amp_env);
    synth_envelope_release(voice->filter_env);
}

void synth_resonate1_all_notes_off(SynthResonate1* synth) {
    if (!synth) return;

    synth_voice_manager_all_notes_off(synth->voice_manager);

    // Release all envelopes
    for (int i = 0; i < synth->num_voices; i++) {
        synth_envelope_release(synth->voices[i].amp_env);
        synth_envelope_release(synth->voices[i].filter_env);
    }
}

// ============================================================================
// Audio Processing
// ============================================================================

void synth_resonate1_process_f32(SynthResonate1* synth, float* buffer,
                                  int frames, int sample_rate) {
    if (!synth || !buffer || frames <= 0) return;

    // Clear buffer
    memset(buffer, 0, frames * 2 * sizeof(float));

    // Process each voice
    for (int v = 0; v < synth->num_voices; v++) {
        const VoiceMeta* meta = synth_voice_manager_get_voice(synth->voice_manager, v);
        if (!meta || meta->state == VOICE_INACTIVE) continue;

        Resonate1Voice* voice = &synth->voices[v];

        // Velocity scaling (MIDI velocity affects amplitude)
        float velocity_scale = meta->velocity / 127.0f;

        for (int i = 0; i < frames; i++) {
            // Generate oscillator sample
            float sample = synth_oscillator_process(voice->osc, sample_rate);

            // Process filter envelope
            float filter_env = synth_envelope_process(voice->filter_env, sample_rate);

            // Modulate filter cutoff with envelope
            float modulated_cutoff = synth->filter_cutoff +
                                    (filter_env * synth->filter_env_amount * 0.5f);
            modulated_cutoff = fmaxf(0.0f, fminf(1.0f, modulated_cutoff));

            synth_filter_ladder_set_cutoff(voice->filter, modulated_cutoff);
            synth_filter_ladder_set_resonance(voice->filter, synth->filter_resonance);

            // Apply filter
            sample = synth_filter_ladder_process(voice->filter, sample, sample_rate);

            // Process amplitude envelope
            float amp_env = synth_envelope_process(voice->amp_env, sample_rate);

            // Apply amplitude envelope and velocity
            sample *= amp_env * velocity_scale * 0.3f;  // Scale down to prevent clipping

            // Update voice amplitude for voice stealing decisions
            synth_voice_manager_update_amplitude(synth->voice_manager, v, amp_env);

            // Mix to stereo output (same on both channels for now)
            buffer[i * 2 + 0] += sample;  // Left
            buffer[i * 2 + 1] += sample;  // Right

            // Check if voice is done (envelope finished)
            if (!synth_envelope_is_active(voice->amp_env) && meta->state == VOICE_RELEASING) {
                synth_voice_manager_stop_voice(synth->voice_manager, v);
                voice->active = 0;
            }
        }
    }
}

// ============================================================================
// Parameter Setters/Getters
// ============================================================================

// Oscillator
void synth_resonate1_set_waveform(SynthResonate1* synth, Resonate1Waveform waveform) {
    if (!synth) return;
    synth->waveform = waveform;

    // Map to internal oscillator waveform
    SynthOscWaveform osc_wave;
    switch (waveform) {
        case RESONATE1_WAVE_SAW:      osc_wave = SYNTH_OSC_SAW; break;
        case RESONATE1_WAVE_SQUARE:   osc_wave = SYNTH_OSC_SQUARE; break;
        case RESONATE1_WAVE_TRIANGLE: osc_wave = SYNTH_OSC_TRIANGLE; break;
        case RESONATE1_WAVE_SINE:     osc_wave = SYNTH_OSC_SINE; break;
        default:                      osc_wave = SYNTH_OSC_SAW; break;
    }

    // Apply to all voices
    for (int i = 0; i < MAX_VOICES; i++) {
        synth_oscillator_set_waveform(synth->voices[i].osc, osc_wave);
    }
}

Resonate1Waveform synth_resonate1_get_waveform(SynthResonate1* synth) {
    return synth ? synth->waveform : RESONATE1_WAVE_SAW;
}

// Filter
void synth_resonate1_set_filter_cutoff(SynthResonate1* synth, float cutoff) {
    if (!synth) return;
    synth->filter_cutoff = fmaxf(0.0f, fminf(1.0f, cutoff));
}

void synth_resonate1_set_filter_resonance(SynthResonate1* synth, float resonance) {
    if (!synth) return;
    synth->filter_resonance = fmaxf(0.0f, fminf(1.0f, resonance));
}

float synth_resonate1_get_filter_cutoff(SynthResonate1* synth) {
    return synth ? synth->filter_cutoff : 0.0f;
}

float synth_resonate1_get_filter_resonance(SynthResonate1* synth) {
    return synth ? synth->filter_resonance : 0.0f;
}

// Amplitude Envelope
void synth_resonate1_set_amp_attack(SynthResonate1* synth, float attack) {
    if (!synth) return;
    synth->amp_attack = fmaxf(0.0f, fminf(1.0f, attack));
    float time = param_to_time(synth->amp_attack, 0.001f, 2.0f);
    for (int i = 0; i < MAX_VOICES; i++) {
        synth_envelope_set_attack(synth->voices[i].amp_env, time);
    }
}

void synth_resonate1_set_amp_decay(SynthResonate1* synth, float decay) {
    if (!synth) return;
    synth->amp_decay = fmaxf(0.0f, fminf(1.0f, decay));
    float time = param_to_time(synth->amp_decay, 0.001f, 2.0f);
    for (int i = 0; i < MAX_VOICES; i++) {
        synth_envelope_set_decay(synth->voices[i].amp_env, time);
    }
}

void synth_resonate1_set_amp_sustain(SynthResonate1* synth, float sustain) {
    if (!synth) return;
    synth->amp_sustain = fmaxf(0.0f, fminf(1.0f, sustain));
    for (int i = 0; i < MAX_VOICES; i++) {
        synth_envelope_set_sustain(synth->voices[i].amp_env, synth->amp_sustain);
    }
}

void synth_resonate1_set_amp_release(SynthResonate1* synth, float release) {
    if (!synth) return;
    synth->amp_release = fmaxf(0.0f, fminf(1.0f, release));
    float time = param_to_time(synth->amp_release, 0.001f, 4.0f);
    for (int i = 0; i < MAX_VOICES; i++) {
        synth_envelope_set_release(synth->voices[i].amp_env, time);
    }
}

float synth_resonate1_get_amp_attack(SynthResonate1* synth) {
    return synth ? synth->amp_attack : 0.0f;
}

float synth_resonate1_get_amp_decay(SynthResonate1* synth) {
    return synth ? synth->amp_decay : 0.0f;
}

float synth_resonate1_get_amp_sustain(SynthResonate1* synth) {
    return synth ? synth->amp_sustain : 0.0f;
}

float synth_resonate1_get_amp_release(SynthResonate1* synth) {
    return synth ? synth->amp_release : 0.0f;
}

// Filter Envelope
void synth_resonate1_set_filter_env_amount(SynthResonate1* synth, float amount) {
    if (!synth) return;
    synth->filter_env_amount = fmaxf(0.0f, fminf(1.0f, amount));
}

void synth_resonate1_set_filter_attack(SynthResonate1* synth, float attack) {
    if (!synth) return;
    synth->filter_attack = fmaxf(0.0f, fminf(1.0f, attack));
    float time = param_to_time(synth->filter_attack, 0.001f, 2.0f);
    for (int i = 0; i < MAX_VOICES; i++) {
        synth_envelope_set_attack(synth->voices[i].filter_env, time);
    }
}

void synth_resonate1_set_filter_decay(SynthResonate1* synth, float decay) {
    if (!synth) return;
    synth->filter_decay = fmaxf(0.0f, fminf(1.0f, decay));
    float time = param_to_time(synth->filter_decay, 0.001f, 2.0f);
    for (int i = 0; i < MAX_VOICES; i++) {
        synth_envelope_set_decay(synth->voices[i].filter_env, time);
    }
}

void synth_resonate1_set_filter_sustain(SynthResonate1* synth, float sustain) {
    if (!synth) return;
    synth->filter_sustain = fmaxf(0.0f, fminf(1.0f, sustain));
    for (int i = 0; i < MAX_VOICES; i++) {
        synth_envelope_set_sustain(synth->voices[i].filter_env, synth->filter_sustain);
    }
}

void synth_resonate1_set_filter_release(SynthResonate1* synth, float release) {
    if (!synth) return;
    synth->filter_release = fmaxf(0.0f, fminf(1.0f, release));
    float time = param_to_time(synth->filter_release, 0.001f, 4.0f);
    for (int i = 0; i < MAX_VOICES; i++) {
        synth_envelope_set_release(synth->voices[i].filter_env, time);
    }
}

float synth_resonate1_get_filter_env_amount(SynthResonate1* synth) {
    return synth ? synth->filter_env_amount : 0.0f;
}

float synth_resonate1_get_filter_attack(SynthResonate1* synth) {
    return synth ? synth->filter_attack : 0.0f;
}

float synth_resonate1_get_filter_decay(SynthResonate1* synth) {
    return synth ? synth->filter_decay : 0.0f;
}

float synth_resonate1_get_filter_sustain(SynthResonate1* synth) {
    return synth ? synth->filter_sustain : 0.0f;
}

float synth_resonate1_get_filter_release(SynthResonate1* synth) {
    return synth ? synth->filter_release : 0.0f;
}

// Voice Management
void synth_resonate1_set_polyphony(SynthResonate1* synth, int voices) {
    if (!synth) return;
    synth->num_voices = (voices < 1) ? 1 : (voices > MAX_VOICES ? MAX_VOICES : voices);
}

int synth_resonate1_get_polyphony(SynthResonate1* synth) {
    return synth ? synth->num_voices : 0;
}

// ============================================================================
// Generic Parameter Interface
// ============================================================================

#include "../param_interface.h"

// Parameter groups
typedef enum {
    PARAM_GROUP_OSCILLATOR = 0,
    PARAM_GROUP_FILTER,
    PARAM_GROUP_AMP_ENVELOPE,
    PARAM_GROUP_FILTER_ENVELOPE,
    PARAM_GROUP_COUNT
} Resonate1ParamGroup;

// Parameter indices
typedef enum {
    RESONATE1_PARAM_WAVEFORM = 0,
    RESONATE1_PARAM_FILTER_CUTOFF,
    RESONATE1_PARAM_FILTER_RESONANCE,
    RESONATE1_PARAM_AMP_ATTACK,
    RESONATE1_PARAM_AMP_DECAY,
    RESONATE1_PARAM_AMP_SUSTAIN,
    RESONATE1_PARAM_AMP_RELEASE,
    RESONATE1_PARAM_FILTER_ENV_AMOUNT,
    RESONATE1_PARAM_FILTER_ATTACK,
    RESONATE1_PARAM_FILTER_DECAY,
    RESONATE1_PARAM_FILTER_SUSTAIN,
    RESONATE1_PARAM_FILTER_RELEASE,
    RESONATE1_PARAM_COUNT
} Resonate1ParamIndex;

// Parameter metadata
static const ParameterInfo resonate1_params[RESONATE1_PARAM_COUNT] = {
    {"Waveform", "", 0.0f, 0.0f, 3.0f, PARAM_GROUP_OSCILLATOR, 1},
    {"Filter Cutoff", "Hz", 0.8f, 20.0f, 20000.0f, PARAM_GROUP_FILTER, 0},
    {"Filter Resonance", "%", 0.3f, 0.0f, 100.0f, PARAM_GROUP_FILTER, 0},
    {"Amp Attack", "s", 0.01f, 0.001f, 2.0f, PARAM_GROUP_AMP_ENVELOPE, 0},
    {"Amp Decay", "s", 0.3f, 0.001f, 2.0f, PARAM_GROUP_AMP_ENVELOPE, 0},
    {"Amp Sustain", "%", 0.7f, 0.0f, 100.0f, PARAM_GROUP_AMP_ENVELOPE, 0},
    {"Amp Release", "s", 0.2f, 0.001f, 4.0f, PARAM_GROUP_AMP_ENVELOPE, 0},
    {"Filter Env Amount", "%", 0.5f, 0.0f, 100.0f, PARAM_GROUP_FILTER_ENVELOPE, 0},
    {"Filter Attack", "s", 0.05f, 0.001f, 2.0f, PARAM_GROUP_FILTER_ENVELOPE, 0},
    {"Filter Decay", "s", 0.3f, 0.001f, 2.0f, PARAM_GROUP_FILTER_ENVELOPE, 0},
    {"Filter Sustain", "%", 0.5f, 0.0f, 100.0f, PARAM_GROUP_FILTER_ENVELOPE, 0},
    {"Filter Release", "s", 0.2f, 0.001f, 4.0f, PARAM_GROUP_FILTER_ENVELOPE, 0}
};

static const char* group_names[PARAM_GROUP_COUNT] = {
    "Oscillator",
    "Filter",
    "Amp Envelope",
    "Filter Envelope"
};

int synth_resonate1_get_parameter_count(void) {
    return RESONATE1_PARAM_COUNT;
}

float synth_resonate1_get_parameter_value(SynthResonate1* synth, int index) {
    if (!synth || index < 0 || index >= RESONATE1_PARAM_COUNT) return 0.0f;

    switch (index) {
        case RESONATE1_PARAM_WAVEFORM:
            return (float)synth_resonate1_get_waveform(synth) / 3.0f;
        case RESONATE1_PARAM_FILTER_CUTOFF:
            return synth_resonate1_get_filter_cutoff(synth);
        case RESONATE1_PARAM_FILTER_RESONANCE:
            return synth_resonate1_get_filter_resonance(synth);
        case RESONATE1_PARAM_AMP_ATTACK:
            return synth_resonate1_get_amp_attack(synth);
        case RESONATE1_PARAM_AMP_DECAY:
            return synth_resonate1_get_amp_decay(synth);
        case RESONATE1_PARAM_AMP_SUSTAIN:
            return synth_resonate1_get_amp_sustain(synth);
        case RESONATE1_PARAM_AMP_RELEASE:
            return synth_resonate1_get_amp_release(synth);
        case RESONATE1_PARAM_FILTER_ENV_AMOUNT:
            return synth_resonate1_get_filter_env_amount(synth);
        case RESONATE1_PARAM_FILTER_ATTACK:
            return synth_resonate1_get_filter_attack(synth);
        case RESONATE1_PARAM_FILTER_DECAY:
            return synth_resonate1_get_filter_decay(synth);
        case RESONATE1_PARAM_FILTER_SUSTAIN:
            return synth_resonate1_get_filter_sustain(synth);
        case RESONATE1_PARAM_FILTER_RELEASE:
            return synth_resonate1_get_filter_release(synth);
        default:
            return 0.0f;
    }
}

void synth_resonate1_set_parameter_value(SynthResonate1* synth, int index, float value) {
    if (!synth || index < 0 || index >= RESONATE1_PARAM_COUNT) return;

    switch (index) {
        case RESONATE1_PARAM_WAVEFORM: {
            int waveform = (int)(value * 3.0f + 0.5f);
            if (waveform < 0) waveform = 0;
            if (waveform > 3) waveform = 3;
            synth_resonate1_set_waveform(synth, (Resonate1Waveform)waveform);
            break;
        }
        case RESONATE1_PARAM_FILTER_CUTOFF:
            synth_resonate1_set_filter_cutoff(synth, value);
            break;
        case RESONATE1_PARAM_FILTER_RESONANCE:
            synth_resonate1_set_filter_resonance(synth, value);
            break;
        case RESONATE1_PARAM_AMP_ATTACK:
            synth_resonate1_set_amp_attack(synth, value);
            break;
        case RESONATE1_PARAM_AMP_DECAY:
            synth_resonate1_set_amp_decay(synth, value);
            break;
        case RESONATE1_PARAM_AMP_SUSTAIN:
            synth_resonate1_set_amp_sustain(synth, value);
            break;
        case RESONATE1_PARAM_AMP_RELEASE:
            synth_resonate1_set_amp_release(synth, value);
            break;
        case RESONATE1_PARAM_FILTER_ENV_AMOUNT:
            synth_resonate1_set_filter_env_amount(synth, value);
            break;
        case RESONATE1_PARAM_FILTER_ATTACK:
            synth_resonate1_set_filter_attack(synth, value);
            break;
        case RESONATE1_PARAM_FILTER_DECAY:
            synth_resonate1_set_filter_decay(synth, value);
            break;
        case RESONATE1_PARAM_FILTER_SUSTAIN:
            synth_resonate1_set_filter_sustain(synth, value);
            break;
        case RESONATE1_PARAM_FILTER_RELEASE:
            synth_resonate1_set_filter_release(synth, value);
            break;
    }
}

// Generate all metadata accessor functions using shared macro
DEFINE_PARAM_METADATA_ACCESSORS(synth_resonate1, resonate1_params, RESONATE1_PARAM_COUNT, group_names, PARAM_GROUP_COUNT)
