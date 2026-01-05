#include "mod_player.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Forward declarations
static void process_note(ModPlayer* player, uint8_t channel, const ModNote* note);

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

    // Playback state
    bool playing;
    uint8_t current_pattern_index;  // Index into song_positions
    uint8_t current_row;
    uint8_t tick;
    uint8_t speed;                  // Ticks per row (default 6)
    uint8_t bpm;                    // Beats per minute (default 125)

    // Loop control
    uint8_t loop_start;
    uint8_t loop_end;

    // Timing
    float samples_per_tick;
    float sample_accumulator;

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

// Helper: Get period for note and finetune
static uint16_t get_period(uint8_t note, int8_t finetune) {
    if (note == 0 || note > 36) return 0;

    // Adjust finetune to table index (0-15, with 8 = no finetune)
    int ft_index = 8 + (finetune & 0x0F);
    if (ft_index < 0) ft_index = 0;
    if (ft_index > 15) ft_index = 15;

    return period_table[ft_index][note - 1];
}

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

ModPlayer* mod_player_create(void) {
    ModPlayer* player = (ModPlayer*)calloc(1, sizeof(ModPlayer));
    if (!player) return NULL;

    // Set defaults
    player->speed = 6;
    player->bpm = 125;
    player->loop_start = 0;
    player->loop_end = 0;

    // Initialize channels with default panning (Amiga style)
    player->channels[0].panning = -0.5f;  // Left
    player->channels[1].panning = 0.5f;   // Right
    player->channels[2].panning = 0.5f;   // Right
    player->channels[3].panning = -0.5f;  // Left

    for (int i = 0; i < MOD_MAX_CHANNELS; i++) {
        player->channels[i].user_volume = 1.0f;
        player->channels[i].volume = 64;
    }

    return player;
}

void mod_player_destroy(ModPlayer* player) {
    if (!player) return;

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

    // Find highest pattern number (only scan valid song positions!)
    player->num_patterns = 0;
    for (int i = 0; i < player->song_length; i++) {
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

                // Debug first pattern (disabled)
                // static int debug_count = 0;
                // if (debug_count < 4 && (note->sample > 0 || note->period > 0)) {
                //     fprintf(stderr, "[PARSE] p=%d r=%d c=%d: bytes=%02X %02X %02X %02X -> sample=%d period=%d\n",
                //             p, r, c, b0, b1, b2, b3, note->sample, note->period);
                //     debug_count++;
                // }
            }
        }
    }

    // Load sample data
    // fprintf(stderr, "[OFFSET] After patterns, offset=%u\n", offset);
    // fprintf(stderr, "[LOAD] Loading %d samples...\n", MOD_MAX_SAMPLES);
    for (int i = 0; i < MOD_MAX_SAMPLES; i++) {
        ModSample* sample = &player->samples[i];
        uint32_t sample_length_bytes = sample->length * 2;  // Convert words to bytes

        if (sample_length_bytes > 0 && offset + sample_length_bytes <= size) {
            sample->data = (int8_t*)malloc(sample_length_bytes);
            if (sample->data) {
                memcpy(sample->data, data + offset, sample_length_bytes);
                // fprintf(stderr, "[LOAD] Sample %2d: %5d bytes, vol=%d, repeat=%d+%d\n",
                //         i+1, sample_length_bytes, sample->volume,
                //         sample->repeat_start, sample->repeat_length);
            }
            offset += sample_length_bytes;
        }
    }

    // Set default loop range
    player->loop_start = 0;
    player->loop_end = player->song_length > 0 ? player->song_length - 1 : 0;

    // Debug: print first pattern data that's actually stored (pattern 0, not song_positions[0])
    // fprintf(stderr, "[STORED] Pattern 0 Row 0:\n");
    // for (int c = 0; c < 4; c++) {
    //     uint32_t idx = 0 * (MOD_MAX_CHANNELS * MOD_PATTERN_ROWS) + 0 * MOD_MAX_CHANNELS + c;
    //     ModNote* n = &player->patterns[idx];
    //     fprintf(stderr, "[STORED] Ch%d: sample=%d period=%d effect=%X param=%02X\n",
    //             c, n->sample, n->period, n->effect, n->effect_param);
    // }

    return true;
}

