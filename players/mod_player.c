#include "mod_player.h"
#include "tracker_mixer.h"
#include "pattern_sequencer.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define AMIGA_CLOCK 3546895  // PAL clock rate for period-to-frequency conversion

// Forward declarations
static void process_note(ModPlayer* player, uint8_t channel, const ModNote* note);
static void process_effects(ModPlayer* player, uint8_t channel);
static void trigger_position_callback(ModPlayer* player);

// Pattern sequencer callbacks
static void mod_on_tick(void* user_data, uint8_t tick);
static void mod_on_row(void* user_data, uint16_t pattern_index,
                      uint16_t pattern_number, uint16_t row);

// Period table for notes (ProTracker)
static const uint16_t period_table[16][36] = {
    // Finetune 0
    {856,808,762,720,678,640,604,570,538,508,480,453,
     428,404,381,360,339,320,302,285,269,254,240,226,
     214,202,190,180,170,160,151,143,135,127,120,113},
    // Finetune +1
    {850,802,757,715,674,637,601,567,535,505,477,450,
     425,401,379,357,337,318,300,284,268,253,239,225,
     213,201,189,179,169,159,150,142,134,126,119,113},
    // Finetune +2
    {844,796,752,709,670,632,597,563,532,502,474,447,
     422,398,376,355,335,316,298,282,266,251,237,224,
     211,199,188,177,167,158,149,141,133,125,118,112},
    // Finetune +3
    {838,791,746,704,665,628,592,559,528,498,470,444,
     419,395,373,352,332,314,296,280,264,249,235,222,
     209,198,187,176,166,157,148,140,132,125,118,111},
    // Finetune +4
    {832,785,741,699,660,623,588,555,524,495,467,441,
     416,392,370,350,330,312,294,278,262,247,233,220,
     208,196,185,175,165,156,147,139,131,124,117,110},
    // Finetune +5
    {826,779,736,694,655,619,584,551,520,491,463,437,
     413,390,368,347,328,309,292,276,260,245,232,219,
     206,195,184,174,164,155,146,138,130,123,116,109},
    // Finetune +6
    {820,774,730,689,651,614,580,547,516,487,460,434,
     410,387,365,345,325,307,290,274,258,244,230,217,
     205,193,183,172,163,154,145,137,129,122,115,109},
    // Finetune +7
    {814,768,725,684,646,610,575,543,513,484,457,431,
     407,384,363,342,323,305,287,272,256,242,228,216,
     204,192,181,171,161,152,144,136,128,121,114,108},
    // Finetune 0 (center)
    {856,808,762,720,678,640,604,570,538,508,480,453,
     428,404,381,360,339,320,302,285,269,254,240,226,
     214,202,190,180,170,160,151,143,135,127,120,113},
    // Finetune -1
    {862,814,768,725,684,646,610,575,543,513,484,457,
     431,407,384,363,342,323,305,287,272,256,242,228,
     216,203,192,181,171,161,152,144,136,128,121,114},
    // Finetune -2
    {868,820,774,730,689,651,614,580,547,516,487,460,
     434,410,387,365,345,325,307,290,274,258,244,230,
     217,205,193,183,172,163,154,145,137,129,122,115},
    // Finetune -3
    {874,826,779,736,694,655,619,584,551,520,491,463,
     437,413,390,368,347,328,309,292,276,260,245,232,
     219,206,195,184,174,164,155,146,138,130,123,116},
    // Finetune -4
    {880,832,785,741,699,660,623,588,555,524,494,467,
     440,416,392,370,350,330,312,294,278,262,247,233,
     220,208,196,185,175,165,156,147,139,131,123,117},
    // Finetune -5
    {886,838,791,746,704,665,628,592,559,528,498,470,
     443,419,395,373,352,332,314,296,280,264,249,235,
     222,209,198,187,176,166,157,148,140,132,125,118},
    // Finetune -6
    {892,844,796,752,709,670,632,597,563,532,502,474,
     446,422,398,376,355,335,316,298,282,266,251,237,
     223,211,199,188,177,167,158,149,141,133,125,118},
    // Finetune -7
    {898,850,802,757,715,675,636,601,567,535,505,477,
     449,425,401,379,357,337,318,300,284,268,253,238,
     225,212,200,189,179,169,159,150,142,134,126,119}
};

struct ModPlayer {
    char title[MOD_TITLE_LENGTH + 1];
    ModSample samples[MOD_MAX_SAMPLES];
    uint8_t song_positions[128];
    uint8_t song_length;
    uint8_t num_patterns;
    ModNote* patterns;  // Dynamically allocated: [num_patterns][4][64]

    // Pattern sequencer (NEW - handles timing/position/flow control)
    PatternSequencer* sequencer;

    // Playback state
    bool playing;
    uint8_t current_pattern_index;  // Index into song_positions
    uint16_t current_row;
    uint8_t tick;
    uint8_t speed;                  // Ticks per row (default 6)
    uint8_t bpm;                    // Beats per minute (default 125)

    // Loop control
    uint8_t loop_start;
    uint8_t loop_end;
    bool disable_looping;   // If true, stop playback instead of looping

