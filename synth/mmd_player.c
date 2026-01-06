/*
 * OctaMED MMD Player (MMD2/MMD3)
 * Supports MMD2/MMD3 format (modern 4-byte note format)
 * No MIDI support - pure sample playback
 */

#include "mmd_player.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>

// Endianness conversion (MMD files are big-endian)
#define BE16(x) ((uint16_t)( \
    (((x) & 0xFF00) >> 8) | \
    (((x) & 0x00FF) << 8)))

#define BE32(x) ((uint32_t)( \
    (((x) & 0xFF000000) >> 24) | \
    (((x) & 0x00FF0000) >> 8)  | \
    (((x) & 0x0000FF00) << 8)  | \
    (((x) & 0x000000FF) << 24)))

// MMD format constants
#define MMD2_ID 0x4D4D4432  // 'MMD2'
#define MMD3_ID 0x4D4D4433  // 'MMD3' (same structure as MMD2, just signals advanced mixing)
#define MAX_SAMPLES 63
#define MAX_CHANNELS 64
#define MAX_BLOCKS 256

// Instrument flags
#define INSTR_FLAG_STEREO  0x04
#define INSTR_FLAG_16BIT   0x08

// Period table for Amiga notes (same as MOD)
static const uint16_t period_table[12 * 10] = {
    // Octave 0
    1712, 1616, 1525, 1440, 1357, 1281, 1209, 1141, 1077, 1017, 961, 907,
    // Octave 1
    856, 808, 762, 720, 678, 640, 604, 570, 538, 508, 480, 453,
    // Octave 2
    428, 404, 381, 360, 339, 320, 302, 285, 269, 254, 240, 226,
    // Octave 3
    214, 202, 190, 180, 170, 160, 151, 143, 135, 127, 120, 113,
    // Octave 4
    107, 101, 95, 90, 85, 80, 76, 71, 67, 64, 60, 57,
    // Octave 5
    53, 50, 47, 45, 42, 40, 38, 36, 34, 32, 30, 28,
    // Octave 6
    27, 25, 24, 22, 21, 20, 19, 18, 17, 16, 15, 14,
    // Octave 7
    13, 13, 12, 11, 11, 10, 9, 9, 8, 8, 7, 7,
    // Octave 8
    7, 6, 6, 6, 5, 5, 5, 4, 4, 4, 4, 4,
    // Octave 9
    3, 3, 3, 3, 3, 2, 2, 2, 2, 2, 2, 2
};

// MMD2 Sample structure
typedef struct {
    uint16_t repeat;        // Repeat start (in words, not bytes)
    uint16_t replen;        // Repeat length (in words)
    uint8_t midich;         // MIDI channel (ignored)
    uint8_t midipreset;     // MIDI preset (ignored)
    uint8_t volume;         // Default volume (0-64)
    int8_t transpose;       // Sample transpose
} __attribute__((packed)) MMD0Sample;

// MMD2 Instrument header
typedef struct {
    uint32_t length;        // Sample length in bytes
    int16_t type;           // Type (-2 = sample)
} __attribute__((packed)) InstrHdr;

// MMD2 Instrument extension
typedef struct {
    uint8_t hold;
    uint8_t decay;
    uint8_t suppress_midi_off;
    int8_t finetune;
    uint8_t default_pitch;
    uint8_t instr_flags;    // Flags (stereo, 16-bit)
    uint16_t long_midi_preset;
    uint8_t output_device;
    uint8_t reserved;
    uint32_t long_repeat;   // Repeat start in bytes
    uint32_t long_replen;   // Repeat length in bytes
} __attribute__((packed)) InstrExt;

// Sample data
typedef struct {
    int8_t* data;           // Sample data (8-bit signed)
    uint32_t length;        // Length in bytes
    uint32_t repeat_start;  // Repeat start in bytes
    uint32_t repeat_length; // Repeat length in bytes
    int8_t volume;          // Default volume (0-64)
    int8_t transpose;       // Transpose
    int8_t finetune;        // Finetune (-8 to +7)
    bool is_stereo;         // Stereo flag
    bool is_16bit;          // 16-bit flag
} MedSample;

