/*
 * Regroove Resampler Effect
 *
 * Based on OpenMPT resampling code (BSD license)
 * Original implementation by Olivier Lapicque and OpenMPT Devs
 *
 * Interpolation algorithms:
 * - Nearest: Zero-order hold (sample and hold)
 * - Linear: 2-point linear interpolation
 * - Cubic: 4-point cubic spline (windowed sinc)
 * - Sinc8: 8-point Kaiser-windowed sinc with polyphase anti-aliasing
 */

#include "fx_resampler.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

// ============================================================================
// Cubic Spline Lookup Table (from OpenMPT)
// ============================================================================

// FastSincTable: 256 phases × 4 taps (cubic spline coefficients)
// Values are int16 scaled by 16384 (14-bit fixed point)
static const int16_t FastSincTable[256 * 4] = {
    0, 16384,     0,     0,   -31, 16383,    32,     0,   -63, 16381,    65,     0,   -93, 16378,   100,    -1,
 -124, 16374,   135,    -1,  -153, 16368,   172,    -3,  -183, 16361,   209,    -4,  -211, 16353,   247,    -5,
 -240, 16344,   287,    -7,  -268, 16334,   327,    -9,  -295, 16322,   368,   -12,  -322, 16310,   410,   -14,
 -348, 16296,   453,   -17,  -374, 16281,   497,   -20,  -400, 16265,   541,   -23,  -425, 16248,   587,   -26,
 -450, 16230,   634,   -30,  -474, 16210,   681,   -33,  -497, 16190,   729,   -37,  -521, 16168,   778,   -41,
 -543, 16145,   828,   -46,  -566, 16121,   878,   -50,  -588, 16097,   930,   -55,  -609, 16071,   982,   -60,
 -630, 16044,  1035,   -65,  -651, 16016,  1089,   -70,  -671, 15987,  1144,   -75,  -691, 15957,  1199,   -81,
 -710, 15926,  1255,   -87,  -729, 15894,  1312,   -93,  -748, 15861,  1370,   -99,  -766, 15827,  1428,  -105,
 -784, 15792,  1488,  -112,  -801, 15756,  1547,  -118,  -818, 15719,  1608,  -125,  -834, 15681,  1669,  -132,
 -850, 15642,  1731,  -139,  -866, 15602,  1794,  -146,  -881, 15561,  1857,  -153,  -896, 15520,  1921,  -161,
 -911, 15477,  1986,  -168,  -925, 15434,  2051,  -176,  -939, 15390,  2117,  -184,  -952, 15344,  2184,  -192,
 -965, 15298,  2251,  -200,  -978, 15251,  2319,  -208,  -990, 15204,  2387,  -216, -1002, 15155,  2456,  -225,
-1014, 15106,  2526,  -234, -1025, 15055,  2596,  -242, -1036, 15004,  2666,  -251, -1046, 14952,  2738,  -260,
-1056, 14899,  2810,  -269, -1066, 14846,  2882,  -278, -1075, 14792,  2955,  -287, -1084, 14737,  3028,  -296,
-1093, 14681,  3102,  -306, -1102, 14624,  3177,  -315, -1110, 14567,  3252,  -325, -1118, 14509,  3327,  -334,
-1125, 14450,  3403,  -344, -1132, 14390,  3480,  -354, -1139, 14330,  3556,  -364, -1145, 14269,  3634,  -374,
-1152, 14208,  3712,  -384, -1157, 14145,  3790,  -394, -1163, 14082,  3868,  -404, -1168, 14018,  3947,  -414,
-1173, 13954,  4027,  -424, -1178, 13889,  4107,  -434, -1182, 13823,  4187,  -445, -1186, 13757,  4268,  -455,
-1190, 13690,  4349,  -465, -1193, 13623,  4430,  -476, -1196, 13555,  4512,  -486, -1199, 13486,  4594,  -497,
-1202, 13417,  4676,  -507, -1204, 13347,  4759,  -518, -1206, 13276,  4842,  -528, -1208, 13205,  4926,  -539,
-1210, 13134,  5010,  -550, -1211, 13061,  5094,  -560, -1212, 12989,  5178,  -571, -1212, 12915,  5262,  -581,
-1213, 12842,  5347,  -592, -1213, 12767,  5432,  -603, -1213, 12693,  5518,  -613, -1213, 12617,  5603,  -624,
-1212, 12542,  5689,  -635, -1211, 12466,  5775,  -645, -1210, 12389,  5862,  -656, -1209, 12312,  5948,  -667,
-1208, 12234,  6035,  -677, -1206, 12156,  6122,  -688, -1204, 12078,  6209,  -698, -1202, 11999,  6296,  -709,
-1200, 11920,  6384,  -720, -1197, 11840,  6471,  -730, -1194, 11760,  6559,  -740, -1191, 11679,  6647,  -751,
-1188, 11598,  6735,  -761, -1184, 11517,  6823,  -772, -1181, 11436,  6911,  -782, -1177, 11354,  6999,  -792,
-1173, 11271,  7088,  -802, -1168, 11189,  7176,  -812, -1164, 11106,  7265,  -822, -1159, 11022,  7354,  -832,
-1155, 10939,  7442,  -842, -1150, 10855,  7531,  -852, -1144, 10771,  7620,  -862, -1139, 10686,  7709,  -872,
-1134, 10602,  7798,  -882, -1128, 10516,  7886,  -891, -1122, 10431,  7975,  -901, -1116, 10346,  8064,  -910,
-1110, 10260,  8153,  -919, -1103, 10174,  8242,  -929, -1097, 10088,  8331,  -938, -1090, 10001,  8420,  -947,
-1083,  9915,  8508,  -956, -1076,  9828,  8597,  -965, -1069,  9741,  8686,  -973, -1062,  9654,  8774,  -982,
-1054,  9566,  8863,  -991, -1047,  9479,  8951,  -999, -1039,  9391,  9039, -1007, -1031,  9303,  9127, -1015,
-1024,  9216,  9216, -1024, -1015,  9127,  9303, -1031, -1007,  9039,  9391, -1039,  -999,  8951,  9479, -1047,
 -991,  8863,  9566, -1054,  -982,  8774,  9654, -1062,  -973,  8686,  9741, -1069,  -965,  8597,  9828, -1076,
 -956,  8508,  9915, -1083,  -947,  8420, 10001, -1090,  -938,  8331, 10088, -1097,  -929,  8242, 10174, -1103,
 -919,  8153, 10260, -1110,  -910,  8064, 10346, -1116,  -901,  7975, 10431, -1122,  -891,  7886, 10516, -1128,
 -882,  7798, 10602, -1134,  -872,  7709, 10686, -1139,  -862,  7620, 10771, -1144,  -852,  7531, 10855, -1150,
 -842,  7442, 10939, -1155,  -832,  7354, 11022, -1159,  -822,  7265, 11106, -1164,  -812,  7176, 11189, -1168,
 -802,  7088, 11271, -1173,  -792,  6999, 11354, -1177,  -782,  6911, 11436, -1181,  -772,  6823, 11517, -1184,
 -761,  6735, 11598, -1188,  -751,  6647, 11679, -1191,  -740,  6559, 11760, -1194,  -730,  6471, 11840, -1197,
 -720,  6384, 11920, -1200,  -709,  6296, 11999, -1202,  -698,  6209, 12078, -1204,  -688,  6122, 12156, -1206,
 -677,  6035, 12234, -1208,  -667,  5948, 12312, -1209,  -656,  5862, 12389, -1210,  -645,  5775, 12466, -1211,
 -635,  5689, 12542, -1212,  -624,  5603, 12617, -1213,  -613,  5518, 12693, -1213,  -603,  5432, 12767, -1213,
 -592,  5347, 12842, -1213,  -581,  5262, 12915, -1212,  -571,  5178, 12989, -1212,  -560,  5094, 13061, -1211,
 -550,  5010, 13134, -1210,  -539,  4926, 13205, -1208,  -528,  4842, 13276, -1206,  -518,  4759, 13347, -1204,
 -507,  4676, 13417, -1202,  -497,  4594, 13486, -1199,  -486,  4512, 13555, -1196,  -476,  4430, 13623, -1193,
 -465,  4349, 13690, -1190,  -455,  4268, 13757, -1186,  -445,  4187, 13823, -1182,  -434,  4107, 13889, -1178,
 -424,  4027, 13954, -1173,  -414,  3947, 14018, -1168,  -404,  3868, 14082, -1163,  -394,  3790, 14145, -1157,
 -384,  3712, 14208, -1152,  -374,  3634, 14269, -1145,  -364,  3556, 14330, -1139,  -354,  3480, 14390, -1132,
 -344,  3403, 14450, -1125,  -334,  3327, 14509, -1118,  -325,  3252, 14567, -1110,  -315,  3177, 14624, -1102,
 -306,  3102, 14681, -1093,  -296,  3028, 14737, -1084,  -287,  2955, 14792, -1075,  -278,  2882, 14846, -1066,
 -269,  2810, 14899, -1056,  -260,  2738, 14952, -1046,  -251,  2666, 15004, -1036,  -242,  2596, 15055, -1025,
 -234,  2526, 15106, -1014,  -225,  2456, 15155, -1002,  -216,  2387, 15204,  -990,  -208,  2319, 15251,  -978,
 -200,  2251, 15298,  -965,  -192,  2184, 15344,  -952,  -184,  2117, 15390,  -939,  -176,  2051, 15434,  -925,
 -168,  1986, 15477,  -911,  -161,  1921, 15520,  -896,  -153,  1857, 15561,  -881,  -146,  1794, 15602,  -866,
 -139,  1731, 15642,  -850,  -132,  1669, 15681,  -834,  -125,  1608, 15719,  -818,  -118,  1547, 15756,  -801,
 -112,  1488, 15792,  -784,  -105,  1428, 15827,  -766,   -99,  1370, 15861,  -748,   -93,  1312, 15894,  -729,
  -87,  1255, 15926,  -710,   -81,  1199, 15957,  -691,   -75,  1144, 15987,  -671,   -70,  1089, 16016,  -651,
  -65,  1035, 16044,  -630,   -60,   982, 16071,  -609,   -55,   930, 16097,  -588,   -50,   878, 16121,  -566,
  -46,   828, 16145,  -543,   -41,   778, 16168,  -521,   -37,   729, 16190,  -497,   -33,   681, 16210,  -474,
  -30,   634, 16230,  -450,   -26,   587, 16248,  -425,   -23,   541, 16265,  -400,   -20,   497, 16281,  -374,
  -17,   453, 16296,  -348,   -14,   410, 16310,  -322,   -12,   368, 16322,  -295,    -9,   327, 16334,  -268,
   -7,   287, 16344,  -240,    -5,   247, 16353,  -211,    -4,   209, 16361,  -183,    -3,   172, 16368,  -153,
   -1,   135, 16374,  -124,    -1,   100, 16378,   -93,     0,    65, 16381,   -63,     0,    32, 16383,   -31
};

