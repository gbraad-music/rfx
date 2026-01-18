/**
 * RGSlicer Implementation
 * Core slicing sampler engine
 */

#include "rgslicer.h"
#include "sample_fx.h"
#include "wav_loader.h"
#include "wav_cue.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>

// ============================================================================
// Lifecycle
// ============================================================================

RGSlicer* rgslicer_create(uint32_t sample_rate) {
    RGSlicer* slicer = (RGSlicer*)calloc(1, sizeof(RGSlicer));
    if (!slicer) return NULL;

    slicer->target_sample_rate = sample_rate;
    slicer->sample_data = NULL;
    slicer->sample_loaded = false;
    slicer->num_slices = 0;

    // Default parameters
    slicer->master_pitch = 0.0f;
    slicer->master_time = 1.0f;
    slicer->master_volume = 1.0f;
    slicer->root_note = 60;

    strcpy(slicer->sample_name, "Untitled");

    // Initialize note mapping (0xFF = unmapped)
    memset(slicer->note_map, 0xFF, sizeof(slicer->note_map));
    slicer->use_note_map = false;

    // Initialize random sequencer
    slicer->random_seq_active = false;
    slicer->random_seq_phase = 0;
    rgslicer_set_bpm(slicer, 125.0f);  // Sets BPM and calculates interval

    // Initialize voices
    for (int i = 0; i < RGSLICER_MAX_VOICES; i++) {
        slicer->voices[i].active = false;
        slicer->voices[i].fx = sample_fx_create(sample_rate);
    }

    return slicer;
}

void rgslicer_destroy(RGSlicer* slicer) {
    if (!slicer) return;

    // Free sample data
    if (slicer->sample_data) {
        free(slicer->sample_data);
    }

    // Free voice FX
    for (int i = 0; i < RGSLICER_MAX_VOICES; i++) {
        if (slicer->voices[i].fx) {
            sample_fx_destroy(slicer->voices[i].fx);
        }
    }

    free(slicer);
}

void rgslicer_reset(RGSlicer* slicer) {
    if (!slicer) return;

    // Stop all voices
    for (int i = 0; i < RGSLICER_MAX_VOICES; i++) {
        slicer->voices[i].active = false;
        if (slicer->voices[i].fx) {
            sample_fx_reset(slicer->voices[i].fx);
        }
    }

    slicer->voice_allocator = 0;
}

// ============================================================================
// Sample Loading
// ============================================================================

int rgslicer_load_sample(RGSlicer* slicer, const char* wav_path) {
    if (!slicer) return 0;

    // Use wav_loader to load file
    WAVSample* wav = wav_load_file(wav_path);
    if (!wav) {
        printf("[RGSlicer] Failed to load WAV: %s\n", wav_path);
        return 0;
    }

    // Load into slicer
    int result = rgslicer_load_sample_memory(slicer, wav->pcm_data, wav->num_samples, wav->sample_rate);

    // Extract filename for name
    const char* name = strrchr(wav_path, '/');
    if (!name) name = strrchr(wav_path, '\\');
    if (name) {
        name++;  // Skip slash
        strncpy(slicer->sample_name, name, RGSLICER_MAX_NAME - 1);
    }

    // Free WAV structure (but not pcm_data, we copied it)
    free(wav);

    if (!result) return 0;

    // PRIORITY: Try to load CUE points from WAV file
    WAVCueData* cue_data = wav_cue_read(wav_path);
    if (cue_data && cue_data->num_points > 0) {
        printf("[RGSlicer] Found %u CUE points in WAV file\n", cue_data->num_points);

        // Clear existing slices
        rgslicer_clear_slices(slicer);

        // Extract slice offsets from CUE points
        uint32_t slice_offsets[RGSLICER_MAX_SLICES];
        uint8_t num_slices = wav_cue_extract_slices(cue_data, slice_offsets, RGSLICER_MAX_SLICES);

        // Create slices from CUE points
        for (uint8_t i = 0; i < num_slices; i++) {
            rgslicer_add_slice(slicer, slice_offsets[i]);
        }

        printf("[RGSlicer] Created %d slices from CUE points\n", num_slices);

        wav_cue_free(cue_data);
    } else {
        printf("[RGSlicer] No CUE points found - use auto-slice or load SFZ\n");
    }

    return result;
}