// MMD2 Note (4 bytes, unpacked - much simpler than MMD0!)
typedef struct {
    uint8_t note;           // Note (0 = none, 1-132)
    uint8_t instrument;     // Instrument (0 = none, 1-63)
    uint8_t command;        // Effect command
    uint8_t param;          // Effect parameter
} __attribute__((packed)) MMD2Note;

// Pattern/Block data
typedef struct {
    uint8_t num_tracks;     // Number of tracks
    uint16_t num_lines;     // Number of lines (rows)
    MMD2Note* notes;        // Note data [tracks][lines]
} MedBlock;

// Channel state
typedef struct {
    MedSample* sample;      // Current sample
    float position;         // Playback position
    float increment;        // Playback speed
    uint16_t period;        // Current period
    uint8_t volume;         // Current volume (0-64)
    int8_t finetune;        // Finetune
    bool muted;             // Mute flag
    float user_volume;      // User volume (0.0-1.0)
    int8_t panning;         // Panning (-16 to +16)

    // Effect state
    uint8_t vibrato_pos;
    uint8_t vibrato_depth;
    uint8_t vibrato_speed;
} MedChannel;

// Main player structure
struct MedPlayer {
    // File data
    uint8_t* file_data;
    size_t file_size;
    uint32_t song_offset;   // Offset to song structure (for reading MMD0sample array)

    // Song data
    uint8_t num_tracks;     // Number of tracks (4-64)
    uint16_t num_blocks;    // Number of blocks/patterns
    uint16_t song_length;   // Play sequence length
    uint16_t* play_seq;     // Play sequence
    MedBlock* blocks;       // Pattern data
    MedSample samples[MAX_SAMPLES];

    // Per-track settings
    uint8_t track_volumes[MAX_CHANNELS];
    int8_t track_pans[MAX_CHANNELS];

    // Playback state
    bool playing;
    uint16_t current_order;
    uint16_t current_pattern;
    uint8_t current_row;
    uint16_t bpm;
    uint8_t speed;          // Ticks per row
    uint32_t tick;
    uint32_t samples_per_tick;
    uint32_t sample_counter;

    // Channels
    MedChannel channels[MAX_CHANNELS];

    // Position callback
    MedPositionCallback position_callback;
    void* callback_user_data;
};

// Create player instance
MedPlayer* med_player_create(void) {
    MedPlayer* player = (MedPlayer*)calloc(1, sizeof(MedPlayer));
    if (!player) return NULL;

    player->bpm = 125;
    player->speed = 6;

    // Initialize channel volumes to max
    for (int i = 0; i < MAX_CHANNELS; i++) {
        player->channels[i].user_volume = 1.0f;
        player->track_volumes[i] = 64;
        player->track_pans[i] = 0;  // Center
    }

    return player;
}

// Destroy player instance
void med_player_destroy(MedPlayer* player) {
    if (!player) return;

    // Free sample data
    for (int i = 0; i < MAX_SAMPLES; i++) {
        if (player->samples[i].data) {
            free(player->samples[i].data);
        }
    }

    // Free blocks
    if (player->blocks) {
        for (int i = 0; i < player->num_blocks; i++) {
            if (player->blocks[i].notes) {
                free(player->blocks[i].notes);
            }
        }
        free(player->blocks);
    }

    // Free play sequence
    if (player->play_seq) {
        free(player->play_seq);
    }

    // Free file data
    if (player->file_data) {
        free(player->file_data);
    }

    free(player);
}

// Read pointer from file (convert big-endian offset to usable pointer)
static const uint8_t* read_ptr(const uint8_t* base, uint32_t offset) {
    if (offset == 0) return NULL;
    return base + offset;
}

