/*
 * SID Player - Commodore 64 SID Music Player
 * Simplified but accurate C64 music playback
 */

#include "sid_player.h"
#include "../synth/synth_sid.h"
#include "../common/cpu_6502.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* Fallback for log2f if not available */
#ifndef log2f
#define log2f(x) (logf(x) / logf(2.0f))
#endif

/* SID chip base address */
#define SID_BASE 0xD400

/* Memory size (64KB) */
#define MEMORY_SIZE 0x10000

/* Maximum number of CPU cycles per frame to prevent infinite loops */
#define MAX_CYCLES_PER_FRAME 100000

/* PSID file header structure */
typedef struct {
    char magic[4];           /* 'PSID' or 'RSID' */
    uint16_t version;        /* Version (1 or 2) */
    uint16_t data_offset;    /* Offset to C64 data */
    uint16_t load_address;   /* Load address (0 = use data's first 2 bytes) */
    uint16_t init_address;   /* Init routine address */
    uint16_t play_address;   /* Play routine address (0 = use IRQ) */
    uint16_t songs;          /* Number of songs */
    uint16_t start_song;     /* Default starting song (1-based) */
    uint32_t speed;          /* Speed flags (bit per song, 0=60Hz, 1=50Hz) */
    char name[32];           /* Song name (ISO-8859-1) */
    char author[32];         /* Author name */
    char copyright[32];      /* Copyright/release info */
    uint16_t flags;          /* Play flags (PSID v2+) */
    uint8_t start_page;      /* Relocation start page (PSID v2+) */
    uint8_t page_length;     /* Relocation page length (PSID v2+) */
    uint16_t reserved;       /* Reserved */
} __attribute__((packed)) PsidHeader;

/* Player state */
struct SidPlayer {
    /* SID synthesizer */
    SynthSID* synth;

    /* CPU and memory */
    CPU6502Context cpu_ctx;
    uint8_t memory[MEMORY_SIZE];

    /* Song information */
    char title[33];
    char author[33];
    char copyright[33];
    uint16_t init_address;
    uint16_t play_address;
    uint16_t load_address;
    uint16_t load_end;
    uint8_t num_songs;
    uint8_t current_song;
    uint8_t start_song;

    /* Timing */
    uint32_t speed_flags;      /* Speed bits for each song */
    bool is_pal;               /* Current song PAL (50Hz) or NTSC (60Hz) */
    double frame_counter;      /* Sample accumulator for timing */
    uint32_t time_ms;          /* Playback time in milliseconds */

    /* Playback state */
    bool playing;
    bool disable_looping;
    float boost;
    bool voice_mute[3];

    /* Position callback */
    SidPositionCallback position_callback;
    void* callback_user_data;

    /* SID register cache (for change detection) */
    uint8_t sid_regs[32];
    uint8_t prev_gate[3];      /* Previous gate state for each voice */
    uint16_t base_frequency[3]; /* Base frequency when gate triggered (for pitch bend) */

    /* Debug output */
    bool debug_enabled;
    uint32_t debug_frame_count;  /* For periodic output */
};

/* ============================================================================
 * Utility Functions
 * ============================================================================ */

static inline uint16_t read_be16(const uint8_t* data) {
    return (data[0] << 8) | data[1];
}

static inline uint32_t read_be32(const uint8_t* data) {
    return (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3];
}

/* ============================================================================
 * Memory Access
 * ============================================================================ */

/* Forward declaration */
static void handle_sid_voice_control(SidPlayer* player, uint8_t voice, uint8_t value);

/* Memory access callbacks for CPU emulator */
static uint8_t cpu_mem_read(void* userdata, uint16_t addr) {
    SidPlayer* player = (SidPlayer*)userdata;
    /* SID registers (read mostly return 0 or noise) */
    if (addr >= SID_BASE && addr < SID_BASE + 0x20) {
        return player->sid_regs[addr - SID_BASE];
    }
    return player->memory[addr];
}

/* Legacy wrapper (for internal use) */
static uint8_t mem_read(SidPlayer* player, uint16_t addr) {
    return cpu_mem_read(player, addr);
}

