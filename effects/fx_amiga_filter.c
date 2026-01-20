/*
 * Regroove Amiga Filter Effect
 *
 * Based on OpenMPT's Paula emulation (BSD license)
 * Original implementation by Antti S. Lankila and OpenMPT Devs
 *
 * Filter design:
 * - A500: Fixed RC lowpass (4.9 kHz) + optional LED Butterworth (3275 Hz)
 * - A1200: Leakage RC lowpass (32 kHz) + optional LED Butterworth (3275 Hz)
 */

#include "fx_amiga_filter.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ============================================================================
// Biquad IIR Filter (Second-Order Direct Form II)
// ============================================================================

typedef struct {
    // Filter coefficients
    double b0, b1, b2;  // Numerator (feedforward)
    double a1, a2;      // Denominator (feedback) - a0 is normalized to 1

    // State variables (separate for stereo)
    double x1_l, x2_l, y1_l, y2_l;  // Left channel
    double x1_r, x2_r, y1_r, y2_r;  // Right channel
} BiquadFilter;

static void biquad_init(BiquadFilter* f, double b0, double b1, double b2, double a1, double a2) {
    f->b0 = b0;
    f->b1 = b1;
    f->b2 = b2;
    f->a1 = a1;
    f->a2 = a2;

    // Reset state
    f->x1_l = f->x2_l = f->y1_l = f->y2_l = 0.0;
    f->x1_r = f->x2_r = f->y1_r = f->y2_r = 0.0;
}

static void biquad_reset(BiquadFilter* f) {
    f->x1_l = f->x2_l = f->y1_l = f->y2_l = 0.0;
    f->x1_r = f->x2_r = f->y1_r = f->y2_r = 0.0;
}

// Process one sample (left channel)
static inline double biquad_process_left(BiquadFilter* f, double x0) {
    double y0 = f->b0 * x0 + f->b1 * f->x1_l + f->b2 * f->x2_l
                - f->a1 * f->y1_l - f->a2 * f->y2_l;
    f->x2_l = f->x1_l;
    f->x1_l = x0;
    f->y2_l = f->y1_l;
    f->y1_l = y0;
    return y0;
}

// Process one sample (right channel)
static inline double biquad_process_right(BiquadFilter* f, double x0) {
    double y0 = f->b0 * x0 + f->b1 * f->x1_r + f->b2 * f->x2_r
                - f->a1 * f->y1_r - f->a2 * f->y2_r;
    f->x2_r = f->x1_r;
    f->x1_r = x0;
    f->y2_r = f->y1_r;
    f->y1_r = y0;
    return y0;
}

// ============================================================================
// Filter Design Functions (from OpenMPT)
// ============================================================================

/**
 * Bilinear transform - converts analog filter to digital
 * Prewarp frequencies and apply bilinear transform
 */
static void z_transform(BiquadFilter* out,
                       double a0, double a1, double a2,  // Numerator
                       double b0, double b1, double b2,  // Denominator
                       double fc, double fs)              // Cutoff, sample rate
{
    // Prewarp s-domain coefficients
    const double wp = 2.0 * fs * tan(M_PI * fc / fs);
    a2 /= wp * wp;
    a1 /= wp;
    b2 /= wp * wp;
    b1 /= wp;

    // Compute bilinear transform
    const double bd = 4.0 * b2 * fs * fs + 2.0 * b1 * fs + b0;

    biquad_init(out,
        (4.0 * a2 * fs * fs + 2.0 * a1 * fs + a0) / bd,  // b0
        (2.0 * a0 - 8.0 * a2 * fs * fs) / bd,            // b1
        (4.0 * a2 * fs * fs - 2.0 * a1 * fs + a0) / bd,  // b2
        (2.0 * b0 - 8.0 * b2 * fs * fs) / bd,            // a1
        (4.0 * b2 * fs * fs - 2.0 * b1 * fs + b0) / bd   // a2
    );
}

/**
 * RC Lowpass Filter
 * Simple one-pole lowpass (like hardware RC circuit)
 */
static void make_rc_lowpass(BiquadFilter* out, double sample_rate, double freq) {
    const double omega = (2.0 * M_PI) * freq / sample_rate;
    const double term = 1.0 + 1.0 / omega;

    biquad_init(out,
        1.0 / term,        // b0
        0.0,               // b1
        0.0,               // b2
        -1.0 + 1.0 / term, // a1
        0.0                // a2
    );
}

/**
 * Butterworth Lowpass Filter (2nd order)
 * Maximally flat passband response
 *
 * Standard Butterworth s-domain coefficients:
 *   Numerator:   b0=1, b1=0, b2=0
 *   Denominator: a0=1, a1=sqrt(2), a2=1
 *
 * res_dB: Resonance in dB (negative values reduce Q factor)
 */