// Load MMD2 file
bool med_player_load(MedPlayer* player, const uint8_t* data, size_t size) {
    if (!player || !data || size < 52) return false;

    // Copy file data
    player->file_data = (uint8_t*)malloc(size);
    if (!player->file_data) return false;
    memcpy(player->file_data, data, size);
    player->file_size = size;

    const uint8_t* base = player->file_data;
    const uint8_t* ptr = base;

    // Read header (52 bytes)
    uint32_t id = BE32(*(uint32_t*)(ptr + 0));
    uint32_t modlen = BE32(*(uint32_t*)(ptr + 4));
    player->song_offset = BE32(*(uint32_t*)(ptr + 8));  // Save for later use
    uint32_t blockarr_offset = BE32(*(uint32_t*)(ptr + 16));
    uint32_t smplarr_offset = BE32(*(uint32_t*)(ptr + 24));

    // Check format (accept both MMD2 and MMD3 - they're structurally identical)
    if (id != MMD2_ID && id != MMD3_ID) {
        fprintf(stderr, "med_player: Invalid format ID: 0x%08X (expected MMD2 0x%08X or MMD3 0x%08X)\n",
                id, MMD2_ID, MMD3_ID);
        free(player->file_data);
        player->file_data = NULL;
        return false;
    }

    fprintf(stderr, "med_player: Detected format: %s\n", id == MMD2_ID ? "MMD2" : "MMD3");
    fprintf(stderr, "med_player: Module length: %u bytes\n", modlen);

    // Read MMD2song structure (simplified - skip to important parts)
    const uint8_t* song_ptr = read_ptr(base, player->song_offset);
    if (!song_ptr) return false;

    // Skip sample array (63 * 8 = 504 bytes)
    song_ptr += 504;

    // Read song info
    player->num_blocks = BE16(*(uint16_t*)(song_ptr + 0));
    uint16_t songlen_deprecated = BE16(*(uint16_t*)(song_ptr + 2));

    // Read playseqtable offset
    uint32_t playseq_offset = BE32(*(uint32_t*)(song_ptr + 4));

    // Read numtracks (offset 520 from song start = song_ptr + 16)
    player->num_tracks = BE16(*(uint16_t*)(song_ptr + 16));

    // Read numpseqs (offset 522 from song start = song_ptr + 18)
    // This is the actual song length in MMD2, not songlen which is deprecated
    uint16_t numpseqs = BE16(*(uint16_t*)(song_ptr + 18));

    // Read first play sequence to get actual length
    if (playseq_offset != 0 && numpseqs > 0) {
        const uint8_t* playseq_table = read_ptr(base, playseq_offset);
        if (playseq_table) {
            // Read first PlaySeq structure
            uint32_t first_seq_offset = BE32(*(uint32_t*)(playseq_table + 0));
            const uint8_t* seq_ptr = read_ptr(base, first_seq_offset);
            if (seq_ptr) {
                // PlaySeq structure: name[32] + reserved1[4] + reserved2[4] + length[2] + seq[...]
                player->song_length = BE16(*(uint16_t*)(seq_ptr + 40));  // offset 40 = 32 + 4 + 4

                // Read sequence data
                player->play_seq = (uint16_t*)malloc(player->song_length * sizeof(uint16_t));
                for (int i = 0; i < player->song_length; i++) {
                    player->play_seq[i] = BE16(*(uint16_t*)(seq_ptr + 42 + i * 2));
                }
            }
        }
    }

    if (player->song_length == 0) {
        // Fallback to deprecated songlen
        player->song_length = songlen_deprecated;
        if (player->song_length == 0) player->song_length = 1;

        // Create simple linear sequence
        player->play_seq = (uint16_t*)malloc(player->song_length * sizeof(uint16_t));
        for (int i = 0; i < player->song_length; i++) {
            player->play_seq[i] = i;
        }
    }

    if (player->num_tracks > MAX_CHANNELS) {
        fprintf(stderr, "med_player: WARNING: Clamping tracks from %d to %d\n",
                player->num_tracks, MAX_CHANNELS);
        player->num_tracks = MAX_CHANNELS;
    }

    fprintf(stderr, "med_player: Tracks: %d, Blocks: %d, Song length: %d\n",
            player->num_tracks, player->num_blocks, player->song_length);

    // Read BPM and flags from later in structure
    const uint8_t* song_base = read_ptr(base, player->song_offset);
    uint16_t deftempo = BE16(*(uint16_t*)(song_base + 764));
    uint8_t flags = song_base[767];
    uint8_t flags2 = song_base[768];
    uint8_t tempo2 = song_base[769];

    fprintf(stderr, "med_player: Raw deftempo: %d, flags: 0x%02X, flags2: 0x%02X, tempo2: %d\n",
            deftempo, flags, flags2, tempo2);

    // OctaMED tempo handling (from OpenMPT's MMDTempoToBPM):
    bool bpmMode = (flags2 & 0x20) != 0;  // FLAG2_BPM
    bool softwareMixing = (flags2 & 0x80) != 0;  // FLAG2_MIX
    bool is8Ch = (flags & 0x40) != 0;  // FLAG_8CHANNEL
    uint8_t rowsPerBeat = 1 + (flags2 & 0x1F);  // FLAG2_BMASK

    if (bpmMode && !is8Ch) {
        // BPM mode: tempo = (deftempo * rowsPerBeat) / 4
        player->bpm = (deftempo * rowsPerBeat) / 4;
    } else if (softwareMixing && deftempo < 8) {
        // Software mixing with low tempo
        player->bpm = 158;  // 157.86 rounded
    } else {
        // Default: tempo = deftempo / 0.264
        player->bpm = (uint16_t)(deftempo / 0.264f);
    }

    // Read ticks per row from tempo2 (if non-zero)
    if (tempo2 > 0) {
        player->speed = tempo2;
    }

    fprintf(stderr, "med_player: BPM mode: %d, Software mixing: %d, 8ch: %d, Rows/beat: %d\n",
            bpmMode, softwareMixing, is8Ch, rowsPerBeat);
    fprintf(stderr, "med_player: Calculated BPM: %d, Speed (ticks/row): %d\n",
            player->bpm, player->speed);

    // Read blocks
    player->blocks = (MedBlock*)calloc(player->num_blocks, sizeof(MedBlock));
    const uint8_t* blockarr_ptr = read_ptr(base, blockarr_offset);

    int blocks_with_data = 0;
    for (int i = 0; i < player->num_blocks; i++) {
        uint32_t block_offset = BE32(*(uint32_t*)(blockarr_ptr + i * 4));
        const uint8_t* block_ptr = read_ptr(base, block_offset);
        if (!block_ptr) continue;

        // MMD BlockInfo structure:
        // +0: BE16 numtracks
        // +2: BE16 numlines (stored as lines-1)
        // +4: BE32 reserved
        // +8: Note data begins here (4 bytes per note)
        uint16_t numtracks = BE16(*(uint16_t*)(block_ptr + 0));
        uint16_t lines = BE16(*(uint16_t*)(block_ptr + 2));

        player->blocks[i].num_tracks = numtracks;
        player->blocks[i].num_lines = lines + 1;  // Stored as lines-1

        // Note data starts immediately after 8-byte BlockInfo header
        const uint8_t* notes_ptr = block_ptr + 8;
        size_t note_count = numtracks * (lines + 1);
        player->blocks[i].notes = (MMD2Note*)malloc(note_count * sizeof(MMD2Note));
        memcpy(player->blocks[i].notes, notes_ptr, note_count * sizeof(MMD2Note));
        blocks_with_data++;

        // Debug first few notes
        if (blocks_with_data == 1) {  // First block with data
            fprintf(stderr, "med_player: Block %d has %d tracks, %d lines, note_count=%zu\n",
                    i, numtracks, lines+1, note_count);
            fprintf(stderr, "med_player: First note bytes in block %d:\n", i);
            for (int n = 0; n < 16 && n < (int)note_count; n++) {
                fprintf(stderr, "  Note %d (track %d, row %d): [%02X %02X %02X %02X]\n",
                        n, n % numtracks, n / numtracks,
                        notes_ptr[n*4+0], notes_ptr[n*4+1],
                        notes_ptr[n*4+2], notes_ptr[n*4+3]);
            }
        }
    }

    fprintf(stderr, "med_player: Loaded %d/%d blocks with note data\n", blocks_with_data, player->num_blocks);

    // Read instruments/samples
    fprintf(stderr, "med_player: Loading samples from offset 0x%X...\n", smplarr_offset);
    int samples_loaded = 0;
    const uint8_t* smplarr_ptr = read_ptr(base, smplarr_offset);

    if (!smplarr_ptr) {
        fprintf(stderr, "med_player: WARNING: No sample array found\n");
    } else {
        for (int i = 0; i < MAX_SAMPLES; i++) {
            uint32_t instr_offset = BE32(*(uint32_t*)(smplarr_ptr + i * 4));
            if (instr_offset == 0) continue;

            fprintf(stderr, "med_player: Loading sample %d from offset 0x%X\n", i, instr_offset);

            const uint8_t* instr_ptr = read_ptr(base, instr_offset);
            if (!instr_ptr) {
                fprintf(stderr, "med_player: ERROR: Invalid instrument pointer for sample %d\n", i);
                continue;
            }

            // Bounds check
            if (instr_ptr + 6 >= base + player->file_size) {
                fprintf(stderr, "med_player: ERROR: Instrument header out of bounds for sample %d\n", i);
                continue;
            }

            // Read InstrHdr
            uint32_t length = BE32(*(uint32_t*)(instr_ptr + 0));
            int16_t type_and_flags = (int16_t)BE16(*(uint16_t*)(instr_ptr + 4));

            // Extract type and flags
            int16_t type = type_and_flags & 0x0F;  // Low nibble is type
            bool is_16bit = (type_and_flags & 0x10) != 0;  // S_16 flag
            bool is_stereo = (type_and_flags & 0x20) != 0;  // STEREO flag

            fprintf(stderr, "med_player:   Sample %d: length=%u, type=%d, flags=0x%02X (16bit=%d, stereo=%d)\n",
                    i, length, type, type_and_flags, is_16bit, is_stereo);

            // Handle modern sample format (type -2)
            if (type == -2) {  // INSTR_SAMPLE
                fprintf(stderr, "med_player:   Handling type %d sample...\n", type);

                // Bounds check for InstrExt
                size_t instrext_end = (instr_ptr - base) + 6 + 18;
                fprintf(stderr, "med_player:   InstrExt ends at offset 0x%zX (file size 0x%zX)\n",
                        instrext_end, player->file_size);
                if (instrext_end > player->file_size) {
                    fprintf(stderr, "med_player: ERROR: InstrExt out of bounds for sample %d\n", i);
                    continue;
                }

                // Read InstrExt
                const uint8_t* ext_ptr = instr_ptr + 6;
                int8_t finetune = ext_ptr[3];
                uint8_t instr_flags = ext_ptr[5];
                uint32_t long_repeat = BE32(*(uint32_t*)(ext_ptr + 10));
                uint32_t long_replen = BE32(*(uint32_t*)(ext_ptr + 14));

                // Bounds check for sample data
                size_t sample_end = instrext_end + length;
                fprintf(stderr, "med_player:   Sample data ends at offset 0x%zX\n", sample_end);
                if (sample_end > player->file_size) {
                    fprintf(stderr, "med_player: ERROR: Sample data out of bounds for sample %d (needs %u bytes)\n",
                            i, length);
                    continue;
                }

                // Allocate and copy sample data
                player->samples[i].length = length;
                player->samples[i].data = (int8_t*)malloc(length);
                if (!player->samples[i].data) {
                    fprintf(stderr, "med_player: ERROR: Failed to allocate %u bytes for sample %d\n", length, i);
                    continue;
                }
                memcpy(player->samples[i].data, ext_ptr + 18, length);

                player->samples[i].repeat_start = long_repeat;
                player->samples[i].repeat_length = long_replen;
                player->samples[i].finetune = finetune;
                player->samples[i].volume = 64;  // Default volume
                player->samples[i].is_stereo = (instr_flags & INSTR_FLAG_STEREO) != 0;
                player->samples[i].is_16bit = (instr_flags & INSTR_FLAG_16BIT) != 0;

                samples_loaded++;
                fprintf(stderr, "med_player:   ✓ Loaded sample %d (%u bytes)\n", i, length);
            }
            // Handle old octave-based sample formats (type 0-6)
            else if (type >= 0 && type <= 6) {
                fprintf(stderr, "med_player:   Old octave-based sample (type %d)\n", type);

                // Bounds check for sample data (comes right after InstrHdr, no InstrExt)
                if (instr_ptr + 6 + length > base + player->file_size) {
                    fprintf(stderr, "med_player: ERROR: Sample data out of bounds for sample %d (needs %u bytes)\n",
                            i, length);
                    continue;
                }

                // Allocate and copy sample data (starts right after InstrHdr)
                player->samples[i].length = length;
                player->samples[i].data = (int8_t*)malloc(length);
                if (!player->samples[i].data) {
                    fprintf(stderr, "med_player: ERROR: Failed to allocate %u bytes for sample %d\n", length, i);
                    continue;
                }
                memcpy(player->samples[i].data, instr_ptr + 6, length);

                // For old samples, get repeat/volume info from MMD0sample array at start of song
                // song_base + 0 to song_base + 503 is the sample array (63 samples * 8 bytes each)
                const uint8_t* song_base = read_ptr(base, player->song_offset);
                const uint8_t* sample_info = song_base + (i * 8);  // Each MMD0sample is 8 bytes

                uint16_t rep_words = BE16(*(uint16_t*)(sample_info + 0));
                uint16_t replen_words = BE16(*(uint16_t*)(sample_info + 2));
                uint8_t svol = sample_info[6];
                int8_t strans = (int8_t)sample_info[7];

                player->samples[i].repeat_start = rep_words * 2;  // Convert words to bytes
                player->samples[i].repeat_length = replen_words * 2;
                player->samples[i].volume = svol;
                player->samples[i].transpose = strans;
                player->samples[i].finetune = 0;
                player->samples[i].is_stereo = is_stereo;
                player->samples[i].is_16bit = is_16bit;

                samples_loaded++;
                fprintf(stderr, "med_player:   ✓ Loaded old-format sample %d (%u bytes, vol=%d, 16bit=%d, stereo=%d)\n",
                        i, length, svol, is_16bit, is_stereo);
            }
        }
    }

    fprintf(stderr, "med_player: Loaded %d samples\n", samples_loaded);

    // Show which sample slots have data
    fprintf(stderr, "med_player: Sample slots with data: ");
    for (int i = 0; i < MAX_SAMPLES; i++) {
        if (player->samples[i].data) {
            fprintf(stderr, "%d ", i + 1);  // Show as 1-based
        }
    }
    fprintf(stderr, "\n");

    fprintf(stderr, "med_player: File loaded successfully!\n");

    return true;
}

