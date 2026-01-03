/*
 * RG-303 Kaotic Bass - NTS-3 kaoss pad
 * TB-303 style bass synthesizer
 */

#include "unit_genericfx.h"

const __unit_header genericfx_unit_header_t unit_header = {
  .common = {
    .header_size = sizeof(genericfx_unit_header_t),
    .target = UNIT_TARGET_PLATFORM | k_unit_module_genericfx,
    .api = UNIT_API_VERSION,
    .dev_id = 0x0,
    .unit_id = 0x3U,
    .version = 0x00010000U,
    .name = "RG-303 Kaotic",
    .num_params = 3,

    .params = {
      // FILTER - Filter cutoff/resonance (X-axis)
      {0, 1023, 0, 307, k_unit_param_type_none, 0, 0, 0, {"FILTER"}},

      // PATTERN - Pattern variation (Y-axis)
      {0, 1023, 0, 512, k_unit_param_type_none, 0, 0, 0, {"PATTERN"}},

      // SLIDE - Slide/portamento amount (unmapped, editor only)
      {0, 1023, 0, 512, k_unit_param_type_none, 0, 0, 0, {"SLIDE"}},

      {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
      {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
      {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
      {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
      {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}}
    },
  },
  .default_mappings = {
    // FILTER mapped to X axis
    {k_genericfx_param_assign_x, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 1023, 307},

    // PATTERN mapped to Y axis
    {k_genericfx_param_assign_y, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 1023, 512},

    // SLIDE unmapped (editor only)
    {k_genericfx_param_assign_none, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 1023, 512},

    {k_genericfx_param_assign_none, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 0, 0},
    {k_genericfx_param_assign_none, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 0, 0},
    {k_genericfx_param_assign_none, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 0, 0},
    {k_genericfx_param_assign_none, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 0, 0},
    {k_genericfx_param_assign_none, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 0, 0}
  }
};