int rgslicer_load_sample_memory(RGSlicer* slicer, int16_t* data, uint32_t num_samples, uint32_t sample_rate) {
    if (!slicer || !data || num_samples == 0) return 0;

    // Free existing sample
    rgslicer_unload_sample(slicer);

    // Allocate and copy sample data
    slicer->sample_data = (int16_t*)malloc(num_samples * sizeof(int16_t));
    if (!slicer->sample_data) {
        printf("[RGSlicer] Failed to allocate sample memory\n");
        return 0;
    }

    memcpy(slicer->sample_data, data, num_samples * sizeof(int16_t));
    slicer->sample_length = num_samples;
    slicer->sample_rate = sample_rate;
    slicer->sample_loaded = true;

    printf("[RGSlicer] Loaded sample: %u samples @ %u Hz\n", num_samples, sample_rate);

    return 1;
}

void rgslicer_unload_sample(RGSlicer* slicer) {
    if (!slicer) return;

    if (slicer->sample_data) {
        free(slicer->sample_data);
        slicer->sample_data = NULL;
    }

    slicer->sample_loaded = false;
    slicer->sample_length = 0;
    slicer->num_slices = 0;

    rgslicer_reset(slicer);
}

bool rgslicer_has_sample(RGSlicer* slicer) {
    return slicer && slicer->sample_loaded;
}

// ============================================================================
// Slicing - Basic Operations
// ============================================================================

int rgslicer_add_slice(RGSlicer* slicer, uint32_t offset) {
    if (!slicer || slicer->num_slices >= RGSLICER_MAX_SLICES) return -1;
    if (!slicer->sample_loaded || offset >= slicer->sample_length) return -1;

    uint8_t index = slicer->num_slices;

    // Calculate length (to next slice or end of sample)
    uint32_t length = slicer->sample_length - offset;
    if (index > 0) {
        // Update previous slice length
        slicer->slices[index - 1].length = offset - slicer->slices[index - 1].offset;
        slicer->slices[index - 1].end = offset;
    }

    // Initialize new slice
    slicer->slices[index].offset = offset;
    slicer->slices[index].length = length;
    slicer->slices[index].end = offset + length;
    slicer->slices[index].pitch_semitones = 0.0f;
    slicer->slices[index].time_stretch = 1.0f;
    slicer->slices[index].volume = 1.0f;
    slicer->slices[index].pan = 0.0f;
    slicer->slices[index].reverse = false;
    slicer->slices[index].loop = false;
    slicer->slices[index].one_shot = false;  // Default: respect note-off

    slicer->num_slices++;

    return index;
}

void rgslicer_remove_slice(RGSlicer* slicer, uint8_t slice_index) {
    if (!slicer || slice_index >= slicer->num_slices) return;

    // Shift slices down
    for (uint8_t i = slice_index; i < slicer->num_slices - 1; i++) {
        slicer->slices[i] = slicer->slices[i + 1];
    }

    slicer->num_slices--;

    // Recalculate last slice length
    if (slicer->num_slices > 0) {
        uint8_t last = slicer->num_slices - 1;
        slicer->slices[last].length = slicer->sample_length - slicer->slices[last].offset;
        slicer->slices[last].end = slicer->sample_length;
    }
}

void rgslicer_move_slice(RGSlicer* slicer, uint8_t slice_index, uint32_t new_offset) {
    if (!slicer || slice_index >= slicer->num_slices) return;
    if (new_offset >= slicer->sample_length) return;

    slicer->slices[slice_index].offset = new_offset;

    // Recalculate lengths
    for (uint8_t i = 0; i < slicer->num_slices; i++) {
        if (i < slicer->num_slices - 1) {
            slicer->slices[i].length = slicer->slices[i + 1].offset - slicer->slices[i].offset;
            slicer->slices[i].end = slicer->slices[i + 1].offset;
        } else {
            slicer->slices[i].length = slicer->sample_length - slicer->slices[i].offset;
            slicer->slices[i].end = slicer->sample_length;
        }
    }
}

void rgslicer_clear_slices(RGSlicer* slicer) {
    if (!slicer) return;
    slicer->num_slices = 0;

    // Reset note mapping
    memset(slicer->note_map, 0xFF, sizeof(slicer->note_map));
    slicer->use_note_map = false;
}

uint8_t rgslicer_get_num_slices(RGSlicer* slicer) {
    return slicer ? slicer->num_slices : 0;
}

// ============================================================================
// Per-Slice Parameters
// ============================================================================

