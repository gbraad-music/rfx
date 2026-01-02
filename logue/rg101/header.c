/**
 * Regroove RG101 - Drumlogue Synth Header
 * SH-101 inspired monophonic synthesizer
 */

#include "unit.h"

const __unit_header unit_header_t unit_header = {
    .header_size = sizeof(unit_header_t),
    .target = UNIT_TARGET_PLATFORM | k_unit_module_synth,
    .api = UNIT_API_VERSION,
    .dev_id = 0x0U,
    .unit_id = 0x2U,
    .version = 0x00010000U,
    .name = "RG101",
    .num_presets = 0,
    .num_params = 24,
    .params = {
        // Page 1 - Oscillator
        {0, 100, 0, 80, k_unit_param_type_percent, 0, 0, 0, {"SAW LVL"}},      // 0: Saw level
        {0, 100, 0, 0, k_unit_param_type_percent, 0, 0, 0, {"SQR LVL"}},       // 1: Square level
        {0, 100, 0, 30, k_unit_param_type_percent, 0, 0, 0, {"SUB LVL"}},      // 2: Sub oscillator level
        {0, 100, 0, 0, k_unit_param_type_percent, 0, 0, 0, {"NOISE"}},         // 3: Noise level
        {0, 100, 0, 50, k_unit_param_type_percent, 0, 0, 0, {"PW"}},           // 4: Pulse width
        {0, 100, 0, 0, k_unit_param_type_percent, 0, 0, 0, {"PWM"}},           // 5: PWM depth

        // Page 2 - Filter
        {0, 100, 0, 50, k_unit_param_type_percent, 0, 0, 0, {"CUTOFF"}},       // 6: Filter cutoff
        {0, 100, 0, 30, k_unit_param_type_percent, 0, 0, 0, {"RESO"}},         // 7: Resonance
        {0, 100, 0, 50, k_unit_param_type_percent, 0, 0, 0, {"ENV MOD"}},      // 8: Envelope modulation
        {0, 100, 0, 50, k_unit_param_type_percent, 0, 0, 0, {"KEY TRK"}},      // 9: Keyboard tracking

        // Page 3 - Filter Envelope
        {0, 100, 0, 0, k_unit_param_type_percent, 0, 0, 0, {"F ATK"}},         // 10: Filter attack
        {0, 100, 0, 30, k_unit_param_type_percent, 0, 0, 0, {"F DEC"}},        // 11: Filter decay
        {0, 100, 0, 0, k_unit_param_type_percent, 0, 0, 0, {"F SUS"}},         // 12: Filter sustain
        {0, 100, 0, 10, k_unit_param_type_percent, 0, 0, 0, {"F REL"}},        // 13: Filter release

        // Page 4 - Amp Envelope
        {0, 100, 0, 0, k_unit_param_type_percent, 0, 0, 0, {"A ATK"}},         // 14: Amp attack
        {0, 100, 0, 30, k_unit_param_type_percent, 0, 0, 0, {"A DEC"}},        // 15: Amp decay
        {0, 100, 0, 70, k_unit_param_type_percent, 0, 0, 0, {"A SUS"}},        // 16: Amp sustain
        {0, 100, 0, 10, k_unit_param_type_percent, 0, 0, 0, {"A REL"}},        // 17: Amp release

        // Page 5 - Modulation & Performance
        {0, 100, 0, 50, k_unit_param_type_percent, 0, 0, 0, {"LFO RATE"}},     // 18: LFO rate
        {0, 100, 0, 0, k_unit_param_type_percent, 0, 0, 0, {"LFO>FLT"}},       // 19: LFO to filter depth
        {0, 100, 0, 50, k_unit_param_type_percent, 0, 0, 0, {"VEL SEN"}},      // 20: Velocity sensitivity
        {0, 100, 0, 0, k_unit_param_type_percent, 0, 0, 0, {"PORTA"}},         // 21: Portamento
        {0, 100, 0, 70, k_unit_param_type_percent, 0, 0, 0, {"VOLUME"}},       // 22: Master volume
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},                   // 23: Empty

    }
};
