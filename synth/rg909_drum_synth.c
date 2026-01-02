/*
 * RG909 Drum Synthesizer - Circuit-Accurate Implementation
 * Based on actual TR-909 analog circuits with resonators
 */

#include "rg909_drum_synth.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>

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
    PARAM_MASTER_VOLUME,
    // Extended SD Parameters (for advanced snare control)
    PARAM_SD_TONE_GAIN,
    PARAM_SD_FREQ1,
    PARAM_SD_FREQ2,
    PARAM_SD_RES1_LEVEL,
    PARAM_SD_RES2_LEVEL,
    PARAM_SD_NOISE_LEVEL,
    PARAM_SD_LP_NOISE_CUTOFF,
    PARAM_SD_RES1_DECAY,
    PARAM_SD_RES2_DECAY,
    PARAM_SD_NOISE_DECAY,
    PARAM_SD_NOISE_ATTACK,
    PARAM_SD_NOISE_FADE_TIME,
    PARAM_SD_NOISE_FADE_CURVE,
    // BD Sweep-Shape Parameters
    PARAM_BD_SQUIGGLY_END_MS,
    PARAM_BD_FAST_END_MS,
    PARAM_BD_SLOW_END_MS,
    PARAM_BD_TAIL_SLOW_START_MS,
    PARAM_BD_SQUIGGLY_FREQ,
    PARAM_BD_FAST_FREQ,
    PARAM_BD_SLOW_FREQ,
    PARAM_BD_TAIL_FREQ,
    PARAM_BD_TAIL_SLOW_FREQ,
    PARAM_BD_FAST_SAW_PCT,
    PARAM_BD_SLOW_SAW_PCT
};

