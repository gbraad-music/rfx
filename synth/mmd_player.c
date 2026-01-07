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
#define SYNTH_CMD_SET_VOLUME      0x00  // 0x00-0x7F: Set volume (0-127)
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
    uint8_t volume;         // Default volume (0-127)
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

    // Script data (stored as individual bytes in file)
    uint8_t vol_table[MAX_SYNTH_SCRIPT];
    uint8_t wave_table[MAX_SYNTH_SCRIPT];
    uint16_t vol_table_len;
    uint16_t wave_table_len;
    uint8_t vol_speed;
    uint8_t wave_speed;

    // Current state
    uint8_t current_waveform;
    uint8_t target_volume;    // Target volume from script (0-127)
    float current_volume;     // Actual interpolated volume (0.0-127.0)
    SynthScript vol_script;
    SynthScript wave_script;

    // Volume envelope (hold/decay from InstrExt)
    uint8_t hold_time;        // Sustain time in ticks
    uint8_t decay_speed;      // Decay speed (0 = no decay)
    uint16_t env_counter;     // Envelope tick counter
    float env_volume;         // Envelope volume multiplier (0.0-1.0)

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
    int8_t volume;          // Default volume (0-127)
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
    uint8_t volume;         // Target volume (0-127)
    float current_volume;   // Actual current volume (for smooth interpolation)
    int8_t finetune;        // Finetune
    bool muted;             // Mute flag
    bool volume_set;        // True if volume has been set (prevents reinit on fade to 0)
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
    uint32_t instr_ext_offset;   // Offset to InstrExt array (from ExpData)
    uint16_t instr_ext_entries;  // Number of InstrExt entries
    uint16_t instr_ext_entry_size;  // Size of each InstrExt entry

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
    uint16_t loop_start;    // Loop start order (for pattern loop)
    uint16_t loop_end;      // Loop end order (for pattern loop)

    // Volume mode
    bool vol_hex;           // True = hex display mode, False = decimal display mode
    uint8_t max_volume;     // Maximum volume value (always 127 in OctaMED)

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

    // Initialize channel volumes and panning
    for (int i = 0; i < MAX_CHANNELS; i++) {
        player->channels[i].user_volume = 1.0f;
        player->track_volumes[i] = 64;  // Default to max in 0-64 range
        player->track_pans[i] = 0;  // Center (will be set from file or classic Amiga panning)
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

// Execute one tick of synth script (byte commands)
// Returns: updated value (volume 0-127 or waveform 0-63)
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

    // Read command byte and optional parameter byte
    uint8_t cmd_byte = table[script->pc++];
    uint8_t param_byte = (script->pc < table_len) ? table[script->pc] : 0;

    // Simple implementation - just handle basic commands for now
    if (cmd_byte >= 0xF0) {
        switch (cmd_byte) {
            case SYNTH_CMD_SPD:  // Set speed
                script->pc++;  // Consume parameter byte
                script->speed = param_byte;
                if (script->speed == 0) script->speed = 1;
                break;

            case SYNTH_CMD_WAI:  // Wait
                script->pc++;  // Consume parameter byte
                script->wait_counter = param_byte;
                break;

            case SYNTH_CMD_JMP:  // Jump
                script->pc++;  // Consume parameter byte
                if (param_byte < table_len) {
                    script->pc = param_byte;
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
    } else if (cmd_byte <= 0x7F) {
        // Direct value (volume 0-127 or waveform 0-63)
        if (is_volume) {
            current_value = (cmd_byte <= 127) ? cmd_byte : 127;
        } else {
            current_value = (cmd_byte < 64) ? cmd_byte : 0;
        }
    }

    return current_value;
}

// Built-in waveform generator for OctaMED synths
static float builtin_waveform(uint8_t waveform_type, float phase) {
    switch (waveform_type) {
        case 0:  // Sawtooth
            return (phase * 2.0f) - 1.0f;
        case 1:  // Square
            return (phase < 0.5f) ? -1.0f : 1.0f;
        case 2:  // Sine
            return sinf(phase * 2.0f * 3.14159265f);
        case 3:  // Triangle
            return (phase < 0.5f) ? (phase * 4.0f - 1.0f) : (3.0f - phase * 4.0f);
        default:  // Default to sawtooth
            return (phase * 2.0f) - 1.0f;
    }
}

// Process synth instrument - generate one sample
static float synth_instrument_process(SynthInstrument* synth, float freq, float sample_rate) {
    if (!synth) {
        return 0.0f;
    }

    float sample;

    // Check if using custom waveforms or built-in waveforms
    if (synth->num_waveforms > 0) {
        // Use custom waveforms from file
        uint8_t wf_idx = synth->current_waveform;

        // If requested waveform doesn't exist, find first available one
        if (wf_idx >= synth->num_waveforms || !synth->waveforms[wf_idx]) {
            // Search for first valid waveform
            uint8_t old_idx = wf_idx;
            wf_idx = 0;
            for (int i = 0; i < synth->num_waveforms; i++) {
                if (synth->waveforms[i]) {
                    wf_idx = i;
                    break;
                }
            }
            // static int fallback_debug = 0;
            // if (fallback_debug < 3) {
            //     fprintf(stderr, "WAVEFORM FALLBACK: requested %d, found %d (has_data=%d)\n",
            //             old_idx, wf_idx, synth->waveforms[wf_idx] != NULL);
            //     fallback_debug++;
            // }
        }

        if (wf_idx < synth->num_waveforms && synth->waveforms[wf_idx]) {
            // Generate sample from current waveform
            int8_t* waveform = synth->waveforms[wf_idx];
            uint16_t wf_len = synth->waveform_lengths[wf_idx];

            // Wavetable playback with linear interpolation for smooth output
            float phase_pos = synth->phase * wf_len;
            int pos1 = (int)phase_pos % wf_len;
            int pos2 = (pos1 + 1) % wf_len;
            float frac = phase_pos - (int)phase_pos;

            float samp1 = waveform[pos1] / 128.0f;
            float samp2 = waveform[pos2] / 128.0f;
            sample = samp1 + (samp2 - samp1) * frac;
        } else {
            // No waveforms available - use built-in saw
            sample = builtin_waveform(0, synth->phase);
        }
    } else {
        // Use built-in waveforms
        // OctaMED built-in waveform IDs: 0 = sine wave
        uint8_t waveform_type;
        if (synth->current_waveform == 0) {
            waveform_type = 2;  // Sine
        } else if (synth->current_waveform == 1) {
            waveform_type = 0;  // Sawtooth
        } else if (synth->current_waveform == 2) {
            waveform_type = 1;  // Square
        } else if (synth->current_waveform == 3) {
            waveform_type = 3;  // Triangle
        } else {
            // Default to sine for unknown waveforms
            waveform_type = 2;
        }
        sample = builtin_waveform(waveform_type, synth->phase);
    }

    // Advance phase
    float phase_inc = freq / sample_rate;
    synth->phase += phase_inc;
    if (synth->phase >= 1.0f) {
        synth->phase -= (int)synth->phase;  // Keep fractional part
    }

    // Apply volume (OctaMED synth volumes are 0-127)
    float volume = (synth->current_volume / 127.0f) * synth->env_volume;
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
    uint32_t expdata_offset = BE32(*(uint32_t*)(ptr + 32));  // ExpData offset

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

    // Read ExpData structure (if present) to get InstrExt information
    player->instr_ext_offset = 0;
    player->instr_ext_entries = 0;
    player->instr_ext_entry_size = 0;

    if (expdata_offset > 0 && expdata_offset < size) {
        const uint8_t* expdata_ptr = base + expdata_offset;
        if (expdata_ptr + 8 <= base + size) {
            // Read ExpData fields we need
            // uint32 nextModOffset at +0
            player->instr_ext_offset = BE32(*(uint32_t*)(expdata_ptr + 4));     // instrExtOffset at +4
            player->instr_ext_entries = BE16(*(uint16_t*)(expdata_ptr + 8));    // instrExtEntries at +8
            player->instr_ext_entry_size = BE16(*(uint16_t*)(expdata_ptr + 10)); // instrExtEntrySize at +10

            // fprintf(stderr, "med_player: ExpData: instrExtOffset=0x%X, entries=%u, entry_size=%u\n",
            //         player->instr_ext_offset, player->instr_ext_entries, player->instr_ext_entry_size);
        }
    }

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

    // Initialize loop range to full song
    player->loop_start = 0;
    player->loop_end = (player->song_length > 0) ? (player->song_length - 1) : 0;

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

    // Volume mode: OctaMED uses 0-127 range for all volumes (pattern commands, samples, tracks)
    // The FLAG_VOLHEX just affects how they're displayed in the UI (hex vs decimal)
    player->vol_hex = (flags & 0x10) != 0;
    player->max_volume = 127;  // Always use 127 for all volume values
    // fprintf(stderr, "med_player: Volume mode: FLAG_VOLHEX=%d (hex display), using max_volume=%d\n",
    //         player->vol_hex, player->max_volume);

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
    // fprintf(stderr, "med_player: Track volumes and panning:\n");

    for (int i = 0; i < player->num_tracks && i < 64; i++) {
        uint8_t tvol = song_base[770 + i];
        // If track volume is 0, use 127 (meaning: don't scale, full volume)
        // Otherwise store the value as-is (OctaMED uses 0-64 range for actual values)
        player->track_volumes[i] = (tvol > 0) ? tvol : 127;

        // For 4-channel files: ALWAYS use classic Amiga L-R-R-L hard panning
        // (ignore panning data from file, which may have softer panning)
        if (player->num_tracks == 4 && i < 4) {
            const int8_t amiga_pan[4] = {-16, 16, 16, -16};  // L-R-R-L
            player->track_pans[i] = amiga_pan[i];
            // fprintf(stderr, "  Track %d: vol=%d, pan=%d (Amiga L-R-R-L)\n",
            //         i, player->track_volumes[i], player->track_pans[i]);
        } else {
            // For 8+ channel files, read panning from file
            uint8_t pan_raw = song_base[834 + i];
            int8_t pan_signed = (int8_t)pan_raw;

            if (pan_signed >= -16 && pan_signed <= 16) {
                player->track_pans[i] = pan_signed;
            } else {
                player->track_pans[i] = 0;  // Center if invalid
            }
            // fprintf(stderr, "  Track %d: vol=%d, pan=%d (from file)\n",
            //         i, player->track_volumes[i], player->track_pans[i]);
        }
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

            // fprintf(stderr, "med_player:   Sample %d: length=%u, type=%d (0x%04X), is_synth=%d\n",
            //         i+1, length, type, (uint16_t)type_and_flags, is_synth);

#ifdef MMD_SYNTH_SUPPORT
            // Handle synthetic/hybrid instruments (type -1 or -2 with synth data)
            if (type == INSTR_TYPE_SYNTHETIC || type == INSTR_TYPE_HYBRID) {
                // fprintf(stderr, "med_player:   %s instrument detected (sample %d, type=%d)\n",
                //         type == INSTR_TYPE_SYNTHETIC ? "SYNTHETIC" : "HYBRID", i+1, type);

                // Synth data starts immediately after the 6-byte MMDInstrHeader
                // (InstrExt is stored separately in expData.instrExtOffset, not here!)
                const uint8_t* synth_ptr = instr_ptr + 6;

                // Read InstrExt data for this instrument (hold, decay, finetune)
                uint8_t hold = 0, decay = 0;
                int8_t finetune = 0;

                if (player->instr_ext_offset > 0 && i < player->instr_ext_entries) {
                    const uint8_t* instrext_ptr = base + player->instr_ext_offset + (i * player->instr_ext_entry_size);
                    if (instrext_ptr + 4 <= base + player->file_size) {
                        hold = instrext_ptr[0];
                        decay = instrext_ptr[1];
                        // instrext_ptr[2] = suppress_midi_off
                        finetune = (int8_t)instrext_ptr[3];

                        // fprintf(stderr, "med_player:   InstrExt[%d]: hold=%u, decay=%u, finetune=%d\n",
                        //         i, hold, decay, finetune);
                    }
                }

                // File format: 16-byte header + variable vol_table + variable wave_table + waveform pointers
                // We can only read the 16-byte fixed header with memcpy
                if (synth_ptr + 16 > base + player->file_size) {
                    fprintf(stderr, "med_player: ERROR: SynthInstr header out of bounds\n");
                    continue;
                }

                // Read only the fixed 16-byte header
                uint8_t default_decay = synth_ptr[0];
                uint16_t loop_start = BE16(*(uint16_t*)(synth_ptr + 4));
                uint16_t loop_length = BE16(*(uint16_t*)(synth_ptr + 6));
                uint16_t vol_table_len = BE16(*(uint16_t*)(synth_ptr + 8));
                uint16_t wave_table_len = BE16(*(uint16_t*)(synth_ptr + 10));
                uint8_t vol_speed = synth_ptr[12];
                uint8_t wave_speed = synth_ptr[13];
                uint16_t num_waveforms = BE16(*(uint16_t*)(synth_ptr + 14));

                // fprintf(stderr, "med_player:   Synth: vol_len=%u, wave_len=%u, waveforms=%u, vol_speed=%u, wave_speed=%u\n",
                //         vol_table_len, wave_table_len, num_waveforms, vol_speed, wave_speed);

                // Variable-length data follows the header
                const uint8_t* vol_table_ptr = synth_ptr + 16;
                const uint8_t* wave_table_ptr = synth_ptr + 16 + vol_table_len;

                // Bounds check for variable data
                if (wave_table_ptr + wave_table_len > base + player->file_size) {
                    fprintf(stderr, "med_player: ERROR: SynthInstr script tables out of bounds\n");
                    continue;
                }

                // Debug: show first few bytes of scripts (from actual file location)
                // fprintf(stderr, "med_player:     Vol script (from file): ");
                // for (int j = 0; j < 16 && j < vol_table_len; j++) {
                //     fprintf(stderr, "%02X ", vol_table_ptr[j]);
                // }
                // fprintf(stderr, "\n");
                // fprintf(stderr, "med_player:     Wave script (from file): ");
                // for (int j = 0; j < 16 && j < wave_table_len; j++) {
                //     fprintf(stderr, "%02X ", wave_table_ptr[j]);
                // }
                // fprintf(stderr, "\n");

                // Create synth instrument
                SynthInstrument* synth = (SynthInstrument*)calloc(1, sizeof(SynthInstrument));
                if (!synth) {
                    fprintf(stderr, "med_player: ERROR: Failed to allocate synth\\n");
                    continue;
                }

                // Copy script tables (stored as individual bytes in file)
                synth->vol_table_len = (vol_table_len <= MAX_SYNTH_SCRIPT) ? vol_table_len : MAX_SYNTH_SCRIPT;
                synth->wave_table_len = (wave_table_len <= MAX_SYNTH_SCRIPT) ? wave_table_len : MAX_SYNTH_SCRIPT;

                // Read individual bytes from actual file location
                for (int j = 0; j < synth->vol_table_len; j++) {
                    synth->vol_table[j] = vol_table_ptr[j];
                }
                for (int j = 0; j < synth->wave_table_len; j++) {
                    synth->wave_table[j] = wave_table_ptr[j];
                }

                synth->vol_speed = vol_speed;
                synth->wave_speed = wave_speed;

                // Initialize scripts
                synth_script_init(&synth->vol_script, synth->vol_speed);
                synth_script_init(&synth->wave_script, synth->wave_speed);
                synth->target_volume = 0;     // Will be set by script on first tick
                synth->current_volume = 0.0f; // Start silent, ramp to target
                synth->current_waveform = 0;

                // Initialize volume envelope (from InstrExt hold/decay)
                synth->hold_time = hold;
                synth->decay_speed = decay;
                synth->env_counter = 0;
                synth->env_volume = 1.0f;  // Start at full volume

                // fprintf(stderr, "med_player:   Envelope: hold=%u, decay=%u\n", hold, decay);

                // Initialize phase for wavetable playback
                synth->phase = 0.0f;

                // Load waveforms (pointer array follows variable-length script tables)
                // MMDSynthInstr header is 16 bytes, then vol_table_len bytes, then wave_table_len bytes
                const uint8_t* wf_ptr = synth_ptr + 16 + vol_table_len + wave_table_len;
                synth->num_waveforms = (num_waveforms <= MAX_WAVEFORMS) ? num_waveforms : MAX_WAVEFORMS;

                // fprintf(stderr, "med_player:     Waveform array at offset: synth+16+%u+%u = synth+%u\n",
                //         vol_table_len, wave_table_len, 16 + vol_table_len + wave_table_len);

                for (int w = 0; w < synth->num_waveforms; w++) {
                    if (wf_ptr + 4 > base + player->file_size) {
                        // fprintf(stderr, "med_player:     Waveform %d: pointer out of bounds\n", w);
                        break;
                    }

                    uint32_t wf_offset_rel = BE32(*(uint32_t*)wf_ptr);
                    wf_ptr += 4;

                    if (wf_offset_rel == 0) {
                        // fprintf(stderr, "med_player:     Waveform %d: NULL offset (skipped)\n", w);
                        continue;
                    }

                    // Waveform offset is RELATIVE to instrument start, not absolute!
                    uint32_t wf_offset = instr_offset + wf_offset_rel;
                    // fprintf(stderr, "med_player:     Waveform %d: offset=0x%X (instr+0x%X, file_size=0x%zX)\n",
                    //         w, wf_offset, wf_offset_rel, player->file_size);

                    const uint8_t* waveform_data = read_ptr(base, wf_offset);
                    if (!waveform_data || waveform_data + 2 > base + player->file_size) {
                        // fprintf(stderr, "med_player:     Waveform %d: invalid data pointer (waveform_data=%p, base=%p)\n", w, (void*)waveform_data, (void*)base);
                        continue;
                    }

                    // Read waveform length (stored as word count)
                    uint16_t wf_len_words = BE16(*(uint16_t*)waveform_data);
                    uint16_t wf_len = wf_len_words * 2;  // Convert to bytes
                    waveform_data += 2;

                    if (waveform_data + wf_len > base + player->file_size) {
                        // fprintf(stderr, "med_player:     Waveform %d: out of bounds (needs %u bytes)\n", w, wf_len);
                        continue;
                    }

                    // Allocate and copy waveform
                    synth->waveforms[w] = (int8_t*)malloc(wf_len);
                    if (synth->waveforms[w]) {
                        memcpy(synth->waveforms[w], waveform_data, wf_len);
                        synth->waveform_lengths[w] = wf_len;
                        // fprintf(stderr, "med_player:     Waveform %d: %u bytes (offset 0x%X)\n", w, wf_len, wf_offset);
                    } else {
                        // fprintf(stderr, "med_player:     Waveform %d: malloc failed\n", w);
                    }
                }

                // Read volume from MMD0sample array (like regular samples)
                const uint8_t* song_base_vol = read_ptr(base, player->song_offset);
                const uint8_t* sample_info_vol = song_base_vol + (i * 8);
                uint8_t svol = sample_info_vol[6];
                int8_t strans = (int8_t)sample_info_vol[7];

                // Set up sample structure
                player->samples[i].is_synth = (type == INSTR_TYPE_SYNTHETIC);
                player->samples[i].is_hybrid = (type == INSTR_TYPE_HYBRID);
                player->samples[i].synth = synth;
                player->samples[i].finetune = finetune;
                player->samples[i].volume = svol;  // Read from MMD0sample array
                player->samples[i].transpose = strans;

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
                // fprintf(stderr, "med_player:   ✓ Loaded %s instrument %d (vol=%d, transpose=%d, waveforms=%d)\n",
                //         type == INSTR_TYPE_SYNTHETIC ? "SYNTHETIC" : "HYBRID", i+1, svol, strans, synth->num_waveforms);
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

                // Read volume from MMD0sample array (like old samples do)
                const uint8_t* song_base_vol = read_ptr(base, player->song_offset);
                const uint8_t* sample_info_vol = song_base_vol + (i * 8);
                uint8_t svol = sample_info_vol[6];
                int8_t strans = (int8_t)sample_info_vol[7];

                player->samples[i].repeat_start = long_repeat;
                player->samples[i].repeat_length = long_replen;
                player->samples[i].finetune = finetune;
                player->samples[i].volume = svol;  // Read from MMD0sample array
                player->samples[i].transpose = strans;
                player->samples[i].is_stereo = (instr_flags & INSTR_FLAG_STEREO) != 0;
                player->samples[i].is_16bit = (instr_flags & INSTR_FLAG_16BIT) != 0;

                // fprintf(stderr, "med_player:   Sample %d: len=%u, vol=%d, repeat=%u/%u, 16bit=%d, stereo=%d\n",
                //         i+1, length, player->samples[i].volume, long_repeat, long_replen, is_16bit, is_stereo);

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

                // fprintf(stderr, "med_player:   Sample %d (old): len=%u, vol=%d, transpose=%d, repeat=%u/%u\n",
                //         i+1, length, svol, strans, player->samples[i].repeat_start, player->samples[i].repeat_length);

                samples_loaded++;
            } else {
                // Unknown/unsupported sample type
                // fprintf(stderr, "med_player:   WARNING: Skipping unsupported sample type %d (masked=%d) for sample %d\n",
                //         type, masked_type, i);
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
#ifdef MMD_SYNTH_SUPPORT
        // Accept instruments with sample data OR synth data
        if (smp->data || (smp->is_synth && smp->synth)) {
#else
        // Without synth support, only accept instruments with sample data
        if (smp->data) {
#endif
            // Check if instrument actually changed
            instrument_changed = (chan->sample != smp);
            chan->sample = smp;
            chan->finetune = chan->sample->finetune;
        } else {
            // No sample or synth data - stop the channel
            // static int reject_count = 0;
            // if (reject_count < 5) {
            //     fprintf(stderr, "REJECT INST: ch=%d inst=%d (has_data=%d, is_synth=%d, has_synth_ptr=%d)\n",
            //             channel, instrument, smp->data != NULL, smp->is_synth, smp->synth != NULL);
            //     reject_count++;
            // }
            chan->sample = NULL;
            chan->volume = 0;
        }
    }

    // Set note
#ifdef MMD_SYNTH_SUPPORT
    // For synth support: accept instruments with sample data OR synth data
    bool can_play = (chan->sample != NULL) &&
                    (chan->sample->data || (chan->sample->is_synth && chan->sample->synth));
#else
    // Without synth support: only accept instruments with sample data
    bool can_play = (chan->sample != NULL) && (chan->sample->data != NULL);
#endif

    if (note > 0 && can_play) {
        // Apply sample transpose
        int16_t transposed_note = note + chan->sample->transpose;

#ifdef MMD_SYNTH_SUPPORT
        // For SYNTHETIC instruments in MMD2/MMD3, apply additional -12 transpose
        // (OpenMPT does -24 total, we already do -12 in get_note_period, so add -12 more here)
        if (chan->sample->is_synth && chan->sample->synth) {
            transposed_note -= 12;
        }
#endif

        if (transposed_note < 1) transposed_note = 1;
        if (transposed_note > 132) transposed_note = 132;

        chan->period = get_note_period((uint8_t)transposed_note, chan->finetune);
        chan->position = 0.0f;

#ifdef MMD_SYNTH_SUPPORT
        // Reset synth envelope and scripts when note triggers
        if (chan->sample->is_synth && chan->sample->synth) {
            chan->sample->synth->env_counter = 0;
            chan->sample->synth->env_volume = 1.0f;

            // Reset scripts to start from beginning
            synth_script_init(&chan->sample->synth->vol_script, chan->sample->synth->vol_speed);
            synth_script_init(&chan->sample->synth->wave_script, chan->sample->synth->wave_speed);

            // Execute scripts once to get initial values
            chan->sample->synth->target_volume = synth_script_tick(
                &chan->sample->synth->vol_script,
                chan->sample->synth->vol_table,
                chan->sample->synth->vol_table_len,
                chan->sample->synth->target_volume,
                true
            );
            chan->sample->synth->current_waveform = synth_script_tick(
                &chan->sample->synth->wave_script,
                chan->sample->synth->wave_table,
                chan->sample->synth->wave_table_len,
                chan->sample->synth->current_waveform,
                false
            );

            // Set current volume to target immediately (no ramp at start)
            chan->sample->synth->current_volume = chan->sample->synth->target_volume;
        }
#endif

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
            if (note->note > 0) {
                bool inst_changed = trigger_note(player, ch, note->note, note->instrument);

                // Debug: show synth triggers
                // if (chan->sample && chan->sample->is_synth) {
                //     fprintf(stderr, "SYNTH TRIGGER: ch=%d note=%d inst=%d (sample %d is synth) pattern=%d row=%d\n",
                //             ch, note->note, note->instrument, note->instrument,
                //             player->current_pattern, player->current_row);
                // }

                // When instrument NUMBER is specified, ALWAYS reset volume to that instrument's default
                // This happens even if it's the same instrument (classic tracker behavior)
                if (note->instrument > 0 && chan->sample) {
                    // Instrument specified - reset to its default volume
                    uint8_t default_vol = chan->sample->volume;
                    // Scale from 0-64 to 0-127
                    chan->volume = default_vol * 2;
                    if (chan->volume > 127) chan->volume = 127;
                    chan->current_volume = chan->volume;
                    chan->volume_set = true;

                    // if (ch == 0) {  // Track 0 only
                    //     fprintf(stderr, "VOL RESET: ch=%d inst=%d default_vol=%d → vol=%d\n",
                    //             ch, note->instrument, default_vol, chan->volume);
                    // }
                } else if (!chan->volume_set) {
                    // No instrument specified, first time - initialize to max
                    chan->volume = player->max_volume;
                    chan->current_volume = player->max_volume;
                    chan->volume_set = true;
                }
                // Otherwise volume persists (when note triggers without instrument)
            }

            // Process volume command AFTER note trigger (so it can override sample default)
            if (note->command == 0x0C) {  // Set volume
                uint8_t old_vol = chan->volume;
                // OctaMED volume interpretation depends on vol_hex flag:
                // - If vol_hex (hex mode): values are 0x00-0x7F (0-127), use directly
                // - If NOT vol_hex (decimal mode): values are 0-64 decimal, scale to 0-127
                if (player->vol_hex) {
                    // Hex mode: 0x00-0x7F maps to 0-127
                    chan->volume = (note->param > 127) ? 127 : note->param;
                } else {
                    // Decimal mode: 0-64 maps to 0-127
                    uint8_t new_vol = (note->param > 64) ? 64 : note->param;
                    chan->volume = new_vol * 2;
                    if (chan->volume > 127) chan->volume = 127;
                }
                chan->volume_set = true;
                chan->current_volume = chan->volume;  // Apply immediately

                // if (ch == 0) {  // Track 0 only
                //     fprintf(stderr, "VOL CMD: ch=%d %d→%d (param=0x%02X note=%02X inst=%02X)\n",
                //             ch, old_vol, chan->volume, note->param, note->note, note->instrument);
                // }
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
    }

    // Process synth scripts every tick (both tick 0 and other ticks)
#ifdef MMD_SYNTH_SUPPORT
    for (int ch = 0; ch < player->num_tracks; ch++) {
        MedChannel* chan = &player->channels[ch];
        if (chan->sample && chan->sample->is_synth && chan->sample->synth) {
            SynthInstrument* synth = chan->sample->synth;

            // Tick volume script (updates target_volume)
            synth->target_volume = synth_script_tick(
                &synth->vol_script,
                synth->vol_table,
                synth->vol_table_len,
                synth->target_volume,
                true
            );

            // Tick waveform script
            synth->current_waveform = synth_script_tick(
                &synth->wave_script,
                synth->wave_table,
                synth->wave_table_len,
                synth->current_waveform,
                false
            );

            // Update volume envelope (hold/decay)
            if (synth->hold_time > 0 || synth->decay_speed > 0) {
                synth->env_counter++;

                if (synth->env_counter <= synth->hold_time) {
                    // Sustain phase: volume stays at 1.0
                    synth->env_volume = 1.0f;
                } else {
                    // Decay phase: linear decay to 0
                    if (synth->decay_speed > 0) {
                        uint16_t decay_ticks = synth->env_counter - synth->hold_time;
                        uint16_t total_decay_ticks = 64 / synth->decay_speed;
                        if (decay_ticks >= total_decay_ticks) {
                            synth->env_volume = 0.0f;
                        } else {
                            synth->env_volume = 1.0f - ((float)decay_ticks / (float)total_decay_ticks);
                        }
                    }
                }
            }
        }
    }
#endif

    if (player->tick != 0) {
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

            // Handle loop range
            if (player->current_order > player->loop_end || player->current_order >= player->song_length) {
                player->current_order = player->loop_start;

                // Reset channel volumes when song loops (OctaMED behavior)
                for (int ch = 0; ch < MAX_CHANNELS; ch++) {
                    player->channels[ch].volume = 0;
                    player->channels[ch].current_volume = 0;
                    player->channels[ch].volume_set = false;
                }
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

    // Volume ramp rate: 10ms ramp time for smooth transitions
    float ramp_time = 0.010f;  // 10 milliseconds
    float ramp_rate = player->max_volume / (ramp_time * sample_rate);

    for (size_t i = 0; i < frames; i++) {
        float left = 0.0f;
        float right = 0.0f;

        // Mix all channels
        for (int ch = 0; ch < player->num_tracks; ch++) {
            MedChannel* chan = &player->channels[ch];
            if (!chan->sample || chan->muted) continue;

            // Smooth volume interpolation toward target volume
            if (chan->current_volume < chan->volume) {
                chan->current_volume += ramp_rate;
                if (chan->current_volume > chan->volume) {
                    chan->current_volume = chan->volume;
                }
            } else if (chan->current_volume > chan->volume) {
                chan->current_volume -= ramp_rate;
                if (chan->current_volume < chan->volume) {
                    chan->current_volume = chan->volume;
                }
            }

#ifdef MMD_SYNTH_SUPPORT
            // Handle synth instruments
            if (chan->sample->is_synth && chan->sample->synth) {
                SynthInstrument* synth = chan->sample->synth;

                // Smooth volume interpolation toward target (10ms ramp time)
                float ramp_rate = 127.0f / (0.010f * sample_rate);
                if (synth->current_volume < synth->target_volume) {
                    synth->current_volume += ramp_rate;
                    if (synth->current_volume > synth->target_volume) {
                        synth->current_volume = synth->target_volume;
                    }
                } else if (synth->current_volume > synth->target_volume) {
                    synth->current_volume -= ramp_rate;
                    if (synth->current_volume < synth->target_volume) {
                        synth->current_volume = synth->target_volume;
                    }
                }

                // static int synth_debug_count = 0;
                // if (synth_debug_count < 5 && chan->period > 0) {
                //     fprintf(stderr, "SYNTH PROCESS: ch=%d period=%d vol=%d\n",
                //             ch, chan->period, chan->volume);
                //     synth_debug_count++;
                // }

                if (chan->period > 0) {
                    // For synth instruments, calculate frequency from period using C-2 as reference
                    // C-2 (period 428) = 130.81 Hz (MIDI note 48)
                    // Formula: freq = (130.81 * 428) / period = 55986.68 / period
                    float freq = 55986.68f / (float)chan->period;
                    float sample = synth_instrument_process(chan->sample->synth, freq, sample_rate);

                    // static int sample_debug_count = 0;
                    // if (sample_debug_count < 3) {
                    //     fprintf(stderr, "SYNTH SAMPLE: ch=%d wf=%d synth_vol=%d sample=%.3f freq=%.1f\n",
                    //             ch, chan->sample->synth->current_waveform,
                    //             chan->sample->synth->current_volume, sample, freq);
                    //     sample_debug_count++;
                    // }

                    // Apply volume: channel volume * sample volume * track volume * user volume
                    // Channel volume: 0-127 (from pattern commands, smoothly interpolated)
                    // Sample volume: 0-64 (stored in MMD0sample array)
                    // Track volume: 0-127 (stored in trackvols array) - but wait, OctaMED docs say 0-64!
                    // FIXME: Track volume scaling might be wrong
                    float vol = (chan->current_volume / (float)player->max_volume) * (chan->sample->volume / 64.0f) *
                               (player->track_volumes[ch] / 127.0f) * chan->user_volume;

                    // if (sample_debug_count < 3) {
                    //     fprintf(stderr, "  vol components: chan_vol=%.3f smp_vol=%.3f trk_vol=%.3f user_vol=%.3f => final_vol=%.3f\n",
                    //             chan->current_volume / (float)player->max_volume,
                    //             chan->sample->volume / 64.0f,
                    //             player->track_volumes[ch] / 127.0f,
                    //             chan->user_volume, vol);
                    //     fprintf(stderr, "  output: left+=%.3f right+=%.3f\n",
                    //             sample * vol * (1.0f - ((player->track_pans[ch] + 16) / 32.0f)),
                    //             sample * vol * ((player->track_pans[ch] + 16) / 32.0f));
                    // }

                    // Apply panning (use track panning for classic Amiga hard panning)
                    float pan = (player->track_pans[ch] + 16) / 32.0f;
                    left += sample * vol * (1.0f - pan);
                    right += sample * vol * pan;
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
                // Channel volume: 0-127 (from pattern commands, smoothly interpolated)
                // Sample volume: 0-64 (stored in MMD0sample array)
                // Track volume: 0-127 (127 = full volume / no scaling)
                float vol = (chan->current_volume / (float)player->max_volume) * (chan->sample->volume / 64.0f) *
                           (player->track_volumes[ch] / 127.0f) * chan->user_volume;

                // Apply panning (use track panning for classic Amiga hard panning)
                float pan = (player->track_pans[ch] + 16) / 32.0f;  // -16..+16 -> 0..1
                // Amiga-style mixing: simple addition without normalization (0.5x scaling)
                left += sample * vol * (1.0f - pan) * 0.5f;
                right += sample * vol * pan * 0.5f;
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

// Set loop range
void med_player_set_loop_range(MedPlayer* player, uint16_t start_order, uint16_t end_order) {
    if (!player) return;

    // Clamp to valid range
    if (start_order >= player->song_length) start_order = player->song_length - 1;
    if (end_order >= player->song_length) end_order = player->song_length - 1;
    if (start_order > end_order) start_order = end_order;

    player->loop_start = start_order;
    player->loop_end = end_order;
}
