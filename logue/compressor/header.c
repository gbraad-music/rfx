/*
 * Regroove Compressor - NTS-3 kaoss pad header
 * Dynamic range compressor
 */

#include "unit_genericfx.h"

const __unit_header genericfx_unit_header_t unit_header = {
  .common = {
    .header_size = sizeof(genericfx_unit_header_t),
    .target = UNIT_TARGET_PLATFORM | k_unit_module_genericfx,
    .api = UNIT_API_VERSION,
    .dev_id = 0x0,
    .unit_id = 0x4U,
    .version = 0x00010000U,
    .name = "RFX Compressor",
    .num_params = 5,

    .params = {
      // Threshold: 0-1023 (-40dB to -6dB)
      {0, 1023, 0, 512, k_unit_param_type_none, 0, 0, 0, {"THRES"}},

      // Ratio: 0-1023 (1:1 to 20:1)
      {0, 1023, 0, 512, k_unit_param_type_none, 0, 0, 0, {"RATIO"}},

      // Attack: 0-1023 (0.5ms to 50ms)
      {0, 1023, 0, 512, k_unit_param_type_none, 0, 0, 0, {"ATTACK"}},

      // Release: 0-1023 (10ms to 500ms)
      {0, 1023, 0, 512, k_unit_param_type_none, 0, 0, 0, {"REL"}},

      // Makeup: 0-1023 (0.125x to 8x)
      {0, 1023, 0, 512, k_unit_param_type_none, 0, 0, 0, {"MAKEUP"}},

      {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
      {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
      {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}}
    },
  },
  .default_mappings = {
    // All unmapped - always-on effect (not touch-dependent)
    {k_genericfx_param_assign_none, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 1023, 512},
    {k_genericfx_param_assign_none, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 1023, 512},
    {k_genericfx_param_assign_none, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 1023, 512},
    {k_genericfx_param_assign_none, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 1023, 512},
    {k_genericfx_param_assign_none, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 1023, 512},
    {k_genericfx_param_assign_none, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 0, 0},
    {k_genericfx_param_assign_none, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 0, 0},
    {k_genericfx_param_assign_none, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 0, 0}
  }
};
