/**
 * Regroove RG1Piano - NTS-1 MKII Oscillator
 * M1 Piano sampler
 */

#include "unit_osc.h"

const __unit_header unit_header_t unit_header = {
    .header_size = sizeof(unit_header_t),
    .target = UNIT_TARGET_PLATFORM | k_unit_module_osc,
    .api = UNIT_API_VERSION,
    .dev_id = 0x0U,
    .unit_id = 0x2U,
    .version = 0x00010000U,
    .name = "RG1Piano",
    .num_params = 6,
    .params = {
        // Shape knob - Decay time
        {0, 1023, 0, 512, k_unit_param_type_none, 0, 0, 0, {"DECAY"}},

        // Alt knob - Brightness/filter
        {0, 1023, 0, 700, k_unit_param_type_none, 0, 0, 0, {"BRIGHT"}},

        // Edit menu parameters
        {0, 1023, 0, 800, k_unit_param_type_none, 0, 0, 0, {"VEL SENS"}},
        {0, 1023, 0, 850, k_unit_param_type_none, 0, 0, 0, {"VOLUME"}},
        {0, 1023, 0, 300, k_unit_param_type_none, 0, 0, 0, {"LFO RATE"}},
        {0, 1023, 0, 200, k_unit_param_type_none, 0, 0, 0, {"LFO DPTH"}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
    }
};
