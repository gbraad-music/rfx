#include <math.h>
#include <stdlib.h>
#include <stdbool.h>
#include "fx_common.h"
#include "fx_stereo_widen.h"

struct FXStereoWiden {
    bool enabled;
    float width; // 0..1
    float mix;   // 0..1
};

FXStereoWiden* fx_stereo_widen_create(void)
{
    FXStereoWiden* fx = (FXStereoWiden*)calloc(1, sizeof(FXStereoWiden));
    if (!fx) return NULL;
    fx->enabled = true;
    fx->width = 0.5f;
    fx->mix = 0.5f;
    return fx;
}

void fx_stereo_widen_destroy(FXStereoWiden* fx)
{ if (fx) free(fx); }

void fx_stereo_widen_reset(FXStereoWiden* fx)
{ (void)fx; }

void fx_stereo_widen_set_enabled(FXStereoWiden* fx, bool enabled)
{ if (fx) fx->enabled = enabled; }

void fx_stereo_widen_set_width(FXStereoWiden* fx, fx_param_t width)
{ if (fx) fx->width = fmaxf(0.f, fminf(2.f, width)); } // allow up to 200%
void fx_stereo_widen_set_mix(FXStereoWiden* fx, fx_param_t mix)
{ if (fx) fx->mix = fmaxf(0.f, fminf(1.f, mix)); }

bool fx_stereo_widen_get_enabled(FXStereoWiden* fx)
{ return fx ? fx->enabled : false; }

float fx_stereo_widen_get_width(FXStereoWiden* fx)
{ return fx ? fx->width : 0.5f; }

float fx_stereo_widen_get_mix(FXStereoWiden* fx)
{ return fx ? fx->mix : 0.5f; }

// ============================================================================
// Generic Parameter Interface
// ============================================================================

#include "../param_interface.h"

// Parameter groups
typedef enum {
    FX_STEREO_WIDEN_GROUP_MAIN = 0,
    FX_STEREO_WIDEN_GROUP_COUNT
} FXStereoWidenParamGroup;

// Parameter indices
typedef enum {
    FX_STEREO_WIDEN_PARAM_WIDTH = 0,
    FX_STEREO_WIDEN_PARAM_MIX,
    FX_STEREO_WIDEN_PARAM_COUNT
} FXStereoWidenParamIndex;

// Parameter metadata (ALL VALUES NORMALIZED 0.0-1.0)
static const ParameterInfo stereo_widen_params[FX_STEREO_WIDEN_PARAM_COUNT] = {
    {"Width", "%", 0.5f, 0.0f, 1.0f, FX_STEREO_WIDEN_GROUP_MAIN, 0},
    {"Mix", "%", 0.5f, 0.0f, 1.0f, FX_STEREO_WIDEN_GROUP_MAIN, 0}
};

static const char* group_names[FX_STEREO_WIDEN_GROUP_COUNT] = {
    "Stereo Widener"
};

int fx_stereo_widen_get_parameter_count(void)
{
    return FX_STEREO_WIDEN_PARAM_COUNT;
}

float fx_stereo_widen_get_parameter_value(FXStereoWiden* fx, int index)
{
    if (!fx || index < 0 || index >= FX_STEREO_WIDEN_PARAM_COUNT) return 0.0f;

    switch (index) {
        case FX_STEREO_WIDEN_PARAM_WIDTH:
            return fx_stereo_widen_get_width(fx);
        case FX_STEREO_WIDEN_PARAM_MIX:
            return fx_stereo_widen_get_mix(fx);
        default:
            return 0.0f;
    }
}

void fx_stereo_widen_set_parameter_value(FXStereoWiden* fx, int index, float value)
{
    if (!fx || index < 0 || index >= FX_STEREO_WIDEN_PARAM_COUNT) return;

    switch (index) {
        case FX_STEREO_WIDEN_PARAM_WIDTH:
            fx_stereo_widen_set_width(fx, value);
            break;
        case FX_STEREO_WIDEN_PARAM_MIX:
            fx_stereo_widen_set_mix(fx, value);
            break;
    }
}

// Generate all metadata accessor functions using shared macro
DEFINE_PARAM_METADATA_ACCESSORS(fx_stereo_widen, stereo_widen_params, FX_STEREO_WIDEN_PARAM_COUNT, group_names, FX_STEREO_WIDEN_GROUP_COUNT)

static inline void ms_encode(float l, float r, float* m, float* s)
{ *m = 0.5f*(l + r); *s = 0.5f*(l - r); }

static inline void ms_decode(float m, float s, float* l, float* r)
{ *l = m + s; *r = m - s; }

void fx_stereo_widen_process_f32(FXStereoWiden* fx, const float* inL, const float* inR,
                                 float* outL, float* outR, int frames, int sample_rate)
{
    (void)sample_rate;
    if (!fx || !fx->enabled) {
        // pass-through
        for (int i = 0; i < frames; ++i) { outL[i] = inL[i]; outR[i] = inR[i]; }
        return;
    }

    const float sideGain = 4.0f * fx->width; // stronger widening
    const float midAtten = 1.0f - 0.25f * fx->width; // slightly reduce mid
    const float wet = fx->mix;
    const float dry = 1.0f - wet;

    for (int i = 0; i < frames; ++i) {
        float m, s, l, r;
        ms_encode(inL[i], inR[i], &m, &s);
        m *= midAtten;
        s *= sideGain;
        ms_decode(m, s, &l, &r);
        outL[i] = dry*inL[i] + wet*l;
        outR[i] = dry*inR[i] + wet*r;
    }
}

void fx_stereo_widen_process_frame(FXStereoWiden* fx, float* left, float* right, int sample_rate)
{
    (void)sample_rate;
    if (!fx || !left || !right) return;

    if (!fx->enabled) {
        return; // pass-through, values already in place
    }

    const float inL = *left;
    const float inR = *right;

    const float sideGain = 4.0f * fx->width; // stronger widening
    const float midAtten = 1.0f - 0.25f * fx->width; // slightly reduce mid
    const float wet = fx->mix;
    const float dry = 1.0f - wet;

    float m, s, l, r;
    ms_encode(inL, inR, &m, &s);
    m *= midAtten;
    s *= sideGain;
    ms_decode(m, s, &l, &r);

    *left = dry * inL + wet * l;
    *right = dry * inR + wet * r;
}

void fx_stereo_widen_process_interleaved(FXStereoWiden* fx, float* interleavedLR,
                                         int frames, int sample_rate)
{
    (void)sample_rate;
    if (!fx || !fx->enabled) {
        return; // in-place, do nothing
    }
    const float sideGain = 4.0f * fx->width; // stronger widening
    const float midAtten = 1.0f - 0.25f * fx->width; // slightly reduce mid
    const float wet = fx->mix;
    const float dry = 1.0f - wet;
    for (int i = 0; i < frames; ++i) {
        const float inL = interleavedLR[i*2];
        const float inR = interleavedLR[i*2+1];
        float m, s, l, r;
        ms_encode(inL, inR, &m, &s);
        m *= midAtten;
        s *= sideGain;
        ms_decode(m, s, &l, &r);
        interleavedLR[i*2]   = dry*inL + wet*l;
        interleavedLR[i*2+1] = dry*inR + wet*r;
    }
}
