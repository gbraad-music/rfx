/*
 * AHX-Style Synthesizer Engine Implementation
 */

#include "synth_ahx.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define MAX_WAVE_LENGTH 256
#define NOISE_TABLE_SIZE 256

// ADSR envelope stages
typedef enum {
    ADSR_IDLE = 0,
    ADSR_ATTACK,
    ADSR_DECAY,
    ADSR_SUSTAIN,
    ADSR_RELEASE
} ADSRStage;

// Voice structure
struct SynthAHXVoice {
    // Waveform
    SynthAHXWaveform waveform;
    int wave_length;
    float wavetable[MAX_WAVE_LENGTH];
    float phase;
    float frequency;

    // ADSR envelope
    ADSRStage adsr_stage;
    float adsr_value;
    float attack_time;     // seconds
    float decay_time;      // seconds
    float sustain_level;   // 0.0 - 1.0
    float release_time;    // seconds
    float adsr_time;       // current time in stage

    // Filter
    SynthAHXFilterType filter_type;
    float filter_cutoff;
    float filter_resonance;
    float filter_state[2]; // simple 1-pole/2-pole state

    // Modulation
    float vibrato_depth;
    float vibrato_speed;
    float vibrato_phase;

    // Voice state
    int note;
    int velocity;
    int active;
};

// Noise table (pre-generated white noise)
static float noise_table[NOISE_TABLE_SIZE];
static int noise_initialized = 0;

// Initialize noise table
static void init_noise_table(void)
{
    if (noise_initialized) return;

    // Simple random noise generation
    unsigned int seed = 0x12345678;
    for (int i = 0; i < NOISE_TABLE_SIZE; i++) {
        seed = seed * 1103515245 + 12345;
        noise_table[i] = ((float)((seed >> 16) & 0xFF) / 127.5f) - 1.0f;
    }
    noise_initialized = 1;
}

// Generate waveforms into wavetable
static void generate_waveform(SynthAHXVoice* voice)
{
    int len = voice->wave_length;

    switch (voice->waveform) {
    case SYNTH_AHX_WAVE_TRIANGLE:
        for (int i = 0; i < len; i++) {
            float t = (float)i / (float)len;
            voice->wavetable[i] = (t < 0.5f) ? (4.0f * t - 1.0f) : (3.0f - 4.0f * t);
        }
        break;

    case SYNTH_AHX_WAVE_SAWTOOTH:
        for (int i = 0; i < len; i++) {
            float t = (float)i / (float)len;
            voice->wavetable[i] = 2.0f * t - 1.0f;
        }
        break;

    case SYNTH_AHX_WAVE_SQUARE:
        for (int i = 0; i < len; i++) {
            float t = (float)i / (float)len;
            voice->wavetable[i] = (t < 0.5f) ? 1.0f : -1.0f;
        }
        break;

    case SYNTH_AHX_WAVE_NOISE:
        init_noise_table();
        for (int i = 0; i < len; i++) {
            voice->wavetable[i] = noise_table[i % NOISE_TABLE_SIZE];
        }
        break;
    }
}

// Create voice
SynthAHXVoice* synth_ahx_voice_create(void)
{
    SynthAHXVoice* voice = (SynthAHXVoice*)calloc(1, sizeof(SynthAHXVoice));
    if (!voice) return NULL;

    // Defaults
    voice->waveform = SYNTH_AHX_WAVE_SAWTOOTH;
    voice->wave_length = 32;
    voice->attack_time = 0.01f;
    voice->decay_time = 0.1f;
    voice->sustain_level = 0.7f;
    voice->release_time = 0.1f;
    voice->filter_type = SYNTH_AHX_FILTER_LOWPASS;
    voice->filter_cutoff = 1.0f;
    voice->filter_resonance = 0.0f;

    generate_waveform(voice);

    return voice;
}

void synth_ahx_voice_destroy(SynthAHXVoice* voice)
{
    if (voice) free(voice);
}