// Get period for note
static uint16_t get_note_period(uint8_t note, int8_t finetune) {
    if (note == 0) return 0;

    // MMD2/MMD3: Notes are stored 1 octave higher than ProTracker format
    // OctaMED uses C-3 as middle C, ProTracker uses C-2
    // Subtract 12 semitones (1 octave) to match ProTracker period table
    if (note < 13) return 0;  // Safety check
    note -= 12;

    if (note > 120) return 0;

    // Note 1 = C-0, note 13 = C-1, etc.
    int index = note - 1;
    if (index < 0 || index >= 120) return 0;

    return period_table[index];
}

// Trigger note on channel
static void trigger_note(MedPlayer* player, int channel, uint8_t note, uint8_t instrument) {
    MedChannel* chan = &player->channels[channel];

    static int trigger_count = 0;
    if (trigger_count < 10) {
        fprintf(stderr, "med_player: trigger_note ch=%d note=%d inst=%d\n", channel, note, instrument);
        trigger_count++;
    }

    // Set instrument
    if (instrument > 0 && instrument <= MAX_SAMPLES) {
        MedSample* smp = &player->samples[instrument - 1];
        if (smp->data) {
            chan->sample = smp;
            chan->volume = chan->sample->volume;
            chan->finetune = chan->sample->finetune;
        } else if (trigger_count < 10) {
            fprintf(stderr, "  WARNING: instrument %d has no sample data!\n", instrument);
        }
    }

    // Set note
    if (note > 0 && chan->sample && chan->sample->data) {
        chan->period = get_note_period(note, chan->finetune);
        chan->position = 0.0f;

        if (trigger_count < 10) {
            // Calculate frequency for debugging
            float freq = 7093789.2f / ((float)chan->period * 2.0f);
            fprintf(stderr, "  -> period=%d (%.1f Hz), volume=%d, sample_len=%u\n",
                    chan->period, freq, chan->volume, chan->sample->length);
        }
    }
}

