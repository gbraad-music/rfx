/*
 * MOS 6502 CPU Emulator
 * Minimal implementation for music player use
 */

#include "cpu_6502.h"
#include <string.h>
#include <stdio.h>

// ============================================================================
// Helper Functions
// ============================================================================

static void cpu_set_nz(CPU6502* cpu, uint8_t value) {
    cpu->flag_z = (value == 0);
    cpu->flag_n = (value & 0x80) != 0;
}

// ============================================================================
// Lifecycle
// ============================================================================

void cpu_init(CPU6502Context* ctx, void* userdata,
              cpu_read_func read_func, cpu_write_func write_func) {
    memset(ctx, 0, sizeof(CPU6502Context));
    ctx->userdata = userdata;
    ctx->read = read_func;
    ctx->write = write_func;

    /* Initialize CPU state */
    ctx->cpu.sp = 0xFF;
    ctx->cpu.p = 0x04;  /* IRQ disable */
}

void cpu_reset(CPU6502Context* ctx) {
    memset(&ctx->cpu, 0, sizeof(CPU6502));
    ctx->cpu.sp = 0xFF;
    ctx->cpu.p = 0x04;  /* IRQ disable */
}

// ============================================================================
// Stack Operations
// ============================================================================

void cpu_push(CPU6502Context* ctx, uint8_t value) {
    ctx->write(ctx->userdata, 0x0100 + ctx->cpu.sp, value);
    ctx->cpu.sp--;
}

uint8_t cpu_pull(CPU6502Context* ctx) {
    ctx->cpu.sp++;
    return ctx->read(ctx->userdata, 0x0100 + ctx->cpu.sp);
}

// ============================================================================
// Instruction Execution
// ============================================================================