RG909Synth* rg909_synth_create(void) {
    RG909Synth* synth = (RG909Synth*)malloc(sizeof(RG909Synth));
    if (!synth) return NULL;

    // Initialize default parameters
    synth->bd_level = 0.96f;
    synth->bd_tune = 0.5f;
    synth->bd_decay = 0.13f;
    synth->bd_attack = 0.0f;
    synth->sd_level = 44.0f;  // Master output level (user-tweaked)
    synth->sd_tone = 0.01f;    // High-pass filter cutoff for "snappy" noise (VERY LOW)
    synth->sd_snappy = 0.0115f;  // WALDORF: High-pass noise level (user-tweaked)
    synth->sd_tuning = 0.5f;
    synth->sd_tone_gain = 0.5f;  // Resonator output gain (user-tweaked)

    // Extended snare parameters (user-tweaked for real TR-909 sound)
    // Key finding: frequencies are VERY CLOSE together (120Hz/122Hz - only 2Hz apart!)
    // res2 (swept) contributes very little compared to res1 (constant)
    // Noise path 1: Low-pass filtered, always on
    // Noise path 2: High-pass filtered, envelope controlled by sd_snappy
    synth->sd_freq1 = 120.0f;  // Osc 1: Constant frequency (Hz) - DOMINANT oscillator
    synth->sd_freq2 = 122.0f;  // Osc 2: Base frequency for pitch sweep (Hz) - MINIMAL contribution
    synth->sd_res1_level = 8.5f;  // Osc 1 strike level (STRONG - main tone source)
    synth->sd_res2_level = 1.5f;  // Osc 2 strike level (WEAK - barely contributes)
    synth->sd_noise_level = 0.0f;  // Low-pass noise level (TODO: user said "always on" - need value)
    synth->sd_lp_noise_cutoff = 0.15f;  // LP noise filter cutoff (lower = darker noise)
    synth->sd_res1_decay = 0.46f;  // Osc 1 decay time (seconds) - LONG sustain
    synth->sd_res2_decay = 0.05f;  // Osc 2 decay time (seconds) - VERY SHORT (maybe shouldn't exist)
    synth->sd_noise_decay = 0.180f;  // High-pass noise decay time (seconds)
    synth->sd_noise_attack = 5.0f;  // [NOT USED in Waldorf architecture]
    synth->sd_noise_fade_time = 250.0f;  // [NOT USED in Waldorf architecture]
    synth->sd_noise_fade_curve = 0.18f;  // [NOT USED in Waldorf architecture]

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

    // Initialize BD Sweep-Shape parameters (user-optimized defaults)
    synth->bd_squiggly_end_ms = 1.5f;  // Gradual + steep rise within same duration
    synth->bd_fast_end_ms = 10.1f;
    synth->bd_slow_end_ms = 31.65f;
    synth->bd_tail_slow_start_ms = 74.0f;
    synth->bd_squiggly_freq = 230.0f;
    synth->bd_fast_freq = 216.0f;
    synth->bd_slow_freq = 159.0f;
    synth->bd_tail_freq = 88.0f;
    synth->bd_tail_slow_freq = 53.0f;
    synth->bd_fast_saw_pct = 14.2f;
    synth->bd_slow_saw_pct = 6.0f;

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
        synth->voices[i].phase_offset = -1.0f;       // Not set yet
        synth->voices[i].tail_phase_offset = -1.0f;  // Not set yet
        synth->voices[i].tail_slow_offset = -1.0f;   // Not set yet
    }

    // Initialize modular drum voices
    rg909_bd_init(&synth->bd);
    rg909_sd_init(&synth->sd);

    // Set initial parameters for modular drums
    rg909_bd_set_level(&synth->bd, synth->bd_level);
    rg909_bd_set_tune(&synth->bd, synth->bd_tune);
    rg909_bd_set_decay(&synth->bd, synth->bd_decay);
    rg909_bd_set_attack(&synth->bd, synth->bd_attack);

    rg909_sd_set_level(&synth->sd, synth->sd_level);
    rg909_sd_set_tone(&synth->sd, synth->sd_tone);
    rg909_sd_set_snappy(&synth->sd, synth->sd_snappy);
    rg909_sd_set_tuning(&synth->sd, synth->sd_tuning);

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

    // Reset modular drums
    rg909_bd_reset(&synth->bd);
    rg909_sd_reset(&synth->sd);

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

    // Use modular drums for BD and SD
    if (type == RG909_DRUM_BD) {
        rg909_bd_trigger(&synth->bd, velocity, sample_rate);
        return;
    }
    if (type == RG909_DRUM_SD) {
        rg909_sd_trigger(&synth->sd, velocity, sample_rate);
        return;
    }

    // Find voice for other drum types (LT, MT, HT, RS, HC)
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
    v->phase_offset = -1.0f;       // Reset phase offset for new trigger
    v->tail_phase_offset = -1.0f;  // Reset tail phase offset for new trigger
    v->tail_slow_offset = -1.0f;   // Reset tail slow offset for new trigger

    // Configure drum voice based on TR-909 circuit topology
    // (BD and SD are handled by modular drums and returned early above)
    switch (type) {
        case RG909_DRUM_LT: {
            // Low Tom: Resonator-dominant with strong pitch sweep
            float base_freq = 73.5f + synth->lt_tuning * 55.0f;  // 73-129Hz
            v->base_freq = base_freq;
            v->sweep_pos = 0.0f;
            v->sweep_time = 0.15f;   // 150ms sweep
            v->sweep_amount = 3.5f;  // Pitch sweep multiplier

            float decay = 0.3f + synth->lt_decay * 0.3f;
            synth_resonator_reset(v->res1);
            synth_resonator_set_params(v->res1, base_freq, decay, sample_rate);
            synth_resonator_strike(v->res1, vel * 8.0f);  // Strong strike for punchy tom
            break;
        }

        case RG909_DRUM_MT: {
            // Mid Tom: Resonator-dominant with strong pitch sweep
            float base_freq = 92.0f + synth->mt_tuning * 73.5f;  // 92-165Hz
            v->base_freq = base_freq;
            v->sweep_pos = 0.0f;
            v->sweep_time = 0.15f;   // 150ms sweep
            v->sweep_amount = 5.75f;  // Pitch sweep multiplier

            float decay = 0.3f + synth->mt_decay * 0.3f;
            synth_resonator_reset(v->res1);
            synth_resonator_set_params(v->res1, base_freq, decay, sample_rate);
            synth_resonator_strike(v->res1, vel * 8.0f);  // Strong strike for punchy tom
            break;
        }

        case RG909_DRUM_HT: {
            // High Tom: Resonator-dominant with strong pitch sweep
            float base_freq = 129.0f + synth->ht_tuning * 92.0f;  // 129-221Hz
            v->base_freq = base_freq;
            v->sweep_pos = 0.0f;
            v->sweep_time = 0.12f;   // 120ms sweep
            v->sweep_amount = 1.85f;  // Pitch sweep multiplier

            float decay = 0.25f + synth->ht_decay * 0.25f;
            synth_resonator_reset(v->res1);
            synth_resonator_set_params(v->res1, base_freq, decay, sample_rate);
            synth_resonator_strike(v->res1, vel * 8.0f);  // Strong strike for punchy tom
            break;
        }

        case RG909_DRUM_RS: {
            // Rimshot: High frequency resonator (corrected for 44.1kHz vs 48kHz)
            float freq = 1838.0f + synth->rs_tuning * 919.0f;  // 1838-2757Hz (was 2000-3000Hz)
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

        // Process modular BD and SD
        float bd_sample = rg909_bd_process(&synth->bd, sample_rate) * synth->master_volume;
        float sd_sample = rg909_sd_process(&synth->sd, sample_rate) * synth->master_volume;
        mix_left += bd_sample + sd_sample;
        mix_right += bd_sample + sd_sample;

        for (int i = 0; i < RG909_MAX_VOICES; i++) {
            RG909DrumVoice* voice = &synth->voices[i];
            if (!voice->active) continue;

            float sample = 0.0f;

            switch (voice->type) {
                case RG909_DRUM_BD:
                case RG909_DRUM_SD:
                    // BD and SD are now handled by modular drums above, skip voice processing
                    voice->active = 0;  // Deactivate voice
                    break;


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

    // Clamp normalized parameters [0-1] only for basic parameters
    // Skip: PARAM_BD_LEVEL, extended SD params (PARAM_SD_TONE_GAIN onwards), and BD sweep-shape params
    if (param_index <= PARAM_MASTER_VOLUME) {
        // Standard parameters (0-1 range)
        if (param_index != PARAM_BD_LEVEL && param_index != PARAM_SD_LEVEL) {
            value = (value < 0.0f) ? 0.0f : (value > 1.0f) ? 1.0f : value;
        } else {
            value = (value < 0.0f) ? 0.0f : value;  // bd_level/sd_level: only clamp minimum
        }
    }
    // Extended SD parameters and sweep-shape parameters: no clamping needed

    switch (param_index) {
        case PARAM_BD_LEVEL:
            synth->bd_level = value;
            rg909_bd_set_level(&synth->bd, value);
            break;
        case PARAM_BD_TUNE:
            synth->bd_tune = value;
            rg909_bd_set_tune(&synth->bd, value);
            break;
        case PARAM_BD_DECAY:
            synth->bd_decay = value;
            rg909_bd_set_decay(&synth->bd, value);
            break;
        case PARAM_BD_ATTACK:
            synth->bd_attack = value;
            rg909_bd_set_attack(&synth->bd, value);
            break;
        case PARAM_SD_LEVEL:
            synth->sd_level = value;
            rg909_sd_set_level(&synth->sd, value / 44.0f);  // Normalize from 44.0 scale
            break;
        case PARAM_SD_TONE:
            synth->sd_tone = value;
            rg909_sd_set_tone(&synth->sd, value);
            break;
        case PARAM_SD_SNAPPY:
            synth->sd_snappy = value;
            rg909_sd_set_snappy(&synth->sd, value);
            break;
        case PARAM_SD_TUNING:
            synth->sd_tuning = value;
            rg909_sd_set_tuning(&synth->sd, value);
            break;
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
        // Extended SD Parameters
        case PARAM_SD_TONE_GAIN: synth->sd_tone_gain = value; break;
        case PARAM_SD_FREQ1: synth->sd_freq1 = value; break;
        case PARAM_SD_FREQ2: synth->sd_freq2 = value; break;
        case PARAM_SD_RES1_LEVEL: synth->sd_res1_level = value; break;
        case PARAM_SD_RES2_LEVEL: synth->sd_res2_level = value; break;
        case PARAM_SD_NOISE_LEVEL: synth->sd_noise_level = value; break;
        case PARAM_SD_LP_NOISE_CUTOFF: synth->sd_lp_noise_cutoff = value; break;
        case PARAM_SD_RES1_DECAY: synth->sd_res1_decay = value; break;
        case PARAM_SD_RES2_DECAY: synth->sd_res2_decay = value; break;
        case PARAM_SD_NOISE_DECAY: synth->sd_noise_decay = value; break;
        case PARAM_SD_NOISE_ATTACK: synth->sd_noise_attack = value; break;
        case PARAM_SD_NOISE_FADE_TIME: synth->sd_noise_fade_time = value; break;
        case PARAM_SD_NOISE_FADE_CURVE: synth->sd_noise_fade_curve = value; break;
        // BD Sweep-Shape Parameters (no clamping needed - these are specific values)
        case PARAM_BD_SQUIGGLY_END_MS: synth->bd_squiggly_end_ms = value; break;
        case PARAM_BD_FAST_END_MS: synth->bd_fast_end_ms = value; break;
        case PARAM_BD_SLOW_END_MS: synth->bd_slow_end_ms = value; break;
        case PARAM_BD_TAIL_SLOW_START_MS: synth->bd_tail_slow_start_ms = value; break;
        case PARAM_BD_SQUIGGLY_FREQ: synth->bd_squiggly_freq = value; break;
        case PARAM_BD_FAST_FREQ: synth->bd_fast_freq = value; break;
        case PARAM_BD_SLOW_FREQ: synth->bd_slow_freq = value; break;
        case PARAM_BD_TAIL_FREQ: synth->bd_tail_freq = value; break;
        case PARAM_BD_TAIL_SLOW_FREQ: synth->bd_tail_slow_freq = value; break;
        case PARAM_BD_FAST_SAW_PCT: synth->bd_fast_saw_pct = value; break;
        case PARAM_BD_SLOW_SAW_PCT: synth->bd_slow_saw_pct = value; break;
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
        // Extended SD Parameters
        case PARAM_SD_TONE_GAIN: return synth->sd_tone_gain;
        case PARAM_SD_FREQ1: return synth->sd_freq1;
        case PARAM_SD_FREQ2: return synth->sd_freq2;
        case PARAM_SD_RES1_LEVEL: return synth->sd_res1_level;
        case PARAM_SD_RES2_LEVEL: return synth->sd_res2_level;
        case PARAM_SD_NOISE_LEVEL: return synth->sd_noise_level;
        case PARAM_SD_LP_NOISE_CUTOFF: return synth->sd_lp_noise_cutoff;
        case PARAM_SD_RES1_DECAY: return synth->sd_res1_decay;
        case PARAM_SD_RES2_DECAY: return synth->sd_res2_decay;
        case PARAM_SD_NOISE_DECAY: return synth->sd_noise_decay;
        case PARAM_SD_NOISE_ATTACK: return synth->sd_noise_attack;
        case PARAM_SD_NOISE_FADE_TIME: return synth->sd_noise_fade_time;
        case PARAM_SD_NOISE_FADE_CURVE: return synth->sd_noise_fade_curve;
        // BD Sweep-Shape Parameters
        case PARAM_BD_SQUIGGLY_END_MS: return synth->bd_squiggly_end_ms;
        case PARAM_BD_FAST_END_MS: return synth->bd_fast_end_ms;
        case PARAM_BD_SLOW_END_MS: return synth->bd_slow_end_ms;
        case PARAM_BD_TAIL_SLOW_START_MS: return synth->bd_tail_slow_start_ms;
        case PARAM_BD_SQUIGGLY_FREQ: return synth->bd_squiggly_freq;
        case PARAM_BD_FAST_FREQ: return synth->bd_fast_freq;
        case PARAM_BD_SLOW_FREQ: return synth->bd_slow_freq;
        case PARAM_BD_TAIL_FREQ: return synth->bd_tail_freq;
        case PARAM_BD_TAIL_SLOW_FREQ: return synth->bd_tail_slow_freq;
        case PARAM_BD_FAST_SAW_PCT: return synth->bd_fast_saw_pct;
        case PARAM_BD_SLOW_SAW_PCT: return synth->bd_slow_saw_pct;
        default: return 0.0f;
    }
}