static void mem_write(SidPlayer* player, uint16_t addr, uint8_t value) {
    /* SID register writes */
    if (addr >= SID_BASE && addr < SID_BASE + 0x20) {
        uint8_t reg = addr - SID_BASE;
        player->sid_regs[reg] = value;

        /* Route to synth via hardware register interface */
        if (player->synth) {
            synth_sid_write_register(player->synth, reg, value);
        }

        /* Debug output for SID register writes */
        if (player->debug_enabled) {
            const char* reg_names[] = {
                "V1.FRQ_LO", "V1.FRQ_HI", "V1.PW_LO", "V1.PW_HI", "V1.CTRL", "V1.AD", "V1.SR",
                "V2.FRQ_LO", "V2.FRQ_HI", "V2.PW_LO", "V2.PW_HI", "V2.CTRL", "V2.AD", "V2.SR",
                "V3.FRQ_LO", "V3.FRQ_HI", "V3.PW_LO", "V3.PW_HI", "V3.CTRL", "V3.AD", "V3.SR",
                "FC_LO", "FC_HI", "RES_FILT", "MODE_VOL"
            };

            if (reg < 24) {
                /* Show timestamp */
                uint32_t time_ms = player->time_ms;
                uint32_t minutes = time_ms / 60000;
                uint32_t seconds = (time_ms / 1000) % 60;
                uint32_t millis = time_ms % 1000;

                fprintf(stderr, "[%02u:%02u.%03u] %10s = $%02X",
                        minutes, seconds, millis, reg_names[reg], value);

                /* Decode important registers */
                if (reg == 4 || reg == 11 || reg == 18) {  /* Control registers */
                    uint8_t voice = reg / 7;
                    fprintf(stderr, " [");
                    if (value & 0x80) fprintf(stderr, "NOISE ");
                    if (value & 0x40) fprintf(stderr, "PULSE ");
                    if (value & 0x20) fprintf(stderr, "SAW ");
                    if (value & 0x10) fprintf(stderr, "TRI ");
                    if (value & 0x08) fprintf(stderr, "TEST ");
                    if (value & 0x04) fprintf(stderr, "RING ");
                    if (value & 0x02) fprintf(stderr, "SYNC ");
                    if (value & 0x01) fprintf(stderr, "GATE");
                    fprintf(stderr, "]");

                    /* Show frequency for this voice */
                    uint8_t freq_lo = voice * 7;
                    uint8_t freq_hi = voice * 7 + 1;
                    uint16_t freq = (player->sid_regs[freq_hi] << 8) | player->sid_regs[freq_lo];
                    if (freq > 0) {
                        float hz = freq * 0.0596f;
                        fprintf(stderr, " freq=%uHz", (uint32_t)hz);
                    }
                }
                fprintf(stderr, "\n");
            }
        }

        /* All register parsing is now handled by synth_sid_write_register() above */
        return;
    }

    /* Regular memory write */
    player->memory[addr] = value;
}

/* CPU memory write callback */
static void cpu_mem_write(void* userdata, uint16_t addr, uint8_t value) {
    mem_write((SidPlayer*)userdata, addr, value);
}

/* ============================================================================
 * SID Control
 * ============================================================================ */

/* Helper function to handle SID control register writes */
static void handle_sid_voice_control(SidPlayer* player, uint8_t voice, uint8_t value) {
    /* Control register offsets: voice 0=4, voice 1=11, voice 2=18 */
    uint8_t ctrl_reg = voice * 7 + 4;
    uint8_t freq_lo_reg = voice * 7;
    uint8_t freq_hi_reg = voice * 7 + 1;

    /* Extract waveform bits */
    uint8_t waveform = value & 0xF0;
    uint8_t sid_waveform = 0;

    if (waveform & 0x10) sid_waveform |= SID_WAVE_TRIANGLE;
    if (waveform & 0x20) sid_waveform |= SID_WAVE_SAWTOOTH;
    if (waveform & 0x40) sid_waveform |= SID_WAVE_PULSE;
    if (waveform & 0x80) sid_waveform |= SID_WAVE_NOISE;

    synth_sid_set_waveform(player->synth, voice, sid_waveform);

    /* TEST bit (resets oscillator) */
    synth_sid_set_test(player->synth, voice, (value & 0x08) != 0);

    /* Sync and ring modulation */
    synth_sid_set_sync(player->synth, voice, (value & 0x02) != 0);
    synth_sid_set_ring_mod(player->synth, voice, (value & 0x04) != 0);

    /* Handle gate bit transitions */
    uint8_t new_gate = (value & 0x01);
    uint8_t old_gate = player->prev_gate[voice];
    player->prev_gate[voice] = new_gate;

    if (new_gate && !old_gate) {
        /* Gate 0->1 transition: trigger note on */
        uint16_t freq = (player->sid_regs[freq_hi_reg] << 8) | player->sid_regs[freq_lo_reg];

        if (freq > 0) {
            /* Store base frequency for pitch bend calculations */
            player->base_frequency[voice] = freq;

            /* Reset pitch bend to 0 for new note */
            synth_sid_set_pitch_bend(player->synth, voice, 0.0f);

            /* Convert SID frequency to Hz */
            /* PAL C64: SID clock = 985248 Hz, formula: Hz = freq * clock / 16777216 */
            float hz = freq * 0.0596f;  /* Approximation: 985248 / 16777216 */

            /* Convert Hz to MIDI note */
            /* Avoid log2 issues with very low frequencies */
            if (hz >= 8.0f && hz <= 12543.0f) {
                float note_f = 12.0f * log2f(hz / 440.0f) + 69.0f;
                uint8_t note = (uint8_t)(note_f + 0.5f);  /* Round */

                if (note > 0 && note < 128) {
                    synth_sid_note_on(player->synth, voice, note, 100);
                }
            }
        }
    } else if (!new_gate && old_gate) {
        /* Gate 1->0 transition: trigger note off */
        synth_sid_note_off(player->synth, voice);
        /* Clear base frequency when note ends */
        player->base_frequency[voice] = 0;
    }
}