// Convert int16 table to float (divide by 16384)
static float FastSincTablef[256 * 4];
static int table_initialized = 0;

// ============================================================================
// 8-tap Sinc (Polyphase) Lookup Tables (from OpenMPT)
// ============================================================================

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Sinc configuration (from OpenMPT Resampler.h)
#define SINC_WIDTH 8
#define SINC_PHASES_BITS 12
#define SINC_PHASES (1 << SINC_PHASES_BITS)  // 4096 phases
#define SINC_MASK (SINC_PHASES - 1)

// Three polyphase sinc tables for different downsampling ratios
static float gKaiserSinc[SINC_PHASES * 8];     // Normal upsampling (Kaiser beta=9.6377, cutoff=0.97)
static float gDownsample13x[SINC_PHASES * 8];  // Downsample 1.333x (beta=8.5, cutoff=0.5)
static float gDownsample2x[SINC_PHASES * 8];   // Downsample 2x (beta=7.0, cutoff=0.425)

// Bessel function I0 (modified Bessel function of first kind, order 0)
// Used for Kaiser window calculation
static double izero(double y) {
    double s = 1.0, ds = 1.0, d = 0.0;
    do {
        d = d + 2.0;
        ds = ds * (y * y) / (d * d);
        s = s + ds;
    } while (ds > 1E-7 * s);
    return s;
}

