/*
    BSD 3-Clause License
    Copyright (c) 2026, Regroove Project
    All rights reserved.
*/

#include "unit.h"

#define RGSLICER_VERSION_MAJOR 1
#define RGSLICER_VERSION_MINOR 0
#define RGSLICER_VERSION_PATCH 0

__attribute__((used, section(".unit_header")))
const unit_header_t unit_header = {
    .header_size = sizeof(unit_header_t),
    .target = UNIT_TARGET_PLATFORM | k_unit_module_osc,
    .api = UNIT_API_VERSION,
    .dev_id = 0x52474658U,  // 'RGFX'
    .unit_id = 0x08U,        // RGSlicer MicroKorg2
    .version = 0x00010000U,  // v1.0.0
    .name = "RGSlicer",
    .num_presets = 8,
    .num_params = 9,
    .params = {
        // Master Volume
        {
            .min = 0,
            .max = 200,
            .center = 100,
            .init = 100,
            .type = k_unit_param_type_percent,
            .frac = 0,
            .frac_mode = 0,
            .reserved = 0,
            .name = "Volume"
        },
        // Master Pitch
        {
            .min = -12,
            .max = 12,
            .center = 0,
            .init = 0,
            .type = k_unit_param_type_semi,
            .frac = 0,
            .frac_mode = 0,
            .reserved = 0,
            .name = "Pitch"
        },
        // Master Time
        {
            .min = 50,
            .max = 200,
            .center = 100,
            .init = 100,
            .type = k_unit_param_type_percent,
            .frac = 0,
            .frac_mode = 0,
            .reserved = 0,
            .name = "Time"
        },
        // Slice Mode (0=Transient, 1=Zero-Cross, 2=Grid, 3=BPM)
        {
            .min = 0,
            .max = 3,
            .center = 0,
            .init = 0,
            .type = k_unit_param_type_enum,
            .frac = 0,
            .frac_mode = 0,
            .reserved = 0,
            .name = "Mode"
        },
        // Num Slices
        {
            .min = 1,
            .max = 64,
            .center = 16,
            .init = 16,
            .type = k_unit_param_type_none,
            .frac = 0,
            .frac_mode = 0,
            .reserved = 0,
            .name = "Slices"
        },
        // Sensitivity
        {
            .min = 0,
            .max = 100,
            .center = 50,
            .init = 50,
            .type = k_unit_param_type_percent,
            .frac = 0,
            .frac_mode = 0,
            .reserved = 0,
            .name = "Sense"
        },
        // Note Division (1=1/4, 2=1/8, 4=1/16, 8=1/32)
        {
            .min = 1,
            .max = 8,
            .center = 4,
            .init = 4,
            .type = k_unit_param_type_enum,
            .frac = 0,
            .frac_mode = 0,
            .reserved = 0,
            .name = "Note Div"
        },
        // Pitch Algorithm (0=Rate, 1=Time-Preserving)
        {
            .min = 0,
            .max = 1,
            .center = 0,
            .init = 0,
            .type = k_unit_param_type_enum,
            .frac = 0,
            .frac_mode = 0,
            .reserved = 0,
            .name = "Pitch Alg"
        },
        // Time Algorithm (0=Granular, 1=AKAI/Amiga)
        {
            .min = 0,
            .max = 1,
            .center = 1,
            .init = 1,
            .type = k_unit_param_type_enum,
            .frac = 0,
            .frac_mode = 0,
            .reserved = 0,
            .name = "Time Alg"
        },
        // Unused params (24 total)
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
