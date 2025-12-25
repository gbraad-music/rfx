/*
 * RG909 Drum Synthesizer - Circuit-Accurate Implementation
 * Based on actual TR-909 analog circuits with resonators
 */

#include "rg909_drum_synth.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Parameter indices
enum {
    PARAM_BD_LEVEL = 0,
    PARAM_BD_TUNE,
    PARAM_BD_DECAY,
    PARAM_BD_ATTACK,
    PARAM_SD_LEVEL,
    PARAM_SD_TONE,
    PARAM_SD_SNAPPY,
    PARAM_SD_TUNING,
    PARAM_LT_LEVEL,
    PARAM_LT_TUNING,
    PARAM_LT_DECAY,
    PARAM_MT_LEVEL,
    PARAM_MT_TUNING,
    PARAM_MT_DECAY,
    PARAM_HT_LEVEL,
    PARAM_HT_TUNING,
    PARAM_HT_DECAY,
    PARAM_RS_LEVEL,
    PARAM_RS_TUNING,
    PARAM_HC_LEVEL,
    PARAM_HC_TONE,
    PARAM_MASTER_VOLUME
};

RG909Synth* rg909_synth_create(void) {
    RG909Synth* synth = (RG909Synth*)malloc(sizeof(RG909Synth));
    if (!synth) return NULL;

    // Initialize default parameters
    synth->bd_level = 0.8f;
    synth->bd_tune = 0.5f;
    synth->bd_decay = 0.5f;
    synth->bd_attack = 0.0f;
    synth->sd_level = 0.7f;
    synth->sd_tone = 0.5f;
    synth->sd_snappy = 0.5f;
    synth->sd_tuning = 0.5f;
    synth->lt_level = 0.7f;
    synth->lt_tuning = 0.5f;
    synth->lt_decay = 0.5f;
    synth->mt_level = 0.7f;
    synth->mt_tuning = 0.5f;
    synth->mt_decay = 0.5f;
    synth->ht_level = 0.7f;
    synth->ht_tuning = 0.5f;
    synth->ht_decay = 0.5f;
    synth->rs_level = 0.6f;
    synth->rs_tuning = 0.5f;
    synth->hc_level = 0.6f;
    synth->hc_tone = 0.5f;
    synth->master_volume = 0.6f;

    synth->voice_manager = synth_voice_manager_create(RG909_MAX_VOICES);

    // Initialize voices with their drum types
    RG909DrumType types[RG909_MAX_VOICES] = {
        RG909_DRUM_BD, RG909_DRUM_SD, RG909_DRUM_LT,
        RG909_DRUM_MT, RG909_DRUM_HT, RG909_DRUM_RS, RG909_DRUM_HC
    };

    for (int i = 0; i < RG909_MAX_VOICES; i++) {
        synth->voices[i].type = types[i];
        synth->voices[i].res1 = synth_resonator_create();
        synth->voices[i].res2 = synth_resonator_create();
        synth->voices[i].noise = synth_noise_create();
        synth->voices[i].filter = synth_filter_create();
        synth->voices[i].env = synth_envelope_create();
        synth->voices[i].active = 0;
        synth->voices[i].clap_stage = 0;
        synth->voices[i].clap_timer = 0.0f;
        synth->voices[i].sweep_pos = 0.0f;
        synth->voices[i].sweep_time = 0.0f;
        synth->voices[i].sweep_amount = 1.0f;
        synth->voices[i].noise_env = 0.0f;
        synth->voices[i].noise_decay = 0.0f;
        synth->voices[i].decay_env = 0.0f;
        synth->voices[i].decay_coeff = 0.0f;
    }

    return synth;
}