/* ============================================================================
 * Lifecycle
 * ============================================================================ */

SidPlayer* sid_player_create(void) {
    SidPlayer* player = (SidPlayer*)calloc(1, sizeof(SidPlayer));
    if (!player) return NULL;

    player->synth = synth_sid_create(48000);
    if (!player->synth) {
        free(player);
        return NULL;
    }

    /* Initialize CPU emulator with memory callbacks */
    cpu_init(&player->cpu_ctx, player, cpu_mem_read, cpu_mem_write);

    player->boost = 1.0f;
    player->is_pal = true;  /* Default to PAL */

    return player;
}

void sid_player_destroy(SidPlayer* player) {
    if (!player) return;

    if (player->synth) {
        synth_sid_destroy(player->synth);
    }

    free(player);
}

/* ============================================================================
 * File Loading
 * ============================================================================ */

bool sid_player_detect(const uint8_t* data, size_t size) {
    if (size < 4) return false;

    return (memcmp(data, "PSID", 4) == 0 || memcmp(data, "RSID", 4) == 0);
}

bool sid_player_load(SidPlayer* player, const uint8_t* data, size_t size) {
    if (!player || !data || size < sizeof(PsidHeader)) {
        return false;
    }

    if (!sid_player_detect(data, size)) {
        return false;
    }

    /* Parse header */
    const PsidHeader* hdr = (const PsidHeader*)data;
    uint16_t version = read_be16((const uint8_t*)&hdr->version);
    uint16_t data_offset = read_be16((const uint8_t*)&hdr->data_offset);
    uint16_t load_addr = read_be16((const uint8_t*)&hdr->load_address);
    uint16_t init_addr = read_be16((const uint8_t*)&hdr->init_address);
    uint16_t play_addr = read_be16((const uint8_t*)&hdr->play_address);
    uint16_t num_songs = read_be16((const uint8_t*)&hdr->songs);
    uint16_t start_song = read_be16((const uint8_t*)&hdr->start_song);
    uint32_t speed = read_be32((const uint8_t*)&hdr->speed);

    /* Copy song info */
    memcpy(player->title, hdr->name, 32);
    player->title[32] = '\0';
    memcpy(player->author, hdr->author, 32);
    player->author[32] = '\0';
    memcpy(player->copyright, hdr->copyright, 32);
    player->copyright[32] = '\0';

    /* Load address */
    const uint8_t* song_data = data + data_offset;
    size_t song_size = size - data_offset;

    if (load_addr == 0) {
        /* Load address is in first 2 bytes (little endian) */
        if (song_size < 2) return false;
        load_addr = song_data[0] | (song_data[1] << 8);
        song_data += 2;
        song_size -= 2;
    }

    /* Clear memory */
    memset(player->memory, 0, MEMORY_SIZE);

    /* Load song data into memory */
    if (load_addr + song_size > MEMORY_SIZE) {
        song_size = MEMORY_SIZE - load_addr;
    }
    memcpy(player->memory + load_addr, song_data, song_size);

    /* Store addresses */
    player->load_address = load_addr;
    player->load_end = load_addr + song_size;
    player->init_address = init_addr;
    player->play_address = play_addr;
    player->num_songs = num_songs > 0 ? num_songs : 1;
    player->start_song = start_song > 0 ? start_song - 1 : 0;  /* Convert to 0-based */
    player->current_song = player->start_song;
    player->speed_flags = speed;

    /* Determine if current song is PAL or NTSC
     * Speed flag convention:
     * - Bit set (1) = PAL (50Hz) - European C64
     * - Bit clear (0) = NTSC (60Hz) - American C64
     * - All bits 0 = Unspecified, default to PAL (most common)
     */
    if (speed == 0) {
        /* No speed specified - default to PAL (most C64 games are European) */
        player->is_pal = true;
    } else {
        /* Check bit for this subsong */
        player->is_pal = (speed & (1 << player->current_song)) != 0;
    }

    /* Debug output for timing mode */
    fprintf(stderr, "SID Timing: %s (speed flag: 0x%08X, subsong: %d)%s\n",
            player->is_pal ? "PAL (50Hz)" : "NTSC (60Hz)",
            speed, player->current_song,
            speed == 0 ? " [defaulted to PAL]" : "");

    /* Initialize gate tracking */
    memset(player->prev_gate, 0, sizeof(player->prev_gate));

    return true;
}