void synth_ahx_voice_reset(SynthAHXVoice* voice)
{
    if (!voice) return;

    voice->phase = 0.0f;
    voice->adsr_stage = ADSR_IDLE;
    voice->adsr_value = 0.0f;
    voice->adsr_time = 0.0f;
    voice->vibrato_phase = 0.0f;
    voice->filter_state[0] = 0.0f;
    voice->filter_state[1] = 0.0f;
    voice->active = 0;
}

// Note on
void synth_ahx_voice_note_on(SynthAHXVoice* voice, int note, int velocity)
{
    if (!voice) return;

    voice->note = note;
    voice->velocity = velocity;
    voice->frequency = 440.0f * powf(2.0f, (note - 69) / 12.0f);
    voice->phase = 0.0f;
    voice->vibrato_phase = 0.0f;

    // Reset envelope completely
    voice->adsr_value = 0.0f;
    voice->adsr_stage = ADSR_ATTACK;
    voice->adsr_time = 0.0f;
    voice->active = 1;
}

// Note off
void synth_ahx_voice_note_off(SynthAHXVoice* voice)
{
    if (!voice) return;

    if (voice->adsr_stage != ADSR_IDLE && voice->adsr_stage != ADSR_RELEASE) {
        voice->adsr_stage = ADSR_RELEASE;
        voice->adsr_time = 0.0f;
    }
}

int synth_ahx_voice_is_active(SynthAHXVoice* voice)
{
    return voice ? voice->active : 0;
}

// Waveform setters
void synth_ahx_voice_set_waveform(SynthAHXVoice* voice, SynthAHXWaveform waveform)
{
    if (!voice) return;
    voice->waveform = waveform;
    generate_waveform(voice);
}

void synth_ahx_voice_set_wave_length(SynthAHXVoice* voice, int length)
{
    if (!voice) return;
    if (length < 4) length = 4;
    if (length > MAX_WAVE_LENGTH) length = MAX_WAVE_LENGTH;
    voice->wave_length = length;
    generate_waveform(voice);
}

// ADSR setters
void synth_ahx_voice_set_attack(SynthAHXVoice* voice, float attack)
{
    if (voice) voice->attack_time = attack * 2.0f; // 0-2 seconds
}

void synth_ahx_voice_set_decay(SynthAHXVoice* voice, float decay)
{
    if (voice) voice->decay_time = decay * 2.0f;
}

void synth_ahx_voice_set_sustain(SynthAHXVoice* voice, float sustain)
{
    if (voice) voice->sustain_level = sustain;
}

void synth_ahx_voice_set_release(SynthAHXVoice* voice, float release)
{
    if (voice) voice->release_time = release * 2.0f;
}

// Filter setters
void synth_ahx_voice_set_filter_type(SynthAHXVoice* voice, SynthAHXFilterType type)
{
    if (voice) voice->filter_type = type;
}

void synth_ahx_voice_set_filter_cutoff(SynthAHXVoice* voice, float cutoff)
{
    if (voice) voice->filter_cutoff = cutoff;
}

void synth_ahx_voice_set_filter_resonance(SynthAHXVoice* voice, float resonance)
{
    if (voice) voice->filter_resonance = resonance;
}

// Modulation setters
void synth_ahx_voice_set_vibrato_depth(SynthAHXVoice* voice, float depth)
{
    if (voice) voice->vibrato_depth = depth;
}

void synth_ahx_voice_set_vibrato_speed(SynthAHXVoice* voice, float speed)
{
    if (voice) voice->vibrato_speed = speed * 10.0f; // 0-10 Hz
}