    // Pattern loop (E6x effect)
    uint8_t pattern_loop_row;
    uint8_t pattern_loop_count;
    bool pattern_loop_pending;  // Flag to jump on next row advance

    // Pattern delay (EEx effect)
    uint8_t pattern_delay;
    bool in_pattern_delay_repeat;  // Flag to prevent re-triggering delay during repeats

    // B+D combination (position jump + pattern break)
    bool position_jump_pending;
    uint8_t jump_to_pattern;
    uint8_t jump_to_row;

    // Timing - use double for precise accumulation
    double samples_per_tick;
    double sample_accumulator;

    // Position callback
    ModPlayerPositionCallback position_callback;
    void* callback_user_data;

    // Channels
    ModChannel channels[MOD_MAX_CHANNELS];
};

// Helper: Convert period to frequency
static float period_to_frequency(uint16_t period) {
    if (period == 0) return 0.0f;
    // Amiga period to frequency: freq = 7093789.2 / (period * 2)
    // For PAL: 7093789.2 Hz
    return 7093789.2f / (period * 2.0f);
}

// Helper: Trigger position callback
static void trigger_position_callback(ModPlayer* player) {
    if (!player || !player->position_callback) return;

    uint8_t order = player->current_pattern_index;
    uint8_t row = player->current_row;
    uint8_t pattern = 0;

    if (order < player->song_length) {
        pattern = player->song_positions[order];
    }

    player->position_callback(order, pattern, row, player->callback_user_data);
}

// Helper: Get period for note and finetune
static uint16_t get_period(uint8_t note, int8_t finetune) {
    if (note == 0 || note > 36) return 0;

    // Adjust finetune to table index (0-15, with 8 = no finetune)
    int ft_index = 8 + (finetune & 0x0F);
    if (ft_index < 0) ft_index = 0;
    if (ft_index > 15) ft_index = 15;

    return period_table[ft_index][note - 1];
}

// Sine table for vibrato/tremolo (64 entries, 0-255 range)
static const uint8_t sine_table[64] = {
    0,  24,  49,  74,  97, 120, 141, 161, 180, 197, 212, 224, 235, 244, 250, 253,
  255, 253, 250, 244, 235, 224, 212, 197, 180, 161, 141, 120,  97,  74,  49,  24,
    0, 232, 207, 182, 159, 136, 115,  95,  76,  59,  44,  32,  21,  12,   6,   3,
    1,   3,   6,  12,  21,  32,  44,  59,  76,  95, 115, 136, 159, 182, 207, 232
};

// Helper: Parse MOD file format identifier
static bool is_valid_mod(const uint8_t* data, uint32_t size) {
    if (size < 1084) return false;

    // Check for M.K., M!K!, FLT4, or 4CHN at offset 1080
    const char* tag = (const char*)(data + 1080);
    return (memcmp(tag, "M.K.", 4) == 0 ||
            memcmp(tag, "M!K!", 4) == 0 ||
            memcmp(tag, "FLT4", 4) == 0 ||
            memcmp(tag, "4CHN", 4) == 0);
}

bool mod_player_detect(const uint8_t* data, uint32_t size) {
    return is_valid_mod(data, size);
}

ModPlayer* mod_player_create(void) {
    ModPlayer* player = (ModPlayer*)calloc(1, sizeof(ModPlayer));
    if (!player) return NULL;

    // Create pattern sequencer
    player->sequencer = pattern_sequencer_create();
    if (!player->sequencer) {
        free(player);
        return NULL;
    }

    // Set sequencer to tick-based mode (MOD uses BPM timing)
    pattern_sequencer_set_mode(player->sequencer, PS_MODE_TICK_BASED);

    // Set defaults
    player->speed = 6;
    player->bpm = 125;
    player->loop_start = 0;
    player->loop_end = 0;

    // Initialize channels with default panning (Amiga style)
    player->channels[0].panning = -1.0f;  // Hard Left
    player->channels[1].panning = 1.0f;   // Hard Right
    player->channels[2].panning = 1.0f;   // Hard Right
    player->channels[3].panning = -1.0f;  // Hard Left

    for (int i = 0; i < MOD_MAX_CHANNELS; i++) {
        player->channels[i].user_volume = 1.0f;
        player->channels[i].volume = 64;

        // Initialize TrackerVoice for each channel
        tracker_voice_init(&player->channels[i].voice_playback);
    }

    return player;
}

void mod_player_destroy(ModPlayer* player) {
    if (!player) return;

    // Destroy pattern sequencer
    if (player->sequencer) {
        pattern_sequencer_destroy(player->sequencer);
    }

    // Free sample data
    for (int i = 0; i < MOD_MAX_SAMPLES; i++) {
        if (player->samples[i].data) {
            free(player->samples[i].data);
        }
    }

    // Free pattern data
    if (player->patterns) {
        free(player->patterns);
    }

    free(player);
}

