/**
 * Regroove RG909 Drum - Drumlogue Synth Header
 * TR-909 Bass Drum Synthesizer
 */

#include "unit.h"

const __unit_header unit_header_t unit_header = {
    .header_size = sizeof(unit_header_t),
    .target = UNIT_TARGET_PLATFORM | k_unit_module_synth,
    .api = UNIT_API_VERSION,
    .dev_id = 0x0U,
    .unit_id = 0x3U,
    .version = 0x00010000U,
    .name = "RG909BD",
    .num_presets = 0,
    .num_params = 8,
    .params = {
        // Page 1 - Main BD Controls
        {0, 100, 0, 80, k_unit_param_type_percent, 0, 0, 0, {"LEVEL"}},      // 0: Level
        {0, 100, 0, 50, k_unit_param_type_percent, 0, 0, 0, {"TUNE"}},       // 1: Pitch/Tuning
        {0, 100, 0, 50, k_unit_param_type_percent, 0, 0, 0, {"DECAY"}},      // 2: Decay time
        {0, 100, 0, 10, k_unit_param_type_percent, 0, 0, 0, {"ATTACK"}},     // 3: Attack/Click amount

        // Page 2 - Sweep Shape Controls
        {0, 100, 0, 50, k_unit_param_type_percent, 0, 0, 0, {"SWEEP"}},      // 4: Pitch sweep amount
        {0, 100, 0, 30, k_unit_param_type_percent, 0, 0, 0, {"TONE"}},       // 5: Tone/Harmonics
        {0, 100, 0, 70, k_unit_param_type_percent, 0, 0, 0, {"MASTER"}},     // 6: Master volume
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},                 // 7: Empty

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