/* ============================================================================
 * Playback Control
 * ============================================================================ */

void sid_player_start(SidPlayer* player) {
    if (!player || player->playing) return;

    #if 0
    fprintf(stderr, "=== SID Player Start ===\n");
    fprintf(stderr, "Init: $%04X, Play: $%04X, Subsong: %d\n",
            player->init_address, player->play_address, player->current_song);
    #endif

    /* Reset CPU */
    cpu_reset(&player->cpu_ctx);

    /* Call init routine */
    /* Setup for JSR to init routine */
    player->cpu_ctx.cpu.pc = player->init_address;
    player->cpu_ctx.cpu.a = player->current_song;

    /* Execute init routine (with cycle limit) */
    int total_cycles = 0;
    /* Push fake return address */
    cpu_push(&player->cpu_ctx, 0xFF);
    cpu_push(&player->cpu_ctx, 0xFF);

    /* Run until RTS or cycle limit */
    while (total_cycles < MAX_CYCLES_PER_FRAME) {
        uint16_t pc_before = player->cpu_ctx.cpu.pc;
        int cycles = cpu_step(&player->cpu_ctx);
        total_cycles += cycles;

        /* Check if returned (RTS to 0xFFFF will wrap) */
        if (player->cpu_ctx.cpu.pc == 0x0000 || player->cpu_ctx.cpu.pc == 0xFFFF) {
            break;
        }

        /* Detect infinite loops */
        if (player->cpu_ctx.cpu.pc == pc_before) {
            break;
        }
    }

    if (total_cycles >= MAX_CYCLES_PER_FRAME) {
        fprintf(stderr, "Warning: Init hit cycle limit\n");
    }

    player->playing = true;
    player->frame_counter = 0;
    player->time_ms = 0;

    /* Reset SID chip */
    synth_sid_reset(player->synth);
    memset(player->sid_regs, 0, sizeof(player->sid_regs));
    memset(player->prev_gate, 0, sizeof(player->prev_gate));

    /* If play address is 0, check for IRQ vector */
    if (player->play_address == 0) {
        /* Check CIA1 IRQ vector at $0314/$0315 or $FFFE/$FFFF */
        uint16_t irq_vector = player->memory[0x0314] | (player->memory[0x0315] << 8);
        if (irq_vector != 0 && irq_vector >= player->load_address && irq_vector < player->load_end) {
            player->play_address = irq_vector;
        } else {
            /* Try hardware IRQ vector */
            irq_vector = player->memory[0xFFFE] | (player->memory[0xFFFF] << 8);
            if (irq_vector != 0 && irq_vector >= player->load_address && irq_vector < player->load_end) {
                player->play_address = irq_vector;
            } else {
                fprintf(stderr, "WARNING: No valid play address found!\n");
            }
        }
    }
}

void sid_player_stop(SidPlayer* player) {
    if (!player) return;

    player->playing = false;
    synth_sid_all_notes_off(player->synth);
}

bool sid_player_is_playing(const SidPlayer* player) {
    return player && player->playing;
}

/* ============================================================================
 * Song Information
 * ============================================================================ */

void sid_player_set_subsong(SidPlayer* player, uint8_t subsong) {
    if (!player || subsong >= player->num_songs) return;

    bool was_playing = player->playing;
    if (was_playing) {
        sid_player_stop(player);
    }

    player->current_song = subsong;
    player->is_pal = (player->speed_flags & (1 << subsong)) != 0;

    if (was_playing) {
        sid_player_start(player);
    }
}