bool mod_player_load(ModPlayer* player, const uint8_t* data, uint32_t size) {
    if (!player || !data) return false;
    if (!is_valid_mod(data, size)) return false;

    // Parse title
    memcpy(player->title, data, MOD_TITLE_LENGTH);
    player->title[MOD_TITLE_LENGTH] = '\0';

    // Parse samples
    uint32_t offset = 20;
    for (int i = 0; i < MOD_MAX_SAMPLES; i++) {
        ModSample* sample = &player->samples[i];

        // Sample name
        memcpy(sample->name, data + offset, MOD_SAMPLE_NAME_LENGTH);
        sample->name[MOD_SAMPLE_NAME_LENGTH] = '\0';
        offset += 22;

        // Sample length (in words, big-endian)
        sample->length = ((uint32_t)data[offset] << 8) | data[offset + 1];
        offset += 2;

        // Finetune
        sample->finetune = data[offset] & 0x0F;
        if (sample->finetune > 7) sample->finetune |= 0xF0;  // Sign extend
        offset++;

        // Volume
        sample->volume = data[offset];
        if (sample->volume > 64) sample->volume = 64;
        offset++;

        // Repeat point (in words)
        sample->repeat_start = ((uint32_t)data[offset] << 8) | data[offset + 1];
        offset += 2;

        // Repeat length (in words)
        sample->repeat_length = ((uint32_t)data[offset] << 8) | data[offset + 1];
        offset += 2;

        sample->data = NULL;  // Will be loaded later
    }

    // Song length (at offset 950, NOT 1080!)
    player->song_length = data[950];
    if (player->song_length > 128) player->song_length = 128;

    // Song positions (at offset 952, NOT 1082!)
    memcpy(player->song_positions, data + 952, 128);

    // Find highest pattern number - scan ALL 128 positions because file contains all patterns!
    player->num_patterns = 0;
    for (int i = 0; i < 128; i++) {
        if (player->song_positions[i] > player->num_patterns) {
            player->num_patterns = player->song_positions[i];
        }
    }
    player->num_patterns++;  // Convert to count

    // Allocate pattern data
    uint32_t pattern_data_size = player->num_patterns * MOD_MAX_CHANNELS * MOD_PATTERN_ROWS;
    player->patterns = (ModNote*)calloc(pattern_data_size, sizeof(ModNote));
    if (!player->patterns) return false;

    // Parse patterns (start after header)
    offset = 1084;

    for (uint32_t p = 0; p < player->num_patterns; p++) {
        for (uint32_t r = 0; r < MOD_PATTERN_ROWS; r++) {
            for (uint32_t c = 0; c < MOD_MAX_CHANNELS; c++) {
                uint32_t pattern_index = p * (MOD_MAX_CHANNELS * MOD_PATTERN_ROWS) +
                                        r * MOD_MAX_CHANNELS + c;
                ModNote* note = &player->patterns[pattern_index];

                uint8_t b0 = data[offset++];
                uint8_t b1 = data[offset++];
                uint8_t b2 = data[offset++];
                uint8_t b3 = data[offset++];

                // ProTracker MOD format:
                // b0[7:4] = sample number bits [7:4] (upper nibble)
                // b0[3:0] = period bits [11:8]
                // b1[7:0] = period bits [7:0]
                // b2[7:4] = sample number bits [3:0] (lower nibble)
                // b2[3:0] = effect type
                // b3[7:0] = effect parameter
                note->sample = (b0 & 0xF0) | ((b2 & 0xF0) >> 4);
                note->period = ((b0 & 0x0F) << 8) | b1;
                note->effect = b2 & 0x0F;
                note->effect_param = b3;
            }
        }
    }

    // Load sample data
    for (int i = 0; i < MOD_MAX_SAMPLES; i++) {
        ModSample* sample = &player->samples[i];
        uint32_t sample_length_bytes = sample->length * 2;  // Convert words to bytes

        if (sample_length_bytes > 0 && offset + sample_length_bytes <= size) {
            sample->data = (int8_t*)malloc(sample_length_bytes);
            if (sample->data) {
                memcpy(sample->data, data + offset, sample_length_bytes);
            }
            offset += sample_length_bytes;
        }
    }

    // Set default loop range
    player->loop_start = 0;
    player->loop_end = player->song_length > 0 ? player->song_length - 1 : 0;

    // Configure pattern sequencer with song structure
    // Convert song_positions from uint8_t[] to uint16_t[] for pattern_sequencer
    uint16_t pattern_order_16[128];
    for (int i = 0; i < player->song_length; i++) {
        pattern_order_16[i] = player->song_positions[i];
    }

    pattern_sequencer_set_song(player->sequencer,
                              pattern_order_16,
                              player->song_length,
                              MOD_PATTERN_ROWS);  // 64 rows per pattern
    pattern_sequencer_set_speed(player->sequencer, player->speed);
    pattern_sequencer_set_bpm(player->sequencer, player->bpm);
    pattern_sequencer_set_loop_range(player->sequencer, player->loop_start, player->loop_end);

    // Set up sequencer callbacks
    PatternSequencerCallbacks callbacks = {
        .on_tick = mod_on_tick,
        .on_row = mod_on_row,
        .on_pattern_change = NULL,  // Not needed for MOD
        .on_song_end = NULL          // Not needed for MOD
    };
    pattern_sequencer_set_callbacks(player->sequencer, &callbacks, player);

    return true;
}

