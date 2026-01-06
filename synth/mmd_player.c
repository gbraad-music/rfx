/*
 * OctaMED MMD Player (MMD2/MMD3)
 * Supports MMD2/MMD3 format (modern 4-byte note format)
 * No MIDI support - pure sample playback
 *
 * Conditional features:
 * - MMD_SYNTH_SUPPORT: Enable OctaMED synth instruments (SYNTHETIC/HYBRID types)
 */

#include "mmd_player.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>

#ifdef MMD_SYNTH_SUPPORT
// Include synth components for OctaMED synthetic instruments
#include "synth_oscillator.h"
#include "synth_envelope.h"
#include "synth_lfo.h"
#endif

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

// Instrument types (for type field in InstrHdr)
#define INSTR_TYPE_HYBRID     -2  // Sample with synth scripts
#define INSTR_TYPE_SYNTHETIC  -1  // Pure synth (no sample)
#define INSTR_TYPE_SAMPLE      0  // Regular sample (or octave-based 1-6)

#ifdef MMD_SYNTH_SUPPORT
// Synth script commands
#define SYNTH_CMD_SET_VOLUME      0x00  // 0x00-0x40: Set volume (0-64)
#define SYNTH_CMD_SET_WAVEFORM    0x00  // 0x00-0x3F: Set waveform (0-63)
#define SYNTH_CMD_SPD             0xF0  // Set speed
#define SYNTH_CMD_WAI             0xF1  // Wait
#define SYNTH_CMD_CHD             0xF2  // Change down
#define SYNTH_CMD_CHU             0xF3  // Change up
#define SYNTH_CMD_VBD             0xF4  // Vibrato depth (wave) / Envelope (vol)
#define SYNTH_CMD_VBS             0xF5  // Vibrato speed (wave) / Envelope (vol)
#define SYNTH_CMD_RES             0xF6  // Reset
#define SYNTH_CMD_VWF             0xF7  // Vibrato waveform
#define SYNTH_CMD_JWS             0xFA  // Jump script
#define SYNTH_CMD_ARP             0xFC  // Begin arpeggio
#define SYNTH_CMD_ARE             0xFD  // End arpeggio
#define SYNTH_CMD_JMP             0xFE  // Jump to position
#define SYNTH_CMD_END             0xFF  // End sequence
#define SYNTH_CMD_HLT             0xFB  // Halt

#define MAX_WAVEFORMS 64
#define MAX_SYNTH_SCRIPT 128
#endif

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

#ifdef MMD_SYNTH_SUPPORT
// MMD Synth Instrument (file format)
typedef struct {
    uint8_t default_decay;
    uint8_t reserved[3];
    uint16_t loop_start;      // Only for hybrid
    uint16_t loop_length;
    uint16_t vol_table_len;
    uint16_t wave_table_len;
    uint8_t vol_speed;
    uint8_t wave_speed;
    uint16_t num_waveforms;
    uint8_t vol_table[MAX_SYNTH_SCRIPT];
    uint8_t wave_table[MAX_SYNTH_SCRIPT];
} __attribute__((packed)) MMDSynthInstr;

// Synth script state (runtime)
typedef struct {
    uint8_t pc;              // Program counter
    uint8_t speed;           // Ticks per step
    uint8_t tick_counter;    // Current tick
    uint8_t wait_counter;    // Wait counter
    bool active;             // Script is running
} SynthScript;

// Synth instrument state (runtime)
typedef struct {
    // Waveforms (each is 256 samples)
    int8_t* waveforms[MAX_WAVEFORMS];
    uint16_t waveform_lengths[MAX_WAVEFORMS];
    uint8_t num_waveforms;

    // Script data
    uint8_t vol_table[MAX_SYNTH_SCRIPT];
    uint8_t wave_table[MAX_SYNTH_SCRIPT];
    uint16_t vol_table_len;
    uint16_t wave_table_len;
    uint8_t vol_speed;
    uint8_t wave_speed;

    // Current state
    uint8_t current_waveform;
    uint8_t current_volume;   // 0-64
    SynthScript vol_script;
    SynthScript wave_script;

    // Phase tracking for wavetable playback
    float phase;  // 0.0 to 1.0
} SynthInstrument;
#endif

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

