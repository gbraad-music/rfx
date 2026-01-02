/**
 * Regroove RG303 - Drumlogue Synth Header
 * TB-303 inspired bass synthesizer
 */

#include "unit.h"

const __unit_header unit_header_t unit_header = {
    .header_size = sizeof(unit_header_t),
    .target = UNIT_TARGET_PLATFORM | k_unit_module_synth,
    .api = UNIT_API_VERSION,
    .dev_id = 0x0U,
    .unit_id = 0x1U,
    .version = 0x00010000U,
    .name = "RG303",
    .num_presets = 0,
    .num_params = 8,
    .params = {
        // Page 1 - Oscillator
        {0, 1, 0, 0, k_unit_param_type_strings, 0, 0, 0, {"WAVE"}},       // 0: Saw/Square
        {0, 100, 0, 50, k_unit_param_type_percent, 0, 0, 0, {"CUTOFF"}},  // 1: Filter cutoff
        {0, 100, 0, 50, k_unit_param_type_percent, 0, 0, 0, {"RESO"}},    // 2: Resonance
        {0, 100, 0, 50, k_unit_param_type_percent, 0, 0, 0, {"ENV MOD"}}, // 3: Envelope modulation

        // Page 2 - Envelope & Modulation
        {0, 100, 0, 30, k_unit_param_type_percent, 0, 0, 0, {"DECAY"}},   // 4: Envelope decay
        {0, 100, 0, 0, k_unit_param_type_percent, 0, 0, 0, {"ACCENT"}},   // 5: Accent amount
        {1, 100, 0, 10, k_unit_param_type_percent, 0, 0, 0, {"SLIDE"}},   // 6: Slide time
        {0, 100, 0, 70, k_unit_param_type_percent, 0, 0, 0, {"VOLUME"}},  // 7: Master volume

        // Remaining pages empty
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}}
    }
};