// Generate windowed sinc coefficients (from OpenMPT getsinc function)
// beta: Kaiser window beta parameter (controls sidelobe suppression)
// cutoff: Normalized cutoff frequency (0.0 to 1.0, where 1.0 = Nyquist)
static void generate_sinc_table(float* table, double beta, double cutoff) {
    if (cutoff >= 0.999) {
        cutoff = 0.999;  // Avoid mixer overflows
    }

    const double izeroBeta = izero(beta);
    const double kPi = 4.0 * atan(1.0) * cutoff;

    for (int isrc = 0; isrc < 8 * SINC_PHASES; isrc++) {
        double fsinc;
        int ix = 7 - (isrc & 7);
        ix = (ix * SINC_PHASES) + (isrc >> 3);

        if (ix == (4 * SINC_PHASES)) {
            // Center tap
            fsinc = 1.0;
        } else {
            const double x = (double)(ix - (4 * SINC_PHASES)) / (double)SINC_PHASES;
            const double xPi = x * kPi;
            // Kaiser-windowed sinc
            fsinc = sin(xPi) * izero(beta * sqrt(1.0 - x * x / 16.0)) / (izeroBeta * xPi);
        }

        double coeff = fsinc * cutoff;
        table[isrc] = (float)coeff;
    }
}