void rgslicer_set_slice_pitch(RGSlicer* slicer, uint8_t slice_index, float semitones) {
    if (!slicer || slice_index >= slicer->num_slices) return;
    if (semitones < -12.0f) semitones = -12.0f;
    if (semitones > 12.0f) semitones = 12.0f;
    slicer->slices[slice_index].pitch_semitones = semitones;
}

void rgslicer_set_slice_time(RGSlicer* slicer, uint8_t slice_index, float ratio) {
    if (!slicer || slice_index >= slicer->num_slices) return;
    if (ratio < 0.5f) ratio = 0.5f;
    if (ratio > 2.0f) ratio = 2.0f;
    slicer->slices[slice_index].time_stretch = ratio;
}

void rgslicer_set_slice_volume(RGSlicer* slicer, uint8_t slice_index, float volume) {
    if (!slicer || slice_index >= slicer->num_slices) return;
    if (volume < 0.0f) volume = 0.0f;
    if (volume > 2.0f) volume = 2.0f;
    slicer->slices[slice_index].volume = volume;
}

void rgslicer_set_slice_pan(RGSlicer* slicer, uint8_t slice_index, float pan) {
    if (!slicer || slice_index >= slicer->num_slices) return;
    if (pan < -1.0f) pan = -1.0f;
    if (pan > 1.0f) pan = 1.0f;
    slicer->slices[slice_index].pan = pan;
}

void rgslicer_set_slice_reverse(RGSlicer* slicer, uint8_t slice_index, bool reverse) {
    if (!slicer || slice_index >= slicer->num_slices) return;
    slicer->slices[slice_index].reverse = reverse;
}

void rgslicer_set_slice_loop(RGSlicer* slicer, uint8_t slice_index, bool loop) {
    if (!slicer || slice_index >= slicer->num_slices) return;
    slicer->slices[slice_index].loop = loop;
}

void rgslicer_set_slice_one_shot(RGSlicer* slicer, uint8_t slice_index, bool one_shot) {
    if (!slicer || slice_index >= slicer->num_slices) return;
    slicer->slices[slice_index].one_shot = one_shot;
}

float rgslicer_get_slice_pitch(RGSlicer* slicer, uint8_t slice_index) {
    if (!slicer || slice_index >= slicer->num_slices) return 0.0f;
    return slicer->slices[slice_index].pitch_semitones;
}

float rgslicer_get_slice_time(RGSlicer* slicer, uint8_t slice_index) {
    if (!slicer || slice_index >= slicer->num_slices) return 1.0f;
    return slicer->slices[slice_index].time_stretch;
}

float rgslicer_get_slice_volume(RGSlicer* slicer, uint8_t slice_index) {
    if (!slicer || slice_index >= slicer->num_slices) return 1.0f;
    return slicer->slices[slice_index].volume;
}

float rgslicer_get_slice_pan(RGSlicer* slicer, uint8_t slice_index) {
    if (!slicer || slice_index >= slicer->num_slices) return 0.0f;
    return slicer->slices[slice_index].pan;
}

uint32_t rgslicer_get_slice_offset(RGSlicer* slicer, uint8_t slice_index) {
    if (!slicer || slice_index >= slicer->num_slices) return 0;
    return slicer->slices[slice_index].offset;
}

uint32_t rgslicer_get_slice_length(RGSlicer* slicer, uint8_t slice_index) {
    if (!slicer || slice_index >= slicer->num_slices) return 0;
    return slicer->slices[slice_index].length;
}

// ============================================================================
// Global Parameters
// ============================================================================

void rgslicer_set_global_pitch(RGSlicer* slicer, float semitones) {
    if (!slicer) return;
    slicer->master_pitch = semitones;
}

void rgslicer_set_global_time(RGSlicer* slicer, float ratio) {
    if (!slicer) return;
    slicer->master_time = ratio;
}

void rgslicer_set_global_volume(RGSlicer* slicer, float volume) {
    if (!slicer) return;
    slicer->master_volume = volume;
}

float rgslicer_get_global_pitch(RGSlicer* slicer) {
    return slicer ? slicer->master_pitch : 0.0f;
}

float rgslicer_get_global_time(RGSlicer* slicer) {
    return slicer ? slicer->master_time : 1.0f;
}

float rgslicer_get_global_volume(RGSlicer* slicer) {
    return slicer ? slicer->master_volume : 1.0f;
}