void rg909_synth_destroy(RG909Synth* synth) {
    if (!synth) return;

    for (int i = 0; i < RG909_MAX_VOICES; i++) {
        if (synth->voices[i].res1) synth_resonator_destroy(synth->voices[i].res1);
        if (synth->voices[i].res2) synth_resonator_destroy(synth->voices[i].res2);
        if (synth->voices[i].noise) synth_noise_destroy(synth->voices[i].noise);
        if (synth->voices[i].filter) synth_filter_destroy(synth->voices[i].filter);
        if (synth->voices[i].env) synth_envelope_destroy(synth->voices[i].env);
    }

    if (synth->voice_manager) synth_voice_manager_destroy(synth->voice_manager);
    free(synth);
}

void rg909_synth_reset(RG909Synth* synth) {
    if (!synth) return;

    for (int i = 0; i < RG909_MAX_VOICES; i++) {
        synth->voices[i].active = 0;
        synth->voices[i].sweep_pos = 0.0f;
        synth->voices[i].clap_stage = 0;
        synth->voices[i].clap_timer = 0.0f;
    }
}

static RG909DrumType note_to_drum_type(uint8_t note) {
    switch (note) {
        case RG909_MIDI_NOTE_BD: return RG909_DRUM_BD;
        case RG909_MIDI_NOTE_SD: return RG909_DRUM_SD;
        case RG909_MIDI_NOTE_LT: return RG909_DRUM_LT;
        case RG909_MIDI_NOTE_MT: return RG909_DRUM_MT;
        case RG909_MIDI_NOTE_HT: return RG909_DRUM_HT;
        case RG909_MIDI_NOTE_RS: return RG909_DRUM_RS;
        case RG909_MIDI_NOTE_HC: return RG909_DRUM_HC;
        default: return RG909_DRUM_BD;
    }
}

