/**
 * Regroove RG101 - NTS-1 MKII Oscillator
 * SH-101 inspired bass/lead synthesizer
 */

#include "unit_osc.h"

const __unit_header unit_header_t unit_header = {
    .header_size = sizeof(unit_header_t),
    .target = UNIT_TARGET_PLATFORM | k_unit_module_osc,
    .api = UNIT_API_VERSION,
    .dev_id = 0x0U,
    .unit_id = 0x2U,
    .version = 0x00010000U,
    .name = "RG101",
    .num_params = 10,
    .params = {
        // Shape knob - Waveform selector
        {0, 2, 0, 0, k_unit_param_type_strings, 0, 0, 0, {"WAVE"}},

        // Alt knob - Filter cutoff
        {0, 1023, 0, 512, k_unit_param_type_none, 0, 0, 0, {"CUTOFF"}},

        // Edit menu parameters
        {0, 1023, 0, 512, k_unit_param_type_none, 0, 0, 0, {"RESO"}},
        {0, 1023, 0, 512, k_unit_param_type_none, 0, 0, 0, {"ENV MOD"}},
        {0, 1023, 0, 300, k_unit_param_type_none, 0, 0, 0, {"ATTACK"}},
        {0, 1023, 0, 300, k_unit_param_type_none, 0, 0, 0, {"DECAY"}},
        {0, 1023, 0, 512, k_unit_param_type_none, 0, 0, 0, {"LFO RATE"}},
        {0, 1023, 0, 0, k_unit_param_type_none, 0, 0, 0, {"LFO DEPTH"}},
        {0, 1023, 0, 0, k_unit_param_type_none, 0, 0, 0, {"PWM"}},
        {0, 1023, 0, 700, k_unit_param_type_none, 0, 0, 0, {"VOLUME"}},
    }
};