static void make_butterworth(BiquadFilter* out, double fs, double fc, double res_dB) {
    const double res = pow(10.0, (-res_dB / 10.0 / 2.0));
    z_transform(out,
        1.0, 0.0, 0.0,              // Numerator (flat)
        1.0, sqrt(2.0) * res, 1.0,  // Denominator (Butterworth)
        fc, fs
    );
}

// ============================================================================
// Amiga Filter Effect
// ============================================================================

struct FXAmigaFilter {
    int enabled;
    AmigaFilterType type;
    float mix;
    int sample_rate;

    // Filter chains (cascaded biquads)
    BiquadFilter filter1;  // First stage (RC or leakage)
    BiquadFilter filter2;  // Second stage (LED Butterworth, if enabled)

    int uses_two_filters;  // Track if we need to run both stages
};

FXAmigaFilter* fx_amiga_filter_create(void) {
    FXAmigaFilter* fx = (FXAmigaFilter*)calloc(1, sizeof(FXAmigaFilter));
    if (!fx) return NULL;

    fx->enabled = 1;
    fx->type = AMIGA_FILTER_A500_LED_ON;
    fx->mix = 1.0f;
    fx->sample_rate = 48000;
    fx->uses_two_filters = 0;

    // Initialize filters to passthrough
    biquad_init(&fx->filter1, 1.0, 0.0, 0.0, 0.0, 0.0);
    biquad_init(&fx->filter2, 1.0, 0.0, 0.0, 0.0, 0.0);

    return fx;
}

void fx_amiga_filter_destroy(FXAmigaFilter* fx) {
    if (fx) {
        free(fx);
    }
}

void fx_amiga_filter_reset(FXAmigaFilter* fx) {
    if (!fx) return;

    biquad_reset(&fx->filter1);
    biquad_reset(&fx->filter2);
}

// Update filter coefficients based on type and sample rate
static void update_filters(FXAmigaFilter* fx) {
    if (!fx) return;

    const double fs = (double)fx->sample_rate;

    switch (fx->type) {
        case AMIGA_FILTER_OFF:
        case AMIGA_FILTER_UNFILTERED:
            // Passthrough
            biquad_init(&fx->filter1, 1.0, 0.0, 0.0, 0.0, 0.0);
            biquad_init(&fx->filter2, 1.0, 0.0, 0.0, 0.0, 0.0);
            fx->uses_two_filters = 0;
            break;

        case AMIGA_FILTER_A500_LED_OFF:
            // A500: 4.9 kHz RC lowpass only
            make_rc_lowpass(&fx->filter1, fs, 4900.0);
            fx->uses_two_filters = 0;
            break;

        case AMIGA_FILTER_A500_LED_ON:
            // A500: 4.9 kHz RC lowpass + 3275 Hz LED Butterworth
            make_rc_lowpass(&fx->filter1, fs, 4900.0);
            make_butterworth(&fx->filter2, fs, 3275.0, -0.70);
            fx->uses_two_filters = 1;
            break;

        case AMIGA_FILTER_A1200_LED_OFF:
            // A1200: 32 kHz leakage lowpass only (very subtle)
            make_rc_lowpass(&fx->filter1, fs, 32000.0);
            fx->uses_two_filters = 0;
            break;

        case AMIGA_FILTER_A1200_LED_ON:
            // A1200: 32 kHz leakage + 3275 Hz LED Butterworth
            make_rc_lowpass(&fx->filter1, fs, 32000.0);
            make_butterworth(&fx->filter2, fs, 3275.0, -0.70);
            fx->uses_two_filters = 1;
            break;
    }
}

void fx_amiga_filter_process_frame(FXAmigaFilter* fx, float* left, float* right, int sample_rate) {
    if (!fx || !fx->enabled || fx->type == AMIGA_FILTER_OFF) {
        return;
    }

    // Update filters if sample rate changed
    if (fx->sample_rate != sample_rate) {
        fx->sample_rate = sample_rate;
        update_filters(fx);
    }

    // Store dry signal
    const float dry_l = *left;
    const float dry_r = *right;

    // Process through filter chain
    double out_l = (double)(*left);
    double out_r = (double)(*right);

    // Stage 1 (always active for filtered modes)
    out_l = biquad_process_left(&fx->filter1, out_l);
    out_r = biquad_process_right(&fx->filter1, out_r);

    // Stage 2 (LED filter, if enabled)
    if (fx->uses_two_filters) {
        out_l = biquad_process_left(&fx->filter2, out_l);
        out_r = biquad_process_right(&fx->filter2, out_r);
    }

    // Apply dry/wet mix
    *left = dry_l + fx->mix * ((float)out_l - dry_l);
    *right = dry_r + fx->mix * ((float)out_r - dry_r);
}

void fx_amiga_filter_process_f32(FXAmigaFilter* fx, float* buffer, int frames, int sample_rate) {
    if (!fx || !buffer) return;

    for (int i = 0; i < frames; i++) {
        float left = buffer[i * 2];
        float right = buffer[i * 2 + 1];
        fx_amiga_filter_process_frame(fx, &left, &right, sample_rate);
        buffer[i * 2] = left;
        buffer[i * 2 + 1] = right;
    }
}

