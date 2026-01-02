/*
 * Regroove Distortion - NTS-3 kaoss pad header
 * Distortion effect with drive and mix control
 */

#include "unit_genericfx.h"

const __unit_header genericfx_unit_header_t unit_header = {
  .common = {
    .header_size = sizeof(genericfx_unit_header_t),
    .target = UNIT_TARGET_PLATFORM | k_unit_module_genericfx,
    .api = UNIT_API_VERSION,
    .dev_id = 0x0,
    .unit_id = 0x2U,
    .version = 0x00010000U,
    .name = "RFX Distortion",
    .num_params = 2,

    .params = {
      // Drive: 0-1023 (distortion amount)
      {0, 1023, 0, 0, k_unit_param_type_none, 0, 0, 0, {"DRIVE"}},

      // Mix: 0-1023 (dry/wet mix)
      {0, 1023, 0, 0, k_unit_param_type_none, 0, 0, 0, {"MIX"}},

      {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
      {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
      {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
      {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
      {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
      {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}}
    },
  },
  .default_mappings = {
    // All unmapped - always-on effect (not touch-dependent)
    {k_genericfx_param_assign_none, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 1023, 0},
    {k_genericfx_param_assign_none, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 1023, 0},
    {k_genericfx_param_assign_none, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 0, 0},
    {k_genericfx_param_assign_none, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 0, 0},
    {k_genericfx_param_assign_none, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 0, 0},
    {k_genericfx_param_assign_none, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 0, 0},
    {k_genericfx_param_assign_none, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 0, 0},
    {k_genericfx_param_assign_none, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 0, 0}
  }
};