static void init_cubic_table(void) {
    if (table_initialized) return;

    // Convert cubic table from int16 to float
    for (int i = 0; i < 256 * 4; i++) {
        FastSincTablef[i] = (float)FastSincTable[i] / 16384.0f;
    }

    // Generate 8-tap sinc tables (from OpenMPT coefficients)
    // gKaiserSinc: General purpose (beta=9.6377, cutoff=0.97)
    generate_sinc_table(gKaiserSinc, 9.6377, 0.97);

    // gDownsample13x: For 1.333x downsample (beta=8.5, cutoff=0.5)
    generate_sinc_table(gDownsample13x, 8.5, 0.5);

    // gDownsample2x: For 2x downsample (beta=7.0, cutoff=0.425)
    generate_sinc_table(gDownsample2x, 7.0, 0.425);

    table_initialized = 1;
}

// ============================================================================
// Interpolation Functions
// ============================================================================

// Nearest neighbor (zero-order hold)
static inline float interpolate_nearest(const float* input, double position, int channels, int channel) {
    int idx = (int)position;
    return input[idx * channels + channel];
}

// Linear interpolation (2-point)
static inline float interpolate_linear(const float* input, double position, int channels, int channel) {
    int idx = (int)position;
    float fract = (float)(position - idx);

    float s0 = input[idx * channels + channel];
    float s1 = input[(idx + 1) * channels + channel];

    return s0 + fract * (s1 - s0);
}

// Cubic spline interpolation (4-point windowed sinc)
static inline float interpolate_cubic(const float* input, double position, int channels, int channel) {
    int idx = (int)position;
    float fract = (float)(position - idx);

    // Get 4 samples: [-1, 0, 1, 2] relative to position
    float s_m1 = input[(idx - 1) * channels + channel];
    float s0   = input[idx * channels + channel];
    float s1   = input[(idx + 1) * channels + channel];
    float s2   = input[(idx + 2) * channels + channel];

    // Lookup cubic coefficients (256 phases)
    int phase = (int)(fract * 256.0f);
    if (phase >= 256) phase = 255;  // Clamp

    const float* lut = &FastSincTablef[phase * 4];

    // Apply 4-tap filter
    return lut[0] * s_m1 + lut[1] * s0 + lut[2] * s1 + lut[3] * s2;
}