// Pattern sequencer callbacks
static void mod_on_tick(void* user_data, uint8_t tick) {
    ModPlayer* player = (ModPlayer*)user_data;

    // Process per-tick effects for all channels
    for (uint8_t c = 0; c < MOD_MAX_CHANNELS; c++) {
        process_effects(player, c);
    }
}

static void mod_on_row(void* user_data, uint16_t pattern_index,
                      uint16_t pattern_number, uint16_t row) {
    ModPlayer* player = (ModPlayer*)user_data;

    // Process notes for this row
    if (pattern_number < player->num_patterns) {
        for (uint8_t c = 0; c < MOD_MAX_CHANNELS; c++) {
            uint32_t note_index = pattern_number * (MOD_MAX_CHANNELS * MOD_PATTERN_ROWS) +
                                 row * MOD_MAX_CHANNELS + c;
            const ModNote* note = &player->patterns[note_index];
            process_note(player, c, note);
        }
    }

    // Trigger position callback
    trigger_position_callback(player);
}

void mod_player_start(ModPlayer* player) {
    if (!player) return;
    player->playing = true;

    // Start pattern sequencer (handles position initialization and first row processing)
    pattern_sequencer_start(player->sequencer);
}

void mod_player_stop(ModPlayer* player) {
    if (!player) return;
    player->playing = false;

    // Stop pattern sequencer
    pattern_sequencer_stop(player->sequencer);

    // Stop all channels
    for (int i = 0; i < MOD_MAX_CHANNELS; i++) {
        player->channels[i].sample = NULL;
    }
}

bool mod_player_is_playing(const ModPlayer* player) {
    return player ? player->playing : false;
}

void mod_player_set_loop_range(ModPlayer* player, uint8_t start_pattern, uint8_t end_pattern) {
    if (!player) return;
    if (start_pattern >= player->song_length) start_pattern = 0;
    if (end_pattern >= player->song_length) end_pattern = player->song_length - 1;
    if (start_pattern > end_pattern) start_pattern = end_pattern;

    player->loop_start = start_pattern;
    player->loop_end = end_pattern;

    // Update sequencer loop range
    pattern_sequencer_set_loop_range(player->sequencer, start_pattern, end_pattern);
}

void mod_player_get_position(const ModPlayer* player, uint8_t* pattern, uint16_t* row) {
    if (!player) return;

    uint16_t pattern_index, pattern_num, row16;
    pattern_sequencer_get_position(player->sequencer, &pattern_index, &pattern_num, &row16);

    if (pattern) *pattern = (uint8_t)pattern_index;
    if (row) *row = row16;
}

void mod_player_set_position_callback(ModPlayer* player, ModPlayerPositionCallback callback, void* user_data) {
    if (!player) return;
    player->position_callback = callback;
    player->callback_user_data = user_data;
}

void mod_player_set_position(ModPlayer* player, uint8_t pattern, uint16_t row) {
    if (!player) return;
    if (pattern < player->song_length && row < MOD_PATTERN_ROWS) {
        pattern_sequencer_set_position(player->sequencer, pattern, row);
    }
}

void mod_player_set_bpm(ModPlayer* player, uint8_t bpm) {
    if (!player) return;
    if (bpm < 32) bpm = 32;
    if (bpm > 255) bpm = 255;
    player->bpm = bpm;
    pattern_sequencer_set_bpm(player->sequencer, bpm);
}

void mod_player_set_speed(ModPlayer* player, uint8_t speed) {
    if (!player) return;
    if (speed == 0) speed = 1;
    player->speed = speed;
    pattern_sequencer_set_speed(player->sequencer, speed);
}

void mod_player_set_channel_mute(ModPlayer* player, uint8_t channel, bool muted) {
    if (!player || channel >= MOD_MAX_CHANNELS) return;
    player->channels[channel].muted = muted;
}

void mod_player_set_channel_volume(ModPlayer* player, uint8_t channel, float volume) {
    if (!player || channel >= MOD_MAX_CHANNELS) return;
    if (volume < 0.0f) volume = 0.0f;
    if (volume > 1.0f) volume = 1.0f;
    player->channels[channel].user_volume = volume;
}

void mod_player_set_channel_panning(ModPlayer* player, uint8_t channel, float panning) {
    if (!player || channel >= MOD_MAX_CHANNELS) return;
    if (panning < -1.0f) panning = -1.0f;
    if (panning > 1.0f) panning = 1.0f;
    player->channels[channel].panning = panning;
}

bool mod_player_get_channel_mute(const ModPlayer* player, uint8_t channel) {
    if (!player || channel >= MOD_MAX_CHANNELS) return false;
    return player->channels[channel].muted;
}

float mod_player_get_channel_volume(const ModPlayer* player, uint8_t channel) {
    if (!player || channel >= MOD_MAX_CHANNELS) return 0.0f;
    return player->channels[channel].user_volume;
}

float mod_player_get_channel_panning(const ModPlayer* player, uint8_t channel) {
    if (!player || channel >= MOD_MAX_CHANNELS) return 0.0f;
    return player->channels[channel].panning;
}

const char* mod_player_get_title(const ModPlayer* player) {
    return player ? player->title : "";
}

uint8_t mod_player_get_song_length(const ModPlayer* player) {
    return player ? player->song_length : 0;
}