void mod_player_start(ModPlayer* player) {
    if (!player) return;
    player->playing = true;
    player->current_pattern_index = player->loop_start;
    player->current_row = 0;
    player->tick = 0;
    player->sample_accumulator = 0.0f;

    // Process the first row immediately
    if (player->current_pattern_index < player->song_length) {
        uint8_t pattern_num = player->song_positions[player->current_pattern_index];

        if (pattern_num < player->num_patterns) {
            for (uint8_t c = 0; c < MOD_MAX_CHANNELS; c++) {
                uint32_t pattern_index = pattern_num * (MOD_MAX_CHANNELS * MOD_PATTERN_ROWS) +
                                        player->current_row * MOD_MAX_CHANNELS + c;
                const ModNote* note = &player->patterns[pattern_index];

                // Debug: print first row notes
                // if (note->sample > 0 || note->period > 0) {
                //     fprintf(stderr, "[NOTE] Ch%d: sample=%d period=%d effect=%X param=%02X\n",
                //             c, note->sample, note->period, note->effect, note->effect_param);
                // }

                process_note(player, c, note);

                // Debug: print channel state after processing
                // ModChannel* chan = &player->channels[c];
                // if (chan->sample && chan->sample->data) {
                //     fprintf(stderr, "[NOTE] Ch%d -> vol=%d period=%d sample_len=%d bytes\n",
                //             c, chan->volume, chan->period, chan->sample->length * 2);
                // } else if (chan->sample) {
                //     fprintf(stderr, "[NOTE] Ch%d -> vol=%d period=%d NO DATA!\n",
                //             c, chan->volume, chan->period);
                // }
            }
        }
    }
}

void mod_player_stop(ModPlayer* player) {
    if (!player) return;
    player->playing = false;

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
}

void mod_player_get_position(const ModPlayer* player, uint8_t* pattern, uint8_t* row) {
    if (!player) return;
    if (pattern) *pattern = player->current_pattern_index;
    if (row) *row = player->current_row;
}

void mod_player_set_position(ModPlayer* player, uint8_t pattern, uint8_t row) {
    if (!player) return;
    if (pattern < player->song_length && row < MOD_PATTERN_ROWS) {
        player->current_pattern_index = pattern;
        player->current_row = row;
        player->tick = 0;
    }
}

void mod_player_set_bpm(ModPlayer* player, uint8_t bpm) {
    if (!player) return;
    if (bpm < 32) bpm = 32;
    if (bpm > 255) bpm = 255;
    player->bpm = bpm;
}

