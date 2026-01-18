/**
 *  @file header.c
 *  @brief RGSFZ SFZ Player for MicroKorg2
 *
 *  Copyright (c) 2024 Regroove Project
 */

#include "unit.h"
#include "runtime.h"

#define RGSFZ_VERSION_MAJOR 1
#define RGSFZ_VERSION_MINOR 0
#define RGSFZ_VERSION_PATCH 0

// ---- Unit header definition  --------------------------------------------------------------------

const __unit_header unit_header_t unit_header = {
    .header_size = sizeof(unit_header_t),
    .target = UNIT_TARGET_PLATFORM | k_unit_module_osc,
    .api = UNIT_API_VERSION,
    .dev_id = 0x52474658U,  // 'RGFX'
    .unit_id = 0x07U,        // RGSFZ MicroKorg2
    .version = 0x00010000U,  // v1.0.0
    .name = "RGSFZ",
    .num_presets = 8,
    .num_params = 3,
    .params = {
        // Volume
        {
            .min = 0,
            .max = 200,
            .center = 100,
            .init = 150,
            .type = k_unit_param_type_percent,
            .frac = 0,
            .frac_mode = 0,
            .reserved = 0,
            .name = "VOLUME"
        },
        // Pan
        {
            .min = 0,
            .max = 200,
            .center = 100,
            .init = 100,
            .type = k_unit_param_type_pan,
            .frac = 0,
            .frac_mode = 0,
            .reserved = 0,
            .name = "PAN"
        },
        // Decay
        {
            .min = 0,
            .max = 100,
            .center = 50,
            .init = 50,
            .type = k_unit_param_type_percent,
            .frac = 0,
            .frac_mode = 0,
            .reserved = 0,
            .name = "DECAY"
        },
        // Unused slots
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}}
    }
};