// Process ADSR envelope
static float process_adsr(SynthAHXVoice* voice, float dt)
{
    switch (voice->adsr_stage) {
    case ADSR_IDLE:
        return 0.0f;

    case ADSR_ATTACK:
        voice->adsr_time += dt;
        if (voice->attack_time > 0.001f) {
            voice->adsr_value = voice->adsr_time / voice->attack_time;
            if (voice->adsr_value >= 1.0f) {
                voice->adsr_value = 1.0f;
                voice->adsr_stage = ADSR_DECAY;
                voice->adsr_time = 0.0f;
            }
        } else {
            voice->adsr_value = 1.0f;
            voice->adsr_stage = ADSR_DECAY;
            voice->adsr_time = 0.0f;
        }
        return voice->adsr_value;

    case ADSR_DECAY:
        voice->adsr_time += dt;
        if (voice->decay_time > 0.001f) {
            voice->adsr_value = 1.0f - (1.0f - voice->sustain_level) * (voice->adsr_time / voice->decay_time);
            if (voice->adsr_value <= voice->sustain_level) {
                voice->adsr_value = voice->sustain_level;
                voice->adsr_stage = ADSR_SUSTAIN;
            }
        } else {
            voice->adsr_value = voice->sustain_level;
            voice->adsr_stage = ADSR_SUSTAIN;
        }
        return voice->adsr_value;

    case ADSR_SUSTAIN:
        return voice->sustain_level;

    case ADSR_RELEASE:
        voice->adsr_time += dt;
        if (voice->release_time > 0.001f) {
            voice->adsr_value = voice->sustain_level * (1.0f - voice->adsr_time / voice->release_time);
            if (voice->adsr_value <= 0.0f || voice->adsr_time >= voice->release_time) {
                voice->adsr_value = 0.0f;
                voice->adsr_stage = ADSR_IDLE;
                voice->active = 0;
            }
        } else {
            voice->adsr_value = 0.0f;
            voice->adsr_stage = ADSR_IDLE;
            voice->active = 0;
        }
        return voice->adsr_value;
    }

    return 0.0f;
}

// Simple 1-pole lowpass/highpass filter
static float process_filter(SynthAHXVoice* voice, float input, int sample_rate)
{
    if (voice->filter_type == SYNTH_AHX_FILTER_NONE) {
        return input;
    }

    // Calculate filter coefficient (cutoff 20Hz - 20kHz)
    float fc = 20.0f * powf(1000.0f, voice->filter_cutoff);
    if (fc > sample_rate * 0.45f) fc = sample_rate * 0.45f;

    float rc = 1.0f / (2.0f * M_PI * fc);
    float dt = 1.0f / sample_rate;
    float alpha = dt / (rc + dt);

    if (voice->filter_type == SYNTH_AHX_FILTER_LOWPASS) {
        voice->filter_state[0] += alpha * (input - voice->filter_state[0]);
        return voice->filter_state[0];
    } else {
        // Highpass
        voice->filter_state[0] += alpha * (input - voice->filter_state[0]);
        return input - voice->filter_state[0];
    }
}

// Process one sample
float synth_ahx_voice_process(SynthAHXVoice* voice, int sample_rate)
{
    if (!voice || !voice->active) return 0.0f;

    float dt = 1.0f / (float)sample_rate;

    // Process ADSR
    float env = process_adsr(voice, dt);
    if (env <= 0.0f) return 0.0f;

    // Apply vibrato to frequency
    float freq = voice->frequency;
    if (voice->vibrato_depth > 0.0f) {
        float vib = sinf(2.0f * M_PI * voice->vibrato_phase);
        freq *= 1.0f + (vib * voice->vibrato_depth * 0.1f); // +/- 10% max
        voice->vibrato_phase += voice->vibrato_speed * dt;
        if (voice->vibrato_phase >= 1.0f) voice->vibrato_phase -= 1.0f;
    }

    // Read from wavetable
    float phase_inc = (freq * voice->wave_length) / (float)sample_rate;
    int idx = (int)voice->phase;
    float frac = voice->phase - idx;

    // Linear interpolation
    float s1 = voice->wavetable[idx % voice->wave_length];
    float s2 = voice->wavetable[(idx + 1) % voice->wave_length];
    float sample = s1 + frac * (s2 - s1);

    voice->phase += phase_inc;
    while (voice->phase >= voice->wave_length) {
        voice->phase -= voice->wave_length;
    }

    // Apply filter
    sample = process_filter(voice, sample, sample_rate);

    // Apply envelope and velocity
    sample *= env * (voice->velocity / 127.0f);

    return sample;
}