void rg909_synth_trigger_drum(RG909Synth* synth, uint8_t note, uint8_t velocity, float sample_rate) {
    if (!synth) return;

    RG909DrumType type = note_to_drum_type(note);
    float vel = velocity / 127.0f;

    // Find voice for this drum type
    RG909DrumVoice* v = NULL;
    for (int i = 0; i < RG909_MAX_VOICES; i++) {
        if (synth->voices[i].type == type) {
            v = &synth->voices[i];
            break;
        }
    }
    if (!v) return;

    v->active = 1;
    v->sweep_pos = 0.0f;
    v->type = type;

    // Configure drum voice based on TR-909 circuit topology
    switch (type) {
        case RG909_DRUM_BD: {
            // REAL 909 BD: Swept sine oscillator (274Hz → 53Hz like andremichelle)
            float start_freq = 250.0f + synth->bd_tune * 50.0f;  // 250-300Hz start
            float end_freq = 50.0f + synth->bd_tune * 10.0f;     // 50-60Hz end

            v->base_freq = start_freq;   // Start frequency
            v->sweep_amount = end_freq;  // End frequency
            v->sweep_time = 0.060f;      // 60ms pitch sweep
            v->sweep_pos = 0.0f;

            // Amplitude envelope - TR-909: Fast attack, balanced decay
            v->decay_env = 1.0f;
            // Real 909 decay: 45-130ms (balanced)
            float decay_time = 0.045f + synth->bd_decay * 0.085f;  // 45-130ms decay
            v->decay_coeff = decay_time;

            // Phase accumulator for oscillator (use noise_env field)
            v->noise_env = 0.0f;
            break;
        }

        case RG909_DRUM_SD: {
            // Snare: Twin-T resonators (real 909 values!)
            // Resonator 1: 185 Hz, 120ms decay
            // Resonator 2: 330 Hz, 80ms decay (higher!)
            float freq1 = 165.0f + synth->sd_tuning * 40.0f;  // 165-205Hz (centered on 185)
            float freq2 = 310.0f + synth->sd_tuning * 40.0f;  // 310-350Hz (centered on 330)

            float decay1 = 0.12f;  // 120ms
            float decay2 = 0.08f;  // 80ms

            synth_resonator_reset(v->res1);
            synth_resonator_reset(v->res2);
            synth_resonator_set_params(v->res1, freq1, decay1, sample_rate);
            synth_resonator_set_params(v->res2, freq2, decay2, sample_rate);

            // Strike both resonators
            synth_resonator_strike(v->res1, vel * 0.3f);
            synth_resonator_strike(v->res2, vel * 0.2f);

            v->sweep_pos = 0.0f;

            // Exponential noise envelope (fast 5-20ms based on snappy)
            v->noise_env = vel * 3.0f;
            v->noise_decay = 0.005f + synth->sd_snappy * 0.015f;  // 5-20ms

            // Noise filter (1-3kHz emphasis)
            synth_filter_set_type(v->filter, SYNTH_FILTER_BPF);
            synth_filter_set_cutoff(v->filter, 0.35f + synth->sd_tone * 0.3f);
            synth_filter_set_resonance(v->filter, 0.5f);
            break;
        }

        case RG909_DRUM_LT: {
            // Low Tom: Exponential pitch sweep
            float base_freq = 80.0f + synth->lt_tuning * 60.0f;  // 80-140Hz end
            v->base_freq = base_freq;
            v->sweep_pos = 0.0f;
            v->sweep_time = 0.15f;   // 150ms sweep
            v->sweep_amount = 3.5f;  // Pitch sweep multiplier

            float decay = 0.3f + synth->lt_decay * 0.3f;
            synth_resonator_reset(v->res1);
            synth_resonator_set_params(v->res1, base_freq, decay, sample_rate);
            synth_resonator_strike(v->res1, vel * 0.3f);
            break;
        }

        case RG909_DRUM_MT: {
            // Mid Tom: Exponential pitch sweep
            float base_freq = 100.0f + synth->mt_tuning * 80.0f;  // 100-180Hz end
            v->base_freq = base_freq;
            v->sweep_pos = 0.0f;
            v->sweep_time = 0.15f;   // 150ms sweep
            v->sweep_amount = 5.75f;  // Pitch sweep multiplier

            float decay = 0.3f + synth->mt_decay * 0.3f;
            synth_resonator_reset(v->res1);
            synth_resonator_set_params(v->res1, base_freq, decay, sample_rate);
            synth_resonator_strike(v->res1, vel * 0.3f);
            break;
        }

        case RG909_DRUM_HT: {
            // High Tom: Exponential pitch sweep
            float base_freq = 140.0f + synth->ht_tuning * 100.0f;  // 140-240Hz end
            v->base_freq = base_freq;
            v->sweep_pos = 0.0f;
            v->sweep_time = 0.12f;   // 120ms sweep
            v->sweep_amount = 1.85f;  // Pitch sweep multiplier

            float decay = 0.25f + synth->ht_decay * 0.25f;
            synth_resonator_reset(v->res1);
            synth_resonator_set_params(v->res1, base_freq, decay, sample_rate);
            synth_resonator_strike(v->res1, vel * 0.3f);
            break;
        }

        case RG909_DRUM_RS: {
            // Rimshot: High frequency resonator
            float freq = 2000.0f + synth->rs_tuning * 1000.0f;  // 2-3kHz
            v->sweep_pos = 0.0f;
            synth_resonator_reset(v->res1);
            synth_resonator_set_params(v->res1, freq, 0.015f, sample_rate);
            synth_resonator_strike(v->res1, vel * 0.5f);
            break;
        }

        case RG909_DRUM_HC: {
            // Hand Clap: Diffusion network (4-tap)
            v->clap_stage = 0;
            v->clap_timer = 0.0f;

            // Setup bandpass filter for clap
            synth_filter_set_type(v->filter, SYNTH_FILTER_BPF);
            synth_filter_set_cutoff(v->filter, 0.5f + synth->hc_tone * 0.3f);
            synth_filter_set_resonance(v->filter, 0.7f);

            // First burst
            synth_envelope_set_attack(v->env, 0.001f);
            synth_envelope_set_decay(v->env, 0.015f);
            synth_envelope_set_sustain(v->env, 0.0f);
            synth_envelope_set_release(v->env, 0.01f);
            synth_envelope_trigger(v->env);
            break;
        }
    }
}