// Process one tick
static void process_tick(MedPlayer* player) {
    if (!player->playing) return;

    MedBlock* block = &player->blocks[player->current_pattern];
    if (!block->notes) return;

    if (player->tick == 0) {
        // Process new row
        for (int ch = 0; ch < player->num_tracks; ch++) {
            int note_idx = player->current_row * block->num_tracks + ch;
            MMD2Note* note = &block->notes[note_idx];

            if (note->note > 0) {
                trigger_note(player, ch, note->note, note->instrument);
            }

            // Process effects (simplified)
            if (note->command == 0x0C) {  // Set volume
                player->channels[ch].volume = note->param;
            }
        }

        // Fire position callback
        if (player->position_callback) {
            player->position_callback(player->current_order, player->current_pattern,
                                    player->current_row, player->callback_user_data);
        }
    }

    player->tick++;
    if (player->tick >= player->speed) {
        player->tick = 0;
        player->current_row++;

        if (player->current_row >= block->num_lines) {
            player->current_row = 0;
            player->current_order++;

            if (player->current_order >= player->song_length) {
                player->current_order = 0;
            }

            player->current_pattern = player->play_seq[player->current_order];
        }
    }
}

// Start playback
void med_player_start(MedPlayer* player) {
    if (player) {
        player->playing = true;
        fprintf(stderr, "med_player: Playback started\n");
    }
}