void mod_player_set_disable_looping(ModPlayer* player, bool disable) {
    if (!player) return;
    player->disable_looping = disable;
    // Update sequencer looping (inverse logic: disable -> !enabled)
    pattern_sequencer_set_looping(player->sequencer, !disable);
}

// Process note for a channel
static void process_note(ModPlayer* player, uint8_t channel, const ModNote* note) {
    ModChannel* chan = &player->channels[channel];

    // Handle sample trigger
    // In ProTracker:
    // - If there's a sample number AND a period: trigger new sample
    // - If there's a sample number but NO period: set sample but don't retrigger (rare case)
    // - If there's NO sample number but there IS a period: change pitch of current sample (if any)
    if (note->sample > 0 && note->sample <= MOD_MAX_SAMPLES) {
        const ModSample* sample = &player->samples[note->sample - 1];

        // Trigger sample if there's a period OR if there's effect 9 (offset retrigger)
        if (note->period > 0 || note->effect == 0x9) {
            chan->sample = sample;
            chan->finetune = sample->finetune;

            // Volume handling
            if (note->effect == 0xC) {
                // Explicit C effect - always use it
                chan->volume = note->effect_param;
                if (chan->volume > 64) chan->volume = 64;
            } else if (note->period > 0) {
                // New note (with or without offset) - reset to sample default volume
                chan->volume = sample->volume;
            }
            // else: offset-only retrigger (no period, no C effect) - keep current volume

            // Initialize TrackerVoice for sample playback
            tracker_voice_set_waveform(&chan->voice_playback, sample->data, sample->length * 2);
            tracker_voice_set_loop(&chan->voice_playback,
                                  sample->repeat_start * 2,  // Convert words to bytes
                                  sample->repeat_length * 2,
                                  2);  // ProTracker: 1 word (2 bytes) = one-shot
            tracker_voice_reset_position(&chan->voice_playback);

            // Reset old position field for compatibility
            chan->position = 0.0;

            // Don't clear period when offset effect is present - offset just repositions playback
            // Period will be set by period handling code below if note has a period value
        } else {
            // Sample number without period and without offset - just remember the sample, don't retrigger
            // This is used by some effects (like porta + volume slide)
            chan->sample = sample;
            chan->finetune = sample->finetune;
            chan->volume = sample->volume;  // Reset volume to sample default
        }
    }

    // Handle period (pitch)
    if (note->period > 0) {
        // ProTracker: period without sample number = retrigger last sample
        // This must happen BEFORE we process tone portamento
        if (note->sample == 0 && chan->sample != NULL && note->effect != 0x3 && note->effect != 0x5) {
            // Retrigger: Use last offset ONLY if it belongs to the current sample
            if (chan->last_sample_offset > 0 && chan->sample == chan->last_sample_with_offset) {
                uint32_t byte_offset = chan->last_sample_offset * 256;
                uint32_t sample_len_bytes = chan->sample->length * 2;  // Convert words to bytes

                if (byte_offset < sample_len_bytes) {
                    tracker_voice_set_position(&chan->voice_playback, byte_offset);
                    chan->position = (double)byte_offset;
                } else {
                    // Offset beyond sample - start from 0
                    tracker_voice_reset_position(&chan->voice_playback);
                    chan->position = 0.0;
                }
            } else {
                // Different sample or no offset - start from 0
                tracker_voice_reset_position(&chan->voice_playback);
                chan->position = 0.0;
            }
        }

        // Effect 0x3 (tone portamento) sets target instead of changing period
        if (note->effect == 0x3 || note->effect == 0x5) {
            // Set portamento target
            chan->portamento_target = note->period;
            // In ProTracker: if there's no note currently playing (period=0),
            // tone portamento with a new note should NOT slide, it should just
            // play the note normally (this is the first note on the channel)
            if (chan->period == 0 && note->sample > 0) {
                // First note with portamento effect - treat as normal note
                chan->period = note->period;
                chan->position = 0.0f;
            }
            // Otherwise: current note keeps playing and slides toward target
        } else {
            chan->period = note->period;
            // Don't reset position - let sample continue playing from current position
            // (Changing pitch/volume shouldn't restart the sample)

            // Reset vibrato/tremolo phase when new note triggered (not for portamento/vibrato)
            if (note->effect != 0x4 && note->effect != 0x6) {
                chan->vibrato_pos = 0;
            }
            if (note->effect != 0x7) {
                chan->tremolo_pos = 0;
            }
        }
    }

    // Store effect for processing
    chan->effect = note->effect;
    chan->effect_param = note->effect_param;

    // Process immediate effects
    switch (note->effect) {
        case 0x4:  // Vibrato
            if (note->effect_param > 0) {
                if ((note->effect_param & 0x0F) > 0) {
                    chan->vibrato_depth = note->effect_param & 0x0F;
                }
                if ((note->effect_param >> 4) > 0) {
                    chan->vibrato_speed = note->effect_param >> 4;
                }
            }
            break;

        case 0x5:  // Tone portamento + volume slide
            // Portamento target already set if there was a period
            // Volume slide will be handled in process_effects
            break;

        case 0x6:  // Vibrato + volume slide
            // Vibrato parameters already set
            // Volume slide will be handled in process_effects
            break;

        case 0x7:  // Tremolo
            if (note->effect_param > 0) {
                if ((note->effect_param & 0x0F) > 0) {
                    chan->tremolo_depth = note->effect_param & 0x0F;
                }
                if ((note->effect_param >> 4) > 0) {
                    chan->tremolo_speed = note->effect_param >> 4;
                }
            }
            break;

        case 0x8:  // Set panning (00 = left, 80 = center, FF = right)
            {
                // Convert 0-255 to -1.0 to 1.0
                float pan = ((float)note->effect_param / 127.5f) - 1.0f;
                if (pan < -1.0f) pan = -1.0f;
                if (pan > 1.0f) pan = 1.0f;
                chan->panning = pan;
            }
            break;

        case 0x9:  // Sample offset - start sample at offset
            {
                // Sample offset is applied when there's a note trigger OR sample number
                // (period=0 with sample+offset means retrigger at offset with same pitch)
                if ((note->period > 0 || note->sample > 0) && chan->sample) {
                    uint8_t offset_param = note->effect_param;
                    if (offset_param == 0) {
                        offset_param = chan->last_sample_offset;  // Use last offset
                    } else {
                        chan->last_sample_offset = offset_param;  // Remember for next time
                        chan->last_sample_with_offset = chan->sample;  // Track which sample this offset is for
                    }

                    // ProTracker sample offset: offset in 256-byte units
                    uint32_t byte_offset = offset_param * 256;
                    uint32_t sample_len_bytes = chan->sample->length * 2;  // Convert words to bytes

                    // Bounds check - if offset is beyond sample, clamp to start
                    if (byte_offset < sample_len_bytes) {
                        tracker_voice_set_position(&chan->voice_playback, byte_offset);
                        chan->position = (double)byte_offset;
                    }
                    // If offset is beyond sample, ignore it (keep current position)
                } else if (note->effect_param > 0) {
                    // Remember offset param even without period (for future use)
                    chan->last_sample_offset = note->effect_param;
                    // Don't set last_sample_with_offset here - we don't have a sample yet
                }
            }
            break;

        case 0xB:  // Position jump - jump to pattern
            pattern_sequencer_position_jump(player->sequencer, note->effect_param);
            break;

        case 0xC:  // Set volume
            chan->volume = note->effect_param;
            if (chan->volume > 64) chan->volume = 64;
            break;

        case 0xD:  // Pattern break - skip to next pattern at row N
            {
                // Parameter is in BCD format: high nibble = tens, low nibble = ones
                uint8_t row = ((note->effect_param >> 4) * 10) + (note->effect_param & 0x0F);
                if (row >= MOD_PATTERN_ROWS) row = 0;
                pattern_sequencer_pattern_break(player->sequencer, row);
            }
            break;

        case 0xE:  // Extended effects
            {
                uint8_t sub_effect = (note->effect_param >> 4) & 0x0F;
                uint8_t sub_param = note->effect_param & 0x0F;

                switch (sub_effect) {
                    case 0x1:  // Fine portamento up
                        if (chan->period > 0 && chan->period > sub_param) {
                            chan->period -= sub_param;
                        }
                        if (chan->period < 113) chan->period = 113;
                        break;

                    case 0x2:  // Fine portamento down
                        if (chan->period > 0) {
                            chan->period += sub_param;
                        }
                        if (chan->period > 856) chan->period = 856;
                        break;

                    case 0x5:  // Set finetune
                        chan->finetune = sub_param;
                        if (chan->finetune > 7) chan->finetune |= 0xF0;  // Sign extend
                        break;

                    case 0x6:  // Pattern loop
                        if (sub_param == 0) {
                            // E60: Set loop start point
                            pattern_sequencer_set_pattern_loop_start(player->sequencer);
                        } else {
                            // E6x: Loop back x times
                            pattern_sequencer_execute_pattern_loop(player->sequencer, sub_param);
                        }
                        break;

                    case 0x9:  // Retrigger note
                        chan->retrigger_count = 0;
                        break;

                    case 0xA:  // Fine volume slide up
                        chan->volume += sub_param;
                        if (chan->volume > 64) chan->volume = 64;
                        break;

                    case 0xB:  // Fine volume slide down
                        if (chan->volume >= sub_param) {
                            chan->volume -= sub_param;
                        } else {
                            chan->volume = 0;
                        }
                        break;

                    case 0xC:  // Note cut (cut after N ticks)
                        // Will be handled in process_effects
                        break;

                    case 0xD:  // Note delay (delay note by N ticks)
                        chan->note_delay_ticks = sub_param;
                        // Will be handled in process_effects
                        break;

                    case 0xE:  // Pattern delay
                        // Delay pattern by N rows (repeat current row N times)
                        pattern_sequencer_pattern_delay(player->sequencer, sub_param);
                        break;
                }
            }
            break;

        case 0xF:  // Set speed/BPM
            if (note->effect_param > 0) {
                if (note->effect_param < 32) {
                    // Speed (ticks per row)
                    pattern_sequencer_set_speed(player->sequencer, note->effect_param);
                    player->speed = note->effect_param;  // Keep in sync for legacy code
                } else {
                    // BPM
                    pattern_sequencer_set_bpm(player->sequencer, note->effect_param);
                    player->bpm = note->effect_param;  // Keep in sync for legacy code
                }
            }
            break;
    }
}