void rg909_synth_process_interleaved(RG909Synth* synth, float* buffer, int frames, float sample_rate) {
    if (!synth || !buffer) return;

    for (int frame = 0; frame < frames; frame++) {
        float mix_left = 0.0f;
        float mix_right = 0.0f;

        for (int i = 0; i < RG909_MAX_VOICES; i++) {
            RG909DrumVoice* voice = &synth->voices[i];
            if (!voice->active) continue;

            float sample = 0.0f;

            switch (voice->type) {
                case RG909_DRUM_BD: {
                    // REAL 909 BD: Swept sine oscillator with attack/decay envelope

                    // Exponential frequency sweep (high → low over 60ms)
                    float t = voice->sweep_pos / voice->sweep_time;
                    if (t > 1.0f) t = 1.0f;

                    // Exponential sweep
                    float freq_ratio = voice->base_freq / voice->sweep_amount;  // e.g. 274/53 = 5.17
                    float freq = voice->sweep_amount * powf(freq_ratio, 1.0f - t);

                    // Generate sine wave (909 uses bridged-T oscillator)
                    float phase_inc = freq / sample_rate;
                    voice->noise_env += phase_inc;
                    if (voice->noise_env >= 1.0f) voice->noise_env -= 1.0f;

                    // Bridged-T sine with analog character (subtle harmonics)
                    float phase = voice->noise_env * 2.0f * M_PI;
                    sample = sinf(phase);

                    // Add subtle 2nd and 3rd harmonics for analog warmth
                    sample += sinf(phase * 2.0f) * 0.08f;  // 2nd harmonic (even)
                    sample += sinf(phase * 3.0f) * 0.04f;  // 3rd harmonic (odd)

                    // Very subtle soft saturation (transistor/opamp character)
                    sample = sample * 0.92f + tanhf(sample * 0.4f) * 0.08f;

                    // Amplitude envelope: SHARP attack to 5ms peak, then fast initial decay
                    float amp_env;
                    float attack_time = 0.005f;  // 5ms to envelope peak
                    if (voice->sweep_pos < attack_time) {
                        // Very fast rise to peak
                        amp_env = voice->sweep_pos / attack_time;
                    } else {
                        // Exponential decay after peak
                        voice->decay_env -= voice->decay_env * (1.0f / (voice->decay_coeff * sample_rate));
                        if (voice->decay_env < 0.0001f) voice->decay_env = 0.0f;
                        amp_env = voice->decay_env;
                    }

                    sample *= amp_env;

                    // Sharp initial spike (separate from envelope) for pronounced attack
                    if (voice->sweep_pos < 0.002f) {
                        float spike_t = voice->sweep_pos / 0.002f;
                        float spike_env = 1.0f - spike_t;
                        sample += (0.25f + synth->bd_attack * 0.25f) * spike_env;
                    }

                    // Output gain - match real 909's pronounced peaks
                    // 2.40 * 0.48 (bd_level*master_volume) = 1.152, slight overdrive for punch
                    sample = sample * 2.40f;

                    voice->sweep_pos += 1.0f / sample_rate;
                    sample *= synth->bd_level;

                    if (voice->decay_env < 0.001f && voice->sweep_pos > attack_time) {
                        voice->active = 0;
                    }
                    break;
                }

                case RG909_DRUM_SD: {
                    // Tone: Two free-running resonators (185Hz + 330Hz)
                    float t1 = synth_resonator_process(voice->res1, 0.0f);
                    float t2 = synth_resonator_process(voice->res2, 0.0f);
                    float tone = (t1 + t2) * 0.5f;

                    // Exponential noise decay (one-pole)
                    voice->noise_env -= voice->noise_env * (1.0f / (voice->noise_decay * sample_rate));
                    if (voice->noise_env < 0.0001f) voice->noise_env = 0.0f;

                    float noise = synth_noise_process(voice->noise);
                    noise = synth_filter_process(voice->filter, noise, sample_rate);
                    noise *= voice->noise_env;

                    // Time-dependent crossfade (crack → body transition)
                    float t = voice->sweep_pos / 0.15f;  // Normalized time over 150ms
                    if (t > 1.0f) t = 1.0f;
                    float tone_weight = 0.3f + 0.7f * t;     // Grows over time
                    float noise_weight = (1.0f - t) * synth->sd_snappy;  // Dies quickly

                    sample = tone * tone_weight + noise * noise_weight;

                    // Nonlinear VCA
                    sample = tanhf(sample * 1.5f) * 0.7f;

                    sample *= synth->sd_level;

                    // Update time
                    voice->sweep_pos += 1.0f / sample_rate;

                    // Deactivate when both resonators and noise decay
                    if (fabsf(t1) < 0.001f && fabsf(t2) < 0.001f && voice->noise_env < 0.001f) {
                        voice->active = 0;
                    }
                    break;
                }

                case RG909_DRUM_LT:
                case RG909_DRUM_MT:
                case RG909_DRUM_HT: {
                    // Exponential pitch sweep for tom resonator
                    float t = voice->sweep_pos / voice->sweep_time;
                    if (t > 1.0f) t = 1.0f;

                    float sweep = powf(voice->sweep_amount, 1.0f - t);
                    float freq = voice->base_freq * sweep;

                    float decay = (voice->type == RG909_DRUM_LT) ? (0.3f + synth->lt_decay * 0.3f) :
                                 (voice->type == RG909_DRUM_MT) ? (0.3f + synth->mt_decay * 0.3f) :
                                                                  (0.25f + synth->ht_decay * 0.25f);

                    synth_resonator_set_params(voice->res1, freq, decay, sample_rate);

                    voice->sweep_pos += 1.0f / sample_rate;

                    sample = synth_resonator_process(voice->res1, 0.0f);

                    // Minimal noise (< 2%)
                    float noise = synth_noise_process(voice->noise);
                    sample = sample * 0.99f + noise * 0.01f * (1.0f - t);

                    float level = (voice->type == RG909_DRUM_LT) ? synth->lt_level :
                                 (voice->type == RG909_DRUM_MT) ? synth->mt_level :
                                                                  synth->ht_level;
                    sample *= level;

                    if (fabsf(sample) < 0.001f && t > 0.5f) {
                        voice->active = 0;
                    }
                    break;
                }

                case RG909_DRUM_RS: {
                    sample = synth_resonator_process(voice->res1, 0.0f);
                    sample *= synth->rs_level;

                    voice->sweep_pos += 1.0f / sample_rate;

                    if (fabsf(sample) < 0.001f && voice->sweep_pos > 0.05f) {
                        voice->active = 0;
                    }
                    break;
                }

                case RG909_DRUM_HC: {
                    // Clap diffusion: 4 bursts
                    float env_val = synth_envelope_process(voice->env, sample_rate);

                    float noise = synth_noise_process(voice->noise);
                    noise = synth_filter_process(voice->filter, noise, sample_rate);

                    sample = noise * env_val;

                    voice->clap_timer += 1.0f / sample_rate;

                    // Trigger next burst
                    if (voice->clap_stage < 3 && voice->clap_timer > (voice->clap_stage + 1) * 0.015f) {
                        voice->clap_stage++;
                        synth_envelope_trigger(voice->env);
                    }

                    sample *= synth->hc_level;

                    if (env_val < 0.001f && voice->clap_stage >= 3) {
                        voice->active = 0;
                    }
                    break;
                }
            }

            // Mix to stereo (mono for now)
            mix_left += sample;
            mix_right += sample;
        }

        // Apply master volume
        mix_left *= synth->master_volume;
        mix_right *= synth->master_volume;

        // Write interleaved stereo
        buffer[frame * 2] = mix_left;
        buffer[frame * 2 + 1] = mix_right;
    }
}

