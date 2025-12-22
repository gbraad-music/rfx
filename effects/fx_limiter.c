/*
 * Simple Transparent Limiter
 * True lookahead, instant attack, smooth release, stereo linked
 */

#include "fx_limiter.h"
#include "windows_compat.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define MAX_LOOKAHEAD 512   // ~10ms at 48kHz

struct FXLimiter {
    int enabled;

    float ceiling;     // 0.0–1.0 → -12dB to 0dB
    float release;     // 0.0–1.0 → 20ms to 1000ms
    float lookahead;   // 0.0–1.0 → 0ms to 10ms

    float bufferL[MAX_LOOKAHEAD];
    float bufferR[MAX_LOOKAHEAD];
    int writePos;
    int size;

    float envelope;
    float gainReduction;

    // Smoothed parameters to avoid crackling
    float ceilingSmoothed;
    float releaseSmoothed;
};

static inline float db_to_lin(float db) {
    return powf(10.0f, db / 20.0f);
}

static inline float lin_to_db(float x) {
    return 20.0f * log10f(fmaxf(x, 1e-6f));
}

FXLimiter* fx_limiter_create(void)
{
    FXLimiter* fx = calloc(1, sizeof(FXLimiter));
    if (!fx) return NULL;

    fx->enabled = 0;
    fx->ceiling = 1.0f;   // 0dB
    fx->release = 0.3f;   // ~200ms
    fx->lookahead = 0.3f; // ~3ms
    fx->envelope = 1.0f;

    // Initialize smoothed parameters
    fx->ceilingSmoothed = 1.0f;
    fx->releaseSmoothed = 0.3f;

    // Initialize size
    fx->size = (int)(fx->lookahead * MAX_LOOKAHEAD);
    if (fx->size < 1) fx->size = 1;
    if (fx->size > MAX_LOOKAHEAD) fx->size = MAX_LOOKAHEAD;

    return fx;
}

void fx_limiter_destroy(FXLimiter* fx)
{
    if (fx) free(fx);
}

void fx_limiter_reset(FXLimiter* fx)
{
    if (!fx) return;

    memset(fx->bufferL, 0, sizeof(fx->bufferL));
    memset(fx->bufferR, 0, sizeof(fx->bufferR));
    fx->writePos = 0;
    fx->envelope = 1.0f;
    fx->gainReduction = 0.0f;
}

void fx_limiter_process_frame(FXLimiter* fx, float* L, float* R, int sr)
{
    if (!fx || !fx->enabled) return;

    // Write incoming samples
    fx->bufferL[fx->writePos] = *L;
    fx->bufferR[fx->writePos] = *R;

    // Compute read position (future sample)
    int readPos = fx->writePos + 1;
    if (readPos >= fx->size) readPos = 0;

    float futureL = fx->bufferL[readPos];
    float futureR = fx->bufferR[readPos];

    // Stereo peak detection
    float peak = fmaxf(fabsf(futureL), fabsf(futureR));

    // Smooth ceiling to avoid crackling
    float ceilingTarget = db_to_lin(-12.0f + fx->ceiling * 12.0f);
    fx->ceilingSmoothed += 0.001f * (ceilingTarget - fx->ceilingSmoothed);
    float ceiling = fx->ceilingSmoothed;

    // Compute target gain
    float target = 1.0f;
    if (peak > ceiling)
        target = ceiling / peak;

    // Smooth release to avoid crackling
    fx->releaseSmoothed += 0.001f * (fx->release - fx->releaseSmoothed);
    float releaseMs = 20.0f + fx->releaseSmoothed * 980.0f;
    float releaseCoeff = expf(-1.0f / (sr * (releaseMs / 1000.0f)));

    // Envelope: instant attack, smooth release
    if (target < fx->envelope)
        fx->envelope = target;
    else
        fx->envelope = fx->envelope * releaseCoeff + (1.0f - releaseCoeff);

    fx->gainReduction = lin_to_db(fx->envelope);

    // Apply gain to delayed sample
    *L = futureL * fx->envelope;
    *R = futureR * fx->envelope;

    // Advance write pointer
    fx->writePos++;
    if (fx->writePos >= fx->size)
        fx->writePos = 0;
}

void fx_limiter_process_f32(FXLimiter* fx, float* buf, int frames, int sr)
{
    if (!fx) return;

    for (int i = 0; i < frames; i++) {
        float L = buf[i*2];
        float R = buf[i*2+1];
        fx_limiter_process_frame(fx, &L, &R, sr);
        buf[i*2]   = L;
        buf[i*2+1] = R;
    }
}

void fx_limiter_process_i16(FXLimiter* fx, int16_t* buf, int frames, int sr)
{
    if (!fx) return;

    for (int i = 0; i < frames; i++) {
        float L = buf[i*2] / 32768.0f;
        float R = buf[i*2+1] / 32768.0f;

        fx_limiter_process_frame(fx, &L, &R, sr);

        buf[i*2]   = (int16_t)(L * 32767.0f);
        buf[i*2+1] = (int16_t)(R * 32767.0f);
    }
}

