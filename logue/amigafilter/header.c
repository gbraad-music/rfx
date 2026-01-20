/*
 * Regroove Amiga Filter - NTS-3 kaoss pad header
 * Amiga Paula hardware RC filter emulation
 */

#include "unit_genericfx.h"

const __unit_header genericfx_unit_header_t unit_header = {
  .common = {
    .header_size = sizeof(genericfx_unit_header_t),
    .target = UNIT_TARGET_PLATFORM | k_unit_module_genericfx,
    .api = UNIT_API_VERSION,
    .dev_id = 0x0,
    .unit_id = 0x10U,
    .version = 0x00010000U,
    .name = "RFX AmigaFilter",
    .num_params = 2,

    .params = {
      // Type: 0-3 (A500, A500+LED, A1200, A1200+LED)
      {0, 3, 0, 0, k_unit_param_type_strings, 0, 0, 0, {"TYPE"}},

      // Depth: 0-100%
      {0, 100, 0, 100, k_unit_param_type_percent, 0, 0, 0, {"DEPTH"}},

      {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
      {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
      {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
      {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
      {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
      {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}}
    },
  },
  .default_mappings = {
    // TYPE mapped to X axis
    {k_genericfx_param_assign_x, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 3, 0},

    // DEPTH mapped to depth slider
    {k_genericfx_param_assign_depth, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 100, 100},
    {k_genericfx_param_assign_none, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 0, 0},
    {k_genericfx_param_assign_none, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 0, 0},
    {k_genericfx_param_assign_none, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 0, 0},
    {k_genericfx_param_assign_none, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 0, 0},
    {k_genericfx_param_assign_none, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 0, 0},
    {k_genericfx_param_assign_none, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 0, 0}
  }
};