#ifdef MMD_SYNTH_SUPPORT
    bool is_synth;          // Synthetic instrument
    bool is_hybrid;         // Hybrid (sample + synth)
    SynthInstrument* synth; // Synth data (NULL if not synth)
#endif
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
    uint8_t portamento_up;      // Portamento up speed
    uint8_t portamento_down;    // Portamento down speed
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
#ifdef MMD_SYNTH_SUPPORT
        // Free synth data
        if (player->samples[i].synth) {
            for (int w = 0; w < player->samples[i].synth->num_waveforms; w++) {
                if (player->samples[i].synth->waveforms[w]) {
                    free(player->samples[i].synth->waveforms[w]);
                }
            }
            free(player->samples[i].synth);
        }
#endif
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

#ifdef MMD_SYNTH_SUPPORT
// Initialize synth script
static void synth_script_init(SynthScript* script, uint8_t speed) {
    script->pc = 0;
    script->speed = speed > 0 ? speed : 1;
    script->tick_counter = 0;
    script->wait_counter = 0;
    script->active = true;
}

// Execute one tick of synth script
// Returns: updated value (volume 0-64 or waveform 0-63)
static uint8_t synth_script_tick(SynthScript* script, const uint8_t* table, uint16_t table_len,
                                  uint8_t current_value, bool is_volume) {
    if (!script->active || table_len == 0) {
        return current_value;
    }

    // Wait handling
    if (script->wait_counter > 0) {
        script->wait_counter--;
        return current_value;
    }

    // Speed handling (ticks per step)
    script->tick_counter++;
    if (script->tick_counter < script->speed) {
        return current_value;
    }
    script->tick_counter = 0;

    // Execute command
    if (script->pc >= table_len) {
        script->active = false;
        return current_value;
    }

    uint8_t cmd = table[script->pc++];

    // Simple implementation - just handle basic commands for now
    if (cmd >= 0xF0) {
        switch (cmd) {
            case SYNTH_CMD_SPD:  // Set speed
                if (script->pc < table_len) {
                    script->speed = table[script->pc++];
                    if (script->speed == 0) script->speed = 1;
                }
                break;

            case SYNTH_CMD_WAI:  // Wait
                if (script->pc < table_len) {
                    script->wait_counter = table[script->pc++];
                }
                break;

            case SYNTH_CMD_JMP:  // Jump
                if (script->pc < table_len) {
                    uint8_t target = table[script->pc++];
                    if (target < table_len) {
                        script->pc = target;
                    }
                }
                break;

            case SYNTH_CMD_END:  // End
            case SYNTH_CMD_HLT:  // Halt
                script->active = false;
                break;

            default:
                // Unknown command - skip
                break;
        }
    } else {
        // Direct value (volume 0-64 or waveform 0-63)
        if (is_volume) {
            current_value = (cmd <= 64) ? cmd : 64;
        } else {
            current_value = (cmd < 64) ? cmd : 0;
        }
    }

    return current_value;
}

