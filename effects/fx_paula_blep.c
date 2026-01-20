/*
 * Regroove Paula BLEP Effect
 *
 * Based on OpenMPT Paula emulation (BSD license)
 * Original authors: OpenMPT Devs, Antti S. Lankila
 *
 * BLEP (Band-Limited stEP) synthesis emulates the Amiga Paula DAC,
 * which outputs discrete voltage steps. This creates the characteristic
 * Amiga sound while preventing aliasing artifacts.
 */

#include "fx_paula_blep.h"
#include "fx_paula_blep_tables.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

// Paula constants
#define PAULA_HZ 3546895
#define MINIMUM_INTERVAL 4  // Clock cycles between samples
#define MAX_BLEPS 128       // Maximum simultaneous BLEPs to track

// BLEP state (single step event)
typedef struct {
    int16_t level;  // Amplitude of step
    uint16_t age;   // Age in clock cycles
} Blep;

// Paula BLEP state machine
struct FXPaulaBlep {
    int enabled;
    PaulaMode mode;
    float mix;
    int sample_rate;

    // BLEP table pointer (selected based on mode)
    const int16_t* blep_table;

    // Circular buffer of active BLEPs
    Blep blep_state[MAX_BLEPS];
    uint16_t active_bleps;      // Count of active BLEPs
    uint16_t first_blep;        // Index of oldest BLEP
    int16_t global_output_level; // Current output level

    // Clock timing
    int num_steps;              // Full steps per output sample
    double step_remainder;      // Fractional clock cycles
    double remainder;           // Accumulated remainder
};

// Get BLEP table for mode
static const int16_t* get_blep_table(PaulaMode mode) {
    switch (mode) {
        case PAULA_A500_OFF: return blep_a500_off;
        case PAULA_A500_ON: return blep_a500_on;
        case PAULA_A1200_OFF: return blep_a1200_off;
        case PAULA_A1200_ON: return blep_a1200_on;
        case PAULA_UNFILTERED: return blep_unfiltered;
        default: return blep_unfiltered;
    }
}

FXPaulaBlep* fx_paula_blep_create(void) {
    FXPaulaBlep* fx = (FXPaulaBlep*)calloc(1, sizeof(FXPaulaBlep));
    if (!fx) return NULL;

    fx->enabled = 1;
    fx->mode = PAULA_A500_ON;
    fx->mix = 1.0f;
    fx->sample_rate = 48000;
    fx->blep_table = get_blep_table(fx->mode);

    // Initialize clock timing for 48kHz
    double amiga_clocks_per_sample = (double)PAULA_HZ / 48000.0;
    fx->num_steps = (int)(amiga_clocks_per_sample / MINIMUM_INTERVAL);
    fx->step_remainder = amiga_clocks_per_sample - fx->num_steps * MINIMUM_INTERVAL;

    fx_paula_blep_reset(fx);

    return fx;
}

void fx_paula_blep_destroy(FXPaulaBlep* fx) {
    if (fx) {
        free(fx);
    }
}

void fx_paula_blep_reset(FXPaulaBlep* fx) {
    if (!fx) return;

    fx->remainder = 0.0;
    fx->active_bleps = 0;
    fx->first_blep = MAX_BLEPS / 2;
    fx->global_output_level = 0;
    memset(fx->blep_state, 0, sizeof(fx->blep_state));
}

// Update clock timing when sample rate changes
static void update_timing(FXPaulaBlep* fx) {
    if (!fx) return;

    double amiga_clocks_per_sample = (double)PAULA_HZ / (double)fx->sample_rate;
    fx->num_steps = (int)(amiga_clocks_per_sample / MINIMUM_INTERVAL);
    fx->step_remainder = amiga_clocks_per_sample - fx->num_steps * MINIMUM_INTERVAL;
}

