/*
    BSD 3-Clause License
    Copyright (c) 2024, Regroove Project
    All rights reserved.
*/

#include "unit.h"

#define RGSLICER_VERSION_MAJOR 1
#define RGSLICER_VERSION_MINOR 0
#define RGSLICER_VERSION_PATCH 0

__unit_header
const unit_header_t unit_header = {
    .header_size = sizeof(unit_header_t),
    .target = UNIT_TARGET_PLATFORM | k_unit_module_synth,
    .api = UNIT_API_VERSION,
    .dev_id = 0x52474658U,  // 'RGFX'
    .unit_id = 0x07U,        // RGSlicer Drumlogue
    .version = 0x00010000U,  // v1.0.0
    .name = "RGSlicer",
    .num_presets = 8,
    .num_params = 6,
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
            .name = "VOLUME"
        },
        // Master Pitch
        {
            .min = -12,
            .max = 12,
            .center = 0,
            .init = 0,
            .type = k_unit_param_type_none,
            .frac = 0,
            .frac_mode = 0,
            .reserved = 0,
            .name = "PITCH"
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
            .name = "TIME"
        },
        // Slice Mode (0=Transient, 1=Zero-Cross, 2=Grid, 3=BPM)
        {
            .min = 0,
            .max = 3,
            .center = 0,
            .init = 0,
            .type = k_unit_param_type_none,
            .frac = 0,
            .frac_mode = 0,
            .reserved = 0,
            .name = "MODE"
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
            .name = "SLICES"
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
            .name = "SENSE"
        },
        // Unused params (Drumlogue requires 24 total)
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
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}}
    }
};