int cpu_step(CPU6502Context* ctx) {
    CPU6502* cpu = &ctx->cpu;
    uint16_t pc_start = cpu->pc;
    uint8_t opcode = ctx->read(ctx->userdata, cpu->pc++);
    int cycles = 2;  /* Most instructions are 2+ cycles */

    static int unknown_count = 0;
    static int instr_count = 0;
    bool is_unknown = false;

    switch (opcode) {
        /* ADC - Add with Carry */
        case 0x65: /* Zero Page */ {
            uint8_t value = ctx->read(ctx->userdata, ctx->read(ctx->userdata, cpu->pc++));
            uint16_t result = cpu->a + value + cpu->flag_c;
            cpu->flag_c = result > 0xFF;
            cpu->flag_v = ((cpu->a ^ result) & (value ^ result) & 0x80) != 0;
            cpu->a = result & 0xFF;
            cpu_set_nz(cpu, cpu->a);
            cycles = 3;
            break;
        }
        case 0x69: { /* Immediate */
            uint8_t value = ctx->read(ctx->userdata, cpu->pc++);
            uint16_t result = cpu->a + value + cpu->flag_c;
            cpu->flag_c = result > 0xFF;
            cpu->flag_v = ((cpu->a ^ result) & (value ^ result) & 0x80) != 0;
            cpu->a = result & 0xFF;
            cpu_set_nz(cpu, cpu->a);
            break;
        }
        case 0x6D: { /* Absolute */
            uint16_t addr = ctx->read(ctx->userdata, cpu->pc++) | (ctx->read(ctx->userdata, cpu->pc++) << 8);
            uint8_t value = ctx->read(ctx->userdata, addr);
            uint16_t result = cpu->a + value + cpu->flag_c;
            cpu->flag_c = result > 0xFF;
            cpu->flag_v = ((cpu->a ^ result) & (value ^ result) & 0x80) != 0;
            cpu->a = result & 0xFF;
            cpu_set_nz(cpu, cpu->a);
            cycles = 4;
            break;
        }
        case 0x79: { /* Absolute,Y */
            uint16_t base = ctx->read(ctx->userdata, cpu->pc++) | (ctx->read(ctx->userdata, cpu->pc++) << 8);
            uint16_t addr = base + cpu->y;
            uint8_t value = ctx->read(ctx->userdata, addr);
            uint16_t result = cpu->a + value + cpu->flag_c;
            cpu->flag_c = result > 0xFF;
            cpu->flag_v = ((cpu->a ^ result) & (value ^ result) & 0x80) != 0;
            cpu->a = result & 0xFF;
            cpu_set_nz(cpu, cpu->a);
            cycles = 4;
            break;
        }
        case 0x7D: { /* Absolute,X */
            uint16_t base = ctx->read(ctx->userdata, cpu->pc++) | (ctx->read(ctx->userdata, cpu->pc++) << 8);
            uint16_t addr = base + cpu->x;
            uint8_t value = ctx->read(ctx->userdata, addr);
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
            cpu->a &= ctx->read(ctx->userdata, ctx->read(ctx->userdata, cpu->pc++));
            cpu_set_nz(cpu, cpu->a);
            cycles = 3;
            break;
        case 0x29: { /* Immediate */
            uint8_t value = ctx->read(ctx->userdata, cpu->pc++);
            cpu->a &= value;
            cpu_set_nz(cpu, cpu->a);
            break;
        }
        case 0x2D: { /* Absolute */
            uint16_t addr = ctx->read(ctx->userdata, cpu->pc++) | (ctx->read(ctx->userdata, cpu->pc++) << 8);
            cpu->a &= ctx->read(ctx->userdata, addr);
            cpu_set_nz(cpu, cpu->a);
            cycles = 4;
            break;
        }
        case 0x39: { /* Absolute,Y */
            uint16_t base = ctx->read(ctx->userdata, cpu->pc++) | (ctx->read(ctx->userdata, cpu->pc++) << 8);
            uint16_t addr = base + cpu->y;
            cpu->a &= ctx->read(ctx->userdata, addr);
            cpu_set_nz(cpu, cpu->a);
            cycles = 4;
            break;
        }
        case 0x3D: { /* Absolute,X */
            uint16_t base = ctx->read(ctx->userdata, cpu->pc++) | (ctx->read(ctx->userdata, cpu->pc++) << 8);
            uint16_t addr = base + cpu->x;
            cpu->a &= ctx->read(ctx->userdata, addr);
            cpu_set_nz(cpu, cpu->a);
            cycles = 4;
            break;
        }

        /* BCC - Branch if Carry Clear */
        case 0x90: {
            int8_t offset = (int8_t)ctx->read(ctx->userdata, cpu->pc++);
            if (!cpu->flag_c) {
                cpu->pc += offset;
                cycles = 3;
            }
            break;
        }

        /* BCS - Branch if Carry Set */
        case 0xB0: {
            int8_t offset = (int8_t)ctx->read(ctx->userdata, cpu->pc++);
            if (cpu->flag_c) {
                cpu->pc += offset;
                cycles = 3;
            }
            break;
        }

        /* BEQ - Branch if Equal (Z set) */
        case 0xF0: {
            int8_t offset = (int8_t)ctx->read(ctx->userdata, cpu->pc++);
            if (cpu->flag_z) {
                cpu->pc += offset;
                cycles = 3;
            }
            break;
        }

        /* BIT - Test Bits */
        case 0x24: /* Zero Page */ {
            uint8_t value = ctx->read(ctx->userdata, ctx->read(ctx->userdata, cpu->pc++));
            cpu->flag_z = (cpu->a & value) == 0;
            cpu->flag_n = (value & 0x80) != 0;
            cpu->flag_v = (value & 0x40) != 0;
            cycles = 3;
            break;
        }
        case 0x2C: { /* Absolute */
            uint16_t addr = ctx->read(ctx->userdata, cpu->pc++) | (ctx->read(ctx->userdata, cpu->pc++) << 8);
            uint8_t value = ctx->read(ctx->userdata, addr);
            cpu->flag_z = (cpu->a & value) == 0;
            cpu->flag_n = (value & 0x80) != 0;
            cpu->flag_v = (value & 0x40) != 0;
            cycles = 4;
            break;
        }

        /* BMI - Branch if Minus (N set) */
        case 0x30: {
            int8_t offset = (int8_t)ctx->read(ctx->userdata, cpu->pc++);
            if (cpu->flag_n) {
                cpu->pc += offset;
                cycles = 3;
            }
            break;
        }

        /* BNE - Branch if Not Equal (Z clear) */
        case 0xD0: {
            int8_t offset = (int8_t)ctx->read(ctx->userdata, cpu->pc++);
            if (!cpu->flag_z) {
                cpu->pc += offset;
                cycles = 3;
            }
            break;
        }

        /* BPL - Branch if Plus (N clear) */
        case 0x10: {
            int8_t offset = (int8_t)ctx->read(ctx->userdata, cpu->pc++);
            if (!cpu->flag_n) {
                cpu->pc += offset;
                cycles = 3;
            }
            break;
        }

        /* BVC - Branch if Overflow Clear */
        case 0x50: {
            int8_t offset = (int8_t)ctx->read(ctx->userdata, cpu->pc++);
            if (!cpu->flag_v) {
                cpu->pc += offset;
                cycles = 3;
            }
            break;
        }

        /* BVS - Branch if Overflow Set */
        case 0x70: {
            int8_t offset = (int8_t)ctx->read(ctx->userdata, cpu->pc++);
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
            uint8_t zp = ctx->read(ctx->userdata, cpu->pc++);
            uint8_t value = ctx->read(ctx->userdata, zp);
            cpu->flag_c = (value & 0x80) != 0;
            value <<= 1;
            ctx->write(ctx->userdata, zp, value);
            cpu_set_nz(cpu, value);
            cycles = 5;
            break;
        }
        case 0x0E: { /* Absolute */
            uint16_t addr = ctx->read(ctx->userdata, cpu->pc++) | (ctx->read(ctx->userdata, cpu->pc++) << 8);
            uint8_t value = ctx->read(ctx->userdata, addr);
            cpu->flag_c = (value & 0x80) != 0;
            value <<= 1;
            ctx->write(ctx->userdata, addr, value);
            cpu_set_nz(cpu, value);
            cycles = 6;
            break;
        }
        case 0x1E: { /* Absolute,X */
            uint16_t base = ctx->read(ctx->userdata, cpu->pc++) | (ctx->read(ctx->userdata, cpu->pc++) << 8);
            uint16_t addr = base + cpu->x;
            uint8_t value = ctx->read(ctx->userdata, addr);
            cpu->flag_c = (value & 0x80) != 0;
            value <<= 1;
            ctx->write(ctx->userdata, addr, value);
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
            uint8_t value = ctx->read(ctx->userdata, ctx->read(ctx->userdata, cpu->pc++));
            uint16_t result = cpu->a - value;
            cpu->flag_c = cpu->a >= value;
            cpu_set_nz(cpu, result & 0xFF);
            cycles = 3;
            break;
        }
        case 0xC9: { /* Immediate */
            uint8_t value = ctx->read(ctx->userdata, cpu->pc++);
            uint16_t result = cpu->a - value;
            cpu->flag_c = cpu->a >= value;
            cpu_set_nz(cpu, result & 0xFF);
            break;
        }
        case 0xCD: { /* Absolute */
            uint16_t addr = ctx->read(ctx->userdata, cpu->pc++) | (ctx->read(ctx->userdata, cpu->pc++) << 8);
            uint8_t value = ctx->read(ctx->userdata, addr);
            uint16_t result = cpu->a - value;
            cpu->flag_c = cpu->a >= value;
            cpu_set_nz(cpu, result & 0xFF);
            cycles = 4;
            break;
        }
        case 0xD9: { /* Absolute,Y */
            uint16_t base = ctx->read(ctx->userdata, cpu->pc++) | (ctx->read(ctx->userdata, cpu->pc++) << 8);
            uint16_t addr = base + cpu->y;
            uint8_t value = ctx->read(ctx->userdata, addr);
            uint16_t result = cpu->a - value;
            cpu->flag_c = cpu->a >= value;
            cpu_set_nz(cpu, result & 0xFF);
            cycles = 4;
            break;
        }
        case 0xDD: { /* Absolute,X */
            uint16_t base = ctx->read(ctx->userdata, cpu->pc++) | (ctx->read(ctx->userdata, cpu->pc++) << 8);
            uint16_t addr = base + cpu->x;
            uint8_t value = ctx->read(ctx->userdata, addr);
            uint16_t result = cpu->a - value;
            cpu->flag_c = cpu->a >= value;
            cpu_set_nz(cpu, result & 0xFF);
            cycles = 4;
            break;
        }

        /* CPX - Compare X */
        case 0xE0: { /* Immediate */
            uint8_t value = ctx->read(ctx->userdata, cpu->pc++);
            uint16_t result = cpu->x - value;
            cpu->flag_c = cpu->x >= value;
            cpu_set_nz(cpu, result & 0xFF);
            break;
        }
        case 0xEC: { /* Absolute */
            uint16_t addr = ctx->read(ctx->userdata, cpu->pc++) | (ctx->read(ctx->userdata, cpu->pc++) << 8);
            uint8_t value = ctx->read(ctx->userdata, addr);
            uint16_t result = cpu->x - value;
            cpu->flag_c = cpu->x >= value;
            cpu_set_nz(cpu, result & 0xFF);
            cycles = 4;
            break;
        }

        /* CPY - Compare Y */
        case 0xC0: { /* Immediate */
            uint8_t value = ctx->read(ctx->userdata, cpu->pc++);
            uint16_t result = cpu->y - value;
            cpu->flag_c = cpu->y >= value;
            cpu_set_nz(cpu, result & 0xFF);
            break;
        }
        case 0xCC: { /* Absolute */
            uint16_t addr = ctx->read(ctx->userdata, cpu->pc++) | (ctx->read(ctx->userdata, cpu->pc++) << 8);
            uint8_t value = ctx->read(ctx->userdata, addr);
            uint16_t result = cpu->y - value;
            cpu->flag_c = cpu->y >= value;
            cpu_set_nz(cpu, result & 0xFF);
            cycles = 4;
            break;
        }

        /* DEC - Decrement Memory */
        case 0xC6: /* Zero Page */ {
            uint8_t zp = ctx->read(ctx->userdata, cpu->pc++);
            uint8_t value = ctx->read(ctx->userdata, zp) - 1;
            ctx->write(ctx->userdata, zp, value);
            cpu_set_nz(cpu, value);
            cycles = 5;
            break;
        }
        case 0xCE: { /* Absolute */
            uint16_t addr = ctx->read(ctx->userdata, cpu->pc++) | (ctx->read(ctx->userdata, cpu->pc++) << 8);
            uint8_t value = ctx->read(ctx->userdata, addr) - 1;
            ctx->write(ctx->userdata, addr, value);
            cpu_set_nz(cpu, value);
            cycles = 6;
            break;
        }
        case 0xDE: { /* Absolute,X */
            uint16_t base = ctx->read(ctx->userdata, cpu->pc++) | (ctx->read(ctx->userdata, cpu->pc++) << 8);
            uint16_t addr = base + cpu->x;
            uint8_t value = ctx->read(ctx->userdata, addr) - 1;
            ctx->write(ctx->userdata, addr, value);
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
            uint8_t zp = ctx->read(ctx->userdata, cpu->pc++);
            uint8_t value = ctx->read(ctx->userdata, zp) + 1;
            ctx->write(ctx->userdata, zp, value);
            cpu_set_nz(cpu, value);
            cycles = 5;
            break;
        }
        case 0xEE: { /* Absolute */
            uint16_t addr = ctx->read(ctx->userdata, cpu->pc++) | (ctx->read(ctx->userdata, cpu->pc++) << 8);
            uint8_t value = ctx->read(ctx->userdata, addr) + 1;
            ctx->write(ctx->userdata, addr, value);
            cpu_set_nz(cpu, value);
            cycles = 6;
            break;
        }
        case 0xFE: { /* Absolute,X */
            uint16_t base = ctx->read(ctx->userdata, cpu->pc++) | (ctx->read(ctx->userdata, cpu->pc++) << 8);
            uint16_t addr = base + cpu->x;
            uint8_t value = ctx->read(ctx->userdata, addr) + 1;
            ctx->write(ctx->userdata, addr, value);
            cpu_set_nz(cpu, value);
            cycles = 7;
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
            uint16_t addr = ctx->read(ctx->userdata, cpu->pc++) | (ctx->read(ctx->userdata, cpu->pc++) << 8);
            cpu->pc = addr;
            cycles = 3;
            break;
        }
        case 0x6C: { /* Indirect */
            uint16_t ptr = ctx->read(ctx->userdata, cpu->pc++) | (ctx->read(ctx->userdata, cpu->pc++) << 8);
            /* Handle 6502 page boundary bug */
            uint16_t addr;
            if ((ptr & 0xFF) == 0xFF) {
                addr = ctx->read(ctx->userdata, ptr) | (ctx->read(ctx->userdata, ptr & 0xFF00) << 8);
            } else {
                addr = ctx->read(ctx->userdata, ptr) | (ctx->read(ctx->userdata, ptr + 1) << 8);
            }
            cpu->pc = addr;
            cycles = 5;
            break;
        }

        /* JSR - Jump to Subroutine */
        case 0x20: {
            uint16_t addr = ctx->read(ctx->userdata, cpu->pc++) | (ctx->read(ctx->userdata, cpu->pc++) << 8);
            uint16_t ret_addr = cpu->pc - 1;
            cpu_push(ctx, (ret_addr >> 8) & 0xFF);
            cpu_push(ctx, ret_addr & 0xFF);
            cpu->pc = addr;
            cycles = 6;
            break;
        }

        /* LDA - Load Accumulator */
        case 0xA5: /* Zero Page */
            cpu->a = ctx->read(ctx->userdata, ctx->read(ctx->userdata, cpu->pc++));
            cpu_set_nz(cpu, cpu->a);
            cycles = 3;
            break;
        case 0xA9: /* Immediate */
            cpu->a = ctx->read(ctx->userdata, cpu->pc++);
            cpu_set_nz(cpu, cpu->a);
            break;
        case 0xAD: { /* Absolute */
            uint16_t addr = ctx->read(ctx->userdata, cpu->pc++) | (ctx->read(ctx->userdata, cpu->pc++) << 8);
            cpu->a = ctx->read(ctx->userdata, addr);
            cpu_set_nz(cpu, cpu->a);
            cycles = 4;
            break;
        }
        case 0xB9: { /* Absolute,Y */
            uint16_t base = ctx->read(ctx->userdata, cpu->pc++) | (ctx->read(ctx->userdata, cpu->pc++) << 8);
            uint16_t addr = base + cpu->y;
            cpu->a = ctx->read(ctx->userdata, addr);
            cpu_set_nz(cpu, cpu->a);
            cycles = 4;
            break;
        }
        case 0xB1: { /* (Indirect),Y */
            uint8_t zp = ctx->read(ctx->userdata, cpu->pc++);
            uint16_t base = ctx->read(ctx->userdata, zp) | (ctx->read(ctx->userdata, (zp + 1) & 0xFF) << 8);
            uint16_t addr = base + cpu->y;
            cpu->a = ctx->read(ctx->userdata, addr);
            cpu_set_nz(cpu, cpu->a);
            cycles = 5;
            break;
        }
        case 0xBD: { /* Absolute,X */
            uint16_t base = ctx->read(ctx->userdata, cpu->pc++) | (ctx->read(ctx->userdata, cpu->pc++) << 8);
            uint16_t addr = base + cpu->x;
            cpu->a = ctx->read(ctx->userdata, addr);
            cpu_set_nz(cpu, cpu->a);
            cycles = 4;
            break;
        }

        /* LDX - Load X */
        case 0xA2: /* Immediate */
            cpu->x = ctx->read(ctx->userdata, cpu->pc++);
            cpu_set_nz(cpu, cpu->x);
            break;
        case 0xA6: /* Zero Page */
            cpu->x = ctx->read(ctx->userdata, ctx->read(ctx->userdata, cpu->pc++));
            cpu_set_nz(cpu, cpu->x);
            cycles = 3;
            break;
        case 0xAE: { /* Absolute */
            uint16_t addr = ctx->read(ctx->userdata, cpu->pc++) | (ctx->read(ctx->userdata, cpu->pc++) << 8);
            cpu->x = ctx->read(ctx->userdata, addr);
            cpu_set_nz(cpu, cpu->x);
            cycles = 4;
            break;
        }
        case 0xBE: { /* Absolute,Y */
            uint16_t base = ctx->read(ctx->userdata, cpu->pc++) | (ctx->read(ctx->userdata, cpu->pc++) << 8);
            uint16_t addr = base + cpu->y;
            cpu->x = ctx->read(ctx->userdata, addr);
            cpu_set_nz(cpu, cpu->x);
            cycles = 4;
            break;
        }

        /* LDY - Load Y */
        case 0xA0: /* Immediate */
            cpu->y = ctx->read(ctx->userdata, cpu->pc++);
            cpu_set_nz(cpu, cpu->y);
            break;
        case 0xA4: /* Zero Page */
            cpu->y = ctx->read(ctx->userdata, ctx->read(ctx->userdata, cpu->pc++));
            cpu_set_nz(cpu, cpu->y);
            cycles = 3;
            break;
        case 0xAC: { /* Absolute */
            uint16_t addr = ctx->read(ctx->userdata, cpu->pc++) | (ctx->read(ctx->userdata, cpu->pc++) << 8);
            cpu->y = ctx->read(ctx->userdata, addr);
            cpu_set_nz(cpu, cpu->y);
            cycles = 4;
            break;
        }
        case 0xBC: { /* Absolute,X */
            uint16_t base = ctx->read(ctx->userdata, cpu->pc++) | (ctx->read(ctx->userdata, cpu->pc++) << 8);
            uint16_t addr = base + cpu->x;
            cpu->y = ctx->read(ctx->userdata, addr);
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
            cpu->a |= ctx->read(ctx->userdata, ctx->read(ctx->userdata, cpu->pc++));
            cpu_set_nz(cpu, cpu->a);
            cycles = 3;
            break;
        case 0x09: { /* Immediate */
            uint8_t value = ctx->read(ctx->userdata, cpu->pc++);
            cpu->a |= value;
            cpu_set_nz(cpu, cpu->a);
            break;
        }
        case 0x0D: { /* Absolute */
            uint16_t addr = ctx->read(ctx->userdata, cpu->pc++) | (ctx->read(ctx->userdata, cpu->pc++) << 8);
            cpu->a |= ctx->read(ctx->userdata, addr);
            cpu_set_nz(cpu, cpu->a);
            cycles = 4;
            break;
        }
        case 0x11: { /* (Indirect),Y */
            uint8_t zp = ctx->read(ctx->userdata, cpu->pc++);
            uint16_t base = ctx->read(ctx->userdata, zp) | (ctx->read(ctx->userdata, (zp + 1) & 0xFF) << 8);
            uint16_t addr = base + cpu->y;
            cpu->a |= ctx->read(ctx->userdata, addr);
            cpu_set_nz(cpu, cpu->a);
            cycles = 5;
            break;
        }
        case 0x19: { /* Absolute,Y */
            uint16_t base = ctx->read(ctx->userdata, cpu->pc++) | (ctx->read(ctx->userdata, cpu->pc++) << 8);
            uint16_t addr = base + cpu->y;
            cpu->a |= ctx->read(ctx->userdata, addr);
            cpu_set_nz(cpu, cpu->a);
            cycles = 4;
            break;
        }

        /* PHA - Push Accumulator */
        case 0x48:
            cpu_push(ctx, cpu->a);
            cycles = 3;
            break;

        /* PLA - Pull Accumulator */
        case 0x68:
            cpu->a = cpu_pull(ctx);
            cpu_set_nz(cpu, cpu->a);
            cycles = 4;
            break;

        /* RTI - Return from Interrupt */
        case 0x40:
            cpu->p = cpu_pull(ctx);
            cpu->pc = cpu_pull(ctx) | (cpu_pull(ctx) << 8);
            cycles = 6;
            break;

        /* ROR - Rotate Right */
        case 0x6A: { /* Accumulator */
            uint8_t old_carry = cpu->flag_c;
            cpu->flag_c = cpu->a & 0x01;
            cpu->a = (cpu->a >> 1) | (old_carry << 7);
            cpu_set_nz(cpu, cpu->a);
            break;
        }
        case 0x66: { /* Zero Page */
            uint8_t zp = ctx->read(ctx->userdata, cpu->pc++);
            uint8_t value = ctx->read(ctx->userdata, zp);
            uint8_t old_carry = cpu->flag_c;
            cpu->flag_c = value & 0x01;
            value = (value >> 1) | (old_carry << 7);
            ctx->write(ctx->userdata, zp, value);
            cpu_set_nz(cpu, value);
            cycles = 5;
            break;
        }
        case 0x6E: { /* Absolute */
            uint16_t addr = ctx->read(ctx->userdata, cpu->pc++) | (ctx->read(ctx->userdata, cpu->pc++) << 8);
            uint8_t value = ctx->read(ctx->userdata, addr);
            uint8_t old_carry = cpu->flag_c;
            cpu->flag_c = value & 0x01;
            value = (value >> 1) | (old_carry << 7);
            ctx->write(ctx->userdata, addr, value);
            cpu_set_nz(cpu, value);
            cycles = 6;
            break;
        }
        case 0x7E: { /* Absolute,X */
            uint16_t base = ctx->read(ctx->userdata, cpu->pc++) | (ctx->read(ctx->userdata, cpu->pc++) << 8);
            uint16_t addr = base + cpu->x;
            uint8_t value = ctx->read(ctx->userdata, addr);
            uint8_t old_carry = cpu->flag_c;
            cpu->flag_c = value & 0x01;
            value = (value >> 1) | (old_carry << 7);
            ctx->write(ctx->userdata, addr, value);
            cpu_set_nz(cpu, value);
            cycles = 7;
            break;
        }

        /* ROL - Rotate Left */
        case 0x2A: { /* Accumulator */
            uint8_t old_carry = cpu->flag_c;
            cpu->flag_c = (cpu->a & 0x80) != 0;
            cpu->a = (cpu->a << 1) | old_carry;
            cpu_set_nz(cpu, cpu->a);
            break;
        }
        case 0x26: { /* Zero Page */
            uint8_t zp = ctx->read(ctx->userdata, cpu->pc++);
            uint8_t value = ctx->read(ctx->userdata, zp);
            uint8_t old_carry = cpu->flag_c;
            cpu->flag_c = (value & 0x80) != 0;
            value = (value << 1) | old_carry;
            ctx->write(ctx->userdata, zp, value);
            cpu_set_nz(cpu, value);
            cycles = 5;
            break;
        }
        case 0x2E: { /* Absolute */
            uint16_t addr = ctx->read(ctx->userdata, cpu->pc++) | (ctx->read(ctx->userdata, cpu->pc++) << 8);
            uint8_t value = ctx->read(ctx->userdata, addr);
            uint8_t old_carry = cpu->flag_c;
            cpu->flag_c = (value & 0x80) != 0;
            value = (value << 1) | old_carry;
            ctx->write(ctx->userdata, addr, value);
            cpu_set_nz(cpu, value);
            cycles = 6;
            break;
        }
        case 0x36: { /* Zero Page,X */
            uint8_t zp = (ctx->read(ctx->userdata, cpu->pc++) + cpu->x) & 0xFF;
            uint8_t value = ctx->read(ctx->userdata, zp);
            uint8_t old_carry = cpu->flag_c;
            cpu->flag_c = (value & 0x80) != 0;
            value = (value << 1) | old_carry;
            ctx->write(ctx->userdata, zp, value);
            cpu_set_nz(cpu, value);
            cycles = 6;
            break;
        }
        case 0x3E: { /* Absolute,X */
            uint16_t base = ctx->read(ctx->userdata, cpu->pc++) | (ctx->read(ctx->userdata, cpu->pc++) << 8);
            uint16_t addr = base + cpu->x;
            uint8_t value = ctx->read(ctx->userdata, addr);
            uint8_t old_carry = cpu->flag_c;
            cpu->flag_c = (value & 0x80) != 0;
            value = (value << 1) | old_carry;
            ctx->write(ctx->userdata, addr, value);
            cpu_set_nz(cpu, value);
            cycles = 7;
            break;
        }

        /* RTS - Return from Subroutine */
        case 0x60:
            cpu->pc = (cpu_pull(ctx) | (cpu_pull(ctx) << 8)) + 1;
            cycles = 6;
            break;

        /* SBC - Subtract with Carry */
        case 0xE5: { /* Zero Page */
            uint8_t value = ctx->read(ctx->userdata, ctx->read(ctx->userdata, cpu->pc++));
            uint16_t result = cpu->a - value - (1 - cpu->flag_c);
            cpu->flag_c = result < 0x100;
            cpu->flag_v = ((cpu->a ^ value) & (cpu->a ^ result) & 0x80) != 0;
            cpu->a = result & 0xFF;
            cpu_set_nz(cpu, cpu->a);
            cycles = 3;
            break;
        }
        case 0xE9: { /* Immediate */
            uint8_t value = ctx->read(ctx->userdata, cpu->pc++);
            uint16_t result = cpu->a - value - (1 - cpu->flag_c);
            cpu->flag_c = result < 0x100;
            cpu->flag_v = ((cpu->a ^ value) & (cpu->a ^ result) & 0x80) != 0;
            cpu->a = result & 0xFF;
            cpu_set_nz(cpu, cpu->a);
            break;
        }
        case 0xED: { /* Absolute */
            uint16_t addr = ctx->read(ctx->userdata, cpu->pc++) | (ctx->read(ctx->userdata, cpu->pc++) << 8);
            uint8_t value = ctx->read(ctx->userdata, addr);
            uint16_t result = cpu->a - value - (1 - cpu->flag_c);
            cpu->flag_c = result < 0x100;
            cpu->flag_v = ((cpu->a ^ value) & (cpu->a ^ result) & 0x80) != 0;
            cpu->a = result & 0xFF;
            cpu_set_nz(cpu, cpu->a);
            cycles = 4;
            break;
        }
        case 0xF9: { /* Absolute,Y */
            uint16_t base = ctx->read(ctx->userdata, cpu->pc++) | (ctx->read(ctx->userdata, cpu->pc++) << 8);
            uint16_t addr = base + cpu->y;
            uint8_t value = ctx->read(ctx->userdata, addr);
            uint16_t result = cpu->a - value - (1 - cpu->flag_c);
            cpu->flag_c = result < 0x100;
            cpu->flag_v = ((cpu->a ^ value) & (cpu->a ^ result) & 0x80) != 0;
            cpu->a = result & 0xFF;
            cpu_set_nz(cpu, cpu->a);
            cycles = 4;
            break;
        }
        case 0xF1: { /* (Indirect),Y */
            uint8_t zp = ctx->read(ctx->userdata, cpu->pc++);
            uint16_t base = ctx->read(ctx->userdata, zp) | (ctx->read(ctx->userdata, (zp + 1) & 0xFF) << 8);
            uint16_t addr = base + cpu->y;
            uint8_t value = ctx->read(ctx->userdata, addr);
            uint16_t result = cpu->a - value - (1 - cpu->flag_c);
            cpu->flag_c = result < 0x100;
            cpu->flag_v = ((cpu->a ^ value) & (cpu->a ^ result) & 0x80) != 0;
            cpu->a = result & 0xFF;
            cpu_set_nz(cpu, cpu->a);
            cycles = 5;
            break;
        }
        case 0xFD: { /* Absolute,X */
            uint16_t base = ctx->read(ctx->userdata, cpu->pc++) | (ctx->read(ctx->userdata, cpu->pc++) << 8);
            uint16_t addr = base + cpu->x;
            uint8_t value = ctx->read(ctx->userdata, addr);
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
            ctx->write(ctx->userdata, ctx->read(ctx->userdata, cpu->pc++), cpu->a);
            cycles = 3;
            break;
        case 0x95: /* Zero Page,X */
            ctx->write(ctx->userdata, (ctx->read(ctx->userdata, cpu->pc++) + cpu->x) & 0xFF, cpu->a);
            cycles = 4;
            break;
        case 0x8D: { /* Absolute */
            uint16_t addr = ctx->read(ctx->userdata, cpu->pc++) | (ctx->read(ctx->userdata, cpu->pc++) << 8);
            ctx->write(ctx->userdata, addr, cpu->a);
            cycles = 4;
            break;
        }
        case 0x91: { /* (Indirect),Y */
            uint8_t zp = ctx->read(ctx->userdata, cpu->pc++);
            uint16_t base = ctx->read(ctx->userdata, zp) | (ctx->read(ctx->userdata, (zp + 1) & 0xFF) << 8);
            uint16_t addr = base + cpu->y;
            ctx->write(ctx->userdata, addr, cpu->a);
            cycles = 6;
            break;
        }
        case 0x99: { /* Absolute,Y */
            uint16_t base = ctx->read(ctx->userdata, cpu->pc++) | (ctx->read(ctx->userdata, cpu->pc++) << 8);
            uint16_t addr = base + cpu->y;
            ctx->write(ctx->userdata, addr, cpu->a);
            cycles = 5;
            break;
        }
        case 0x9D: { /* Absolute,X */
            uint16_t base = ctx->read(ctx->userdata, cpu->pc++) | (ctx->read(ctx->userdata, cpu->pc++) << 8);
            uint16_t addr = base + cpu->x;
            ctx->write(ctx->userdata, addr, cpu->a);
            cycles = 5;
            break;
        }

        /* STX - Store X */
        case 0x86: /* Zero Page */
            ctx->write(ctx->userdata, ctx->read(ctx->userdata, cpu->pc++), cpu->x);
            cycles = 3;
            break;
        case 0x8E: { /* Absolute */
            uint16_t addr = ctx->read(ctx->userdata, cpu->pc++) | (ctx->read(ctx->userdata, cpu->pc++) << 8);
            ctx->write(ctx->userdata, addr, cpu->x);
            cycles = 4;
            break;
        }

        /* STY - Store Y */
        case 0x84: /* Zero Page */
            ctx->write(ctx->userdata, ctx->read(ctx->userdata, cpu->pc++), cpu->y);
            cycles = 3;
            break;
        case 0x8C: { /* Absolute */
            uint16_t addr = ctx->read(ctx->userdata, cpu->pc++) | (ctx->read(ctx->userdata, cpu->pc++) << 8);
            ctx->write(ctx->userdata, addr, cpu->y);
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

        /* EOR - Exclusive OR */
        case 0x49: /* Immediate */
            cpu->a ^= ctx->read(ctx->userdata, cpu->pc++);
            cpu_set_nz(cpu, cpu->a);
            break;

        /* LSR - Logical Shift Right */
        case 0x4E: { /* Absolute */
            uint16_t addr = ctx->read(ctx->userdata, cpu->pc++) | (ctx->read(ctx->userdata, cpu->pc++) << 8);
            uint8_t value = ctx->read(ctx->userdata, addr);
            cpu->flag_c = value & 0x01;
            value >>= 1;
            ctx->write(ctx->userdata, addr, value);
            cpu_set_nz(cpu, value);
            cycles = 6;
            break;
        }
        case 0x5E: { /* Absolute,X */
            uint16_t base = ctx->read(ctx->userdata, cpu->pc++) | (ctx->read(ctx->userdata, cpu->pc++) << 8);
            uint16_t addr = base + cpu->x;
            uint8_t value = ctx->read(ctx->userdata, addr);
            cpu->flag_c = value & 0x01;
            value >>= 1;
            ctx->write(ctx->userdata, addr, value);
            cpu_set_nz(cpu, value);
            cycles = 7;
            break;
        }

        /* Illegal/undocumented opcodes - implement as close approximations */
        /* Many SID files use these! */

        /* SLO (ASL + ORA) - (Indirect,X) */
        case 0x03: {
            uint8_t zp = (ctx->read(ctx->userdata, cpu->pc++) + cpu->x) & 0xFF;
            uint16_t addr = ctx->read(ctx->userdata, zp) | (ctx->read(ctx->userdata, (zp + 1) & 0xFF) << 8);
            uint8_t value = ctx->read(ctx->userdata, addr);
            cpu->flag_c = (value & 0x80) != 0;
            value <<= 1;
            ctx->write(ctx->userdata, addr, value);
            cpu->a |= value;
            cpu_set_nz(cpu, cpu->a);
            cycles = 8;
            break;
        }

        /* RLA (ROL + AND) - Zero Page,X */
        case 0x37: {
            uint8_t zp = (ctx->read(ctx->userdata, cpu->pc++) + cpu->x) & 0xFF;
            uint8_t value = ctx->read(ctx->userdata, zp);
            uint8_t old_carry = cpu->flag_c;
            cpu->flag_c = (value & 0x80) != 0;
            value = (value << 1) | old_carry;
            ctx->write(ctx->userdata, zp, value);
            cpu->a &= value;
            cpu_set_nz(cpu, cpu->a);
            cycles = 6;
            break;
        }

        /* SRE (LSR + EOR) - (Indirect),Y */
        case 0x53: {
            uint8_t zp = ctx->read(ctx->userdata, cpu->pc++);
            uint16_t base = ctx->read(ctx->userdata, zp) | (ctx->read(ctx->userdata, (zp + 1) & 0xFF) << 8);
            uint16_t addr = base + cpu->y;
            uint8_t value = ctx->read(ctx->userdata, addr);
            cpu->flag_c = value & 0x01;
            value >>= 1;
            ctx->write(ctx->userdata, addr, value);
            cpu->a ^= value;
            cpu_set_nz(cpu, cpu->a);
            cycles = 8;
            break;
        }

        /* DEC Accumulator (illegal NOP) */
        case 0x3A:
            /* On real 6502, this is a NOP */
            cycles = 2;
            break;

        /* NOP - Zero Page,X (illegal) */
        case 0x74:
            cpu->pc++;  /* Skip zero page operand */
            cycles = 4;
            break;

        /* ISC (INC + SBC) - Absolute,X */
        case 0xFF: {
            uint16_t base = ctx->read(ctx->userdata, cpu->pc++) | (ctx->read(ctx->userdata, cpu->pc++) << 8);
            uint16_t addr = base + cpu->x;
            uint8_t value = ctx->read(ctx->userdata, addr) + 1;
            ctx->write(ctx->userdata, addr, value);
            /* Then SBC */
            uint16_t result = cpu->a - value - (1 - cpu->flag_c);
            cpu->flag_c = result < 0x100;
            cpu->flag_v = ((cpu->a ^ value) & (cpu->a ^ result) & 0x80) != 0;
            cpu->a = result & 0xFF;
            cpu_set_nz(cpu, cpu->a);
            cycles = 7;
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