// Process synth instrument - generate one sample
static float synth_instrument_process(SynthInstrument* synth, float freq, float sample_rate) {
    if (!synth || synth->num_waveforms == 0) {
        return 0.0f;
    }

    // Get current waveform
    uint8_t wf_idx = synth->current_waveform;
    if (wf_idx >= synth->num_waveforms || !synth->waveforms[wf_idx]) {
        return 0.0f;
    }

    // Generate sample from current waveform
    int8_t* waveform = synth->waveforms[wf_idx];
    uint16_t wf_len = synth->waveform_lengths[wf_idx];

    // Wavetable playback with phase tracking
    int pos = (int)(synth->phase * wf_len) % wf_len;
    float sample = waveform[pos] / 128.0f;

    // Advance phase
    float phase_inc = freq / sample_rate;
    synth->phase += phase_inc;
    if (synth->phase >= 1.0f) {
        synth->phase -= 1.0f;
    }

    // Apply volume
    float volume = synth->current_volume / 64.0f;
    return sample * volume;
}
#endif

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

    // fprintf(stderr, "med_player: Detected format: %s\n", id == MMD2_ID ? "MMD2" : "MMD3");
    // fprintf(stderr, "med_player: Module length: %u bytes\n", modlen);

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

                // Debug: show first entries in play sequence
                // fprintf(stderr, "med_player: Play sequence (first 10 entries): ");
                // int debug_count = (player->song_length < 10) ? player->song_length : 10;
                // for (int i = 0; i < debug_count; i++) {
                //     fprintf(stderr, "%d ", player->play_seq[i]);
                // }
                // fprintf(stderr, "\n");
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

    // fprintf(stderr, "med_player: Tracks: %d, Blocks: %d, Song length: %d\n",
    //         player->num_tracks, player->num_blocks, player->song_length);

    // Initialize playback position from play sequence
    if (player->play_seq && player->song_length > 0) {
        player->current_order = 0;
        player->current_pattern = player->play_seq[0];
        player->current_row = 0;
        // fprintf(stderr, "med_player: Initial position: order 0, pattern %d\n", player->current_pattern);
    }

    // Read BPM and flags from later in structure
    const uint8_t* song_base = read_ptr(base, player->song_offset);
    uint16_t deftempo = BE16(*(uint16_t*)(song_base + 764));
    uint8_t flags = song_base[767];
    uint8_t flags2 = song_base[768];
    uint8_t tempo2 = song_base[769];

    // fprintf(stderr, "med_player: Raw deftempo: %d, flags: 0x%02X, flags2: 0x%02X, tempo2: %d\n",
    //         deftempo, flags, flags2, tempo2);

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

    // fprintf(stderr, "med_player: BPM mode: %d, Software mixing: %d, 8ch: %d, Rows/beat: %d\n",
    //         bpmMode, softwareMixing, is8Ch, rowsPerBeat);
    // fprintf(stderr, "med_player: Calculated BPM: %d, Speed (ticks/row): %d\n",
    //         player->bpm, player->speed);

    // Read track volumes and pans (offset 770 = after tempo2)
    // trackvols[64] at offset 770, trackpans[64] at offset 834
    for (int i = 0; i < player->num_tracks && i < 64; i++) {
        uint8_t tvol = song_base[770 + i];
        // If track volume is 0, default to 64 (full volume)
        player->track_volumes[i] = (tvol > 0) ? tvol : 64;
        player->track_pans[i] = (int8_t)song_base[834 + i];
    }

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

        // Debug first few notes in early blocks
        // if (i < 3) {  // First 3 blocks
        //     fprintf(stderr, "med_player: Block %d has %d tracks, %d lines, note_count=%zu\n",
        //             i, numtracks, lines+1, note_count);
        //     fprintf(stderr, "med_player: First 8 rows of block %d:\n", i);
        //     for (int row = 0; row < 8 && row < (int)(lines+1); row++) {
        //         for (int trk = 0; trk < numtracks; trk++) {
        //             int n = row * numtracks + trk;
        //             uint8_t note = notes_ptr[n*4+0];
        //             uint8_t inst = notes_ptr[n*4+1];
        //             uint8_t cmd = notes_ptr[n*4+2];
        //             uint8_t param = notes_ptr[n*4+3];
        //             if (note || inst || cmd || param) {
        //                 fprintf(stderr, "  Row %d Trk %d: note=%02X inst=%02X cmd=%02X param=%02X\n",
        //                         row, trk, note, inst, cmd, param);
        //             }
        //         }
        //     }
        // }
    }

    // fprintf(stderr, "med_player: Loaded %d/%d blocks with note data\n", blocks_with_data, player->num_blocks);

    // Read instruments/samples
    // fprintf(stderr, "med_player: Loading samples from offset 0x%X...\n", smplarr_offset);
    int samples_loaded = 0;
    const uint8_t* smplarr_ptr = read_ptr(base, smplarr_offset);

    if (!smplarr_ptr) {
        fprintf(stderr, "med_player: WARNING: No sample array found\n");
    } else {
        for (int i = 0; i < MAX_SAMPLES; i++) {
            uint32_t instr_offset = BE32(*(uint32_t*)(smplarr_ptr + i * 4));
            if (instr_offset == 0) continue;

            // fprintf(stderr, "med_player: Loading sample %d from offset 0x%X\n", i, instr_offset);

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
            // For synth types (< 0), use the full value
            // For regular samples (>= 0), the low nibble is the type, high bits are flags
            int16_t type = type_and_flags;  // Full value preserves sign
            bool is_synth = (type < 0);
            int16_t masked_type = type & 0x0F;  // Low nibble for octave-based samples
            bool is_16bit = (type_and_flags & 0x10) != 0;  // S_16 flag
            bool is_stereo = (type_and_flags & 0x20) != 0;  // STEREO flag

            // fprintf(stderr, "med_player:   Sample %d: length=%u, type=%d, is_synth=%d, masked=%d, flags=0x%04X (16bit=%d, stereo=%d)\n",
            //         i, length, type, is_synth, masked_type, (uint16_t)type_and_flags, is_16bit, is_stereo);

#ifdef MMD_SYNTH_SUPPORT
            // Handle synthetic/hybrid instruments (type -1 or -2 with synth data)
            if (type == INSTR_TYPE_SYNTHETIC || type == INSTR_TYPE_HYBRID) {
                fprintf(stderr, "med_player:   %s instrument detected\\n",
                        type == INSTR_TYPE_SYNTHETIC ? "SYNTHETIC" : "HYBRID");

                // Read InstrExt first
                const uint8_t* ext_ptr = instr_ptr + 6;
                if (ext_ptr + 18 > base + player->file_size) {
                    fprintf(stderr, "med_player: ERROR: InstrExt out of bounds\\n");
                    continue;
                }

                int8_t finetune = ext_ptr[3];

                // Read MMDSynthInstr structure (after InstrExt)
                const uint8_t* synth_ptr = ext_ptr + 18;
                if (synth_ptr + sizeof(MMDSynthInstr) > base + player->file_size) {
                    fprintf(stderr, "med_player: ERROR: SynthInstr out of bounds\\n");
                    continue;
                }

                MMDSynthInstr synth_data;
                memcpy(&synth_data, synth_ptr, sizeof(MMDSynthInstr));

                // Convert endianness
                uint16_t vol_table_len = BE16(synth_data.vol_table_len);
                uint16_t wave_table_len = BE16(synth_data.wave_table_len);
                uint16_t num_waveforms = BE16(synth_data.num_waveforms);

                fprintf(stderr, "med_player:   Synth: vol_len=%u, wave_len=%u, waveforms=%u\\n",
                        vol_table_len, wave_table_len, num_waveforms);

                // Create synth instrument
                SynthInstrument* synth = (SynthInstrument*)calloc(1, sizeof(SynthInstrument));
                if (!synth) {
                    fprintf(stderr, "med_player: ERROR: Failed to allocate synth\\n");
                    continue;
                }

                // Copy script tables
                synth->vol_table_len = (vol_table_len <= MAX_SYNTH_SCRIPT) ? vol_table_len : MAX_SYNTH_SCRIPT;
                synth->wave_table_len = (wave_table_len <= MAX_SYNTH_SCRIPT) ? wave_table_len : MAX_SYNTH_SCRIPT;
                memcpy(synth->vol_table, synth_data.vol_table, synth->vol_table_len);
                memcpy(synth->wave_table, synth_data.wave_table, synth->wave_table_len);
                synth->vol_speed = synth_data.vol_speed;
                synth->wave_speed = synth_data.wave_speed;

                // Initialize scripts
                synth_script_init(&synth->vol_script, synth->vol_speed);
                synth_script_init(&synth->wave_script, synth->wave_speed);
                synth->current_volume = 64;
                synth->current_waveform = 0;

                // Initialize phase for wavetable playback
                synth->phase = 0.0f;

                // Load waveforms (pointer array follows synth data)
                const uint8_t* wf_ptr = synth_ptr + sizeof(MMDSynthInstr);
                synth->num_waveforms = (num_waveforms <= MAX_WAVEFORMS) ? num_waveforms : MAX_WAVEFORMS;

                for (int w = 0; w < synth->num_waveforms; w++) {
                    if (wf_ptr + 4 > base + player->file_size) break;

                    uint32_t wf_offset = BE32(*(uint32_t*)wf_ptr);
                    wf_ptr += 4;

                    if (wf_offset == 0) continue;

                    const uint8_t* waveform_data = read_ptr(base, wf_offset);
                    if (!waveform_data || waveform_data + 2 > base + player->file_size) continue;

                    // Read waveform length (stored as word count)
                    uint16_t wf_len_words = BE16(*(uint16_t*)waveform_data);
                    uint16_t wf_len = wf_len_words * 2;  // Convert to bytes
                    waveform_data += 2;

                    if (waveform_data + wf_len > base + player->file_size) {
                        fprintf(stderr, "med_player: WARNING: Waveform %d out of bounds\\n", w);
                        continue;
                    }

                    // Allocate and copy waveform
                    synth->waveforms[w] = (int8_t*)malloc(wf_len);
                    if (synth->waveforms[w]) {
                        memcpy(synth->waveforms[w], waveform_data, wf_len);
                        synth->waveform_lengths[w] = wf_len;
                    }
                }

                // Set up sample structure
                player->samples[i].is_synth = (type == INSTR_TYPE_SYNTHETIC);
                player->samples[i].is_hybrid = (type == INSTR_TYPE_HYBRID);
                player->samples[i].synth = synth;
                player->samples[i].finetune = finetune;
                player->samples[i].volume = 64;

                // For hybrid, also load the sample data
                if (type == INSTR_TYPE_HYBRID && length > 0) {
                    const uint8_t* sample_data = wf_ptr;
                    player->samples[i].length = length;
                    player->samples[i].data = (int8_t*)malloc(length);
                    if (player->samples[i].data) {
                        memcpy(player->samples[i].data, sample_data, length);
                    }
                }

                samples_loaded++;
                fprintf(stderr, "med_player:   ✓ Loaded %s instrument %d\\n",
                        type == INSTR_TYPE_SYNTHETIC ? "SYNTHETIC" : "HYBRID", i);
                continue;
            }
#endif

            // Handle modern sample format (type -2 without synth support = regular sample)
            if (type == -2) {  // INSTR_SAMPLE
                // fprintf(stderr, "med_player:   Handling type %d sample...\n", type);

                // Bounds check for InstrExt
                size_t instrext_end = (instr_ptr - base) + 6 + 18;
                // fprintf(stderr, "med_player:   InstrExt ends at offset 0x%zX (file size 0x%zX)\n",
                //         instrext_end, player->file_size);
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
                // fprintf(stderr, "med_player:   Sample data ends at offset 0x%zX\n", sample_end);
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
                // fprintf(stderr, "med_player:   ✓ Loaded sample %d (%u bytes)\n", i, length);
            }
            // Handle old octave-based sample formats (type 0-7)
            // Note: Use masked_type (low nibble) for octave-based samples
            else if (!is_synth && masked_type >= 0 && masked_type <= 7) {
                // fprintf(stderr, "med_player:   Old octave-based sample (type %d)\n", masked_type);

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
                // fprintf(stderr, "med_player:   ✓ Loaded old-format sample %d (%u bytes, vol=%d, transpose=%d, 16bit=%d, stereo=%d)\n",
                //         i, length, svol, strans, is_16bit, is_stereo);
            } else {
                // Unknown/unsupported sample type
                fprintf(stderr, "med_player:   WARNING: Skipping unsupported sample type %d (masked=%d) for sample %d\n",
                        type, masked_type, i);
            }
        }
    }

    // fprintf(stderr, "med_player: Loaded %d samples\n", samples_loaded);

    // Show which sample slots have data
    // fprintf(stderr, "med_player: Sample slots with data: ");
    // for (int i = 0; i < MAX_SAMPLES; i++) {
    //     if (player->samples[i].data) {
    //         fprintf(stderr, "%d ", i + 1);  // Show as 1-based
    //     }
    // }
    // fprintf(stderr, "\n");

    // fprintf(stderr, "med_player: File loaded successfully!\n");

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
// Returns true if instrument changed
static bool trigger_note(MedPlayer* player, int channel, uint8_t note, uint8_t instrument) {
    MedChannel* chan = &player->channels[channel];
    bool instrument_changed = false;

    // static int trigger_count = 0;
    // if (trigger_count < 10) {
    //     fprintf(stderr, "med_player: trigger_note ch=%d note=%d inst=%d\n", channel, note, instrument);
    //     trigger_count++;
    // }

    // Set instrument
    if (instrument > 0 && instrument <= MAX_SAMPLES) {
        MedSample* smp = &player->samples[instrument - 1];
        if (smp->data) {
            // Check if instrument actually changed
            instrument_changed = (chan->sample != smp);
            chan->sample = smp;
            chan->finetune = chan->sample->finetune;
        } else {
            // No sample data (e.g., SYNTHETIC instrument without synth support)
            // Stop the channel to avoid playing wrong sample at wrong pitch
            chan->sample = NULL;
            chan->volume = 0;
        }
    }

    // Set note
    if (note > 0 && chan->sample && chan->sample->data) {
        // Apply sample transpose
        int16_t transposed_note = note + chan->sample->transpose;
        if (transposed_note < 1) transposed_note = 1;
        if (transposed_note > 132) transposed_note = 132;

        chan->period = get_note_period((uint8_t)transposed_note, chan->finetune);
        chan->position = 0.0f;

        // Clear portamento effects when new note triggers
        chan->portamento_up = 0;
        chan->portamento_down = 0;

        // if (trigger_count < 10) {
        //     // Calculate frequency for debugging
        //     float freq = 7093789.2f / ((float)chan->period * 2.0f);
        //     fprintf(stderr, "  -> note=%d, transpose=%d, transposed=%d, period=%d (%.1f Hz), volume=%d, sample_len=%u\n",
        //             note, chan->sample->transpose, transposed_note, chan->period, freq, chan->volume, chan->sample->length);
        // }
    }

    return instrument_changed;
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
            MedChannel* chan = &player->channels[ch];

            // Trigger new note if present
            bool instrument_changed = false;
            if (note->note > 0) {
                instrument_changed = trigger_note(player, ch, note->note, note->instrument);

                // When instrument ACTUALLY changes: reset channel volume to sample's default
                // (unless there's a volume command on this row)
                if (instrument_changed && note->command != 0x0C && chan->sample) {
                    chan->volume = chan->sample->volume;
                }
            }

            // Process volume command (overrides everything)
            if (note->command == 0x0C) {  // Set volume
                chan->volume = note->param;
            }

            // Process other effects
            if (note->command == 0x01) {  // Portamento up
                if (note->param != 0) {
                    chan->portamento_up = note->param;
                    chan->portamento_down = 0;  // Clear opposite direction
                }
            } else if (note->command == 0x02) {  // Portamento down
                if (note->param != 0) {
                    chan->portamento_down = note->param;
                    chan->portamento_up = 0;  // Clear opposite direction
                }
            } else if (note->command == 0x00 && note->param == 0x00) {
                // No effect - clear portamento (important for empty patterns)
                chan->portamento_up = 0;
                chan->portamento_down = 0;
            }
            // TODO: Add more effects (vibrato, arpeggio, etc.)
        }

        // Fire position callback
        if (player->position_callback) {
            player->position_callback(player->current_order, player->current_pattern,
                                    player->current_row, player->callback_user_data);
        }
    } else {
        // Process continuous effects (every tick except tick 0)
        for (int ch = 0; ch < player->num_tracks; ch++) {
            MedChannel* chan = &player->channels[ch];

            // Apply portamento up
            if (chan->portamento_up > 0 && chan->period > 0) {
                if (chan->period > chan->portamento_up) {
                    chan->period -= chan->portamento_up;
                } else {
                    chan->period = 1;  // Minimum period
                }
            }

            // Apply portamento down
            if (chan->portamento_down > 0 && chan->period > 0) {
                chan->period += chan->portamento_down;
                if (chan->period > 65535) {
                    chan->period = 65535;  // Maximum period
                }
            }
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
        // fprintf(stderr, "med_player: Playback started\n");
    }
}

