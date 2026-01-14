/*
 * SID Player - Commodore 64 SID Music Player
 * Simplified but accurate C64 music playback
 */

#include "sid_player.h"
#include "synth_sid.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

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

/* 6502 CPU state */
typedef struct {
    uint16_t pc;        /* Program counter */
    uint8_t a;          /* Accumulator */
    uint8_t x;          /* X register */
    uint8_t y;          /* Y register */
    uint8_t sp;         /* Stack pointer */
    uint8_t p;          /* Processor status */

    /* Status flags */
    uint8_t flag_n;     /* Negative */
    uint8_t flag_v;     /* Overflow */
    uint8_t flag_b;     /* Break */
    uint8_t flag_d;     /* Decimal */
    uint8_t flag_i;     /* Interrupt disable */
    uint8_t flag_z;     /* Zero */
    uint8_t flag_c;     /* Carry */
} CPU6502;

/* Player state */
struct SidPlayer {
    /* SID synthesizer */
    SynthSID* synth;

    /* CPU and memory */
    CPU6502 cpu;
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

static uint8_t mem_read(SidPlayer* player, uint16_t addr) {
    /* SID registers (read mostly return 0 or noise) */
    if (addr >= SID_BASE && addr < SID_BASE + 0x20) {
        return player->sid_regs[addr - SID_BASE];
    }

    return player->memory[addr];
}

static void mem_write(SidPlayer* player, uint16_t addr, uint8_t value) {
    /* SID register writes */
    if (addr >= SID_BASE && addr < SID_BASE + 0x20) {
        uint8_t reg = addr - SID_BASE;
        player->sid_regs[reg] = value;

        /* Optionally log first few SID writes for debugging */
        #if 0
        static int write_count = 0;
        if (write_count < 10) {
            fprintf(stderr, "SID[$%02X] = $%02X\n", reg, value);
            write_count++;
        }
        #endif

        /* Voice 1 (registers 0-6) */
        if (reg <= 6) {
            uint8_t voice = 0;
            switch (reg) {
                case 0: /* Frequency low */
                case 1: /* Frequency high */
                    /* Frequency will be handled by waveform control register */
                    break;

                case 2: /* Pulse width low */
                case 3: /* Pulse width high */ {
                    uint16_t pw = (player->sid_regs[3] << 8) | player->sid_regs[2];
                    float width = pw / 4095.0f; /* 12-bit value */
                    synth_sid_set_pulse_width(player->synth, voice, width);
                    break;
                }

                case 4: /* Waveform control */
                    handle_sid_voice_control(player, voice, value);
                    break;

                case 5: /* Attack/Decay */ {
                    float attack = ((value >> 4) & 0x0F) / 15.0f;
                    float decay = (value & 0x0F) / 15.0f;
                    synth_sid_set_attack(player->synth, voice, attack);
                    synth_sid_set_decay(player->synth, voice, decay);
                    break;
                }

                case 6: /* Sustain/Release */ {
                    float sustain = ((value >> 4) & 0x0F) / 15.0f;
                    float release = (value & 0x0F) / 15.0f;
                    synth_sid_set_sustain(player->synth, voice, sustain);
                    synth_sid_set_release(player->synth, voice, release);
                    break;
                }
            }
        }
        /* Voice 2 (registers 7-13) */
        else if (reg >= 7 && reg <= 13) {
            uint8_t voice = 1;
            uint8_t vreg = reg - 7;
            switch (vreg) {
                case 2:
                case 3: {
                    uint16_t pw = (player->sid_regs[10] << 8) | player->sid_regs[9];
                    float width = pw / 4095.0f;
                    synth_sid_set_pulse_width(player->synth, voice, width);
                    break;
                }

                case 4:
                    handle_sid_voice_control(player, voice, value);
                    break;

                case 5: {
                    float attack = ((value >> 4) & 0x0F) / 15.0f;
                    float decay = (value & 0x0F) / 15.0f;
                    synth_sid_set_attack(player->synth, voice, attack);
                    synth_sid_set_decay(player->synth, voice, decay);
                    break;
                }

                case 6: {
                    float sustain = ((value >> 4) & 0x0F) / 15.0f;
                    float release = (value & 0x0F) / 15.0f;
                    synth_sid_set_sustain(player->synth, voice, sustain);
                    synth_sid_set_release(player->synth, voice, release);
                    break;
                }
            }
        }
        /* Voice 3 (registers 14-20) */
        else if (reg >= 14 && reg <= 20) {
            uint8_t voice = 2;
            uint8_t vreg = reg - 14;
            switch (vreg) {
                case 2:
                case 3: {
                    uint16_t pw = (player->sid_regs[17] << 8) | player->sid_regs[16];
                    float width = pw / 4095.0f;
                    synth_sid_set_pulse_width(player->synth, voice, width);
                    break;
                }

                case 4:
                    handle_sid_voice_control(player, voice, value);
                    break;

                case 5: {
                    float attack = ((value >> 4) & 0x0F) / 15.0f;
                    float decay = (value & 0x0F) / 15.0f;
                    synth_sid_set_attack(player->synth, voice, attack);
                    synth_sid_set_decay(player->synth, voice, decay);
                    break;
                }

                case 6: {
                    float sustain = ((value >> 4) & 0x0F) / 15.0f;
                    float release = (value & 0x0F) / 15.0f;
                    synth_sid_set_sustain(player->synth, voice, sustain);
                    synth_sid_set_release(player->synth, voice, release);
                    break;
                }
            }
        }
        /* Filter cutoff (registers 21-22) */
        else if (reg == 21 || reg == 22) {
            uint16_t fc = ((player->sid_regs[22] & 0x07) << 8) | player->sid_regs[21];
            float cutoff = fc / 2047.0f;  /* 11-bit value */
            synth_sid_set_filter_cutoff(player->synth, cutoff);
        }
        /* Filter control (register 23) */
        else if (reg == 23) {
            synth_sid_set_filter_voice(player->synth, 0, (value & 0x01) != 0);
            synth_sid_set_filter_voice(player->synth, 1, (value & 0x02) != 0);
            synth_sid_set_filter_voice(player->synth, 2, (value & 0x04) != 0);
            float resonance = ((value >> 4) & 0x0F) / 15.0f;
            synth_sid_set_filter_resonance(player->synth, resonance);
        }
        /* Filter mode/volume (register 24) */
        else if (reg == 24) {
            /* Filter mode */
            if (value & 0x10) {
                synth_sid_set_filter_mode(player->synth, SID_FILTER_LP);
            } else if (value & 0x20) {
                synth_sid_set_filter_mode(player->synth, SID_FILTER_BP);
            } else if (value & 0x40) {
                synth_sid_set_filter_mode(player->synth, SID_FILTER_HP);
            } else {
                synth_sid_set_filter_mode(player->synth, SID_FILTER_OFF);
            }

            /* Volume */
            float volume = (value & 0x0F) / 15.0f;
            synth_sid_set_volume(player->synth, volume);
        }

        return;
    }

    /* Regular memory write */
    player->memory[addr] = value;
}

/* ============================================================================
 * 6502 CPU Emulation (Minimal - just enough for SID players)
 * ============================================================================ */

static void cpu_push(SidPlayer* player, uint8_t value) {
    player->memory[0x0100 + player->cpu.sp] = value;
    player->cpu.sp--;
}

static uint8_t cpu_pull(SidPlayer* player) {
    player->cpu.sp++;
    return player->memory[0x0100 + player->cpu.sp];
}

static void cpu_set_nz(CPU6502* cpu, uint8_t value) {
    cpu->flag_z = (value == 0);
    cpu->flag_n = (value & 0x80) != 0;
}

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
    }
}