uint8_t sid_player_get_current_subsong(const SidPlayer* player) {
    return player ? player->current_song : 0;
}

uint8_t sid_player_get_num_subsongs(const SidPlayer* player) {
    return player ? player->num_songs : 0;
}

const char* sid_player_get_title(const SidPlayer* player) {
    return player ? player->title : NULL;
}

const char* sid_player_get_author(const SidPlayer* player) {
    return player ? player->author : NULL;
}

const char* sid_player_get_copyright(const SidPlayer* player) {
    return player ? player->copyright : NULL;
}

/* ============================================================================
 * Playback Position
 * ============================================================================ */

void sid_player_set_position_callback(SidPlayer* player,
                                       SidPositionCallback callback,
                                       void* user_data) {
    if (!player) return;

    player->position_callback = callback;
    player->callback_user_data = user_data;
}

uint32_t sid_player_get_time_ms(const SidPlayer* player) {
    return player ? player->time_ms : 0;
}

/* ============================================================================
 * Audio Rendering
 * ============================================================================ */

void sid_player_process(SidPlayer* player,
                        float* left,
                        float* right,
                        size_t num_samples,
                        int sample_rate) {
    sid_player_process_voices(player, left, right, NULL, num_samples, sample_rate);
}

void sid_player_process_voices(SidPlayer* player,
                               float* left,
                               float* right,
                               float* voice_outputs[3],
                               size_t num_samples,
                               int sample_rate) {
    if (!player || !left || !right) return;

    /* Clear output buffers */
    memset(left, 0, num_samples * sizeof(float));
    memset(right, 0, num_samples * sizeof(float));

    if (voice_outputs) {
        for (int v = 0; v < 3; v++) {
            if (voice_outputs[v]) {
                memset(voice_outputs[v], 0, num_samples * sizeof(float));
            }
        }
    }

    if (!player->playing) {
        return;
    }

    /* Calculate frame rate (PAL = 50Hz, NTSC = 60Hz) */
    double frame_rate = player->is_pal ? 50.0 : 60.0;
    double samples_per_frame = sample_rate / frame_rate;

    for (size_t i = 0; i < num_samples; i++) {
        /* Call play routine at frame rate */
        if (player->frame_counter >= samples_per_frame) {
            player->frame_counter -= samples_per_frame;

            /* Setup CPU for play routine call */
            player->cpu_ctx.cpu.pc = player->play_address;

            /* Push fake return address */
            uint8_t saved_sp = player->cpu_ctx.cpu.sp;
            cpu_push(&player->cpu_ctx, 0xFF);
            cpu_push(&player->cpu_ctx, 0xFF);

            /* Execute play routine */
            int total_cycles = 0;
            while (total_cycles < MAX_CYCLES_PER_FRAME) {
                int cycles = cpu_step(&player->cpu_ctx);
                total_cycles += cycles;

                /* Check if returned */
                if (player->cpu_ctx.cpu.pc == 0x0000 || player->cpu_ctx.cpu.pc == 0xFFFF) {
                    break;
                }

                /* Restore SP if it looks stuck */
                if (player->cpu_ctx.cpu.sp < saved_sp - 10) {
                    player->cpu_ctx.cpu.sp = saved_sp;
                    break;
                }
            }

            /* Update time */
            player->time_ms += (uint32_t)(1000.0 / frame_rate);

            /* Call position callback */
            if (player->position_callback) {
                player->position_callback(player->current_song,
                                         player->time_ms,
                                         player->callback_user_data);
            }
        }

        /* Increment sample accumulator AFTER frame processing */
        player->frame_counter += 1.0;

        /* Generate one sample from SID chip */
        float stereo[2];
        synth_sid_process_f32(player->synth, stereo, 1, sample_rate);

        /* Apply voice muting */
        float sample = stereo[0] * player->boost;

        left[i] = sample;
        right[i] = sample;
    }
}

/* ============================================================================
 * Mixer Controls
 * ============================================================================ */

void sid_player_set_voice_mute(SidPlayer* player, uint8_t voice, bool muted) {
    if (!player || voice >= 3) return;
    player->voice_mute[voice] = muted;
}

bool sid_player_get_voice_mute(const SidPlayer* player, uint8_t voice) {
    if (!player || voice >= 3) return false;
    return player->voice_mute[voice];
}