void fx_paula_blep_input_sample(FXPaulaBlep* fx, int16_t sample) {
    if (!fx) return;

    // Detect level change (create new BLEP when output changes)
    if (sample != fx->global_output_level) {
        // Add new BLEP at age 0
        fx->first_blep = (fx->first_blep - 1) % MAX_BLEPS;
        if (fx->active_bleps < MAX_BLEPS) {
            fx->active_bleps++;
        }

        fx->blep_state[fx->first_blep].age = 0;
        fx->blep_state[fx->first_blep].level = sample - fx->global_output_level;
        fx->global_output_level = sample;
    }
}

int fx_paula_blep_output_sample(FXPaulaBlep* fx) {
    if (!fx) return 0;

    // Start with current output level (scaled by 2^17)
    int output = fx->global_output_level * 131072;

    // Accumulate all active BLEPs
    uint32_t last_blep = fx->first_blep + fx->active_bleps;
    for (uint32_t i = fx->first_blep; i != last_blep; i++) {
        const Blep* blep = &fx->blep_state[i % MAX_BLEPS];
        if (blep->age < BLEP_SIZE) {
            output -= fx->blep_table[blep->age] * blep->level;
        }
    }

    // Scale down (compensate for 2^17 scale and 2^2 for bit depth)
    output /= (131072 / 4);

    return output;
}

void fx_paula_blep_clock(FXPaulaBlep* fx, int cycles) {
    if (!fx) return;

    // Age all active BLEPs
    uint32_t last_blep = fx->first_blep + fx->active_bleps;
    for (uint32_t i = fx->first_blep; i != last_blep; i++) {
        Blep* blep = &fx->blep_state[i % MAX_BLEPS];
        blep->age += (uint16_t)cycles;

        // Remove BLEP if it's fully decayed
        if (blep->age >= BLEP_SIZE) {
            fx->active_bleps = (uint16_t)(i - fx->first_blep);
            return;
        }
    }
}

void fx_paula_blep_process_f32(FXPaulaBlep* fx, const float* input, float* output,
                                int input_frames, int output_frames, int sample_rate) {
    if (!fx || !fx->enabled || !input || !output) return;

    // Update timing if sample rate changed
    if (fx->sample_rate != sample_rate) {
        fx->sample_rate = sample_rate;
        update_timing(fx);
    }

    const float i2f = 1.0f / 32768.0f;
    const float f2i = 32768.0f;

    // Process stereo frames
    for (int i = 0; i < output_frames; i++) {
        float dry_l = input[i * 2];
        float dry_r = input[i * 2 + 1];

        // Convert to int16 (Paula uses 8-bit, but we use 16-bit for quality)
        int16_t sample_l = (int16_t)(dry_l * f2i);
        int16_t sample_r = (int16_t)(dry_r * f2i);

        // Input samples (left channel only for now - mono Paula emulation)
        fx_paula_blep_input_sample(fx, sample_l);

        // Clock Paula for appropriate cycles
        int total_cycles = fx->num_steps * MINIMUM_INTERVAL;
        fx->remainder += fx->step_remainder;
        if (fx->remainder >= 1.0) {
            total_cycles += MINIMUM_INTERVAL;
            fx->remainder -= 1.0;
        }
        fx_paula_blep_clock(fx, total_cycles);

        // Get BLEP output
        int blep_out = fx_paula_blep_output_sample(fx);
        float wet_l = (float)blep_out * i2f;
        float wet_r = wet_l;  // Mono output for now

        // Apply mix
        output[i * 2] = dry_l + fx->mix * (wet_l - dry_l);
        output[i * 2 + 1] = dry_r + fx->mix * (wet_r - dry_r);
    }
}

// ============================================================================
// Parameter Interface
// ============================================================================

void fx_paula_blep_set_enabled(FXPaulaBlep* fx, int enabled) {
    if (fx) fx->enabled = enabled;
}

void fx_paula_blep_set_mode(FXPaulaBlep* fx, PaulaMode mode) {
    if (!fx) return;
    if (mode >= 0 && mode < PAULA_NUM_MODES) {
        fx->mode = mode;
        fx->blep_table = get_blep_table(mode);
    }
}