void rg909_synth_set_parameter(RG909Synth* synth, int param_index, float value) {
    if (!synth) return;

    value = (value < 0.0f) ? 0.0f : (value > 1.0f) ? 1.0f : value;

    switch (param_index) {
        case PARAM_BD_LEVEL: synth->bd_level = value; break;
        case PARAM_BD_TUNE: synth->bd_tune = value; break;
        case PARAM_BD_DECAY: synth->bd_decay = value; break;
        case PARAM_BD_ATTACK: synth->bd_attack = value; break;
        case PARAM_SD_LEVEL: synth->sd_level = value; break;
        case PARAM_SD_TONE: synth->sd_tone = value; break;
        case PARAM_SD_SNAPPY: synth->sd_snappy = value; break;
        case PARAM_SD_TUNING: synth->sd_tuning = value; break;
        case PARAM_LT_LEVEL: synth->lt_level = value; break;
        case PARAM_LT_TUNING: synth->lt_tuning = value; break;
        case PARAM_LT_DECAY: synth->lt_decay = value; break;
        case PARAM_MT_LEVEL: synth->mt_level = value; break;
        case PARAM_MT_TUNING: synth->mt_tuning = value; break;
        case PARAM_MT_DECAY: synth->mt_decay = value; break;
        case PARAM_HT_LEVEL: synth->ht_level = value; break;
        case PARAM_HT_TUNING: synth->ht_tuning = value; break;
        case PARAM_HT_DECAY: synth->ht_decay = value; break;
        case PARAM_RS_LEVEL: synth->rs_level = value; break;
        case PARAM_RS_TUNING: synth->rs_tuning = value; break;
        case PARAM_HC_LEVEL: synth->hc_level = value; break;
        case PARAM_HC_TONE: synth->hc_tone = value; break;
        case PARAM_MASTER_VOLUME: synth->master_volume = value; break;
    }
}