// Process effects per tick
static void process_effects(ModPlayer* player, uint8_t channel) {
    ModChannel* chan = &player->channels[channel];

    switch (chan->effect) {
        case 0x0:  // Arpeggio
            if (chan->effect_param != 0 && chan->period > 0) {
                uint8_t tick_mod = player->tick % 3;
                uint16_t period = chan->period;

                if (tick_mod == 1) {
                    // First note in arpeggio
                    uint8_t note1 = (chan->effect_param >> 4) & 0x0F;
                    // Find current note and add semitones
                    // (Simplified - should use period table lookup)
                } else if (tick_mod == 2) {
                    // Second note in arpeggio
                    uint8_t note2 = chan->effect_param & 0x0F;
                }
            }
            break;

        case 0x1:  // Portamento up
            if (chan->period > 0) {
                uint8_t param = chan->effect_param;
                if (param == 0) param = chan->last_portamento_up;
                else chan->last_portamento_up = param;

                chan->period -= param;
                if (chan->period < 113) chan->period = 113;
            }
            break;

        case 0x2:  // Portamento down
            if (chan->period > 0) {
                uint8_t param = chan->effect_param;
                if (param == 0) param = chan->last_portamento_down;
                else chan->last_portamento_down = param;

                chan->period += param;
                if (chan->period > 856) chan->period = 856;
            }
            break;

        case 0x3:  // Tone portamento (slide to note)
            if (chan->portamento_target > 0 && chan->period > 0) {
                uint8_t slide_speed = chan->effect_param;
                if (slide_speed == 0) slide_speed = chan->last_tone_portamento;
                else chan->last_tone_portamento = slide_speed;

                if (slide_speed == 0) break;

                if (chan->period < chan->portamento_target) {
                    // Slide down (increase period)
                    chan->period += slide_speed;
                    if (chan->period > chan->portamento_target) {
                        chan->period = chan->portamento_target;
                    }
                } else if (chan->period > chan->portamento_target) {
                    // Slide up (decrease period)
                    if (chan->period > slide_speed) {
                        chan->period -= slide_speed;
                    } else {
                        chan->period = chan->portamento_target;
                    }
                    if (chan->period < chan->portamento_target) {
                        chan->period = chan->portamento_target;
                    }
                }
            }
            break;

        case 0x4:  // Vibrato
            if (chan->vibrato_depth > 0 && chan->vibrato_speed > 0) {
                // Get vibrato value from sine table
                uint8_t vibrato_val = sine_table[chan->vibrato_pos & 0x3F];
                int16_t vibrato_delta = ((int16_t)vibrato_val * chan->vibrato_depth) / 128;

                // Apply vibrato to period (don't modify base period)
                // This is applied during rendering in render_channel

                // Advance vibrato position
                chan->vibrato_pos += chan->vibrato_speed;
                chan->vibrato_pos &= 0x3F;  // Wrap at 64
            }
            break;

        case 0x5:  // Tone portamento + volume slide
            // Do tone portamento (uses last 3xx speed)
            if (chan->portamento_target > 0 && chan->period > 0) {
                uint8_t slide_speed = chan->last_tone_portamento;
                if (slide_speed == 0) break;

                if (chan->period < chan->portamento_target) {
                    chan->period += slide_speed;
                    if (chan->period > chan->portamento_target) {
                        chan->period = chan->portamento_target;
                    }
                } else if (chan->period > chan->portamento_target) {
                    if (chan->period > slide_speed) {
                        chan->period -= slide_speed;
                    } else {
                        chan->period = chan->portamento_target;
                    }
                    if (chan->period < chan->portamento_target) {
                        chan->period = chan->portamento_target;
                    }
                }
            }
            // Volume slide handled in effect 0xA logic
            break;

        case 0x6:  // Vibrato + volume slide
            // Do vibrato (same as effect 4)
            if (chan->vibrato_depth > 0 && chan->vibrato_speed > 0) {
                chan->vibrato_pos += chan->vibrato_speed;
                chan->vibrato_pos &= 0x3F;
            }
            // Volume slide handled in effect 0xA logic
            break;

        case 0x7:  // Tremolo
            if (chan->tremolo_depth > 0 && chan->tremolo_speed > 0) {
                // Tremolo modulates volume (applied during rendering)
                chan->tremolo_pos += chan->tremolo_speed;
                chan->tremolo_pos &= 0x3F;
            }
            break;

        case 0xA:  // Volume slide
            if (player->tick != 0) {  // Not on first tick
                uint8_t param = chan->effect_param;
                if (param == 0) param = chan->last_volume_slide;
                else chan->last_volume_slide = param;

                uint8_t up = (param >> 4) & 0x0F;
                uint8_t down = param & 0x0F;

                if (up > 0) {
                    chan->volume += up;
                    if (chan->volume > 64) chan->volume = 64;
                } else if (down > 0) {
                    if (chan->volume > down) {
                        chan->volume -= down;
                    } else {
                        chan->volume = 0;
                    }
                }
            }
            break;

        case 0xE:  // Extended effects (per-tick processing)
            {
                uint8_t sub_effect = (chan->effect_param >> 4) & 0x0F;
                uint8_t sub_param = chan->effect_param & 0x0F;

                switch (sub_effect) {
                    case 0x9:  // Retrigger note
                        if (sub_param > 0 && player->tick > 0) {
                            chan->retrigger_count++;
                            if (chan->retrigger_count >= sub_param) {
                                chan->position = 0.0f;  // Restart sample
                                chan->retrigger_count = 0;

                                // Reset TrackerVoice position
                                tracker_voice_reset_position(&chan->voice_playback);
                            }
                        }
                        break;

                    case 0xC:  // Note cut
                        if (player->tick == sub_param) {
                            chan->volume = 0;
                        }
                        break;

                    case 0xD:  // Note delay
                        // Note delay is handled in the row processing
                        // (would need to delay triggering the note)
                        break;
                }
            }
            break;
    }
}

