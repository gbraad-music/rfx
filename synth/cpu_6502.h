/*
 * MOS 6502 CPU Emulator
 * Minimal implementation for music player use (SID, NSF, etc.)
 *
 * Copyright (C) 2024
 * SPDX-License-Identifier: ISC
 */

#ifndef CPU_6502_H
#define CPU_6502_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

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

/* Memory access callbacks */
typedef uint8_t (*cpu_read_func)(void* userdata, uint16_t addr);
typedef void (*cpu_write_func)(void* userdata, uint16_t addr, uint8_t value);

/* CPU context with memory callbacks */
typedef struct {
    CPU6502 cpu;
    void* userdata;
    cpu_read_func read;
    cpu_write_func write;
} CPU6502Context;

// ============================================================================
// Lifecycle
// ============================================================================

/**
 * Initialize CPU context
 * @param ctx CPU context
 * @param userdata User data passed to memory callbacks
 * @param read_func Memory read callback
 * @param write_func Memory write callback
 */
void cpu_init(CPU6502Context* ctx, void* userdata,
              cpu_read_func read_func, cpu_write_func write_func);

/**
 * Reset CPU to power-on state
 * @param ctx CPU context
 */
void cpu_reset(CPU6502Context* ctx);

// ============================================================================
// Execution
// ============================================================================

/**
 * Execute one instruction
 * @param ctx CPU context
 * @return Number of cycles consumed
 */
int cpu_step(CPU6502Context* ctx);

// ============================================================================
// Stack Operations
// ============================================================================

/**
 * Push a byte onto the stack
 * @param ctx CPU context
 * @param value Value to push
 */
void cpu_push(CPU6502Context* ctx, uint8_t value);

/**
 * Pull a byte from the stack
 * @param ctx CPU context
 * @return Pulled value
 */
uint8_t cpu_pull(CPU6502Context* ctx);

#ifdef __cplusplus
}
#endif

#endif // CPU_6502_H
