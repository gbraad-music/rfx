/*
    BSD 3-Clause License

    Copyright (c) 2023, KORG INC.
    All rights reserved.
*/

#include "unit_genericfx.h"

const __unit_header genericfx_unit_header_t unit_header = {
  .common = {
    .api = UNIT_API_VERSION,
    .target = unit_target_nts3_kaoss,
    .platform = unit_platform_nts3_kaoss,
    .module = unit_module_genericfx,
    .dev_id = 0U,
    .unit_id = 17U,
    .version = 0x00010000U,
    .name = "RFX Lofi",
    .num_presets = 0,
    .num_params = 7,
    .params = {
      // 0: Bit Depth (X axis) - 0=2-bit, 1=8-bit, 2=12-bit, 3=16-bit
      {0, 3, 0, 3, k_unit_param_type_strings, 0, 0, 0, {"BIT"}},
      // 1: Sample Rate (Y axis) - 0=7.5k, 1=8.3k(Amiga), 2=10k, 3=15k, 4=16.7k, 5=22k, 6=32k, 7=48k
      {0, 7, 0, 7, k_unit_param_type_strings, 0, 0, 0, {"RATE"}},
      // 2: Filter Cutoff (Depth slider) - 0-100%
      {0, 100, 0, 100, k_unit_param_type_percent, 0, 0, 0, {"FILTER"}},
      // 3: Saturation - 0-100% (maps to 0.0-2.0)
      {0, 100, 0, 0, k_unit_param_type_percent, 0, 0, 0, {"SAT"}},
      // 4: Noise Level - 0-100%
      {0, 100, 0, 0, k_unit_param_type_percent, 0, 0, 0, {"NOISE"}},
      // 5: Wow/Flutter Depth - 0-100%
      {0, 100, 0, 0, k_unit_param_type_percent, 0, 0, 0, {"WFDEP"}},
      // 6: Wow/Flutter Rate - 1-100 (maps to 0.1-10.0 Hz)
      {1, 100, 0, 5, k_unit_param_type_none, 0, 0, 0, {"WFRAT"}},
      // Empty params
      {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
      {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
    },
  },
  .default_mappings = {
    // Bit Depth -> X axis (0-3)
    {k_genericfx_param_assign_x, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 3, 3},
    // Sample Rate -> Y axis (0-7)
    {k_genericfx_param_assign_y, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 7, 7},
    // Filter -> Depth slider (0-100)
    {k_genericfx_param_assign_depth, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 100, 100},
    // Saturation -> unmapped
    {k_genericfx_param_assign_none, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 100, 0},
    // Noise -> unmapped
    {k_genericfx_param_assign_none, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 100, 0},
    // Wow/Flutter Depth -> unmapped
    {k_genericfx_param_assign_none, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 100, 0},
    // Wow/Flutter Rate -> unmapped
    {k_genericfx_param_assign_none, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 1, 100, 5},
    // Empty mappings
    {k_genericfx_param_assign_none, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 0, 0},
    {k_genericfx_param_assign_none, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 0, 0},
  }
};