// Render one sample for a channel
static float render_channel(ModChannel* chan, uint32_t sample_rate, ModPlayer* player, uint8_t channel_num) {
    if (!chan->sample || chan->period == 0 || chan->muted) {
        return 0.0f;
    }

    const ModSample* sample = chan->sample;

    if (!sample->data || sample->length == 0) {
        return 0.0f;
    }

    // Calculate playback increment with vibrato
    // On Amiga, samples are played at: amiga_clock / (period * 2) Hz
    // This is how fast we traverse through the sample data
    // At our output sample rate, we need to advance through source samples at:
    // increment = amiga_playback_rate / output_sample_rate

    // Apply vibrato to period
    uint16_t effective_period = chan->period;
    if (chan->vibrato_depth > 0) {
        uint8_t vibrato_val = sine_table[chan->vibrato_pos & 0x3F];
        // Sine table values 0-127 are positive, 128-255 are negative (signed int8)
        int8_t signed_vibrato = (int8_t)vibrato_val;
        int16_t vibrato_delta = ((int16_t)signed_vibrato * chan->vibrato_depth) / 128;
        effective_period = chan->period + vibrato_delta;
        if (effective_period < 113) effective_period = 113;
        if (effective_period > 856) effective_period = 856;
    }

    float amiga_playback_rate = period_to_frequency(effective_period);
    chan->increment = amiga_playback_rate / (float)sample_rate;

    // Set TrackerVoice frequency/delta based on period
    tracker_voice_set_period(&chan->voice_playback, effective_period, AMIGA_CLOCK, sample_rate);

    // Get sample from TrackerVoice (handles looping automatically)
    int32_t sample_val = tracker_voice_get_sample(&chan->voice_playback);
    float output = (float)sample_val / 128.0f;

    // Sync old position field for compatibility with effects
    chan->position += chan->increment;

    // Apply volume with tremolo
    uint8_t effective_volume = chan->volume;

    if (chan->tremolo_depth > 0) {
        uint8_t tremolo_val = sine_table[chan->tremolo_pos & 0x3F];
        // Sine table values 0-127 are positive, 128-255 are negative (signed int8)
        int8_t signed_tremolo = (int8_t)tremolo_val;
        int16_t tremolo_delta = ((int16_t)signed_tremolo * chan->tremolo_depth) / 64;
        int16_t new_volume = (int16_t)chan->volume + tremolo_delta;
        if (new_volume < 0) new_volume = 0;
        if (new_volume > 64) new_volume = 64;
        effective_volume = (uint8_t)new_volume;
    }

    output *= (float)effective_volume / 64.0f;
    output *= chan->user_volume;

    return output;
}

