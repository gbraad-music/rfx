/*
 * Regroove RG909 Kaoss - NTS-3 Effect Header
 * TR-909 BD with tempo-synced rhythmic triggering via touch
 */

#include "unit_genericfx.h"

const __unit_header genericfx_unit_header_t unit_header = {
  .common = {
    .header_size = sizeof(genericfx_unit_header_t),
    .target = UNIT_TARGET_PLATFORM | k_unit_module_genericfx,
    .api = UNIT_API_VERSION,
    .dev_id = 0x0,
    .unit_id = 0x42524756U,  // "BRGV" - Regroove
    .version = 0x00010000U,
    .name = "RG909 Kaoss",
    .num_params = 8,

    .params = {
      // BD Level: 0-1023
      {0, 1023, 0, 819, k_unit_param_type_none, 0, 0, 0, {"LEVEL"}},

      // BD Tune: 0-1023
      {0, 1023, 0, 512, k_unit_param_type_none, 0, 0, 0, {"TUNE"}},

      // BD Decay: 0-1023
      {0, 1023, 0, 512, k_unit_param_type_none, 0, 0, 0, {"DECAY"}},

      // BD Attack: 0-1023
      {0, 1023, 0, 102, k_unit_param_type_none, 0, 0, 0, {"ATTACK"}},

      // Dry/Wet mix: 0-1023
      {0, 1023, 0, 512, k_unit_param_type_none, 0, 0, 0, {"DRY/WET"}},

      // Pattern density: 0-1023 (controlled by Y-axis when touched)
      {0, 1023, 0, 1023, k_unit_param_type_none, 0, 0, 0, {"PATTERN"}},

      // Swing: 0-1023
      {0, 1023, 0, 512, k_unit_param_type_none, 0, 0, 0, {"SWING"}},

      // Tone: 0-1023
      {0, 1023, 0, 307, k_unit_param_type_none, 0, 0, 0, {"TONE"}}
    },
  },
  .default_mappings = {
    // PATTERN mapped to Y axis (full range) - four-on-the-floor at top, sparse at bottom
    {k_genericfx_param_assign_y, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 1023, 1023},

    {k_genericfx_param_assign_none, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 0, 0},
    {k_genericfx_param_assign_none, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 0, 0},
    {k_genericfx_param_assign_none, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 0, 0},
    {k_genericfx_param_assign_none, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 0, 0},
    {k_genericfx_param_assign_none, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 0, 0},
    {k_genericfx_param_assign_none, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 0, 0},
    {k_genericfx_param_assign_none, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 0, 0}
  }
};