void rgslicer_set_bpm(RGSlicer* slicer, float bpm) {
    if (!slicer) return;
    if (bpm < 20.0f) bpm = 20.0f;
    if (bpm > 300.0f) bpm = 300.0f;
    slicer->bpm = bpm;

    // Recalculate interval (quarter notes - one beat)
    slicer->random_seq_interval = (uint32_t)((60.0f / bpm) * slicer->target_sample_rate);
}

float rgslicer_get_bpm(RGSlicer* slicer) {
    return slicer ? slicer->bpm : 125.0f;
}

// ============================================================================
// MIDI / Playback
// ============================================================================

static int find_free_voice(RGSlicer* slicer) {
    // Find inactive voice
    for (int i = 0; i < RGSLICER_MAX_VOICES; i++) {
        if (!slicer->voices[i].active) {
            return i;
        }
    }

    // All voices active, steal oldest (round-robin)
    int voice = slicer->voice_allocator;
    slicer->voice_allocator = (slicer->voice_allocator + 1) % RGSLICER_MAX_VOICES;
    return voice;
}

// Map MIDI note to slice index (WHITE KEYS ONLY)
// White keys: C, D, E, F, G, A, B (7 per octave)
// Starting from C2 (MIDI 36): 36, 38, 40, 41, 43, 45, 47, 48, ...
static int note_to_slice_index(uint8_t note) {
    if (note < 36) return -1;

    // White key offsets within octave (C=0, D=2, E=4, F=5, G=7, A=9, B=11)
    // Returns slice offset within octave, or -1 for black keys
    const int8_t white_key_map[12] = {0, -1, 1, -1, 2, 3, -1, 4, -1, 5, -1, 6};

    int offset_from_c2 = note - 36;
    uint8_t octave = offset_from_c2 / 12;
    uint8_t note_in_octave = offset_from_c2 % 12;

    int8_t key_offset = white_key_map[note_in_octave];
    if (key_offset == -1) return -1;  // Black key

    return octave * 7 + key_offset;
}

void rgslicer_note_on(RGSlicer* slicer, uint8_t note, uint8_t velocity) {
    if (!slicer || !slicer->sample_loaded) return;

    // Special case: Note 37 (C#2/Db2) plays ENTIRE sample (preview mode)
    if (note == 37) {
        int voice_idx = find_free_voice(slicer);
        SliceVoice* v = &slicer->voices[voice_idx];

        v->active = true;
        v->slice_index = 0;  // Use slice 0 as template
        v->note = note;
        v->velocity = velocity;
        v->playback_pos = 0;  // Start at beginning of sample
        v->reverse = false;
        v->volume = velocity / 127.0f;

        // Configure FX with global parameters only
        if (v->fx) {
            sample_fx_reset(v->fx);
            sample_fx_set_pitch(v->fx, slicer->master_pitch);
            sample_fx_set_time_stretch(v->fx, slicer->master_time);
        }

        printf("[RGSlicer] Note On: %d (FULL SAMPLE, voice %d)\n", note, voice_idx);
        return;
    }

    // Special case: Note 39 (Eb2/D#2) starts RANDOM SLICE SEQUENCER
    if (note == 39) {
        if (slicer->num_slices == 0) return;
        slicer->random_seq_active = true;
        slicer->random_seq_phase = slicer->random_seq_interval; // Trigger IMMEDIATELY on next buffer
        printf("[RGSlicer] Note On: %d (RANDOM SEQUENCER START @ %.1f BPM)\n", note, slicer->bpm);
        return;
    }

    // Determine slice index from note
    int slice_index;

    if (slicer->use_note_map && note < 128) {
        // CUE labels provided explicit note assignments - use them
        slice_index = slicer->note_map[note];
        if (slice_index == 0xFF) return;  // Unmapped note
    } else {
        // Default: Map WHITE KEYS to slices: C2, D2, E2, F2, G2, A2, B2, C3, ...
        slice_index = note_to_slice_index(note);
        if (slice_index < 0 || slice_index >= slicer->num_slices) return;
    }

    // MONOPHONIC MODE for one-shot slices: Stop all voices playing this slice
    // This prevents overlapping when retriggering one-shot slices (important for breakbeats!)
    SliceData* slice = &slicer->slices[slice_index];
    if (slice->one_shot) {
        for (int i = 0; i < RGSLICER_MAX_VOICES; i++) {
            if (slicer->voices[i].active && slicer->voices[i].slice_index == slice_index) {
                slicer->voices[i].active = false;
            }
        }
    }

    // Get free voice
    int voice_idx = find_free_voice(slicer);
    SliceVoice* v = &slicer->voices[voice_idx];

    // Initialize voice
    v->active = true;
    v->slice_index = slice_index;
    v->note = note;
    v->velocity = velocity;
    v->playback_pos = slicer->slices[slice_index].offset;
    v->reverse = slicer->slices[slice_index].reverse;
    v->volume = velocity / 127.0f;

    // Configure FX (slice pointer already defined above)
    if (v->fx) {
        sample_fx_reset(v->fx);
        sample_fx_set_pitch(v->fx, slice->pitch_semitones + slicer->master_pitch);
        sample_fx_set_time_stretch(v->fx, slice->time_stretch * slicer->master_time);
    }

    printf("[RGSlicer] Note On: %d (slice %d, voice %d)\n", note, slice_index, voice_idx);
}