// 8-tap windowed sinc interpolation (polyphase with anti-aliasing)
static inline float interpolate_sinc8(const float* input, double position, int channels, int channel, float rate) {
    int idx = (int)position;
    float fract = (float)(position - idx);

    // Select appropriate polyphase table based on resampling ratio
    const float* sinc_table;
    if (rate > 1.5f) {
        sinc_table = gDownsample2x;       // 2x downsample
    } else if (rate > 1.2f) {
        sinc_table = gDownsample13x;      // 1.33x downsample
    } else {
        sinc_table = gKaiserSinc;         // Normal/upsample
    }

    // Calculate phase (4096 phases)
    uint32_t phase = (uint32_t)(fract * (float)SINC_PHASES);
    phase &= SINC_MASK;  // Wrap to valid range

    const float* lut = &sinc_table[phase * SINC_WIDTH];

    // Get 8 samples: [-3, -2, -1, 0, 1, 2, 3, 4] relative to position
    float sum = 0.0f;
    for (int i = 0; i < 8; i++) {
        int sample_idx = (idx - 3 + i) * channels + channel;
        sum += lut[i] * input[sample_idx];
    }

    return sum;
}

// ============================================================================
// Resampler Effect
// ============================================================================

struct FXResampler {
    int enabled;
    ResamplerMode mode;
    float rate;  // Playback rate (1.0 = normal)

    double position;  // Current position in source buffer
};

FXResampler* fx_resampler_create(void) {
    init_cubic_table();

    FXResampler* fx = (FXResampler*)calloc(1, sizeof(FXResampler));
    if (!fx) return NULL;

    fx->enabled = 1;
    fx->mode = RESAMPLER_CUBIC;
    fx->rate = 1.0f;
    fx->position = 0.0;

    return fx;
}

void fx_resampler_destroy(FXResampler* fx) {
    if (fx) {
        free(fx);
    }
}

void fx_resampler_reset(FXResampler* fx) {
    if (!fx) return;
    fx->position = 0.0;
}

void fx_resampler_process_frame(FXResampler* fx, const float* input, float* output,
                                double position, int channels) {
    if (!fx || !input || !output) return;

    for (int ch = 0; ch < channels; ch++) {
        switch (fx->mode) {
            case RESAMPLER_NEAREST:
                output[ch] = interpolate_nearest(input, position, channels, ch);
                break;
            case RESAMPLER_LINEAR:
                output[ch] = interpolate_linear(input, position, channels, ch);
                break;
            case RESAMPLER_CUBIC:
                output[ch] = interpolate_cubic(input, position, channels, ch);
                break;
            case RESAMPLER_SINC8:
                output[ch] = interpolate_sinc8(input, position, channels, ch, fx->rate);
                break;
            default:
                output[ch] = input[(int)position * channels + ch];
                break;
        }
    }
}

void fx_resampler_process_f32(FXResampler* fx, const float* input, float* output,
                              int input_frames, int output_frames, int sample_rate) {
    if (!fx || !fx->enabled || !input || !output) return;

    (void)sample_rate;  // Not used for now

    const int channels = 2;  // Stereo
    const int margin = 3;    // Safety margin for cubic (needs [-1, 0, 1, 2])

    // Resample
    fx->position = margin;  // Start with margin for cubic interpolation

    for (int i = 0; i < output_frames; i++) {
        // Clamp position to valid range
        if (fx->position < margin) {
            fx->position = margin;
        }
        if (fx->position >= input_frames - margin - 1) {
            // Past end - output silence
            output[i * channels] = 0.0f;
            output[i * channels + 1] = 0.0f;
            continue;
        }

        // Interpolate
        fx_resampler_process_frame(fx, input, &output[i * channels], fx->position, channels);

        // Advance position
        fx->position += fx->rate;
    }
}

// ============================================================================
// Parameter Interface
// ============================================================================

void fx_resampler_set_enabled(FXResampler* fx, int enabled) {
    if (fx) fx->enabled = enabled;
}

void fx_resampler_set_mode(FXResampler* fx, ResamplerMode mode) {
    if (fx && mode >= 0 && mode < RESAMPLER_NUM_MODES) {
        fx->mode = mode;
    }
}

void fx_resampler_set_rate(FXResampler* fx, float rate) {
    if (fx) {
        // Clamp to reasonable range (0.25x to 4.0x)
        fx->rate = rate < 0.25f ? 0.25f : (rate > 4.0f ? 4.0f : rate);
    }
}

