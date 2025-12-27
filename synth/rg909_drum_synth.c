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
    v->phase_offset = -1.0f;       // Reset phase offset for new trigger
    v->tail_phase_offset = -1.0f;  // Reset tail phase offset for new trigger
    v->tail_slow_offset = -1.0f;   // Reset tail slow offset for new trigger

    // Configure drum voice based on TR-909 circuit topology
    switch (type) {
        case RG909_DRUM_BD: {
            // REAL 909 BD: Swept sine oscillator (corrected for 44.1kHz vs 48kHz ~8% difference)
            // Original was tuned at wrong sample rate - apply 0.91875 correction (44100/48000)
            float start_freq = 211.0f + synth->bd_tune * 46.0f;  // 211-257Hz (was 230-280Hz)
            float end_freq = 46.0f + synth->bd_tune * 9.0f;      // 46-55Hz (was 50-60Hz)

            v->base_freq = start_freq;   // Start frequency
            v->sweep_amount = end_freq;  // End frequency
            v->sweep_time = 0.060f;      // 60ms pitch sweep
            v->sweep_pos = 0.0f;

            // Amplitude envelope - TR-909: Fast attack, balanced decay
            v->decay_env = 1.0f;
            // Real 909 decay: 45-130ms (balanced)
            float decay_time = 0.045f + synth->bd_decay * 0.085f;  // 45-130ms decay
            v->decay_coeff = decay_time;

            // Phase accumulator for oscillator - start at 0.0 for positive-oriented start
            // sin(0) = 0, and after increment sin(2π*small) ≈ small (positive, going up)
            v->noise_env = 0.0f;
            break;
        }

        case RG909_DRUM_SD: {
            // Snare: Waldorf TR-909 architecture
            // - Osc 1 (res1): Constant pitch, slightly detuned
            // - Osc 2 (res2): Pitch envelope modulation
            // - Noise path 1: Low-pass filtered, always present (texture)
            // - Noise path 2: High-pass filtered, controlled by "Snappy"

            fprintf(stderr, "DEBUG: Snare trigger! vel=%.2f\n", vel);

            // Pitch sweep parameters (now applied to BOTH resonators)
            v->sweep_amount = 1.8f;          // Pitch multiplier at start (lower for less "tinny" sound)
            v->sweep_time = 0.012f;          // 12ms fast pitch envelope for snap
            v->sweep_pos = 0.0f;

            // Use configurable resonator frequencies
            float freq1 = synth->sd_freq1;  // Oscillator 1: constant pitch
            float freq2 = synth->sd_freq2;  // Oscillator 2: pitch modulated

            synth_resonator_reset(v->res1);
            synth_resonator_reset(v->res2);

            // BOTH resonators start at swept-up frequency for characteristic attack peaks
            synth_resonator_set_params(v->res1, freq1 * v->sweep_amount, synth->sd_res1_decay, sample_rate);
            synth_resonator_set_params(v->res2, freq2 * v->sweep_amount, synth->sd_res2_decay, sample_rate);

            // Use configurable resonator strikes
            synth_resonator_strike(v->res1, vel * synth->sd_res1_level);
            synth_resonator_strike(v->res2, vel * synth->sd_res2_level);

            // High-pass noise envelope (controlled by sd_snappy)
            v->noise_env = vel * synth->sd_snappy;  // HP noise level
            v->noise_decay = synth->sd_noise_decay;

            // High-pass filter for "snappy" noise (Waldorf architecture)
            synth_filter_set_type(v->filter, SYNTH_FILTER_HPF);
            synth_filter_set_cutoff(v->filter, 0.4f + synth->sd_tone * 0.3f);  // High-pass for snap
            synth_filter_set_resonance(v->filter, 0.5f);

            // Initialize LP noise filter state (simple one-pole)
            v->decay_env = 0.0f;  // Reuse as LP filter state
            break;
        }

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

        for (int i = 0; i < RG909_MAX_VOICES; i++) {
            RG909DrumVoice* voice = &synth->voices[i];
            if (!voice->active) continue;

            float sample = 0.0f;

            switch (voice->type) {
                case RG909_DRUM_BD: {
                    // REAL 909 BD: Swept sine oscillator with sweep-shape transient

                    // Convert timing parameters from ms to seconds
                    float squiggly_end = synth->bd_squiggly_end_ms / 1000.0f;
                    float fast_end = synth->bd_fast_end_ms / 1000.0f;
                    float slow_end = synth->bd_slow_end_ms / 1000.0f;
                    float tail_slow_start = synth->bd_tail_slow_start_ms / 1000.0f;

                    // Calculate phase increment based on current phase
                    float phase_inc;
                    float freq;

                    if (voice->sweep_pos < squiggly_end) {
                        // Phase 1: Squiggly
                        freq = synth->bd_squiggly_freq;
                    } else if (voice->sweep_pos < fast_end) {
                        // Phase 2a: Fast sweep-shape
                        freq = synth->bd_fast_freq;
                    } else if (voice->sweep_pos < slow_end) {
                        // Phase 2b: Slow sweep-shape
                        freq = synth->bd_slow_freq;
                    } else if (voice->sweep_pos < tail_slow_start) {
                        // Phase 3: Tail
                        freq = synth->bd_tail_freq;
                    } else {
                        // Phase 4: Tail slow
                        freq = synth->bd_tail_slow_freq;
                    }

                    phase_inc = freq / sample_rate;

                    // Continue sine phase accumulator throughout all phases
                    voice->noise_env += phase_inc;
                    if (voice->noise_env >= 1.0f) voice->noise_env -= 1.0f;

                    // TR-909 Waveform with Sweep-Shape Transient
                    // Phase 1 (0-1.5ms): Initial squiggly attack on SINE
                    // Phase 2a (1.5-10.1ms): Fast sweep-shape at 216 Hz, SAW 14.2%
                    // Phase 2b (10.1-31.5ms): Slow sweep-shape at 159 Hz, SAW 6.0%
                    // Phase 3 (31.5-74ms): Tail at 88 Hz with decay
                    // Phase 4 (74ms+): Tail slow at 53 Hz (PHASE INVERTED)

                    if (voice->sweep_pos < squiggly_end) {
                        // Phase 1: Two-stage squiggly within 1.5ms
                        // Stage 1: Gradual rise (67%)
                        // Stage 2: Steep rise (33%) with rectified sine
                        float gradual_phase_end = squiggly_end * 0.67f;  // 67% of squiggly duration

                        if (voice->sweep_pos < gradual_phase_end) {
                            // Stage 1: Gradual squiggly rise - SINE with envelope
                            float sine_val = sinf(2.0f * M_PI * voice->noise_env);
                            sample = sine_val;

                            float t = voice->sweep_pos / gradual_phase_end;
                            float amp_env = 0.18f * powf(t, 0.8f);
                            sample = sample * amp_env;
                        } else {
                            // Stage 2: Steep rise/punch - from 0.18 to 0.97
                            // Use rectified sine with saturation for analog character
                            float sine_val = sinf(2.0f * M_PI * voice->noise_env);
                            sample = fabsf(sine_val);  // Full-wave rectification - always positive

                            // Add soft saturation for warmth/character
                            sample = tanhf(sample * 1.3f);

                            float rise_duration = squiggly_end - gradual_phase_end;
                            float t = (voice->sweep_pos - gradual_phase_end) / rise_duration;

                            // Exponential rising envelope (starts gradual, finishes steep)
                            float amp_env = 0.18f + (0.97f - 0.18f) * powf(t, 2.5f);
                            sample = sample * amp_env;
                        }

                    } else if (voice->sweep_pos < slow_end) {
                        // Phase 2: Two-stage sweep-shape
                        // Phase 2a: Fast sweep with SAW%
                        // Phase 2b: Slow sweep with SAW%
                        int is_slow_sweep = 0;

                        if (voice->sweep_pos < fast_end) {
                            // Fast sweep
                            is_slow_sweep = 0;
                        } else {
                            // Slow sweep
                            is_slow_sweep = 1;
                        }

                        // On first entry to sweep-shape, capture phase offset to ensure positive start
                        if (voice->phase_offset < 0.0f) {
                            voice->phase_offset = voice->noise_env;
                        }

                        // Use accumulated phase with offset - same offset for both fast and slow
                        // Use fmodf to properly wrap phase to [0, 1) range
                        float u = fmodf(voice->noise_env - voice->phase_offset + 10.0f, 1.0f);

                        // Sweep-shape waveform (SAW → COSINE down → SAW → COSINE up)
                        // Use configurable SAW widths
                        float saw_width = is_slow_sweep ? (synth->bd_slow_saw_pct / 100.0f) : (synth->bd_fast_saw_pct / 100.0f);
                        float cosine_width = (0.5f - saw_width);

                        // Sweep-shape waveform generation
                        if (u < saw_width) {
                            // Quarter 1: SAW fade (top)
                            float t_quarter = u / saw_width;
                            sample = 0.90f + (0.85f - 0.90f) * t_quarter;
                        } else if (u < 0.5f) {
                            // Quarter 2: COSINE sweep DOWN
                            float t_quarter = (u - saw_width) / cosine_width;
                            float c = cosf(t_quarter * M_PI);
                            sample = 0.5f * (0.85f + (-0.85f)) + 0.5f * (0.85f - (-0.85f)) * c;
                        } else if (u < 0.5f + saw_width) {
                            // Quarter 3: SAW fade (bottom)
                            float t_quarter = (u - 0.5f) / saw_width;
                            sample = -0.85f + ((-0.80f) - (-0.85f)) * t_quarter;
                        } else {
                            // Quarter 4: COSINE sweep UP
                            float t_quarter = (u - 0.5f - saw_width) / cosine_width;
                            float c = cosf((1.0f - t_quarter) * M_PI);
                            sample = 0.5f * ((-0.80f) + 0.90f) + 0.5f * (0.90f - (-0.80f)) * c;
                        }
                        // NO decay envelope! Full amplitude sweep-shape

                    } else {
                        // Phase 3 & 4: Tail continuation with triangular-sine
                        float tri_phase;
                        float phase_invert;

                        if (voice->sweep_pos < tail_slow_start) {
                            // Phase 3: Tail (normal, not inverted)
                            phase_invert = 1.0f;

                            // On first entry to tail, capture phase offset to start at zero crossing
                            if (voice->tail_phase_offset < 0.0f) {
                                // Triangle waveform crosses zero at phase 0.25 (upward slope)
                                voice->tail_phase_offset = voice->noise_env - 0.25f;
                                fprintf(stderr, "DEBUG: Captured tail_phase_offset=%.6f at sweep_pos=%.6f\n",
                                        voice->tail_phase_offset, voice->sweep_pos);
                            }

                            // Use accumulated phase with offset to start at zero crossing
                            tri_phase = fmodf(voice->noise_env - voice->tail_phase_offset + 10.0f, 1.0f);

                        } else {
                            // Phase 4: Tail slow (PHASE AND DIRECTION INVERTED)
                            // Invert both amplitude AND sweep direction by shifting phase by 0.5
                            phase_invert = -1.0f;

                            // Debug first entry
                            static int first_tail_slow_entry = 1;
                            if (first_tail_slow_entry) {
                                float current_tri_phase = fmodf(voice->noise_env - voice->tail_phase_offset + 10.0f, 1.0f);
                                float inverted_tri_phase = fmodf(current_tri_phase + 0.5f, 1.0f);
                                float current_triangle = 4.0f * fabsf(current_tri_phase - 0.5f) - 1.0f;
                                float inverted_triangle = 4.0f * fabsf(inverted_tri_phase - 0.5f) - 1.0f;
                                fprintf(stderr, "DEBUG: tail_slow START at sweep_pos=%.6f\n", voice->sweep_pos);
                                fprintf(stderr, "       original: tri_phase=%.6f, triangle=%+.6f\n", current_tri_phase, current_triangle);
                                fprintf(stderr, "       inverted: tri_phase=%.6f, triangle=%+.6f\n", inverted_tri_phase, inverted_triangle);
                                first_tail_slow_entry = 0;
                            }

                            // Shift phase by 0.5 (half cycle) to invert sweep direction, then invert amplitude
                            float base_tri_phase = fmodf(voice->noise_env - voice->tail_phase_offset + 10.0f, 1.0f);
                            tri_phase = fmodf(base_tri_phase + 0.5f, 1.0f);
                        }

                        // Generate triangular-sine waveform
                        float triangle = 4.0f * fabsf(tri_phase - 0.5f) - 1.0f;
                        float clipped = tanhf(triangle * 1.5f);
                        sample = (triangle * 0.20f + clipped * 0.80f) * phase_invert;

                        // Two-stage decay envelope
                        float amp_env;
                        float sustain_end = slow_end + 0.0085f;  // 8.5ms after sweep end
                        if (voice->sweep_pos < sustain_end) {
                            // Sustain phase: 0.98 → 0.88
                            float t = (voice->sweep_pos - slow_end) / 0.0085f;
                            if (t < 0.0f) t = 0.0f;
                            if (t > 1.0f) t = 1.0f;
                            amp_env = 0.98f - (0.10f * t);  // 0.98 → 0.88 over 8.5ms
                        } else {
                            // Slow decay
                            float t_decay = voice->sweep_pos - sustain_end;
                            float decay_time = 0.045f + synth->bd_decay * 0.085f;
                            float k = 0.6f / decay_time;
                            amp_env = 0.88f * expf(-k * t_decay);

                            if (amp_env < 0.0001f) voice->active = 0;
                        }

                        sample = sample * amp_env;
                    }

                    // Output gain - match real 909 RMS level
                    sample = sample * 2.0f;

                    voice->sweep_pos += 1.0f / sample_rate;
                    sample *= synth->bd_level;
                    break;
                }

                case RG909_DRUM_SD: {
                    // Waldorf TR-909 Snare Architecture:
                    // - Res1: Constant pitch (180Hz)
                    // - Res2: Pitch envelope modulation (330Hz → swept)
                    // - LP noise: Always present (subtle texture)
                    // - HP noise: Snappy-controlled envelope

                    // Pitch envelope for ONLY res2 (Waldorf: one oscillator modulated)
                    float pitch_env = 1.0f;
                    if (voice->sweep_pos < voice->sweep_time) {
                        float t = voice->sweep_pos / voice->sweep_time;
                        // Linear sweep from sweep_amount (2.5x) down to 1.0x
                        pitch_env = voice->sweep_amount - (voice->sweep_amount - 1.0f) * t;
                    }

                    // Update BOTH resonator frequencies with pitch envelope for characteristic peaks
                    float freq1 = synth->sd_freq1 * pitch_env;
                    float freq2 = synth->sd_freq2 * pitch_env;
                    synth_resonator_set_params(voice->res1, freq1, synth->sd_res1_decay, sample_rate);
                    synth_resonator_set_params(voice->res2, freq2, synth->sd_res2_decay, sample_rate);

                    // Get tone from BOTH resonators (both swept together)
                    float t1 = synth_resonator_process(voice->res1, 0.0f);
                    float t2 = synth_resonator_process(voice->res2, 0.0f);
                    float tone = (t1 + t2) * 0.5f * synth->sd_tone_gain;

                    // Generate raw noise once
                    float noise_raw = synth_noise_process(voice->noise);

                    // NOISE PATH 1: Low-pass filtered, always present (Waldorf texture)
                    // Simple one-pole lowpass: y[n] = y[n-1] + k*(x[n] - y[n-1])
                    float lp_cutoff = synth->sd_lp_noise_cutoff;  // User-controllable cutoff (lower = darker)
                    voice->decay_env = voice->decay_env + lp_cutoff * (noise_raw - voice->decay_env);
                    float noise_lp = voice->decay_env * synth->sd_noise_level;  // Direct control (removed 0.03 multiplier)

                    // NOISE PATH 2: High-pass filtered, envelope controlled (Waldorf "Snappy")
                    // HP noise envelope with exponential decay
                    voice->noise_env -= voice->noise_env * (1.0f / (voice->noise_decay * sample_rate));
                    if (voice->noise_env < 0.0001f) voice->noise_env = 0.0f;

                    float noise_hp = synth_filter_process(voice->filter, noise_raw, sample_rate);
                    noise_hp *= voice->noise_env;  // Envelope controlled by sd_snappy

                    // Combine: Tone + LP noise (always) + HP noise (snappy)
                    sample = tone + noise_lp + noise_hp;

                    // MASTER AMPLITUDE DECAY ENVELOPE (fixed decay, not controlled by snappy)
                    // Instant attack, exponential decay for overall snare envelope
                    float decay_time = 0.120f;  // Fixed 120ms master decay
                    float amp_env = expf(-3.0f * voice->sweep_pos / decay_time);

                    sample *= amp_env * synth->sd_level;

                    // Update time
                    voice->sweep_pos += 1.0f / sample_rate;

                    // Deactivate when amplitude envelope has decayed
                    if (voice->sweep_pos > 0.300f || amp_env < 0.001f) {
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