void rgslicer_note_off(RGSlicer* slicer, uint8_t note) {
    if (!slicer) return;

    // Special case: Note 39 stops random sequencer
    if (note == 39) {
        slicer->random_seq_active = false;
        printf("[RGSlicer] Note Off: %d (RANDOM SEQUENCER STOP)\n", note);
        return;
    }

    // Find voices playing this note
    for (int i = 0; i < RGSLICER_MAX_VOICES; i++) {
        if (slicer->voices[i].active && slicer->voices[i].note == note) {
            SliceData* slice = &slicer->slices[slicer->voices[i].slice_index];

            // One-shot mode: ignore note-off, play to completion (or loop forever)
            // WARNING: one_shot + loop kills polyphony!
            if (slice->one_shot) {
                continue;  // Voice keeps playing
            }

            // Normal mode: respect note-off
            slicer->voices[i].active = false;
        }
    }
}

void rgslicer_all_notes_off(RGSlicer* slicer) {
    if (!slicer) return;

    for (int i = 0; i < RGSLICER_MAX_VOICES; i++) {
        slicer->voices[i].active = false;
    }
}

void rgslicer_process_f32(RGSlicer* slicer, float* buffer, uint32_t frames) {
    if (!slicer || !buffer) return;

    // Clear output buffer
    memset(buffer, 0, frames * 2 * sizeof(float));

    if (!slicer->sample_loaded) return;

    // Random slice sequencer (Note 39)
    if (slicer->random_seq_active && slicer->num_slices > 0) {
        slicer->random_seq_phase += frames;

        // Trigger on interval boundaries
        while (slicer->random_seq_phase >= slicer->random_seq_interval) {
            slicer->random_seq_phase -= slicer->random_seq_interval;

            // MONOPHONIC MODE: Stop all voices triggered by random sequencer
            // This prevents overlapping slices (important for breakbeats!)
            for (int i = 0; i < RGSLICER_MAX_VOICES; i++) {
                if (slicer->voices[i].active && slicer->voices[i].note == 39) {
                    slicer->voices[i].active = false;
                }
            }

            // Pick random slice
            uint8_t random_slice = (uint8_t)(rand() % slicer->num_slices);

            // Trigger it as a one-shot
            int voice_idx = find_free_voice(slicer);
            SliceVoice* v = &slicer->voices[voice_idx];

            v->active = true;
            v->slice_index = random_slice;
            v->note = 39;  // Mark as triggered by sequencer
            v->velocity = 100;  // Fixed velocity
            v->playback_pos = slicer->slices[random_slice].offset;
            v->reverse = slicer->slices[random_slice].reverse;
            v->volume = 0.8f;

            // Configure FX
            SliceData* slice = &slicer->slices[random_slice];
            if (v->fx) {
                sample_fx_reset(v->fx);
                sample_fx_set_pitch(v->fx, slice->pitch_semitones + slicer->master_pitch);
                sample_fx_set_time_stretch(v->fx, slice->time_stretch * slicer->master_time);
            }
        }
    }

    // Process each active voice
    for (int v = 0; v < RGSLICER_MAX_VOICES; v++) {
        SliceVoice* voice = &slicer->voices[v];
        if (!voice->active) continue;

        SliceData* slice = &slicer->slices[voice->slice_index];

        for (uint32_t f = 0; f < frames; f++) {
            // Determine playback boundaries (full sample for note 37, else slice boundaries)
            uint32_t playback_end = (voice->note == 37) ? slicer->sample_length : slice->end;
            uint32_t playback_start = (voice->note == 37) ? 0 : slice->offset;

            // Check if playback finished
            if (!voice->reverse && voice->playback_pos >= playback_end) {
                if (slice->loop && voice->note != 37) {  // Note 37 never loops
                    voice->playback_pos = playback_start;  // Loop back
                } else {
                    voice->active = false;
                    break;
                }
            }
            if (voice->reverse && voice->playback_pos <= playback_start) {
                if (slice->loop && voice->note != 37) {
                    voice->playback_pos = playback_end - 1;  // Loop back
                } else {
                    voice->active = false;
                    break;
                }
            }

            // Get sample
            int16_t raw_sample = slicer->sample_data[voice->playback_pos];

            // Apply FX if active
            int16_t processed_sample = raw_sample;
            if (voice->fx && (sample_fx_is_active(voice->fx))) {
                processed_sample = sample_fx_process_sample(voice->fx, raw_sample);
            }

            // Convert to float and apply volume
            float sample_f32 = (float)processed_sample / 32768.0f;
            sample_f32 *= slice->volume * voice->volume * slicer->master_volume;

            // Apply pan (stereo)
            float left = sample_f32 * (1.0f - fmaxf(0.0f, slice->pan));
            float right = sample_f32 * (1.0f + fminf(0.0f, slice->pan));

            // Mix into output buffer
            buffer[f * 2] += left;
            buffer[f * 2 + 1] += right;

            // Advance playback position
            if (voice->reverse) {
                voice->playback_pos--;
            } else {
                voice->playback_pos++;
            }
        }
    }
}