int fx_resampler_get_enabled(FXResampler* fx) {
    return fx ? fx->enabled : 0;
}

ResamplerMode fx_resampler_get_mode(FXResampler* fx) {
    return fx ? fx->mode : RESAMPLER_LINEAR;
}

float fx_resampler_get_rate(FXResampler* fx) {
    return fx ? fx->rate : 1.0f;
}

int fx_resampler_get_required_input_frames(int output_frames, float rate) {
    return (int)(output_frames * rate) + 8;  // +8 margin for sinc8 (needs -3 to +4)
}

const char* fx_resampler_get_mode_name(ResamplerMode mode) {
    switch (mode) {
        case RESAMPLER_NEAREST: return "Nearest";
        case RESAMPLER_LINEAR: return "Linear";
        case RESAMPLER_CUBIC: return "Cubic";
        case RESAMPLER_SINC8: return "Sinc8";
        default: return "Unknown";
    }
}

// ============================================================================
// Generic Parameter Interface
// ============================================================================

enum {
    PARAM_ENABLED = 0,
    PARAM_MODE,
    PARAM_RATE,
    PARAM_COUNT
};

int fx_resampler_get_parameter_count(void) {
    return PARAM_COUNT;
}

float fx_resampler_get_parameter_value(FXResampler* fx, int index) {
    if (!fx) return 0.0f;

    switch (index) {
        case PARAM_ENABLED: return fx->enabled ? 1.0f : 0.0f;
        case PARAM_MODE: return (float)fx->mode / (float)(RESAMPLER_NUM_MODES - 1);
        case PARAM_RATE: return (fx->rate - 0.25f) / (4.0f - 0.25f);  // Normalize 0.25-4.0 to 0.0-1.0
        default: return 0.0f;
    }
}

void fx_resampler_set_parameter_value(FXResampler* fx, int index, float value) {
    if (!fx) return;

    switch (index) {
        case PARAM_ENABLED:
            fx_resampler_set_enabled(fx, value >= 0.5f);
            break;
        case PARAM_MODE:
            fx_resampler_set_mode(fx, (ResamplerMode)(value * (float)(RESAMPLER_NUM_MODES - 1) + 0.5f));
            break;
        case PARAM_RATE:
            fx_resampler_set_rate(fx, value * (4.0f - 0.25f) + 0.25f);  // Denormalize
            break;
    }
}

const char* fx_resampler_get_parameter_name(int index) {
    switch (index) {
        case PARAM_ENABLED: return "Enabled";
        case PARAM_MODE: return "Interpolation";
        case PARAM_RATE: return "Rate";
        default: return "";
    }
}

const char* fx_resampler_get_parameter_label(int index) {
    switch (index) {
        case PARAM_ENABLED: return "";
        case PARAM_MODE: return "";
        case PARAM_RATE: return "x";
        default: return "";
    }
}

float fx_resampler_get_parameter_default(int index) {
    switch (index) {
        case PARAM_ENABLED: return 1.0f;
        case PARAM_MODE: return (float)RESAMPLER_CUBIC / (float)(RESAMPLER_NUM_MODES - 1);
        case PARAM_RATE: return (1.0f - 0.25f) / (4.0f - 0.25f);  // 1.0x normalized
        default: return 0.0f;
    }
}

float fx_resampler_get_parameter_min(int index) {
    switch (index) {
        case PARAM_ENABLED: return 0.0f;
        case PARAM_MODE: return 0.0f;
        case PARAM_RATE: return 0.25f;
        default: return 0.0f;
    }
}

float fx_resampler_get_parameter_max(int index) {
    switch (index) {
        case PARAM_ENABLED: return 1.0f;
        case PARAM_MODE: return (float)(RESAMPLER_NUM_MODES - 1);
        case PARAM_RATE: return 4.0f;
        default: return 1.0f;
    }
}

int fx_resampler_get_parameter_group(int index) {
    (void)index;
    return 0;
}

const char* fx_resampler_get_group_name(int group) {
    return (group == 0) ? "Resampler" : "";
}

int fx_resampler_parameter_is_integer(int index) {
    return (index == PARAM_ENABLED || index == PARAM_MODE);
}