/* Execute one instruction, return number of cycles */
static int cpu_step(SidPlayer* player) {
    CPU6502* cpu = &player->cpu;
    uint16_t pc_start = cpu->pc;
    uint8_t opcode = mem_read(player, cpu->pc++);
    int cycles = 2;  /* Most instructions are 2+ cycles */

    static int unknown_count = 0;
    static int instr_count = 0;
    bool is_unknown = false;

    switch (opcode) {
        /* ADC - Add with Carry */
        case 0x65: /* Zero Page */ {
            uint8_t value = mem_read(player, mem_read(player, cpu->pc++));
            uint16_t result = cpu->a + value + cpu->flag_c;
            cpu->flag_c = result > 0xFF;
            cpu->flag_v = ((cpu->a ^ result) & (value ^ result) & 0x80) != 0;
            cpu->a = result & 0xFF;
            cpu_set_nz(cpu, cpu->a);
            cycles = 3;
            break;
        }
        case 0x69: { /* Immediate */
            uint8_t value = mem_read(player, cpu->pc++);
            uint16_t result = cpu->a + value + cpu->flag_c;
            cpu->flag_c = result > 0xFF;
            cpu->flag_v = ((cpu->a ^ result) & (value ^ result) & 0x80) != 0;
            cpu->a = result & 0xFF;
            cpu_set_nz(cpu, cpu->a);
            break;
        }
        case 0x6D: { /* Absolute */
            uint16_t addr = mem_read(player, cpu->pc++) | (mem_read(player, cpu->pc++) << 8);
            uint8_t value = mem_read(player, addr);
            uint16_t result = cpu->a + value + cpu->flag_c;
            cpu->flag_c = result > 0xFF;
            cpu->flag_v = ((cpu->a ^ result) & (value ^ result) & 0x80) != 0;
            cpu->a = result & 0xFF;
            cpu_set_nz(cpu, cpu->a);
            cycles = 4;
            break;
        }
        case 0x7D: { /* Absolute,X */
            uint16_t base = mem_read(player, cpu->pc++) | (mem_read(player, cpu->pc++) << 8);
            uint16_t addr = base + cpu->x;
            uint8_t value = mem_read(player, addr);
            uint16_t result = cpu->a + value + cpu->flag_c;
            cpu->flag_c = result > 0xFF;
            cpu->flag_v = ((cpu->a ^ result) & (value ^ result) & 0x80) != 0;
            cpu->a = result & 0xFF;
            cpu_set_nz(cpu, cpu->a);
            cycles = 4;
            break;
        }

        /* AND - Logical AND */
        case 0x25: /* Zero Page */
            cpu->a &= mem_read(player, mem_read(player, cpu->pc++));
            cpu_set_nz(cpu, cpu->a);
            cycles = 3;
            break;
        case 0x29: { /* Immediate */
            uint8_t value = mem_read(player, cpu->pc++);
            cpu->a &= value;
            cpu_set_nz(cpu, cpu->a);
            break;
        }
        case 0x2D: { /* Absolute */
            uint16_t addr = mem_read(player, cpu->pc++) | (mem_read(player, cpu->pc++) << 8);
            cpu->a &= mem_read(player, addr);
            cpu_set_nz(cpu, cpu->a);
            cycles = 4;
            break;
        }
        case 0x39: { /* Absolute,Y */
            uint16_t base = mem_read(player, cpu->pc++) | (mem_read(player, cpu->pc++) << 8);
            uint16_t addr = base + cpu->y;
            cpu->a &= mem_read(player, addr);
            cpu_set_nz(cpu, cpu->a);
            cycles = 4;
            break;
        }
        case 0x3D: { /* Absolute,X */
            uint16_t base = mem_read(player, cpu->pc++) | (mem_read(player, cpu->pc++) << 8);
            uint16_t addr = base + cpu->x;
            cpu->a &= mem_read(player, addr);
            cpu_set_nz(cpu, cpu->a);
            cycles = 4;
            break;
        }

        /* BCC - Branch if Carry Clear */
        case 0x90: {
            int8_t offset = (int8_t)mem_read(player, cpu->pc++);
            if (!cpu->flag_c) {
                cpu->pc += offset;
                cycles = 3;
            }
            break;
        }

        /* BCS - Branch if Carry Set */
        case 0xB0: {
            int8_t offset = (int8_t)mem_read(player, cpu->pc++);
            if (cpu->flag_c) {
                cpu->pc += offset;
                cycles = 3;
            }
            break;
        }

        /* BEQ - Branch if Equal (Z set) */
        case 0xF0: {
            int8_t offset = (int8_t)mem_read(player, cpu->pc++);
            if (cpu->flag_z) {
                cpu->pc += offset;
                cycles = 3;
            }
            break;
        }

        /* BIT - Test Bits */
        case 0x24: /* Zero Page */ {
            uint8_t value = mem_read(player, mem_read(player, cpu->pc++));
            cpu->flag_z = (cpu->a & value) == 0;
            cpu->flag_n = (value & 0x80) != 0;
            cpu->flag_v = (value & 0x40) != 0;
            cycles = 3;
            break;
        }
        case 0x2C: { /* Absolute */
            uint16_t addr = mem_read(player, cpu->pc++) | (mem_read(player, cpu->pc++) << 8);
            uint8_t value = mem_read(player, addr);
            cpu->flag_z = (cpu->a & value) == 0;
            cpu->flag_n = (value & 0x80) != 0;
            cpu->flag_v = (value & 0x40) != 0;
            cycles = 4;
            break;
        }

        /* BMI - Branch if Minus (N set) */
        case 0x30: {
            int8_t offset = (int8_t)mem_read(player, cpu->pc++);
            if (cpu->flag_n) {
                cpu->pc += offset;
                cycles = 3;
            }
            break;
        }

        /* BNE - Branch if Not Equal (Z clear) */
        case 0xD0: {
            int8_t offset = (int8_t)mem_read(player, cpu->pc++);
            if (!cpu->flag_z) {
                cpu->pc += offset;
                cycles = 3;
            }
            break;
        }

        /* BPL - Branch if Plus (N clear) */
        case 0x10: {
            int8_t offset = (int8_t)mem_read(player, cpu->pc++);
            if (!cpu->flag_n) {
                cpu->pc += offset;
                cycles = 3;
            }
            break;
        }

        /* BVS - Branch if Overflow Set */
        case 0x70: {
            int8_t offset = (int8_t)mem_read(player, cpu->pc++);
            if (cpu->flag_v) {
                cpu->pc += offset;
                cycles = 3;
            }
            break;
        }

        /* ASL - Arithmetic Shift Left */
        case 0x0A: /* Accumulator */
            cpu->flag_c = (cpu->a & 0x80) != 0;
            cpu->a <<= 1;
            cpu_set_nz(cpu, cpu->a);
            break;
        case 0x06: { /* Zero Page */
            uint8_t zp = mem_read(player, cpu->pc++);
            uint8_t value = mem_read(player, zp);
            cpu->flag_c = (value & 0x80) != 0;
            value <<= 1;
            mem_write(player, zp, value);
            cpu_set_nz(cpu, value);
            cycles = 5;
            break;
        }
        case 0x0E: { /* Absolute */
            uint16_t addr = mem_read(player, cpu->pc++) | (mem_read(player, cpu->pc++) << 8);
            uint8_t value = mem_read(player, addr);
            cpu->flag_c = (value & 0x80) != 0;
            value <<= 1;
            mem_write(player, addr, value);
            cpu_set_nz(cpu, value);
            cycles = 6;
            break;
        }
        case 0x1E: { /* Absolute,X */
            uint16_t base = mem_read(player, cpu->pc++) | (mem_read(player, cpu->pc++) << 8);
            uint16_t addr = base + cpu->x;
            uint8_t value = mem_read(player, addr);
            cpu->flag_c = (value & 0x80) != 0;
            value <<= 1;
            mem_write(player, addr, value);
            cpu_set_nz(cpu, value);
            cycles = 7;
            break;
        }

        /* CLC - Clear Carry */
        case 0x18:
            cpu->flag_c = 0;
            break;

        /* CLD - Clear Decimal */
        case 0xD8:
            cpu->flag_d = 0;
            break;

        /* CLI - Clear Interrupt Disable */
        case 0x58:
            cpu->flag_i = 0;
            break;

        /* CMP - Compare Accumulator */
        case 0xC5: { /* Zero Page */
            uint8_t value = mem_read(player, mem_read(player, cpu->pc++));
            uint16_t result = cpu->a - value;
            cpu->flag_c = cpu->a >= value;
            cpu_set_nz(cpu, result & 0xFF);
            cycles = 3;
            break;
        }
        case 0xC9: { /* Immediate */
            uint8_t value = mem_read(player, cpu->pc++);
            uint16_t result = cpu->a - value;
            cpu->flag_c = cpu->a >= value;
            cpu_set_nz(cpu, result & 0xFF);
            break;
        }
        case 0xCD: { /* Absolute */
            uint16_t addr = mem_read(player, cpu->pc++) | (mem_read(player, cpu->pc++) << 8);
            uint8_t value = mem_read(player, addr);
            uint16_t result = cpu->a - value;
            cpu->flag_c = cpu->a >= value;
            cpu_set_nz(cpu, result & 0xFF);
            cycles = 4;
            break;
        }
        case 0xD9: { /* Absolute,Y */
            uint16_t base = mem_read(player, cpu->pc++) | (mem_read(player, cpu->pc++) << 8);
            uint16_t addr = base + cpu->y;
            uint8_t value = mem_read(player, addr);
            uint16_t result = cpu->a - value;
            cpu->flag_c = cpu->a >= value;
            cpu_set_nz(cpu, result & 0xFF);
            cycles = 4;
            break;
        }
        case 0xDD: { /* Absolute,X */
            uint16_t base = mem_read(player, cpu->pc++) | (mem_read(player, cpu->pc++) << 8);
            uint16_t addr = base + cpu->x;
            uint8_t value = mem_read(player, addr);
            uint16_t result = cpu->a - value;
            cpu->flag_c = cpu->a >= value;
            cpu_set_nz(cpu, result & 0xFF);
            cycles = 4;
            break;
        }

        /* CPX - Compare X */
        case 0xE0: { /* Immediate */
            uint8_t value = mem_read(player, cpu->pc++);
            uint16_t result = cpu->x - value;
            cpu->flag_c = cpu->x >= value;
            cpu_set_nz(cpu, result & 0xFF);
            break;
        }
        case 0xEC: { /* Absolute */
            uint16_t addr = mem_read(player, cpu->pc++) | (mem_read(player, cpu->pc++) << 8);
            uint8_t value = mem_read(player, addr);
            uint16_t result = cpu->x - value;
            cpu->flag_c = cpu->x >= value;
            cpu_set_nz(cpu, result & 0xFF);
            cycles = 4;
            break;
        }

        /* CPY - Compare Y */
        case 0xC0: { /* Immediate */
            uint8_t value = mem_read(player, cpu->pc++);
            uint16_t result = cpu->y - value;
            cpu->flag_c = cpu->y >= value;
            cpu_set_nz(cpu, result & 0xFF);
            break;
        }

        /* DEC - Decrement Memory */
        case 0xC6: /* Zero Page */ {
            uint8_t zp = mem_read(player, cpu->pc++);
            uint8_t value = mem_read(player, zp) - 1;
            mem_write(player, zp, value);
            cpu_set_nz(cpu, value);
            cycles = 5;
            break;
        }
        case 0xCE: { /* Absolute */
            uint16_t addr = mem_read(player, cpu->pc++) | (mem_read(player, cpu->pc++) << 8);
            uint8_t value = mem_read(player, addr) - 1;
            mem_write(player, addr, value);
            cpu_set_nz(cpu, value);
            cycles = 6;
            break;
        }
        case 0xDE: { /* Absolute,X */
            uint16_t base = mem_read(player, cpu->pc++) | (mem_read(player, cpu->pc++) << 8);
            uint16_t addr = base + cpu->x;
            uint8_t value = mem_read(player, addr) - 1;
            mem_write(player, addr, value);
            cpu_set_nz(cpu, value);
            cycles = 7;
            break;
        }

        /* DEX - Decrement X */
        case 0xCA:
            cpu->x--;
            cpu_set_nz(cpu, cpu->x);
            break;

        /* DEY - Decrement Y */
        case 0x88:
            cpu->y--;
            cpu_set_nz(cpu, cpu->y);
            break;

        /* INC - Increment Memory */
        case 0xE6: /* Zero Page */ {
            uint8_t zp = mem_read(player, cpu->pc++);
            uint8_t value = mem_read(player, zp) + 1;
            mem_write(player, zp, value);
            cpu_set_nz(cpu, value);
            cycles = 5;
            break;
        }
        case 0xEE: { /* Absolute */
            uint16_t addr = mem_read(player, cpu->pc++) | (mem_read(player, cpu->pc++) << 8);
            uint8_t value = mem_read(player, addr) + 1;
            mem_write(player, addr, value);
            cpu_set_nz(cpu, value);
            cycles = 6;
            break;
        }

        /* INX - Increment X */
        case 0xE8:
            cpu->x++;
            cpu_set_nz(cpu, cpu->x);
            break;

        /* INY - Increment Y */
        case 0xC8:
            cpu->y++;
            cpu_set_nz(cpu, cpu->y);
            break;

        /* JMP - Jump */
        case 0x4C: { /* Absolute */
            uint16_t addr = mem_read(player, cpu->pc++) | (mem_read(player, cpu->pc++) << 8);
            cpu->pc = addr;
            cycles = 3;
            break;
        }
        case 0x6C: { /* Indirect */
            uint16_t ptr = mem_read(player, cpu->pc++) | (mem_read(player, cpu->pc++) << 8);
            /* Handle 6502 page boundary bug */
            uint16_t addr;
            if ((ptr & 0xFF) == 0xFF) {
                addr = mem_read(player, ptr) | (mem_read(player, ptr & 0xFF00) << 8);
            } else {
                addr = mem_read(player, ptr) | (mem_read(player, ptr + 1) << 8);
            }
            cpu->pc = addr;
            cycles = 5;
            break;
        }

        /* JSR - Jump to Subroutine */
        case 0x20: {
            uint16_t addr = mem_read(player, cpu->pc++) | (mem_read(player, cpu->pc++) << 8);
            uint16_t ret_addr = cpu->pc - 1;
            cpu_push(player, (ret_addr >> 8) & 0xFF);
            cpu_push(player, ret_addr & 0xFF);
            cpu->pc = addr;
            cycles = 6;
            break;
        }

        /* LDA - Load Accumulator */
        case 0xA5: /* Zero Page */
            cpu->a = mem_read(player, mem_read(player, cpu->pc++));
            cpu_set_nz(cpu, cpu->a);
            cycles = 3;
            break;
        case 0xA9: /* Immediate */
            cpu->a = mem_read(player, cpu->pc++);
            cpu_set_nz(cpu, cpu->a);
            break;
        case 0xAD: { /* Absolute */
            uint16_t addr = mem_read(player, cpu->pc++) | (mem_read(player, cpu->pc++) << 8);
            cpu->a = mem_read(player, addr);
            cpu_set_nz(cpu, cpu->a);
            cycles = 4;
            break;
        }
        case 0xB9: { /* Absolute,Y */
            uint16_t base = mem_read(player, cpu->pc++) | (mem_read(player, cpu->pc++) << 8);
            uint16_t addr = base + cpu->y;
            cpu->a = mem_read(player, addr);
            cpu_set_nz(cpu, cpu->a);
            cycles = 4;
            break;
        }
        case 0xB1: { /* (Indirect),Y */
            uint8_t zp = mem_read(player, cpu->pc++);
            uint16_t base = mem_read(player, zp) | (mem_read(player, (zp + 1) & 0xFF) << 8);
            uint16_t addr = base + cpu->y;
            cpu->a = mem_read(player, addr);
            cpu_set_nz(cpu, cpu->a);
            cycles = 5;
            break;
        }
        case 0xBD: { /* Absolute,X */
            uint16_t base = mem_read(player, cpu->pc++) | (mem_read(player, cpu->pc++) << 8);
            uint16_t addr = base + cpu->x;
            cpu->a = mem_read(player, addr);
            cpu_set_nz(cpu, cpu->a);
            cycles = 4;
            break;
        }

        /* LDX - Load X */
        case 0xA2: /* Immediate */
            cpu->x = mem_read(player, cpu->pc++);
            cpu_set_nz(cpu, cpu->x);
            break;
        case 0xA6: /* Zero Page */
            cpu->x = mem_read(player, mem_read(player, cpu->pc++));
            cpu_set_nz(cpu, cpu->x);
            cycles = 3;
            break;
        case 0xAE: { /* Absolute */
            uint16_t addr = mem_read(player, cpu->pc++) | (mem_read(player, cpu->pc++) << 8);
            cpu->x = mem_read(player, addr);
            cpu_set_nz(cpu, cpu->x);
            cycles = 4;
            break;
        }
        case 0xBE: { /* Absolute,Y */
            uint16_t base = mem_read(player, cpu->pc++) | (mem_read(player, cpu->pc++) << 8);
            uint16_t addr = base + cpu->y;
            cpu->x = mem_read(player, addr);
            cpu_set_nz(cpu, cpu->x);
            cycles = 4;
            break;
        }

        /* LDY - Load Y */
        case 0xA0: /* Immediate */
            cpu->y = mem_read(player, cpu->pc++);
            cpu_set_nz(cpu, cpu->y);
            break;
        case 0xA4: /* Zero Page */
            cpu->y = mem_read(player, mem_read(player, cpu->pc++));
            cpu_set_nz(cpu, cpu->y);
            cycles = 3;
            break;
        case 0xAC: { /* Absolute */
            uint16_t addr = mem_read(player, cpu->pc++) | (mem_read(player, cpu->pc++) << 8);
            cpu->y = mem_read(player, addr);
            cpu_set_nz(cpu, cpu->y);
            cycles = 4;
            break;
        }
        case 0xBC: { /* Absolute,X */
            uint16_t base = mem_read(player, cpu->pc++) | (mem_read(player, cpu->pc++) << 8);
            uint16_t addr = base + cpu->x;
            cpu->y = mem_read(player, addr);
            cpu_set_nz(cpu, cpu->y);
            cycles = 4;
            break;
        }

        /* LSR - Logical Shift Right */
        case 0x4A: /* Accumulator */
            cpu->flag_c = cpu->a & 0x01;
            cpu->a >>= 1;
            cpu_set_nz(cpu, cpu->a);
            break;

        /* NOP - No Operation */
        case 0xEA:
            break;

        /* ORA - Logical OR */
        case 0x05: /* Zero Page */
            cpu->a |= mem_read(player, mem_read(player, cpu->pc++));
            cpu_set_nz(cpu, cpu->a);
            cycles = 3;
            break;
        case 0x09: { /* Immediate */
            uint8_t value = mem_read(player, cpu->pc++);
            cpu->a |= value;
            cpu_set_nz(cpu, cpu->a);
            break;
        }
        case 0x0D: { /* Absolute */
            uint16_t addr = mem_read(player, cpu->pc++) | (mem_read(player, cpu->pc++) << 8);
            cpu->a |= mem_read(player, addr);
            cpu_set_nz(cpu, cpu->a);
            cycles = 4;
            break;
        }
        case 0x11: { /* (Indirect),Y */
            uint8_t zp = mem_read(player, cpu->pc++);
            uint16_t base = mem_read(player, zp) | (mem_read(player, (zp + 1) & 0xFF) << 8);
            uint16_t addr = base + cpu->y;
            cpu->a |= mem_read(player, addr);
            cpu_set_nz(cpu, cpu->a);
            cycles = 5;
            break;
        }
        case 0x19: { /* Absolute,Y */
            uint16_t base = mem_read(player, cpu->pc++) | (mem_read(player, cpu->pc++) << 8);
            uint16_t addr = base + cpu->y;
            cpu->a |= mem_read(player, addr);
            cpu_set_nz(cpu, cpu->a);
            cycles = 4;
            break;
        }

        /* PHA - Push Accumulator */
        case 0x48:
            cpu_push(player, cpu->a);
            cycles = 3;
            break;

        /* PLA - Pull Accumulator */
        case 0x68:
            cpu->a = cpu_pull(player);
            cpu_set_nz(cpu, cpu->a);
            cycles = 4;
            break;

        /* RTI - Return from Interrupt */
        case 0x40:
            cpu->p = cpu_pull(player);
            cpu->pc = cpu_pull(player) | (cpu_pull(player) << 8);
            cycles = 6;
            break;

        /* ROR - Rotate Right */
        case 0x66: { /* Zero Page */
            uint8_t zp = mem_read(player, cpu->pc++);
            uint8_t value = mem_read(player, zp);
            uint8_t old_carry = cpu->flag_c;
            cpu->flag_c = value & 0x01;
            value = (value >> 1) | (old_carry << 7);
            mem_write(player, zp, value);
            cpu_set_nz(cpu, value);
            cycles = 5;
            break;
        }
        case 0x6E: { /* Absolute */
            uint16_t addr = mem_read(player, cpu->pc++) | (mem_read(player, cpu->pc++) << 8);
            uint8_t value = mem_read(player, addr);
            uint8_t old_carry = cpu->flag_c;
            cpu->flag_c = value & 0x01;
            value = (value >> 1) | (old_carry << 7);
            mem_write(player, addr, value);
            cpu_set_nz(cpu, value);
            cycles = 6;
            break;
        }

        /* RTS - Return from Subroutine */
        case 0x60:
            cpu->pc = (cpu_pull(player) | (cpu_pull(player) << 8)) + 1;
            cycles = 6;
            break;

        /* SBC - Subtract with Carry */
        case 0xE5: { /* Zero Page */
            uint8_t value = mem_read(player, mem_read(player, cpu->pc++));
            uint16_t result = cpu->a - value - (1 - cpu->flag_c);
            cpu->flag_c = result < 0x100;
            cpu->flag_v = ((cpu->a ^ value) & (cpu->a ^ result) & 0x80) != 0;
            cpu->a = result & 0xFF;
            cpu_set_nz(cpu, cpu->a);
            cycles = 3;
            break;
        }
        case 0xE9: { /* Immediate */
            uint8_t value = mem_read(player, cpu->pc++);
            uint16_t result = cpu->a - value - (1 - cpu->flag_c);
            cpu->flag_c = result < 0x100;
            cpu->flag_v = ((cpu->a ^ value) & (cpu->a ^ result) & 0x80) != 0;
            cpu->a = result & 0xFF;
            cpu_set_nz(cpu, cpu->a);
            break;
        }
        case 0xF9: { /* Absolute,Y */
            uint16_t base = mem_read(player, cpu->pc++) | (mem_read(player, cpu->pc++) << 8);
            uint16_t addr = base + cpu->y;
            uint8_t value = mem_read(player, addr);
            uint16_t result = cpu->a - value - (1 - cpu->flag_c);
            cpu->flag_c = result < 0x100;
            cpu->flag_v = ((cpu->a ^ value) & (cpu->a ^ result) & 0x80) != 0;
            cpu->a = result & 0xFF;
            cpu_set_nz(cpu, cpu->a);
            cycles = 4;
            break;
        }
        case 0xFD: { /* Absolute,X */
            uint16_t base = mem_read(player, cpu->pc++) | (mem_read(player, cpu->pc++) << 8);
            uint16_t addr = base + cpu->x;
            uint8_t value = mem_read(player, addr);
            uint16_t result = cpu->a - value - (1 - cpu->flag_c);
            cpu->flag_c = result < 0x100;
            cpu->flag_v = ((cpu->a ^ value) & (cpu->a ^ result) & 0x80) != 0;
            cpu->a = result & 0xFF;
            cpu_set_nz(cpu, cpu->a);
            cycles = 4;
            break;
        }

        /* SEC - Set Carry */
        case 0x38:
            cpu->flag_c = 1;
            break;

        /* SED - Set Decimal */
        case 0xF8:
            cpu->flag_d = 1;
            break;

        /* SEI - Set Interrupt Disable */
        case 0x78:
            cpu->flag_i = 1;
            break;

        /* STA - Store Accumulator */
        case 0x85: /* Zero Page */
            mem_write(player, mem_read(player, cpu->pc++), cpu->a);
            cycles = 3;
            break;
        case 0x95: /* Zero Page,X */
            mem_write(player, (mem_read(player, cpu->pc++) + cpu->x) & 0xFF, cpu->a);
            cycles = 4;
            break;
        case 0x8D: { /* Absolute */
            uint16_t addr = mem_read(player, cpu->pc++) | (mem_read(player, cpu->pc++) << 8);
            mem_write(player, addr, cpu->a);
            cycles = 4;
            break;
        }
        case 0x91: { /* (Indirect),Y */
            uint8_t zp = mem_read(player, cpu->pc++);
            uint16_t base = mem_read(player, zp) | (mem_read(player, (zp + 1) & 0xFF) << 8);
            uint16_t addr = base + cpu->y;
            mem_write(player, addr, cpu->a);
            cycles = 6;
            break;
        }
        case 0x99: { /* Absolute,Y */
            uint16_t base = mem_read(player, cpu->pc++) | (mem_read(player, cpu->pc++) << 8);
            uint16_t addr = base + cpu->y;
            mem_write(player, addr, cpu->a);
            cycles = 5;
            break;
        }
        case 0x9D: { /* Absolute,X */
            uint16_t base = mem_read(player, cpu->pc++) | (mem_read(player, cpu->pc++) << 8);
            uint16_t addr = base + cpu->x;
            mem_write(player, addr, cpu->a);
            cycles = 5;
            break;
        }

        /* STX - Store X */
        case 0x86: /* Zero Page */
            mem_write(player, mem_read(player, cpu->pc++), cpu->x);
            cycles = 3;
            break;
        case 0x8E: { /* Absolute */
            uint16_t addr = mem_read(player, cpu->pc++) | (mem_read(player, cpu->pc++) << 8);
            mem_write(player, addr, cpu->x);
            cycles = 4;
            break;
        }

        /* STY - Store Y */
        case 0x84: /* Zero Page */
            mem_write(player, mem_read(player, cpu->pc++), cpu->y);
            cycles = 3;
            break;
        case 0x8C: { /* Absolute */
            uint16_t addr = mem_read(player, cpu->pc++) | (mem_read(player, cpu->pc++) << 8);
            mem_write(player, addr, cpu->y);
            cycles = 4;
            break;
        }

        /* TAX - Transfer A to X */
        case 0xAA:
            cpu->x = cpu->a;
            cpu_set_nz(cpu, cpu->x);
            break;

        /* TAY - Transfer A to Y */
        case 0xA8:
            cpu->y = cpu->a;
            cpu_set_nz(cpu, cpu->y);
            break;

        /* TXA - Transfer X to A */
        case 0x8A:
            cpu->a = cpu->x;
            cpu_set_nz(cpu, cpu->a);
            break;

        /* TYA - Transfer Y to A */
        case 0x98:
            cpu->a = cpu->y;
            cpu_set_nz(cpu, cpu->a);
            break;

        /* Illegal/undocumented opcodes - implement as close approximations */
        /* Many SID files use these! */

        /* SRE (LSR + EOR) - (Indirect),Y */
        case 0x53: {
            uint8_t zp = mem_read(player, cpu->pc++);
            uint16_t base = mem_read(player, zp) | (mem_read(player, (zp + 1) & 0xFF) << 8);
            uint16_t addr = base + cpu->y;
            uint8_t value = mem_read(player, addr);
            cpu->flag_c = value & 0x01;
            value >>= 1;
            mem_write(player, addr, value);
            cpu->a ^= value;
            cpu_set_nz(cpu, cpu->a);
            cycles = 8;
            break;
        }

        /* Unknown/unimplemented instruction - treat as NOP */
        default:
            is_unknown = true;
            if (unknown_count < 20) {
                fprintf(stderr, "Unknown opcode $%02X at $%04X (A=%02X X=%02X Y=%02X)\n",
                        opcode, pc_start, cpu->a, cpu->x, cpu->y);
                unknown_count++;
            }
            /* Try to skip operand bytes based on opcode pattern */
            /* This is a rough heuristic */
            switch (opcode & 0x1F) {
                case 0x19: case 0x1D: case 0x1E:  /* Absolute,X/Y or Absolute */
                case 0x0D: case 0x0E: case 0x0C:
                    cpu->pc += 2;
                    break;
                case 0x11: case 0x01:  /* (Indirect),Y or (Indirect,X) */
                case 0x05: case 0x06:  /* Zero Page */
                case 0x15: case 0x16:  /* Zero Page,X/Y */
                    cpu->pc += 1;
                    break;
                default:
                    /* Immediate or implied - no operand or 1 byte */
                    if ((opcode & 0x0F) == 0x09) {
                        cpu->pc += 1;  /* Immediate */
                    }
                    break;
            }
            break;
    }

    instr_count++;

    return cycles;
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

    /* Determine if current song is PAL or NTSC */
    player->is_pal = (speed & (1 << player->current_song)) != 0;

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
    memset(&player->cpu, 0, sizeof(CPU6502));
    player->cpu.sp = 0xFF;
    player->cpu.p = 0x04;  /* IRQ disable */

    /* Call init routine */
    /* Setup for JSR to init routine */
    player->cpu.pc = player->init_address;
    player->cpu.a = player->current_song;

    /* Execute init routine (with cycle limit) */
    int total_cycles = 0;
    /* Push fake return address */
    cpu_push(player, 0xFF);
    cpu_push(player, 0xFF);

    /* Run until RTS or cycle limit */
    while (total_cycles < MAX_CYCLES_PER_FRAME) {
        uint16_t pc_before = player->cpu.pc;
        int cycles = cpu_step(player);
        total_cycles += cycles;

        /* Check if returned (RTS to 0xFFFF will wrap) */
        if (player->cpu.pc == 0x0000 || player->cpu.pc == 0xFFFF) {
            break;
        }

        /* Detect infinite loops */
        if (player->cpu.pc == pc_before) {
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
        player->frame_counter += 1.0;

        /* Call play routine at frame rate */
        if (player->frame_counter >= samples_per_frame) {
            player->frame_counter -= samples_per_frame;

            /* Setup CPU for play routine call */
            player->cpu.pc = player->play_address;

            /* Push fake return address */
            uint8_t saved_sp = player->cpu.sp;
            cpu_push(player, 0xFF);
            cpu_push(player, 0xFF);

            /* Execute play routine */
            int total_cycles = 0;
            while (total_cycles < MAX_CYCLES_PER_FRAME) {
                int cycles = cpu_step(player);
                total_cycles += cycles;

                /* Check if returned */
                if (player->cpu.pc == 0x0000 || player->cpu.pc == 0xFFFF) {
                    break;
                }

                /* Restore SP if it looks stuck */
                if (player->cpu.sp < saved_sp - 10) {
                    player->cpu.sp = saved_sp;
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
