/*
 * Regroove RM1 - NTS-3 kaoss pad header
 * Model1 Channel Strip: Trim -> HPF -> LPF -> Sculpt
 */

#include "unit_genericfx.h"

const __unit_header genericfx_unit_header_t unit_header = {
  .common = {
    .header_size = sizeof(genericfx_unit_header_t),
    .target = UNIT_TARGET_PLATFORM | k_unit_module_genericfx,
    .api = UNIT_API_VERSION,
    .dev_id = 0x0,
    .unit_id = 0x8U,
    .version = 0x00010000U,
    .name = "RFX RM1",
    .num_params = 5,

    .params = {
      // Trim: 0-1023 (overdrive amount)
      {0, 1023, 0, 512, k_unit_param_type_none, 0, 0, 0, {"TRIM"}},

      // Contour Hi (HPF): 0-1023
      {0, 1023, 0, 0, k_unit_param_type_none, 0, 0, 0, {"CONT HI"}},

      // Sculpt Freq: 0-1023 (70Hz to 7kHz)
      {0, 1023, 0, 512, k_unit_param_type_none, 0, 0, 0, {"SCULPT F"}},

      // Sculpt Gain: 0-1023 (-20dB to +8dB)
      {0, 1023, 0, 512, k_unit_param_type_none, 0, 0, 0, {"SCULPT G"}},

      // Contour Lo (LPF): 0-1023
      {0, 1023, 0, 1023, k_unit_param_type_none, 0, 0, 0, {"CONT LO"}},

      {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
      {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
      {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}}
    },
  },
  .default_mappings = {
    // TRIM mapped to X axis
    {k_genericfx_param_assign_x, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 1023, 512},

    // CONT HI unmapped
    {k_genericfx_param_assign_none, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 1023, 0},

    // SCULPT FREQ mapped to Y axis
    {k_genericfx_param_assign_y, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 1023, 512},

    // SCULPT GAIN unmapped
    {k_genericfx_param_assign_none, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 1023, 512},

    // CONT LO unmapped
    {k_genericfx_param_assign_none, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 1023, 1023},

    {k_genericfx_param_assign_none, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 0, 0},
    {k_genericfx_param_assign_none, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 0, 0},
    {k_genericfx_param_assign_none, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 0, 0}
  }
};
