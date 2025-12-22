/*
 * Regroove Reverb Implementation
 * Based on Schroeder/Moorer reverb with comb and allpass filters
 */

#include "fx_reverb.h"
#include <stdlib.h>
#include <string.h>

// Comb filter delays (in samples at 48kHz) - prime numbers for natural sound
#define NUM_COMBS 4
static const int COMB_DELAYS[NUM_COMBS] = { 1557, 1617, 1491, 1422 };

// Allpass filter delays
#define NUM_ALLPASS 2
static const int ALLPASS_DELAYS[NUM_ALLPASS] = { 225, 341 };

#define MAX_DELAY 2048

typedef struct {
    float* buffer;
    int size;
    int pos;
    float feedback;
    float damp;
    float damp_state;
} CombFilter;

typedef struct {
    float* buffer;
    int size;
    int pos;
} AllpassFilter;

struct FXReverb {
    // Parameters
    int enabled;
    float size;       // 0.0 - 1.0
    float damping;    // 0.0 - 1.0
    float mix;        // 0.0 - 1.0

    // Filter banks (stereo - slightly different delays for L/R)
    CombFilter combs_l[NUM_COMBS];
    CombFilter combs_r[NUM_COMBS];
    AllpassFilter allpass_l[NUM_ALLPASS];
    AllpassFilter allpass_r[NUM_ALLPASS];
};

static void comb_init(CombFilter* cf, int delay)
{
    cf->size = delay;
    cf->buffer = (float*)calloc(MAX_DELAY, sizeof(float));
    cf->pos = 0;
    cf->feedback = 0.5f;
    cf->damp = 0.5f;
    cf->damp_state = 0.0f;
}

static void comb_free(CombFilter* cf)
{
    if (cf->buffer) free(cf->buffer);
}

static float comb_process(CombFilter* cf, float input)
{
    float output = cf->buffer[cf->pos];
    
    // Damping (one-pole lowpass)
    cf->damp_state = output * (1.0f - cf->damp) + cf->damp_state * cf->damp;
    
    cf->buffer[cf->pos] = input + cf->damp_state * cf->feedback;
    
    cf->pos++;
    if (cf->pos >= cf->size) cf->pos = 0;
    
    return output;
}

static void allpass_init(AllpassFilter* ap, int delay)
{
    ap->size = delay;
    ap->buffer = (float*)calloc(MAX_DELAY, sizeof(float));
    ap->pos = 0;
}

static void allpass_free(AllpassFilter* ap)
{
    if (ap->buffer) free(ap->buffer);
}

static float allpass_process(AllpassFilter* ap, float input)
{
    float delayed = ap->buffer[ap->pos];
    float output = -input + delayed;
    
    ap->buffer[ap->pos] = input + delayed * 0.5f;
    
    ap->pos++;
    if (ap->pos >= ap->size) ap->pos = 0;
    
    return output;
}

FXReverb* fx_reverb_create(void)
{
    FXReverb* fx = (FXReverb*)malloc(sizeof(FXReverb));
    if (!fx) return NULL;

    fx->enabled = 0;
    fx->size = 0.0f;
    fx->damping = 1.0f;
    fx->mix = 0.3f;

    // Initialize comb filters
    for (int i = 0; i < NUM_COMBS; i++) {
        comb_init(&fx->combs_l[i], COMB_DELAYS[i]);
        comb_init(&fx->combs_r[i], COMB_DELAYS[i] + 23); // Slight offset for stereo
    }

    // Initialize allpass filters
    for (int i = 0; i < NUM_ALLPASS; i++) {
        allpass_init(&fx->allpass_l[i], ALLPASS_DELAYS[i]);
        allpass_init(&fx->allpass_r[i], ALLPASS_DELAYS[i] + 7); // Slight offset for stereo
    }

    return fx;
}

void fx_reverb_destroy(FXReverb* fx)
{
    if (!fx) return;
    
    for (int i = 0; i < NUM_COMBS; i++) {
        comb_free(&fx->combs_l[i]);
        comb_free(&fx->combs_r[i]);
    }
    
    for (int i = 0; i < NUM_ALLPASS; i++) {
        allpass_free(&fx->allpass_l[i]);
        allpass_free(&fx->allpass_r[i]);
    }
    
    free(fx);
}

void fx_reverb_reset(FXReverb* fx)
{
    if (!fx) return;

    for (int i = 0; i < NUM_COMBS; i++) {
        if (fx->combs_l[i].buffer) 
            memset(fx->combs_l[i].buffer, 0, MAX_DELAY * sizeof(float));
        if (fx->combs_r[i].buffer) 
            memset(fx->combs_r[i].buffer, 0, MAX_DELAY * sizeof(float));
        fx->combs_l[i].damp_state = 0.0f;
        fx->combs_r[i].damp_state = 0.0f;
    }

    for (int i = 0; i < NUM_ALLPASS; i++) {
        if (fx->allpass_l[i].buffer) 
            memset(fx->allpass_l[i].buffer, 0, MAX_DELAY * sizeof(float));
        if (fx->allpass_r[i].buffer) 
            memset(fx->allpass_r[i].buffer, 0, MAX_DELAY * sizeof(float));
    }
}

