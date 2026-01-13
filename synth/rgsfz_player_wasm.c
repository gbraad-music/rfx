/**
 * RGSFZ Player - WebAssembly Wrapper
 * Exposes SFZ sampler functionality to JavaScript
 */

#include "sfz_parser.h"
#include "synth_sample_player.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>

#define MAX_VOICES 16

typedef struct {
    SynthSamplePlayer* player;
    const SFZRegion* region;
    uint8_t note;
    bool active;
} SFZVoice;

typedef struct {
    SFZData* sfz;
    SFZVoice voices[MAX_VOICES];
    float volume;
    float pan;
    float decay;
    uint32_t sample_rate;
} RGSFZPlayer;

// ============================================================================
// Player Management
// ============================================================================

RGSFZPlayer* rgsfz_player_create_wasm(uint32_t sample_rate) {
    RGSFZPlayer* player = (RGSFZPlayer*)calloc(1, sizeof(RGSFZPlayer));
    if (!player) return NULL;

    player->sample_rate = sample_rate;
    player->volume = 0.8f;
    player->pan = 0.0f;
    player->decay = 0.5f;

    // Create SFZ data structure (for JavaScript-based parsing)
    player->sfz = (SFZData*)calloc(1, sizeof(SFZData));
    if (!player->sfz) {
        free(player);
        return NULL;
    }
    player->sfz->num_regions = 0;

    // Create voice sample players
    for (int i = 0; i < MAX_VOICES; i++) {
        player->voices[i].player = synth_sample_player_create();
        player->voices[i].region = NULL;
        player->voices[i].note = 0;
        player->voices[i].active = false;
    }

    printf("[RGSFZ C] Player created, sfz allocated\n");
    return player;
}

void rgsfz_player_destroy_wasm(RGSFZPlayer* player) {
    if (!player) return;

    for (int i = 0; i < MAX_VOICES; i++) {
        if (player->voices[i].player) {
            synth_sample_player_destroy(player->voices[i].player);
        }
    }

    if (player->sfz) {
        sfz_free(player->sfz);
    }

    free(player);
}

// ============================================================================
// SFZ Loading (from memory, for web usage)
// ============================================================================

/**
 * Load SFZ from memory buffer (uses C SFZ parser!)
 * data: Pointer to SFZ file content (null-terminated string)
 * length: Length of data
 * Returns: 1 on success, 0 on failure
 */
int rgsfz_player_load_sfz_from_memory(RGSFZPlayer* player, const char* data, uint32_t length) {
    if (!player || !data) {
        printf("[RGSFZ C] load_sfz_from_memory: player or data is NULL\n");
        return 0;
    }

    // Free existing SFZ data
    if (player->sfz) {
        sfz_free(player->sfz);
        player->sfz = NULL;
    }

    // Parse SFZ using the ACTUAL C parser
    player->sfz = sfz_parse_from_memory(data, length);
    if (!player->sfz) {
        printf("[RGSFZ C] load_sfz_from_memory: sfz_parse_from_memory failed\n");
        return 0;
    }

    printf("[RGSFZ C] load_sfz_from_memory: Successfully parsed %d regions\n", player->sfz->num_regions);
    return 1;
}

/**
 * Add a region to the SFZ player (called from JavaScript after parsing)
 */
int rgsfz_player_add_region(RGSFZPlayer* player,
                              const char* sample_path,
                              uint8_t lokey, uint8_t hikey,
                              uint8_t lovel, uint8_t hivel,
                              uint8_t pitch_keycenter,
                              float pan,
                              uint32_t offset, uint32_t end) {
    if (!player || !player->sfz) {
        printf("[RGSFZ C] add_region: player or sfz is NULL\n");
        return 0;
    }
    if (player->sfz->num_regions >= SFZ_MAX_REGIONS) {
        printf("[RGSFZ C] add_region: MAX_REGIONS reached\n");
        return 0;
    }

    SFZRegion* r = &player->sfz->regions[player->sfz->num_regions];
    strncpy(r->sample_path, sample_path, SFZ_MAX_PATH - 1);
    r->lokey = lokey;
    r->hikey = hikey;
    r->lovel = lovel;
    r->hivel = hivel;
    r->pitch_keycenter = pitch_keycenter;
    r->pan = pan;
    r->offset = offset;
    r->end = end;
    r->loop_mode = false;  // Default no loop
    r->sample_data = NULL;
    r->sample_length = 0;
    r->sample_rate = 0;

    player->sfz->num_regions++;
    printf("[RGSFZ C] add_region: Added region %d [%d-%d] vel[%d-%d] pitch=%d\n",
           player->sfz->num_regions, lokey, hikey, lovel, hivel, pitch_keycenter);
    return 1;
}