void mod_player_set_speed(ModPlayer* player, uint8_t speed) {
    if (!player) return;
    if (speed == 0) speed = 1;
    player->speed = speed;
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

// Process note for a channel
static void process_note(ModPlayer* player, uint8_t channel, const ModNote* note) {
    ModChannel* chan = &player->channels[channel];

    // Handle sample trigger
    if (note->sample > 0 && note->sample <= MOD_MAX_SAMPLES) {
        const ModSample* sample = &player->samples[note->sample - 1];
        chan->sample = sample;
        chan->finetune = sample->finetune;
        chan->volume = sample->volume;  // Always set volume when sample changes
    }

    // Handle period (pitch)
    if (note->period > 0) {
        chan->period = note->period;
        chan->position = 0.0f;  // Reset playback position
    }

    // Store effect for processing
    chan->effect = note->effect;
    chan->effect_param = note->effect_param;

    // Process immediate effects
    switch (note->effect) {
        case 0xB:  // Position jump - jump to pattern
            if (note->effect_param < player->song_length) {
                player->current_pattern_index = note->effect_param;
                player->current_row = 0;
            }
            break;

        case 0xC:  // Set volume
            chan->volume = note->effect_param;
            if (chan->volume > 64) chan->volume = 64;
            break;

        case 0xD:  // Pattern break - skip to next pattern at row N
            {
                // Parameter is in BCD format: high nibble = tens, low nibble = ones
                uint8_t row = ((note->effect_param >> 4) * 10) + (note->effect_param & 0x0F);
                player->current_pattern_index++;
                if (player->current_pattern_index >= player->song_length) {
                    player->current_pattern_index = player->loop_start;
                }
                player->current_row = row;
                if (player->current_row >= MOD_PATTERN_ROWS) {
                    player->current_row = 0;
                }
            }
            break;

        case 0xF:  // Set speed/BPM
            if (note->effect_param > 0) {
                if (note->effect_param < 32) {
                    player->speed = note->effect_param;
                } else {
                    player->bpm = note->effect_param;
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
                chan->period -= chan->effect_param;
                if (chan->period < 113) chan->period = 113;
            }
            break;

        case 0x2:  // Portamento down
            if (chan->period > 0) {
                chan->period += chan->effect_param;
                if (chan->period > 856) chan->period = 856;
            }
            break;

        case 0xA:  // Volume slide
            if (player->tick != 0) {  // Not on first tick
                uint8_t up = (chan->effect_param >> 4) & 0x0F;
                uint8_t down = chan->effect_param & 0x0F;

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
    }
}

// Debug counter for render output
static uint64_t render_debug_counter = 0;

// Render one sample for a channel
static float render_channel(ModChannel* chan, uint32_t sample_rate) {
    if (!chan->sample || chan->period == 0 || chan->muted) {
        return 0.0f;
    }

    const ModSample* sample = chan->sample;
    if (!sample->data || sample->length == 0) {
        return 0.0f;
    }

    // Calculate playback increment
    // On Amiga, samples are played at: amiga_clock / (period * 2) Hz
    // This is how fast we traverse through the sample data
    // At our output sample rate, we need to advance through source samples at:
    // increment = amiga_playback_rate / output_sample_rate
    float amiga_playback_rate = period_to_frequency(chan->period);
    chan->increment = amiga_playback_rate / (float)sample_rate;

    // Debug: print first few render calls
    // if (render_debug_counter < 5) {
    //     fprintf(stderr, "[RENDER] period=%d freq=%.1f inc=%.6f pos=%.2f vol=%d\n",
    //             chan->period, amiga_playback_rate, chan->increment, chan->position, chan->volume);
    //     render_debug_counter++;
    // }

    // Get current sample
    uint32_t pos = (uint32_t)chan->position;
    uint32_t sample_len_bytes = sample->length * 2;

    if (pos >= sample_len_bytes) {
        // Handle looping
        if (sample->repeat_length > 1) {
            uint32_t loop_start = sample->repeat_start * 2;
            uint32_t loop_len = sample->repeat_length * 2;

            if (loop_len > 0 && loop_start + loop_len <= sample_len_bytes) {
                // Wrap within loop
                chan->position = loop_start + fmodf(chan->position - loop_start, (float)loop_len);
                pos = (uint32_t)chan->position;
            } else {
                return 0.0f;  // End of sample
            }
        } else {
            return 0.0f;  // End of sample
        }
    }

    // Get sample value (8-bit signed)
    int8_t sample_val = sample->data[pos];
    float output = (float)sample_val / 128.0f;

    // Debug: print first few sample values
    // static uint64_t sample_debug_counter = 0;
    // if (sample_debug_counter < 10) {
    //     fprintf(stderr, "[SAMPLE] pos=%u val=%d output=%.6f vol=%d\n",
    //             pos, sample_val, output, chan->volume);
    //     sample_debug_counter++;
    // }

    // Apply volume
    output *= (float)chan->volume / 64.0f;
    output *= chan->user_volume;

    // Advance position
    chan->position += chan->increment;

    return output;
}

void mod_player_process(ModPlayer* player, float* left, float* right, uint32_t frames, uint32_t sample_rate) {
    if (!player || !left || !right) return;

    // Calculate samples per tick
    // BPM to samples per tick: (2.5 * sample_rate) / BPM
    // NOTE: Speed (ticks/row) does NOT affect tick duration!
    player->samples_per_tick = (2.5f * sample_rate) / (float)player->bpm;

    for (uint32_t i = 0; i < frames; i++) {
        // Process player timing
        if (player->playing) {
            if (player->sample_accumulator >= player->samples_per_tick) {
                player->sample_accumulator -= player->samples_per_tick;
                player->tick++;

                if (player->tick >= player->speed) {
                    // Move to next row
                    player->tick = 0;
                    player->current_row++;

                    if (player->current_row >= MOD_PATTERN_ROWS) {
                        // Move to next pattern
                        player->current_row = 0;
                        player->current_pattern_index++;

                        // Handle loop
                        if (player->current_pattern_index > player->loop_end) {
                            player->current_pattern_index = player->loop_start;
                        }
                    }

                    // Process notes for this row
                    if (player->current_pattern_index < player->song_length) {
                        uint8_t pattern_num = player->song_positions[player->current_pattern_index];

                        if (pattern_num < player->num_patterns) {
                            for (uint8_t c = 0; c < MOD_MAX_CHANNELS; c++) {
                                uint32_t pattern_index = pattern_num * (MOD_MAX_CHANNELS * MOD_PATTERN_ROWS) +
                                                        player->current_row * MOD_MAX_CHANNELS + c;
                                const ModNote* note = &player->patterns[pattern_index];
                                process_note(player, c, note);
                            }
                        }
                    }
                }

                // Process per-tick effects
                for (uint8_t c = 0; c < MOD_MAX_CHANNELS; c++) {
                    process_effects(player, c);
                }
            }

            player->sample_accumulator += 1.0f;
        }

        // Render all channels
        float left_sample = 0.0f;
        float right_sample = 0.0f;

        for (uint8_t c = 0; c < MOD_MAX_CHANNELS; c++) {
            float sample = render_channel(&player->channels[c], sample_rate);

            // Apply panning
            float pan = player->channels[c].panning;
            float left_gain = 1.0f - (pan * 0.5f + 0.5f);   // 1.0 when pan = -1, 0.0 when pan = 1
            float right_gain = pan * 0.5f + 0.5f;            // 0.0 when pan = -1, 1.0 when pan = 1

            left_sample += sample * left_gain;
            right_sample += sample * right_gain;
        }

        // Output with some headroom
        left[i] = left_sample * 0.25f;
        right[i] = right_sample * 0.25f;
    }
}