void fx_paula_blep_set_mix(FXPaulaBlep* fx, float mix) {
    if (fx) fx->mix = mix < 0.0f ? 0.0f : (mix > 1.0f ? 1.0f : mix);
}

int fx_paula_blep_get_enabled(FXPaulaBlep* fx) {
    return fx ? fx->enabled : 0;
}

PaulaMode fx_paula_blep_get_mode(FXPaulaBlep* fx) {
    return fx ? fx->mode : PAULA_UNFILTERED;
}

float fx_paula_blep_get_mix(FXPaulaBlep* fx) {
    return fx ? fx->mix : 1.0f;
}

const char* fx_paula_blep_get_mode_name(PaulaMode mode) {
    switch (mode) {
        case PAULA_A500_OFF: return "A500";
        case PAULA_A500_ON: return "A500+LED";
        case PAULA_A1200_OFF: return "A1200";
        case PAULA_A1200_ON: return "A1200+LED";
        case PAULA_UNFILTERED: return "Unfiltered";
        default: return "Unknown";
    }
}

// ============================================================================
// Generic Parameter Interface
// ============================================================================

enum {
    PARAM_ENABLED = 0,
    PARAM_MODE,
    PARAM_MIX,
    PARAM_COUNT
};

int fx_paula_blep_get_parameter_count(void) {
    return PARAM_COUNT;
}

float fx_paula_blep_get_parameter_value(FXPaulaBlep* fx, int index) {
    if (!fx) return 0.0f;

    switch (index) {
        case PARAM_ENABLED: return fx->enabled ? 1.0f : 0.0f;
        case PARAM_MODE: return (float)fx->mode / (float)(PAULA_NUM_MODES - 1);
        case PARAM_MIX: return fx->mix;
        default: return 0.0f;
    }
}

void fx_paula_blep_set_parameter_value(FXPaulaBlep* fx, int index, float value) {
    if (!fx) return;

    switch (index) {
        case PARAM_ENABLED:
            fx_paula_blep_set_enabled(fx, value >= 0.5f);
            break;
        case PARAM_MODE:
            fx_paula_blep_set_mode(fx, (PaulaMode)(value * (float)(PAULA_NUM_MODES - 1) + 0.5f));
            break;
        case PARAM_MIX:
            fx_paula_blep_set_mix(fx, value);
            break;
    }
}

const char* fx_paula_blep_get_parameter_name(int index) {
    switch (index) {
        case PARAM_ENABLED: return "Enabled";
        case PARAM_MODE: return "Paula Mode";
        case PARAM_MIX: return "Mix";
        default: return "";
    }
}

const char* fx_paula_blep_get_parameter_label(int index) {
    switch (index) {
        case PARAM_ENABLED: return "";
        case PARAM_MODE: return "";
        case PARAM_MIX: return "%";
        default: return "";
    }
}

float fx_paula_blep_get_parameter_default(int index) {
    switch (index) {
        case PARAM_ENABLED: return 1.0f;
        case PARAM_MODE: return (float)PAULA_A500_ON / (float)(PAULA_NUM_MODES - 1);
        case PARAM_MIX: return 1.0f;
        default: return 0.0f;
    }
}

float fx_paula_blep_get_parameter_min(int index) {
    switch (index) {
        case PARAM_ENABLED: return 0.0f;
        case PARAM_MODE: return 0.0f;
        case PARAM_MIX: return 0.0f;
        default: return 0.0f;
    }
}

float fx_paula_blep_get_parameter_max(int index) {
    switch (index) {
        case PARAM_ENABLED: return 1.0f;
        case PARAM_MODE: return (float)(PAULA_NUM_MODES - 1);
        case PARAM_MIX: return 1.0f;
        default: return 1.0f;
    }
}

int fx_paula_blep_get_parameter_group(int index) {
    (void)index;
    return 0;
}

const char* fx_paula_blep_get_group_name(int group) {
    return (group == 0) ? "Paula BLEP" : "";
}

int fx_paula_blep_parameter_is_integer(int index) {
    return (index == PARAM_ENABLED || index == PARAM_MODE);
}