/**
 * Load a WAV sample for a specific region (called from JavaScript)
 * region_index: Index of the region (0-based)
 * sample_data: Pointer to int16_t PCM data
 * length: Number of samples
 * sample_rate: Sample rate in Hz
 */
int rgsfz_player_load_region_sample(RGSFZPlayer* player, uint32_t region_index,
                                      int16_t* sample_data, uint32_t length,
                                      uint32_t sample_rate) {
    if (!player || !player->sfz) {
        printf("[RGSFZ C] load_region_sample: player or sfz is NULL\n");
        return 0;
    }
    if (region_index >= player->sfz->num_regions) {
        printf("[RGSFZ C] load_region_sample: region_index %d >= num_regions %d\n", region_index, player->sfz->num_regions);
        return 0;
    }

    SFZRegion* r = &player->sfz->regions[region_index];

    // Allocate and copy sample data
    if (r->sample_data) {
        free(r->sample_data);
    }
    r->sample_data = (int16_t*)malloc(length * sizeof(int16_t));
    if (!r->sample_data) {
        printf("[RGSFZ C] load_region_sample: Failed to allocate memory\n");
        return 0;
    }

    memcpy(r->sample_data, sample_data, length * sizeof(int16_t));
    r->sample_length = length;
    r->sample_rate = sample_rate;

    printf("[RGSFZ C] load_region_sample: Loaded region %d, length=%d, rate=%d\n", region_index, length, sample_rate);
    return 1;
}

// ============================================================================
// MIDI Handling
// ============================================================================

static int find_free_voice(RGSFZPlayer* player) {
    for (int i = 0; i < MAX_VOICES; i++) {
        if (!player->voices[i].active) return i;
    }
    return 0;  // Steal oldest
}

void rgsfz_player_note_on(RGSFZPlayer* player, uint8_t note, uint8_t velocity) {
    if (!player || !player->sfz) {
        printf("[RGSFZ C] note_on: player or sfz is NULL\n");
        return;
    }

    printf("[RGSFZ C] note_on: note=%d, vel=%d, num_regions=%d\n", note, velocity, player->sfz->num_regions);

    // Find matching region
    const SFZRegion* region = sfz_find_region(player->sfz, note, velocity);
    if (!region) {
        printf("[RGSFZ C] note_on: No matching region found for note %d, vel %d\n", note, velocity);
        return;
    }

    if (!region->sample_data) {
        printf("[RGSFZ C] note_on: Region found but sample_data is NULL\n");
        return;
    }

    printf("[RGSFZ C] note_on: Region found, sample_length=%d, offset=%d\n", region->sample_length, region->offset);

    int voice_idx = find_free_voice(player);
    SFZVoice* v = &player->voices[voice_idx];

    // Setup sample data for synth_sample_player
    SampleData sample_data;

    // Apply offset to sample pointer
    sample_data.attack_data = region->sample_data + region->offset;

    // Calculate length accounting for offset and end
    if (region->end > 0 && region->end > region->offset) {
        sample_data.attack_length = region->end - region->offset;
    } else {
        sample_data.attack_length = region->sample_length - region->offset;
    }

    sample_data.loop_data = NULL;
    sample_data.loop_length = 0;
    sample_data.sample_rate = region->sample_rate;

    // Determine root note based on pitch_keytrack
    if (region->pitch_keytrack == 0) {
        // No pitch tracking (drums): always play at natural rate
        sample_data.root_note = note;
    } else {
        // Full pitch tracking (melodic): use pitch_keycenter if set
        sample_data.root_note = (region->pitch_keycenter == 255) ? note : region->pitch_keycenter;
    }

    // Load sample into player
    synth_sample_player_load_sample(v->player, &sample_data);

    // Set decay time
    float decay_time = 0.5f + player->decay * 7.5f;
    synth_sample_player_set_loop_decay(v->player, decay_time);

    // Trigger playback
    synth_sample_player_trigger(v->player, note, velocity);

    v->region = region;
    v->note = note;
    v->active = true;
}

void rgsfz_player_note_off(RGSFZPlayer* player, uint8_t note) {
    if (!player) return;

    for (int i = 0; i < MAX_VOICES; i++) {
        if (player->voices[i].active && player->voices[i].note == note) {
            synth_sample_player_release(player->voices[i].player);
        }
    }
}

void rgsfz_player_all_notes_off(RGSFZPlayer* player) {
    if (!player) return;

    for (int i = 0; i < MAX_VOICES; i++) {
        if (player->voices[i].active) {
            synth_sample_player_release(player->voices[i].player);
            player->voices[i].active = false;
        }
    }
}