// Stop playback
void med_player_stop(MedPlayer* player) {
    if (player) {
        player->playing = false;
        fprintf(stderr, "med_player: Playback stopped\n");
    }
}

// Process audio
void med_player_process(MedPlayer* player, float* left_out, float* right_out,
                       size_t frames, float sample_rate) {
    if (!player || !left_out || !right_out) return;

    static int debug_counter = 0;
    if (debug_counter++ % 1000 == 0) {
        fprintf(stderr, "med_player: process() called, playing=%d, pattern=%d, row=%d\n",
                player->playing, player->current_pattern, player->current_row);
    }

    // Calculate samples per tick
    // Formula scales with speed (ticks per row)
    player->samples_per_tick = (uint32_t)((sample_rate * (float)player->speed) / (2.0f * (float)player->bpm));

    for (size_t i = 0; i < frames; i++) {
        float left = 0.0f;
        float right = 0.0f;

        // Mix all channels
        for (int ch = 0; ch < player->num_tracks; ch++) {
            MedChannel* chan = &player->channels[ch];
            if (!chan->sample || !chan->sample->data || chan->muted) continue;

            // Get sample
            int sample_idx = (int)chan->position;
            uint32_t sample_len = chan->sample->length;

            // For 16-bit samples, length is in bytes but we need sample count
            if (chan->sample->is_16bit) {
                sample_len /= 2;
            }

            if (sample_idx >= 0 && sample_idx < (int)sample_len) {
                float sample;
                if (chan->sample->is_16bit) {
                    // Read 16-bit sample (stored as big-endian int16)
                    int16_t* data16 = (int16_t*)chan->sample->data;
                    int16_t sample16 = BE16(data16[sample_idx]);
                    sample = sample16 / 32768.0f;
                } else {
                    // Read 8-bit sample
                    sample = chan->sample->data[sample_idx] / 128.0f;
                }

                float vol = (chan->volume / 64.0f) * chan->user_volume;

                // Apply panning
                float pan = (chan->panning + 16) / 32.0f;  // -16..+16 -> 0..1
                left += sample * vol * (1.0f - pan);
                right += sample * vol * pan;
            }

            // Advance position
            if (chan->period > 0) {
                // For OctaMED, use standard Amiga frequency calculation
                // Same as MOD player: freq = PAL_CLOCK / (period * 2)
                float freq = 7093789.2f / ((float)chan->period * 2.0f);
                chan->increment = freq / sample_rate;
                chan->position += chan->increment;

                // Handle loop
                if (chan->sample->repeat_length > 1) {
                    float loop_end = chan->sample->repeat_start + chan->sample->repeat_length;
                    if (chan->position >= loop_end) {
                        chan->position = chan->sample->repeat_start +
                                       fmodf(chan->position - chan->sample->repeat_start,
                                            chan->sample->repeat_length);
                    }
                }
            }
        }

        left_out[i] = left;
        right_out[i] = right;

        // Process tick timing
        player->sample_counter++;
        if (player->sample_counter >= player->samples_per_tick) {
            player->sample_counter = 0;
            process_tick(player);
        }
    }
}

