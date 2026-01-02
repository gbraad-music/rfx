/*
 * Regroove Distortion - NTS-1 MKII Modulation Effect
 * Analog-style saturation with drive and mix controls
 */

#include "unit_modfx.h"

const __unit_header unit_header_t unit_header = {
    .header_size = sizeof(unit_header_t),
    .target = UNIT_TARGET_PLATFORM | k_unit_module_modfx,
    .api = UNIT_API_VERSION,
    .dev_id = 0x0,
    .unit_id = 0x10U,
    .version = 0x00010000U,
    .name = "DISTORT",
    .num_params = 2,

    .params = {
        // A knob - Drive
        {0, 1023, 0, 512, k_unit_param_type_none, 0, 0, 0, {"DRIVE"}},

        // B knob - Mix (dry/wet)
        {0, 1023, 0, 512, k_unit_param_type_none, 0, 0, 0, {"MIX"}},

        // 8 Edit menu parameters (unused)
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
    }
};
