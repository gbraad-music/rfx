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
    .version = 0x00010001U,  // Incremented version for modal piano
    .name = "RG1Piano",
    .num_params = 7,
    .params = {
        // Shape knob - Decay time
        {0, 1023, 0, 512, k_unit_param_type_none, 0, 0, 0, {"DECAY"}},

        // Alt knob - Modal resonance amount
        {0, 1023, 0, 0, k_unit_param_type_none, 0, 0, 0, {"RESONANC"}},

        // Edit menu parameters
        {0, 1023, 0, 614, k_unit_param_type_none, 0, 0, 0, {"BRIGHT"}},  // Filter envelope sustain (60%)
        {0, 1023, 0, 819, k_unit_param_type_none, 0, 0, 0, {"VEL SENS"}},
        {0, 1023, 0, 716, k_unit_param_type_none, 0, 0, 0, {"VOLUME"}},  // 70%
        {0, 1023, 0, 307, k_unit_param_type_none, 0, 0, 0, {"LFO RATE"}},
        {0, 1023, 0, 204, k_unit_param_type_none, 0, 0, 0, {"LFO DPTH"}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
    }
};
