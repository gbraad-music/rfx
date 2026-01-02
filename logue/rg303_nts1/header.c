/**
 * Regroove RG303 - NTS-1 MKII Oscillator
 * TB-303 inspired bass synthesizer
 */

#include "unit_osc.h"

const __unit_header unit_header_t unit_header = {
    .header_size = sizeof(unit_header_t),
    .target = UNIT_TARGET_PLATFORM | k_unit_module_osc,
    .api = UNIT_API_VERSION,
    .dev_id = 0x0U,
    .unit_id = 0x1U,
    .version = 0x00010000U,
    .name = "RG303",
    .num_params = 8,
    .params = {
        // Shape knob - Waveform selector
        {0, 1, 0, 0, k_unit_param_type_strings, 0, 0, 0, {"WAVE"}},

        // Alt knob - Filter cutoff
        {0, 1023, 0, 512, k_unit_param_type_none, 0, 0, 0, {"CUTOFF"}},

        // Edit menu parameters
        {0, 1023, 0, 512, k_unit_param_type_none, 0, 0, 0, {"RESO"}},
        {0, 1023, 0, 512, k_unit_param_type_none, 0, 0, 0, {"ENV MOD"}},
        {0, 1023, 0, 300, k_unit_param_type_none, 0, 0, 0, {"DECAY"}},
        {0, 1023, 0, 0, k_unit_param_type_none, 0, 0, 0, {"ACCENT"}},
        {0, 1023, 0, 100, k_unit_param_type_none, 0, 0, 0, {"SLIDE"}},
        {0, 1023, 0, 700, k_unit_param_type_none, 0, 0, 0, {"VOLUME"}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
    }
};