void fx_reverb_process_frame(FXReverb* fx, float* left, float* right, int sample_rate)
{
    if (!fx || !fx->enabled) return;

    (void)sample_rate; // Delays are fixed for 48kHz

    // Update filter parameters based on size and damping
    float feedback = 0.28f + fx->size * 0.7f;
    float damp = fx->damping;
    
    for (int i = 0; i < NUM_COMBS; i++) {
        fx->combs_l[i].feedback = feedback;
        fx->combs_l[i].damp = damp;
        fx->combs_r[i].feedback = feedback;
        fx->combs_r[i].damp = damp;
    }

    float dry_l = *left;
    float dry_r = *right;
    
    // Process through comb filters (parallel)
    float wet_l = 0.0f;
    float wet_r = 0.0f;
    
    for (int i = 0; i < NUM_COMBS; i++) {
        wet_l += comb_process(&fx->combs_l[i], dry_l);
        wet_r += comb_process(&fx->combs_r[i], dry_r);
    }
    
    wet_l *= 0.25f; // Scale down after summing
    wet_r *= 0.25f;
    
    // Process through allpass filters (series)
    for (int i = 0; i < NUM_ALLPASS; i++) {
        wet_l = allpass_process(&fx->allpass_l[i], wet_l);
        wet_r = allpass_process(&fx->allpass_r[i], wet_r);
    }
    
    // Mix dry and wet
    *left = dry_l * (1.0f - fx->mix) + wet_l * fx->mix;
    *right = dry_r * (1.0f - fx->mix) + wet_r * fx->mix;
}

void fx_reverb_process_f32(FXReverb* fx, float* buffer, int frames, int sample_rate)
{
    if (!fx || !fx->enabled) return;

    for (int i = 0; i < frames; i++) {
        fx_reverb_process_frame(fx, &buffer[i * 2], &buffer[i * 2 + 1], sample_rate);
    }
}

void fx_reverb_process_i16(FXReverb* fx, int16_t* buffer, int frames, int sample_rate)
{
    if (!fx || !fx->enabled) return;

    for (int i = 0; i < frames; i++) {
        float left = buffer[i * 2] / 32768.0f;
        float right = buffer[i * 2 + 1] / 32768.0f;

        fx_reverb_process_frame(fx, &left, &right, sample_rate);

        buffer[i * 2] = (int16_t)(left * 32767.0f);
        buffer[i * 2 + 1] = (int16_t)(right * 32767.0f);
    }
}

void fx_reverb_set_enabled(FXReverb* fx, int enabled)
{
    if (fx) fx->enabled = enabled;
}

void fx_reverb_set_size(FXReverb* fx, float size)
{
    if (fx) fx->size = size < 0.0f ? 0.0f : (size > 1.0f ? 1.0f : size);
}

void fx_reverb_set_damping(FXReverb* fx, float damping)
{
    if (fx) fx->damping = damping < 0.0f ? 0.0f : (damping > 1.0f ? 1.0f : damping);
}

void fx_reverb_set_mix(FXReverb* fx, float mix)
{
    if (fx) fx->mix = mix < 0.0f ? 0.0f : (mix > 1.0f ? 1.0f : mix);
}

int fx_reverb_get_enabled(FXReverb* fx)
{
    return fx ? fx->enabled : 0;
}

float fx_reverb_get_size(FXReverb* fx)
{
    return fx ? fx->size : 0.0f;
}

float fx_reverb_get_damping(FXReverb* fx)
{
    return fx ? fx->damping : 0.0f;
}

float fx_reverb_get_mix(FXReverb* fx)
{
    return fx ? fx->mix : 0.0f;
}
// ============================================================================
// Generic Parameter Interface
// ============================================================================

#include "../param_interface.h"

// Parameter groups
typedef enum {
    FX_REVERB_GROUP_MAIN = 0,
    FX_REVERB_GROUP_COUNT
} FXReverbParamGroup;

// Parameter indices
typedef enum {
    FX_REVERB_PARAM_SIZE = 0,
    FX_REVERB_PARAM_DAMPING,
    FX_REVERB_PARAM_MIX,
    FX_REVERB_PARAM_COUNT
} FXReverbParamIndex;

// Parameter metadata (ALL VALUES NORMALIZED 0.0-1.0)
static const ParameterInfo reverb_params[FX_REVERB_PARAM_COUNT] = {
    {"Size", "%", 0.5f, 0.0f, 1.0f, FX_REVERB_GROUP_MAIN, 0},
    {"Damping", "%", 0.5f, 0.0f, 1.0f, FX_REVERB_GROUP_MAIN, 0},
    {"Mix", "%", 0.3f, 0.0f, 1.0f, FX_REVERB_GROUP_MAIN, 0}
};

static const char* group_names[FX_REVERB_GROUP_COUNT] = {
    "Reverb"
};

int fx_reverb_get_parameter_count(void)
{
    return FX_REVERB_PARAM_COUNT;
}

float fx_reverb_get_parameter_value(FXReverb* fx, int index)
{
    if (!fx || index < 0 || index >= FX_REVERB_PARAM_COUNT) return 0.0f;

    switch (index) {
        case FX_REVERB_PARAM_SIZE:
            return fx_reverb_get_size(fx);
        case FX_REVERB_PARAM_DAMPING:
            return fx_reverb_get_damping(fx);
        case FX_REVERB_PARAM_MIX:
            return fx_reverb_get_mix(fx);
        default:
            return 0.0f;
    }
}

void fx_reverb_set_parameter_value(FXReverb* fx, int index, float value)
{
    if (!fx || index < 0 || index >= FX_REVERB_PARAM_COUNT) return;

    switch (index) {
        case FX_REVERB_PARAM_SIZE:
            fx_reverb_set_size(fx, value);
            break;
        case FX_REVERB_PARAM_DAMPING:
            fx_reverb_set_damping(fx, value);
            break;
        case FX_REVERB_PARAM_MIX:
            fx_reverb_set_mix(fx, value);
            break;
    }
}

// Generate all metadata accessor functions using shared macro
DEFINE_PARAM_METADATA_ACCESSORS(fx_reverb, reverb_params, FX_REVERB_PARAM_COUNT, group_names, FX_REVERB_GROUP_COUNT)