// Get position
void med_player_get_position(const MedPlayer* player, uint8_t* out_pattern, uint8_t* out_row) {
    if (!player) return;
    if (out_pattern) *out_pattern = player->current_pattern;
    if (out_row) *out_row = player->current_row;
}

// Set position
void med_player_set_position(MedPlayer* player, uint8_t pattern, uint8_t row) {
    if (!player) return;
    player->current_pattern = pattern;
    player->current_row = row;
    player->tick = 0;
}

// Get song length
uint8_t med_player_get_song_length(const MedPlayer* player) {
    return player ? player->song_length : 0;
}

// Set position callback
void med_player_set_position_callback(MedPlayer* player, MedPositionCallback callback, void* user_data) {
    if (!player) return;
    player->position_callback = callback;
    player->callback_user_data = user_data;
}

// Set channel mute
void med_player_set_channel_mute(MedPlayer* player, uint8_t channel, bool muted) {
    if (!player || channel >= MAX_CHANNELS) return;
    player->channels[channel].muted = muted;
}

// Get channel mute
bool med_player_get_channel_mute(const MedPlayer* player, uint8_t channel) {
    if (!player || channel >= MAX_CHANNELS) return false;
    return player->channels[channel].muted;
}

// Set channel volume
void med_player_set_channel_volume(MedPlayer* player, uint8_t channel, float volume) {
    if (!player || channel >= MAX_CHANNELS) return;
    player->channels[channel].user_volume = volume;
}

// Get channel volume
float med_player_get_channel_volume(const MedPlayer* player, uint8_t channel) {
    if (!player || channel >= MAX_CHANNELS) return 0.0f;
    return player->channels[channel].user_volume;
}

// Set BPM
void med_player_set_bpm(MedPlayer* player, uint16_t bpm) {
    if (!player) return;
    if (bpm > 0) player->bpm = bpm;
}

// Get BPM
uint16_t med_player_get_bpm(const MedPlayer* player) {
    return player ? player->bpm : 0;
}
