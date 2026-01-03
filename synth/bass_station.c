/*
 * Regroove Bass Station Synthesizer
 * Implementation
 */

#include "bass_station.h"
#include "synth_oscillator.h"
#include "synth_filter.h"
#include "synth_filter_ladder.h"
#include "synth_envelope.h"
#include "synth_lfo.h"
#include <stdlib.h>
#include <math.h>
#include <string.h>

struct BassStation {
    // Oscillators
    SynthOscillator* osc1;
    SynthOscillator* osc2;
    SynthOscillator* sub_osc;

    // Filters
    SynthFilter* acid_filter;          // TB-303 style
    SynthFilterLadder* classic_filter; // Moog ladder

    // Envelopes
    SynthEnvelope* amp_env;
    SynthEnvelope* mod_env;

    // LFOs
    SynthLFO* lfo1;
    SynthLFO* lfo2;

    // Voice state
    uint8_t current_note;
    uint8_t current_velocity;
    int active;
    int gate;  // Note is being held

    // Pitch/frequency state
    float base_freq;       // Target frequency (from MIDI note)
    float current_freq;    // Current frequency (for portamento)
    int sliding;

    // Oscillator parameters
    int osc1_waveform;     // 0-3
    int osc1_octave;       // -2 to +2
    float osc1_fine;       // -12.0 to +12.0
    float osc1_pw;

    int osc2_waveform;
    int osc2_octave;
    float osc2_fine;
    float osc2_pw;

    float osc_mix;         // 0.0-1.0
    int osc_sync;          // 0 or 1

    // Sub-oscillator parameters
    BassStationSubMode sub_mode;
    BassStationSubWave sub_wave;
    float sub_level;

    // Filter parameters
    BassStationFilterMode filter_mode;
    BassStationFilterType filter_type;
    float filter_cutoff;
    float filter_resonance;
    float filter_drive;

    // Modulation amounts
    float mod_env_to_filter;
    float mod_env_to_pitch;
    float mod_env_to_pw;
    float lfo1_to_pitch;
    float lfo2_to_pw;
    float lfo2_to_filter;

    // Performance parameters
    float portamento_time;
    float volume;
    float distortion;
};

// ============================================================================
// Helper functions
// ============================================================================