void mod_player_process_channels(ModPlayer* player,
                                  float* left,
                                  float* right,
                                  float* channel_outputs[MOD_MAX_CHANNELS],
                                  uint32_t frames,
                                  uint32_t sample_rate) {
    if (!player || !left || !right) return;

    // Process pattern sequencer timing (calls mod_on_tick and mod_on_row callbacks)
    if (player->playing) {
        pattern_sequencer_process(player->sequencer, frames, sample_rate);
    }

    // Render audio for each frame
    for (uint32_t i = 0; i < frames; i++) {
        // Render each channel and optionally write to individual outputs
        TrackerMixerChannel mix_channels[MOD_MAX_CHANNELS];
        for (uint8_t c = 0; c < MOD_MAX_CHANNELS; c++) {
            float channel_sample = render_channel(&player->channels[c], sample_rate, player, c);
            mix_channels[c].sample = channel_sample;
            mix_channels[c].panning = player->channels[c].panning;
            mix_channels[c].enabled = 1;

            // Write to individual channel output if buffer provided
            if (channel_outputs && channel_outputs[c]) {
                channel_outputs[c][i] = channel_sample;
            }
        }

        // Mix to stereo using shared mixer
        tracker_mixer_mix_stereo(mix_channels, MOD_MAX_CHANNELS,
                                &left[i], &right[i],
                                0.5f);  // Amiga-style headroom
    }
}

void mod_player_process(ModPlayer* player, float* left, float* right, uint32_t frames, uint32_t sample_rate) {
    // Call the full version with no per-channel outputs
    mod_player_process_channels(player, left, right, NULL, frames, sample_rate);
}