// ============================================================================
// WAV+CUE Export
// ============================================================================

int rgslicer_export_wav_cue(RGSlicer* slicer, const char* output_path) {
    if (!slicer || !slicer->sample_loaded || !output_path) {
        printf("[RGSlicer] Cannot export WAV+CUE: no sample loaded\n");
        return 0;
    }

    if (slicer->num_slices == 0) {
        printf("[RGSlicer] No slices to export\n");
        return 0;
    }

    // Collect slice offsets
    uint32_t slice_offsets[RGSLICER_MAX_SLICES];
    uint32_t loop_offsets[RGSLICER_MAX_SLICES];

    for (uint8_t i = 0; i < slicer->num_slices; i++) {
        slice_offsets[i] = slicer->slices[i].offset;

        // If slice has loop enabled, add loop point at slice end
        // (loop point will be used for sustain/decay like RG1Piano)
        if (slicer->slices[i].loop) {
            loop_offsets[i] = slicer->slices[i].end;
        } else {
            loop_offsets[i] = 0;  // No loop point
        }
    }

    // Create CUE data with MIDI note labels (starting at C1 = 36)
    WAVCueData* cue_data = wav_cue_create_from_slices(
        slice_offsets,
        slicer->num_slices,
        loop_offsets,
        36  // Start at MIDI note 36 (C1)
    );

    if (!cue_data) {
        printf("[RGSlicer] Failed to create CUE data\n");
        return 0;
    }

    // Write WAV file with embedded CUE points
    int result = wav_cue_write(
        slicer->sample_data,
        slicer->sample_length,
        slicer->sample_rate,
        cue_data,
        output_path
    );

    wav_cue_free(cue_data);

    if (result) {
        printf("[RGSlicer] Exported WAV+CUE: %s (%u slices)\n",
               output_path, slicer->num_slices);
    } else {
        printf("[RGSlicer] Failed to write WAV+CUE file\n");
    }

    return result;
}

// ============================================================================
// Metadata
// ============================================================================

void rgslicer_set_name(RGSlicer* slicer, const char* name) {
    if (!slicer || !name) return;
    strncpy(slicer->sample_name, name, RGSLICER_MAX_NAME - 1);
    slicer->sample_name[RGSLICER_MAX_NAME - 1] = '\0';
}

void rgslicer_set_root_note(RGSlicer* slicer, uint8_t note) {
    if (!slicer) return;
    slicer->root_note = note;
}

const char* rgslicer_get_name(RGSlicer* slicer) {
    return slicer ? slicer->sample_name : "";
}

uint8_t rgslicer_get_root_note(RGSlicer* slicer) {
    return slicer ? slicer->root_note : 60;
}