// ============================================================================
// Audio Processing
// ============================================================================

/**
 * Process audio and fill stereo buffer
 * buffer: Float32 array [L0, R0, L1, R1, ...]
 * frames: Number of frames to render
 */
void rgsfz_player_process_f32(RGSFZPlayer* player, float* buffer, uint32_t frames) {
    if (!player || !buffer) return;

    for (uint32_t frame = 0; frame < frames; frame++) {
        float mixL = 0.0f, mixR = 0.0f;

        for (int i = 0; i < MAX_VOICES; i++) {
            SFZVoice* v = &player->voices[i];
            if (!v->active) continue;

            float sample = synth_sample_player_process(v->player, player->sample_rate);

            if (!synth_sample_player_is_active(v->player)) {
                v->active = false;
                continue;
            }

            // Apply region panning + master pan
            float total_pan = player->pan + (v->region ? v->region->pan / 100.0f : 0.0f);
            if (total_pan < -1.0f) total_pan = -1.0f;
            if (total_pan > 1.0f) total_pan = 1.0f;

            // Constant power panning
            float pan_angle = (total_pan + 1.0f) * 0.25f * 3.14159265f;
            float pan_left = cosf(pan_angle);
            float pan_right = sinf(pan_angle);

            mixL += sample * pan_left;
            mixR += sample * pan_right;
        }

        // Apply master volume (reduce per-voice to avoid clipping)
        mixL *= player->volume * 0.3f;
        mixR *= player->volume * 0.3f;

        // Write to interleaved stereo buffer
        buffer[frame * 2] = mixL;
        buffer[frame * 2 + 1] = mixR;
    }
}

// ============================================================================
// Parameters
// ============================================================================

void rgsfz_player_set_volume(RGSFZPlayer* player, float volume) {
    if (player) player->volume = volume;
}

void rgsfz_player_set_pan(RGSFZPlayer* player, float pan) {
    if (player) player->pan = pan;
}

void rgsfz_player_set_decay(RGSFZPlayer* player, float decay) {
    if (!player) return;
    player->decay = decay;

    // Update all voice decay times
    for (int i = 0; i < MAX_VOICES; i++) {
        float decay_time = 0.5f + player->decay * 7.5f;
        synth_sample_player_set_loop_decay(player->voices[i].player, decay_time);
    }
}

// ============================================================================
// Info / Query Functions
// ============================================================================

uint32_t rgsfz_player_get_num_regions(RGSFZPlayer* player) {
    return (player && player->sfz) ? player->sfz->num_regions : 0;
}

uint32_t rgsfz_player_get_active_voices(RGSFZPlayer* player) {
    if (!player) return 0;

    uint32_t count = 0;
    for (int i = 0; i < MAX_VOICES; i++) {
        if (player->voices[i].active) count++;
    }
    return count;
}

// Get region info for display
const char* rgsfz_player_get_region_sample(RGSFZPlayer* player, uint32_t index) {
    if (!player || !player->sfz || index >= player->sfz->num_regions) return "";
    return player->sfz->regions[index].sample_path;
}

uint8_t rgsfz_player_get_region_lokey(RGSFZPlayer* player, uint32_t index) {
    if (!player || !player->sfz || index >= player->sfz->num_regions) return 0;
    return player->sfz->regions[index].lokey;
}

uint8_t rgsfz_player_get_region_hikey(RGSFZPlayer* player, uint32_t index) {
    if (!player || !player->sfz || index >= player->sfz->num_regions) return 0;
    return player->sfz->regions[index].hikey;
}

uint8_t rgsfz_player_get_region_lovel(RGSFZPlayer* player, uint32_t index) {
    if (!player || !player->sfz || index >= player->sfz->num_regions) return 0;
    return player->sfz->regions[index].lovel;
}

uint8_t rgsfz_player_get_region_hivel(RGSFZPlayer* player, uint32_t index) {
    if (!player || !player->sfz || index >= player->sfz->num_regions) return 0;
    return player->sfz->regions[index].hivel;
}

uint8_t rgsfz_player_get_region_pitch(RGSFZPlayer* player, uint32_t index) {
    if (!player || !player->sfz || index >= player->sfz->num_regions) return 0;
    return player->sfz->regions[index].pitch_keycenter;
}

// ============================================================================
// Audio Buffer Management
// ============================================================================

float* rgsfz_create_audio_buffer(uint32_t frames) {
    // Stereo buffer: [L0, R0, L1, R1, ...]
    return (float*)calloc(frames * 2, sizeof(float));
}

void rgsfz_destroy_audio_buffer(float* buffer) {
    if (buffer) free(buffer);
}
