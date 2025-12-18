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