// Parameter setters
void fx_limiter_set_enabled(FXLimiter* fx, int enabled) {
    if (fx) fx->enabled = enabled;
}

void fx_limiter_set_threshold(FXLimiter* fx, float threshold) {
    // Threshold parameter unused in this simplified design
    // (keeping for API compatibility)
    (void)fx;
    (void)threshold;
}

void fx_limiter_set_release(FXLimiter* fx, float release) {
    if (fx) fx->release = release;
}

void fx_limiter_set_ceiling(FXLimiter* fx, float ceiling) {
    if (fx) fx->ceiling = ceiling;
}

void fx_limiter_set_lookahead(FXLimiter* fx, float lookahead) {
    if (!fx) return;

    fx->lookahead = lookahead;
    fx->size = (int)(lookahead * MAX_LOOKAHEAD);
    if (fx->size < 1) fx->size = 1;
    if (fx->size > MAX_LOOKAHEAD) fx->size = MAX_LOOKAHEAD;
}

// Parameter getters
int fx_limiter_get_enabled(FXLimiter* fx) {
    return fx ? fx->enabled : 0;
}

float fx_limiter_get_threshold(FXLimiter* fx) {
    return fx ? 0.75f : 0.75f;  // Unused, return default
}

float fx_limiter_get_release(FXLimiter* fx) {
    return fx ? fx->release : 0.3f;
}

float fx_limiter_get_ceiling(FXLimiter* fx) {
    return fx ? fx->ceiling : 1.0f;
}

float fx_limiter_get_lookahead(FXLimiter* fx) {
    return fx ? fx->lookahead : 0.3f;
}

float fx_limiter_get_gain_reduction(FXLimiter* fx) {
    return fx ? fx->gainReduction : 0.0f;
}

// ============================================================================
// Generic Parameter Interface
// ============================================================================

#include "../param_interface.h"

typedef enum {
    FX_LIMITER_GROUP_MAIN = 0,
    FX_LIMITER_GROUP_COUNT
} FXLimiterParamGroup;

typedef enum {
    FX_LIMITER_PARAM_THRESHOLD = 0,
    FX_LIMITER_PARAM_RELEASE,
    FX_LIMITER_PARAM_CEILING,
    FX_LIMITER_PARAM_LOOKAHEAD,
    FX_LIMITER_PARAM_COUNT
} FXLimiterParamIndex;

// Parameter metadata (ALL VALUES NORMALIZED 0.0-1.0)
static const ParameterInfo limiter_params[FX_LIMITER_PARAM_COUNT] = {
    {"Threshold", "dB", 0.5f, 0.0f, 1.0f, FX_LIMITER_GROUP_MAIN, 0},
    {"Release", "ms", 0.5f, 0.0f, 1.0f, FX_LIMITER_GROUP_MAIN, 0},
    {"Ceiling", "dB", 0.5f, 0.0f, 1.0f, FX_LIMITER_GROUP_MAIN, 0},
    {"Lookahead", "ms", 0.3f, 0.0f, 1.0f, FX_LIMITER_GROUP_MAIN, 0}
};

static const char* group_names[FX_LIMITER_GROUP_COUNT] = {"Limiter"};

int fx_limiter_get_parameter_count(void) { return FX_LIMITER_PARAM_COUNT; }

float fx_limiter_get_parameter_value(FXLimiter* fx, int index) {
    if (!fx || index < 0 || index >= FX_LIMITER_PARAM_COUNT) return 0.0f;
    switch (index) {
        case FX_LIMITER_PARAM_THRESHOLD: return fx_limiter_get_threshold(fx);
        case FX_LIMITER_PARAM_RELEASE: return fx_limiter_get_release(fx);
        case FX_LIMITER_PARAM_CEILING: return fx_limiter_get_ceiling(fx);
        case FX_LIMITER_PARAM_LOOKAHEAD: return fx_limiter_get_lookahead(fx);
        default: return 0.0f;
    }
}

void fx_limiter_set_parameter_value(FXLimiter* fx, int index, float value) {
    if (!fx || index < 0 || index >= FX_LIMITER_PARAM_COUNT) return;
    switch (index) {
        case FX_LIMITER_PARAM_THRESHOLD: fx_limiter_set_threshold(fx, value); break;
        case FX_LIMITER_PARAM_RELEASE: fx_limiter_set_release(fx, value); break;
        case FX_LIMITER_PARAM_CEILING: fx_limiter_set_ceiling(fx, value); break;
        case FX_LIMITER_PARAM_LOOKAHEAD: fx_limiter_set_lookahead(fx, value); break;
    }
}

DEFINE_PARAM_METADATA_ACCESSORS(fx_limiter, limiter_params, FX_LIMITER_PARAM_COUNT, group_names, FX_LIMITER_GROUP_COUNT)