void fx_amiga_filter_process_i16(FXAmigaFilter* fx, int16_t* buffer, int frames, int sample_rate) {
    if (!fx || !buffer) return;

    const float i2f = 1.0f / 32768.0f;
    const float f2i = 32768.0f;

    for (int i = 0; i < frames; i++) {
        float left = buffer[i * 2] * i2f;
        float right = buffer[i * 2 + 1] * i2f;
        fx_amiga_filter_process_frame(fx, &left, &right, sample_rate);
        buffer[i * 2] = (int16_t)(left * f2i);
        buffer[i * 2 + 1] = (int16_t)(right * f2i);
    }
}

// ============================================================================
// Parameter Interface
// ============================================================================

void fx_amiga_filter_set_enabled(FXAmigaFilter* fx, int enabled) {
    if (fx) fx->enabled = enabled;
}

void fx_amiga_filter_set_type(FXAmigaFilter* fx, AmigaFilterType type) {
    if (!fx) return;
    if (fx->type != type) {
        fx->type = type;
        update_filters(fx);
    }
}

void fx_amiga_filter_set_mix(FXAmigaFilter* fx, float mix) {
    if (fx) fx->mix = mix < 0.0f ? 0.0f : (mix > 1.0f ? 1.0f : mix);
}

int fx_amiga_filter_get_enabled(FXAmigaFilter* fx) {
    return fx ? fx->enabled : 0;
}

AmigaFilterType fx_amiga_filter_get_type(FXAmigaFilter* fx) {
    return fx ? fx->type : AMIGA_FILTER_OFF;
}

float fx_amiga_filter_get_mix(FXAmigaFilter* fx) {
    return fx ? fx->mix : 1.0f;
}

// ============================================================================
// Generic Parameter Interface
// ============================================================================

enum {
    PARAM_ENABLED = 0,
    PARAM_TYPE,
    PARAM_MIX,
    PARAM_COUNT
};

int fx_amiga_filter_get_parameter_count(void) {
    return PARAM_COUNT;
}

float fx_amiga_filter_get_parameter_value(FXAmigaFilter* fx, int index) {
    if (!fx) return 0.0f;

    switch (index) {
        case PARAM_ENABLED: return fx->enabled ? 1.0f : 0.0f;
        case PARAM_TYPE: return (float)fx->type / (float)(AMIGA_FILTER_UNFILTERED);
        case PARAM_MIX: return fx->mix;
        default: return 0.0f;
    }
}

void fx_amiga_filter_set_parameter_value(FXAmigaFilter* fx, int index, float value) {
    if (!fx) return;

    switch (index) {
        case PARAM_ENABLED:
            fx_amiga_filter_set_enabled(fx, value >= 0.5f);
            break;
        case PARAM_TYPE:
            fx_amiga_filter_set_type(fx, (AmigaFilterType)(value * (float)AMIGA_FILTER_UNFILTERED + 0.5f));
            break;
        case PARAM_MIX:
            fx_amiga_filter_set_mix(fx, value);
            break;
    }
}

const char* fx_amiga_filter_get_parameter_name(int index) {
    switch (index) {
        case PARAM_ENABLED: return "Enabled";
        case PARAM_TYPE: return "Filter Type";
        case PARAM_MIX: return "Mix";
        default: return "";
    }
}

const char* fx_amiga_filter_get_parameter_label(int index) {
    switch (index) {
        case PARAM_ENABLED: return "";
        case PARAM_TYPE: return "";
        case PARAM_MIX: return "%";
        default: return "";
    }
}

float fx_amiga_filter_get_parameter_default(int index) {
    switch (index) {
        case PARAM_ENABLED: return 1.0f;
        case PARAM_TYPE: return (float)AMIGA_FILTER_A500_LED_ON / (float)AMIGA_FILTER_UNFILTERED;
        case PARAM_MIX: return 1.0f;
        default: return 0.0f;
    }
}

float fx_amiga_filter_get_parameter_min(int index) {
    switch (index) {
        case PARAM_ENABLED: return 0.0f;
        case PARAM_TYPE: return 0.0f;
        case PARAM_MIX: return 0.0f;
        default: return 0.0f;
    }
}

float fx_amiga_filter_get_parameter_max(int index) {
    switch (index) {
        case PARAM_ENABLED: return 1.0f;
        case PARAM_TYPE: return 5.0f;  // Number of filter types
        case PARAM_MIX: return 1.0f;
        default: return 1.0f;
    }
}

int fx_amiga_filter_get_parameter_group(int index) {
    (void)index;
    return 0;  // All in same group
}

const char* fx_amiga_filter_get_group_name(int group) {
    return (group == 0) ? "Amiga Filter" : "";
}

int fx_amiga_filter_parameter_is_integer(int index) {
    return (index == PARAM_ENABLED || index == PARAM_TYPE);
}