// Stop playback
void med_player_stop(MedPlayer* player) {
    if (player) {
        player->playing = false;
        // fprintf(stderr, "med_player: Playback stopped\n");
    }
}

// Process audio
void med_player_process(MedPlayer* player, float* left_out, float* right_out,
                       size_t frames, float sample_rate) {
    if (!player || !left_out || !right_out) return;

    // Calculate samples per tick
    // Standard ProTracker formula: samples_per_tick = sample_rate * 2.5 / BPM
    // Speed (ticks/row) doesn't affect tick length, only how many ticks per row
    player->samples_per_tick = (uint32_t)(sample_rate * 2.5f / (float)player->bpm);

    for (size_t i = 0; i < frames; i++) {
        float left = 0.0f;
        float right = 0.0f;

        // Mix all channels
        for (int ch = 0; ch < player->num_tracks; ch++) {
            MedChannel* chan = &player->channels[ch];
            if (!chan->sample || chan->muted) continue;

#ifdef MMD_SYNTH_SUPPORT
            // Handle synth instruments
            if (chan->sample->is_synth && chan->sample->synth) {
                if (chan->period > 0) {
                    float freq = 7093789.2f / ((float)chan->period * 2.0f);
                    float sample = synth_instrument_process(chan->sample->synth, freq, sample_rate);
                    // Apply volume: channel volume * sample volume * track volume * user volume
                    float vol = (chan->volume / 64.0f) * (chan->sample->volume / 64.0f) *
                               (player->track_volumes[ch] / 64.0f) * chan->user_volume;

                    // Apply panning
                    float pan = (chan->panning + 16) / 32.0f;
                    left += sample * vol * (1.0f - pan);
                    right += sample * vol * pan;

                    // Tick synth scripts
                    chan->sample->synth->current_volume = synth_script_tick(
                        &chan->sample->synth->vol_script,
                        chan->sample->synth->vol_table,
                        chan->sample->synth->vol_table_len,
                        chan->sample->synth->current_volume,
                        true
                    );
                    chan->sample->synth->current_waveform = synth_script_tick(
                        &chan->sample->synth->wave_script,
                        chan->sample->synth->wave_table,
                        chan->sample->synth->wave_table_len,
                        chan->sample->synth->current_waveform,
                        false
                    );
                }
                continue;
            }
#endif

            // Regular sample playback
            if (!chan->sample->data) continue;

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

                // Apply volume: channel volume * sample volume * track volume * user volume
                // Channel volume: set by effects (0x0C) or initialized from track volume
                // Sample volume: default volume stored in sample data
                // Track volume: per-track scaling factor
                float vol = (chan->volume / 64.0f) * (chan->sample->volume / 64.0f) *
                           (player->track_volumes[ch] / 64.0f) * chan->user_volume;

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