void sid_player_set_boost(SidPlayer* player, float boost) {
    if (!player) return;
    player->boost = boost;
}

void sid_player_set_disable_looping(SidPlayer* player, bool disable) {
    if (!player) return;
    player->disable_looping = disable;
}

void sid_player_set_pal_mode(SidPlayer* player, bool is_pal) {
    if (!player) return;
    player->is_pal = is_pal;
}

bool sid_player_is_pal(const SidPlayer* player) {
    if (!player) return true;  // Default to PAL
    return player->is_pal;
}

void sid_player_set_debug_output(SidPlayer* player, bool enabled) {
    if (!player) return;
    player->debug_enabled = enabled;

    if (enabled) {
        fprintf(stderr, "\n=== SID PLAYER DEBUG MODE ENABLED ===\n");
        fprintf(stderr, "Showing all SID register writes and voice activity\n\n");
    }
}

void sid_player_print_state(SidPlayer* player) {
    if (!player) return;

    uint32_t time_ms = player->time_ms;
    uint32_t minutes = time_ms / 60000;
    uint32_t seconds = (time_ms / 1000) % 60;
    uint32_t millis = time_ms % 1000;

    fprintf(stderr, "\n");
    fprintf(stderr, "============================================================\n");
    fprintf(stderr, "SID STATE SNAPSHOT at %02u:%02u.%03u\n", minutes, seconds, millis);
    fprintf(stderr, "============================================================\n\n");

    for (int voice = 0; voice < 3; voice++) {
        fprintf(stderr, "VOICE %d:\n", voice + 1);

        uint8_t base = voice * 7;
        uint16_t freq = (player->sid_regs[base + 1] << 8) | player->sid_regs[base];
        uint16_t pw = (player->sid_regs[base + 3] << 8) | player->sid_regs[base + 2];
        uint8_t ctrl = player->sid_regs[base + 4];
        uint8_t ad = player->sid_regs[base + 5];
        uint8_t sr = player->sid_regs[base + 6];

        float hz = freq * 0.0596f;
        fprintf(stderr, "  Frequency: $%04X (%u Hz)\n", freq, (uint32_t)hz);
        fprintf(stderr, "  Pulse Width: $%03X\n", pw);
        fprintf(stderr, "  Control: $%02X [", ctrl);
        if (ctrl & 0x80) fprintf(stderr, "NOISE ");
        if (ctrl & 0x40) fprintf(stderr, "PULSE ");
        if (ctrl & 0x20) fprintf(stderr, "SAW ");
        if (ctrl & 0x10) fprintf(stderr, "TRI ");
        if (ctrl & 0x08) fprintf(stderr, "TEST ");
        if (ctrl & 0x04) fprintf(stderr, "RING ");
        if (ctrl & 0x02) fprintf(stderr, "SYNC ");
        if (ctrl & 0x01) fprintf(stderr, "GATE");
        fprintf(stderr, "]\n");
        fprintf(stderr, "  Attack/Decay: $%02X (A=%d D=%d)\n", ad, (ad >> 4) & 0xF, ad & 0xF);
        fprintf(stderr, "  Sustain/Release: $%02X (S=%d R=%d)\n\n", sr, (sr >> 4) & 0xF, sr & 0xF);
    }

    fprintf(stderr, "FILTER:\n");
    uint16_t fc = ((player->sid_regs[22] & 0x07) << 8) | player->sid_regs[21];
    uint8_t res_filt = player->sid_regs[23];
    uint8_t mode_vol = player->sid_regs[24];

    fprintf(stderr, "  Cutoff: $%03X\n", fc);
    fprintf(stderr, "  Resonance: %d\n", (res_filt >> 4) & 0xF);
    fprintf(stderr, "  Voice routing: V1=%s V2=%s V3=%s\n",
            (res_filt & 0x01) ? "FILT" : "DIR",
            (res_filt & 0x02) ? "FILT" : "DIR",
            (res_filt & 0x04) ? "FILT" : "DIR");
    fprintf(stderr, "  Mode: $%02X [", mode_vol);
    if (mode_vol & 0x40) fprintf(stderr, "HP ");
    if (mode_vol & 0x20) fprintf(stderr, "BP ");
    if (mode_vol & 0x10) fprintf(stderr, "LP ");
    fprintf(stderr, "]\n");
    fprintf(stderr, "  Volume: %d\n", mode_vol & 0x0F);

    fprintf(stderr, "\n============================================================\n\n");
}