float rg909_synth_get_parameter(RG909Synth* synth, int param_index) {
    if (!synth) return 0.0f;

    switch (param_index) {
        case PARAM_BD_LEVEL: return synth->bd_level;
        case PARAM_BD_TUNE: return synth->bd_tune;
        case PARAM_BD_DECAY: return synth->bd_decay;
        case PARAM_BD_ATTACK: return synth->bd_attack;
        case PARAM_SD_LEVEL: return synth->sd_level;
        case PARAM_SD_TONE: return synth->sd_tone;
        case PARAM_SD_SNAPPY: return synth->sd_snappy;
        case PARAM_SD_TUNING: return synth->sd_tuning;
        case PARAM_LT_LEVEL: return synth->lt_level;
        case PARAM_LT_TUNING: return synth->lt_tuning;
        case PARAM_LT_DECAY: return synth->lt_decay;
        case PARAM_MT_LEVEL: return synth->mt_level;
        case PARAM_MT_TUNING: return synth->mt_tuning;
        case PARAM_MT_DECAY: return synth->mt_decay;
        case PARAM_HT_LEVEL: return synth->ht_level;
        case PARAM_HT_TUNING: return synth->ht_tuning;
        case PARAM_HT_DECAY: return synth->ht_decay;
        case PARAM_RS_LEVEL: return synth->rs_level;
        case PARAM_RS_TUNING: return synth->rs_tuning;
        case PARAM_HC_LEVEL: return synth->hc_level;
        case PARAM_HC_TONE: return synth->hc_tone;
        case PARAM_MASTER_VOLUME: return synth->master_volume;
        default: return 0.0f;
    }
}