static inline float clamp(float value, float min, float max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

static inline float midi_note_to_freq(uint8_t note) {
    return 440.0f * powf(2.0f, (note - 69) / 12.0f);
}

static inline float soft_clip(float x) {
    // Soft clipping using tanh approximation
    if (x > 3.0f) return 1.0f;
    if (x < -3.0f) return -1.0f;
    return x * (27.0f + x * x) / (27.0f + 9.0f * x * x);
}

static inline SynthOscWaveform int_to_waveform(int wf) {
    switch (wf) {
        case 0: return SYNTH_OSC_SINE;
        case 1: return SYNTH_OSC_SAW;
        case 2: return SYNTH_OSC_SQUARE;
        case 3: return SYNTH_OSC_TRIANGLE;
        default: return SYNTH_OSC_SAW;
    }
}

static inline SynthLFOWaveform float_to_lfo_waveform(float wf) {
    int iwf = (int)(wf + 0.5f);
    switch (iwf) {
        case 0: return SYNTH_LFO_SINE;
        case 1: return SYNTH_LFO_TRIANGLE;
        case 2: return SYNTH_LFO_SQUARE;
        case 3: return SYNTH_LFO_SAW_UP;
        case 4: return SYNTH_LFO_SAW_DOWN;
        case 5: return SYNTH_LFO_RANDOM;
        default: return SYNTH_LFO_SINE;
    }
}

// ============================================================================
// Lifecycle
// ============================================================================

BassStation* bass_station_create(void) {
    BassStation* bs = (BassStation*)malloc(sizeof(BassStation));
    if (!bs) return NULL;

    memset(bs, 0, sizeof(BassStation));

    // Create oscillators
    bs->osc1 = synth_oscillator_create();
    bs->osc2 = synth_oscillator_create();
    bs->sub_osc = synth_oscillator_create();

    // Create filters
    bs->acid_filter = synth_filter_create();
    bs->classic_filter = synth_filter_ladder_create();

    // Create envelopes
    bs->amp_env = synth_envelope_create();
    bs->mod_env = synth_envelope_create();

    // Create LFOs
    bs->lfo1 = synth_lfo_create();
    bs->lfo2 = synth_lfo_create();

    // Check if all components were created
    if (!bs->osc1 || !bs->osc2 || !bs->sub_osc ||
        !bs->acid_filter || !bs->classic_filter ||
        !bs->amp_env || !bs->mod_env ||
        !bs->lfo1 || !bs->lfo2) {
        bass_station_destroy(bs);
        return NULL;
    }

    // Initialize default parameters
    bs->osc1_waveform = 1;  // Sawtooth
    bs->osc1_octave = 0;
    bs->osc1_fine = 0.0f;
    bs->osc1_pw = 0.5f;

    bs->osc2_waveform = 1;  // Sawtooth
    bs->osc2_octave = 0;
    bs->osc2_fine = 0.0f;
    bs->osc2_pw = 0.5f;

    bs->osc_mix = 0.5f;
    bs->osc_sync = 0;

    bs->sub_mode = BASS_STATION_SUB_MINUS_1_OCT;
    bs->sub_wave = BASS_STATION_SUB_SQUARE;
    bs->sub_level = 0.3f;

    bs->filter_mode = BASS_STATION_FILTER_CLASSIC;
    bs->filter_type = BASS_STATION_FILTER_LPF_24DB;
    bs->filter_cutoff = 0.5f;
    bs->filter_resonance = 0.3f;
    bs->filter_drive = 0.0f;

    bs->mod_env_to_filter = 0.5f;
    bs->mod_env_to_pitch = 0.0f;
    bs->mod_env_to_pw = 0.0f;
    bs->lfo1_to_pitch = 0.0f;
    bs->lfo2_to_pw = 0.0f;
    bs->lfo2_to_filter = 0.0f;

    bs->portamento_time = 0.0f;
    bs->volume = 0.7f;
    bs->distortion = 0.0f;

    bs->current_note = 60;
    bs->current_velocity = 0;
    bs->active = 0;
    bs->gate = 0;
    bs->base_freq = 440.0f;
    bs->current_freq = 440.0f;
    bs->sliding = 0;

    // Configure default envelope times
    synth_envelope_set_attack(bs->amp_env, 0.01f);
    synth_envelope_set_decay(bs->amp_env, 0.3f);
    synth_envelope_set_sustain(bs->amp_env, 0.7f);
    synth_envelope_set_release(bs->amp_env, 0.5f);

    synth_envelope_set_attack(bs->mod_env, 0.01f);
    synth_envelope_set_decay(bs->mod_env, 0.5f);
    synth_envelope_set_sustain(bs->mod_env, 0.3f);
    synth_envelope_set_release(bs->mod_env, 0.3f);

    // Configure LFOs
    synth_lfo_set_frequency(bs->lfo1, 5.0f);
    synth_lfo_set_waveform(bs->lfo1, SYNTH_LFO_SINE);

    synth_lfo_set_frequency(bs->lfo2, 3.0f);
    synth_lfo_set_waveform(bs->lfo2, SYNTH_LFO_TRIANGLE);

    // Configure filters
    synth_filter_set_type(bs->acid_filter, SYNTH_FILTER_LPF);
    synth_filter_set_cutoff(bs->acid_filter, bs->filter_cutoff);
    synth_filter_set_resonance(bs->acid_filter, bs->filter_resonance);

    synth_filter_ladder_set_cutoff(bs->classic_filter, bs->filter_cutoff);
    synth_filter_ladder_set_resonance(bs->classic_filter, bs->filter_resonance);

    return bs;
}

void bass_station_destroy(BassStation* bs) {
    if (!bs) return;

    if (bs->osc1) synth_oscillator_destroy(bs->osc1);
    if (bs->osc2) synth_oscillator_destroy(bs->osc2);
    if (bs->sub_osc) synth_oscillator_destroy(bs->sub_osc);
    if (bs->acid_filter) synth_filter_destroy(bs->acid_filter);
    if (bs->classic_filter) synth_filter_ladder_destroy(bs->classic_filter);
    if (bs->amp_env) synth_envelope_destroy(bs->amp_env);
    if (bs->mod_env) synth_envelope_destroy(bs->mod_env);
    if (bs->lfo1) synth_lfo_destroy(bs->lfo1);
    if (bs->lfo2) synth_lfo_destroy(bs->lfo2);

    free(bs);
}

void bass_station_reset(BassStation* bs) {
    if (!bs) return;

    synth_oscillator_reset(bs->osc1);
    synth_oscillator_reset(bs->osc2);
    synth_oscillator_reset(bs->sub_osc);
    synth_filter_reset(bs->acid_filter);
    synth_filter_ladder_reset(bs->classic_filter);
    synth_envelope_reset(bs->amp_env);
    synth_envelope_reset(bs->mod_env);
    synth_lfo_reset(bs->lfo1);
    synth_lfo_reset(bs->lfo2);

    bs->active = 0;
    bs->gate = 0;
}

// ============================================================================
// MIDI Control
// ============================================================================

void bass_station_note_on(BassStation* bs, uint8_t note, uint8_t velocity) {
    if (!bs) return;

    float new_freq = midi_note_to_freq(note);

    // Check if we should slide (legato)
    int should_slide = bs->gate && bs->active && bs->portamento_time > 0.001f;

    bs->current_note = note;
    bs->current_velocity = velocity;
    bs->base_freq = new_freq;
    bs->active = 1;
    bs->gate = 1;

    if (should_slide) {
        // Slide to new note (don't retrigger envelopes)
        bs->sliding = 1;
    } else {
        // New note (retrigger envelopes)
        bs->current_freq = new_freq;
        bs->sliding = 0;
        synth_envelope_trigger(bs->amp_env);
        synth_envelope_trigger(bs->mod_env);
    }
}

void bass_station_note_off(BassStation* bs, uint8_t note) {
    if (!bs) return;

    if (bs->current_note == note && bs->active) {
        bs->gate = 0;
        synth_envelope_release(bs->amp_env);
        synth_envelope_release(bs->mod_env);
    }
}

int bass_station_is_active(BassStation* bs) {
    if (!bs) return 0;
    return bs->active;
}

// ============================================================================
// Oscillator Parameters
// ============================================================================

void bass_station_set_osc1_waveform(BassStation* bs, float waveform) {
    if (!bs) return;
    bs->osc1_waveform = (int)(clamp(waveform, 0.0f, 3.0f) + 0.5f);
}

void bass_station_set_osc1_octave(BassStation* bs, int octave) {
    if (!bs) return;
    bs->osc1_octave = octave < -2 ? -2 : (octave > 2 ? 2 : octave);
}

void bass_station_set_osc1_fine(BassStation* bs, float semitones) {
    if (!bs) return;
    bs->osc1_fine = clamp(semitones, -12.0f, 12.0f);
}

void bass_station_set_osc1_pw(BassStation* bs, float pw) {
    if (!bs) return;
    bs->osc1_pw = clamp(pw, 0.01f, 0.99f);
}

void bass_station_set_osc2_waveform(BassStation* bs, float waveform) {
    if (!bs) return;
    bs->osc2_waveform = (int)(clamp(waveform, 0.0f, 3.0f) + 0.5f);
}

void bass_station_set_osc2_octave(BassStation* bs, int octave) {
    if (!bs) return;
    bs->osc2_octave = octave < -2 ? -2 : (octave > 2 ? 2 : octave);
}

void bass_station_set_osc2_fine(BassStation* bs, float semitones) {
    if (!bs) return;
    bs->osc2_fine = clamp(semitones, -12.0f, 12.0f);
}

void bass_station_set_osc2_pw(BassStation* bs, float pw) {
    if (!bs) return;
    bs->osc2_pw = clamp(pw, 0.01f, 0.99f);
}

void bass_station_set_osc_mix(BassStation* bs, float mix) {
    if (!bs) return;
    bs->osc_mix = clamp(mix, 0.0f, 1.0f);
}

void bass_station_set_osc_sync(BassStation* bs, int enable) {
    if (!bs) return;
    bs->osc_sync = enable ? 1 : 0;
}

// ============================================================================
// Sub-Oscillator Parameters
// ============================================================================

void bass_station_set_sub_mode(BassStation* bs, BassStationSubMode mode) {
    if (!bs) return;
    bs->sub_mode = mode;
}

void bass_station_set_sub_wave(BassStation* bs, BassStationSubWave wave) {
    if (!bs) return;
    bs->sub_wave = wave;
}

void bass_station_set_sub_level(BassStation* bs, float level) {
    if (!bs) return;
    bs->sub_level = clamp(level, 0.0f, 1.0f);
}

// ============================================================================
// Filter Parameters
// ============================================================================

void bass_station_set_filter_mode(BassStation* bs, BassStationFilterMode mode) {
    if (!bs) return;
    bs->filter_mode = mode;
}

void bass_station_set_filter_type(BassStation* bs, BassStationFilterType type) {
    if (!bs) return;
    bs->filter_type = type;
}

void bass_station_set_filter_cutoff(BassStation* bs, float cutoff) {
    if (!bs) return;
    bs->filter_cutoff = clamp(cutoff, 0.0f, 1.0f);
}

void bass_station_set_filter_resonance(BassStation* bs, float resonance) {
    if (!bs) return;
    bs->filter_resonance = clamp(resonance, 0.0f, 1.0f);
}

void bass_station_set_filter_drive(BassStation* bs, float drive) {
    if (!bs) return;
    bs->filter_drive = clamp(drive, 0.0f, 1.0f);
}

// ============================================================================
// Envelope Parameters
// ============================================================================

void bass_station_set_amp_attack(BassStation* bs, float attack) {
    if (!bs) return;
    synth_envelope_set_attack(bs->amp_env, clamp(attack, 0.0f, 5.0f));
}

void bass_station_set_amp_decay(BassStation* bs, float decay) {
    if (!bs) return;
    synth_envelope_set_decay(bs->amp_env, clamp(decay, 0.0f, 5.0f));
}

void bass_station_set_amp_sustain(BassStation* bs, float sustain) {
    if (!bs) return;
    synth_envelope_set_sustain(bs->amp_env, clamp(sustain, 0.0f, 1.0f));
}

void bass_station_set_amp_release(BassStation* bs, float release) {
    if (!bs) return;
    synth_envelope_set_release(bs->amp_env, clamp(release, 0.0f, 5.0f));
}

void bass_station_set_mod_attack(BassStation* bs, float attack) {
    if (!bs) return;
    synth_envelope_set_attack(bs->mod_env, clamp(attack, 0.0f, 5.0f));
}

void bass_station_set_mod_decay(BassStation* bs, float decay) {
    if (!bs) return;
    synth_envelope_set_decay(bs->mod_env, clamp(decay, 0.0f, 5.0f));
}

void bass_station_set_mod_sustain(BassStation* bs, float sustain) {
    if (!bs) return;
    synth_envelope_set_sustain(bs->mod_env, clamp(sustain, 0.0f, 1.0f));
}

void bass_station_set_mod_release(BassStation* bs, float release) {
    if (!bs) return;
    synth_envelope_set_release(bs->mod_env, clamp(release, 0.0f, 5.0f));
}

// ============================================================================
// Modulation Parameters
// ============================================================================

void bass_station_set_mod_env_to_filter(BassStation* bs, float amount) {
    if (!bs) return;
    bs->mod_env_to_filter = clamp(amount, -1.0f, 1.0f);
}

void bass_station_set_mod_env_to_pitch(BassStation* bs, float amount) {
    if (!bs) return;
    bs->mod_env_to_pitch = clamp(amount, -1.0f, 1.0f);
}

void bass_station_set_mod_env_to_pw(BassStation* bs, float amount) {
    if (!bs) return;
    bs->mod_env_to_pw = clamp(amount, -1.0f, 1.0f);
}

void bass_station_set_lfo1_rate(BassStation* bs, float rate) {
    if (!bs) return;
    synth_lfo_set_frequency(bs->lfo1, clamp(rate, 0.1f, 20.0f));
}

void bass_station_set_lfo1_waveform(BassStation* bs, float waveform) {
    if (!bs) return;
    synth_lfo_set_waveform(bs->lfo1, float_to_lfo_waveform(waveform));
}

void bass_station_set_lfo1_to_pitch(BassStation* bs, float amount) {
    if (!bs) return;
    bs->lfo1_to_pitch = clamp(amount, -1.0f, 1.0f);
}

void bass_station_set_lfo2_rate(BassStation* bs, float rate) {
    if (!bs) return;
    synth_lfo_set_frequency(bs->lfo2, clamp(rate, 0.1f, 20.0f));
}

void bass_station_set_lfo2_waveform(BassStation* bs, float waveform) {
    if (!bs) return;
    synth_lfo_set_waveform(bs->lfo2, float_to_lfo_waveform(waveform));
}

void bass_station_set_lfo2_to_pw(BassStation* bs, float amount) {
    if (!bs) return;
    bs->lfo2_to_pw = clamp(amount, -1.0f, 1.0f);
}

void bass_station_set_lfo2_to_filter(BassStation* bs, float amount) {
    if (!bs) return;
    bs->lfo2_to_filter = clamp(amount, -1.0f, 1.0f);
}

// ============================================================================
// Performance Parameters
// ============================================================================

void bass_station_set_portamento(BassStation* bs, float time) {
    if (!bs) return;
    bs->portamento_time = clamp(time, 0.0f, 1.0f);
}

void bass_station_set_volume(BassStation* bs, float volume) {
    if (!bs) return;
    bs->volume = clamp(volume, 0.0f, 1.0f);
}

void bass_station_set_distortion(BassStation* bs, float amount) {
    if (!bs) return;
    bs->distortion = clamp(amount, 0.0f, 1.0f);
}

// ============================================================================
// Audio Processing
// ============================================================================

float bass_station_process(BassStation* bs, int sample_rate) {
    if (!bs || !bs->active) return 0.0f;

    // Process LFOs
    float lfo1_value = synth_lfo_process(bs->lfo1, sample_rate);
    float lfo2_value = synth_lfo_process(bs->lfo2, sample_rate);

    // Process envelopes
    float amp_env_value = synth_envelope_process(bs->amp_env, sample_rate);
    float mod_env_value = synth_envelope_process(bs->mod_env, sample_rate);

    // Handle portamento/glide
    if (bs->sliding && bs->portamento_time > 0.001f) {
        float slide_rate = (bs->base_freq - bs->current_freq) / (bs->portamento_time * sample_rate);
        bs->current_freq += slide_rate;

        // Check if we've reached the target
        if ((slide_rate > 0.0f && bs->current_freq >= bs->base_freq) ||
            (slide_rate < 0.0f && bs->current_freq <= bs->base_freq)) {
            bs->current_freq = bs->base_freq;
            bs->sliding = 0;
        }
    }

    // Calculate modulated pitch
    float pitch_mod = 1.0f;
    pitch_mod *= powf(2.0f, bs->lfo1_to_pitch * lfo1_value / 12.0f);  // LFO1 to pitch
    pitch_mod *= powf(2.0f, bs->mod_env_to_pitch * mod_env_value);    // Mod env to pitch

    // Calculate OSC1 frequency
    float osc1_freq = bs->current_freq * pitch_mod;
    osc1_freq *= powf(2.0f, bs->osc1_octave);                        // Octave shift
    osc1_freq *= powf(2.0f, bs->osc1_fine / 12.0f);                  // Fine tune

    // Calculate OSC2 frequency
    float osc2_freq = bs->current_freq * pitch_mod;
    osc2_freq *= powf(2.0f, bs->osc2_octave);
    osc2_freq *= powf(2.0f, bs->osc2_fine / 12.0f);

    // Calculate modulated pulse width
    float pw1 = bs->osc1_pw + (bs->lfo2_to_pw * lfo2_value * 0.3f) + (bs->mod_env_to_pw * mod_env_value * 0.3f);
    pw1 = clamp(pw1, 0.05f, 0.95f);

    float pw2 = bs->osc2_pw + (bs->lfo2_to_pw * lfo2_value * 0.3f) + (bs->mod_env_to_pw * mod_env_value * 0.3f);
    pw2 = clamp(pw2, 0.05f, 0.95f);

    // Configure and render OSC1
    synth_oscillator_set_waveform(bs->osc1, int_to_waveform(bs->osc1_waveform));
    synth_oscillator_set_frequency(bs->osc1, osc1_freq);
    synth_oscillator_set_pulse_width(bs->osc1, pw1);

    // Configure and render OSC2
    synth_oscillator_set_waveform(bs->osc2, int_to_waveform(bs->osc2_waveform));
    synth_oscillator_set_frequency(bs->osc2, osc2_freq);
    synth_oscillator_set_pulse_width(bs->osc2, pw2);

    // Handle oscillator sync (OSC2 -> OSC1)
    // For sync, we reset OSC2's phase when OSC1 crosses zero
    // This is simplified - a full implementation would need phase tracking

    float osc1_sample = synth_oscillator_process(bs->osc1, sample_rate);
    float osc2_sample = synth_oscillator_process(bs->osc2, sample_rate);

    // Mix oscillators
    float osc_out = (osc1_sample * (1.0f - bs->osc_mix)) + (osc2_sample * bs->osc_mix);

    // Add sub-oscillator
    if (bs->sub_mode != BASS_STATION_SUB_OFF) {
        float sub_freq = bs->current_freq * pitch_mod;
        if (bs->sub_mode == BASS_STATION_SUB_MINUS_1_OCT) {
            sub_freq *= 0.5f;  // -1 octave
        } else {
            sub_freq *= 0.25f; // -2 octaves
        }

        // Set sub-osc waveform
        SynthOscWaveform sub_wf = SYNTH_OSC_SQUARE;
        if (bs->sub_wave == BASS_STATION_SUB_SINE) sub_wf = SYNTH_OSC_SINE;
        else if (bs->sub_wave == BASS_STATION_SUB_PULSE) sub_wf = SYNTH_OSC_SQUARE;

        synth_oscillator_set_waveform(bs->sub_osc, sub_wf);
        synth_oscillator_set_frequency(bs->sub_osc, sub_freq);
        synth_oscillator_set_pulse_width(bs->sub_osc, 0.1f);  // Narrow pulse for sub

        float sub_sample = synth_oscillator_process(bs->sub_osc, sample_rate);
        osc_out += sub_sample * bs->sub_level;
    }

    // Reduce oscillator level to prevent clipping
    osc_out *= 0.2f;

    // Apply filter overdrive/distortion (pre-filter)
    if (bs->filter_drive > 0.01f) {
        float drive_amount = 1.0f + bs->filter_drive * 9.0f;  // 1x to 10x
        osc_out = soft_clip(osc_out * drive_amount) / drive_amount;
    }

    // Calculate modulated filter cutoff
    float filter_cutoff = bs->filter_cutoff;
    filter_cutoff += bs->mod_env_to_filter * mod_env_value;
    filter_cutoff += bs->lfo2_to_filter * lfo2_value * 0.3f;
    filter_cutoff = clamp(filter_cutoff, 0.0f, 1.0f);

    // Apply filter
    float filtered;
    if (bs->filter_mode == BASS_STATION_FILTER_ACID) {
        // TB-303 style filter
        synth_filter_set_cutoff(bs->acid_filter, filter_cutoff);
        synth_filter_set_resonance(bs->acid_filter, bs->filter_resonance);

        // Set filter type based on filter_type setting
        if (bs->filter_type == BASS_STATION_FILTER_HPF_12DB ||
            bs->filter_type == BASS_STATION_FILTER_HPF_24DB) {
            synth_filter_set_type(bs->acid_filter, SYNTH_FILTER_HPF);
        } else if (bs->filter_type == BASS_STATION_FILTER_BPF_12DB ||
                   bs->filter_type == BASS_STATION_FILTER_BPF_24DB) {
            synth_filter_set_type(bs->acid_filter, SYNTH_FILTER_BPF);
        } else {
            synth_filter_set_type(bs->acid_filter, SYNTH_FILTER_LPF);
        }

        filtered = synth_filter_process(bs->acid_filter, osc_out, sample_rate);
    } else {
        // Classic (Moog ladder) filter - always 24dB/octave LPF
        synth_filter_ladder_set_cutoff(bs->classic_filter, filter_cutoff);
        synth_filter_ladder_set_resonance(bs->classic_filter, bs->filter_resonance);
        filtered = synth_filter_ladder_process(bs->classic_filter, osc_out, sample_rate);
    }

    // Apply amplitude envelope
    filtered *= amp_env_value;

    // Apply analog distortion
    if (bs->distortion > 0.01f) {
        float dist_amount = 1.0f + bs->distortion * 4.0f;
        filtered = soft_clip(filtered * dist_amount);
    }

    // Apply master volume
    filtered *= bs->volume;

    // Final clipping
    filtered = clamp(filtered, -1.0f, 1.0f);

    // Check if voice should be deactivated
    if (!synth_envelope_is_active(bs->amp_env)) {
        bs->active = 0;
    }

    return filtered;
}

void bass_station_process_stereo(BassStation* bs, float* output, int num_frames, int sample_rate) {
    if (!bs || !output) return;

    for (int i = 0; i < num_frames; i++) {
        float sample = bass_station_process(bs, sample_rate);
        output[i * 2] = sample;      // Left
        output[i * 2 + 1] = sample;  // Right (mono synth)
    }
}
