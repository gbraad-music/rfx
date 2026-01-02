/*
 * Regroove Filter - NTS-3 kaoss pad header
 * DJ-style resonant low-pass filter
 */

#include "unit_genericfx.h"

const __unit_header genericfx_unit_header_t unit_header = {
  .common = {
    .header_size = sizeof(genericfx_unit_header_t),
    .target = UNIT_TARGET_PLATFORM | k_unit_module_genericfx,
    .api = UNIT_API_VERSION,
    .dev_id = 0x0,
    .unit_id = 0x1U,
    .version = 0x00010000U,
    .name = "RFX Filter",
    .num_params = 2,

    .params = {
      // Cutoff: 0-1023 (20Hz - 20kHz exponential)
      {0, 1023, 0, 819, k_unit_param_type_none, 0, 0, 0, {"CUTOFF"}},

      // Resonance: 0-1023 (Q factor)
      {0, 1023, 0, 307, k_unit_param_type_none, 0, 0, 0, {"RES"}},

      {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
      {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
      {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
      {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
      {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
      {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}}
    },
  },
  .default_mappings = {
    // CUTOFF mapped to Y axis (full range)
    {k_genericfx_param_assign_y, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 1023, 819},

    // RESONANCE mapped to X axis (full range)
    {k_genericfx_param_assign_x, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 1023, 307},
    {k_genericfx_param_assign_none, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 0, 0},
    {k_genericfx_param_assign_none, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 0, 0},
    {k_genericfx_param_assign_none, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 0, 0},
    {k_genericfx_param_assign_none, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 0, 0},
    {k_genericfx_param_assign_none, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 0, 0},
    {k_genericfx_param_assign_none, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 0, 0}
  }
};
