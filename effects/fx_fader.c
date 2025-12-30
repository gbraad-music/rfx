/*
 * Regroove DJ Fader Effect
 * Simple volume fader with smooth transitions
 */

#include <math.h>
#include <stdlib.h>
#include "fx_common.h"
#include "fx_fader.h"

struct FXFader {
	int enabled;
	float level;         // 0.0 - 1.0
	float smooth_level;  // Smoothed for zipper noise prevention
};

FXFader* fx_fader_create(void)
{
	FXFader* fx = (FXFader*)calloc(1, sizeof(FXFader));
	if (!fx) return NULL;
	fx->enabled = 1;
	fx->level = 1.0f;
	fx->smooth_level = 1.0f;
	return fx;
}

void fx_fader_destroy(FXFader* fx)
{
	if (fx) free(fx);
}

void fx_fader_reset(FXFader* fx)
{
	if (!fx) return;
	fx->smooth_level = fx->level;
}

void fx_fader_set_enabled(FXFader* fx, int enabled)
{
	if (fx) fx->enabled = enabled;
}

void fx_fader_set_level(FXFader* fx, float level)
{
	if (fx) fx->level = fmaxf(0.0f, fminf(1.0f, level));
}

int fx_fader_get_enabled(FXFader* fx)
{
	return fx ? fx->enabled : 0;
}

float fx_fader_get_level(FXFader* fx)
{
	return fx ? fx->level : 1.0f;
}

void fx_fader_process_frame(FXFader* fx, float* left, float* right, int sample_rate)
{
	(void)sample_rate;
	if (!fx || !left || !right) return;

	if (!fx->enabled) {
		return; // pass-through
	}

	// Smooth the level to prevent zipper noise (one-pole lowpass)
	const float smoothing = 0.001f; // Very fast smoothing for DJ fader
	fx->smooth_level += (fx->level - fx->smooth_level) * smoothing;

	float gain = fx->smooth_level;

	*left *= gain;
	*right *= gain;
}

void fx_fader_process_f32(FXFader* fx, float* buffer, int frames, int sample_rate)
{
	(void)sample_rate;
	if (!fx || !buffer) return;

	if (!fx->enabled) {
		return; // pass-through
	}

	const float smoothing = 0.001f;

	for (int i = 0; i < frames * 2; i += 2) {
		// Smooth the level
		fx->smooth_level += (fx->level - fx->smooth_level) * smoothing;
		float gain = fx->smooth_level;

		buffer[i] *= gain;     // Left
		buffer[i + 1] *= gain; // Right
	}
}

void fx_fader_process_i16(FXFader* fx, int16_t* buffer, int frames, int sample_rate)
{
	(void)sample_rate;
	if (!fx || !buffer) return;

	if (!fx->enabled) {
		return; // pass-through
	}

	const float smoothing = 0.001f;

	for (int i = 0; i < frames * 2; i += 2) {
		// Smooth the level
		fx->smooth_level += (fx->level - fx->smooth_level) * smoothing;
		float gain = fx->smooth_level;

		float left = buffer[i] * gain;
		float right = buffer[i + 1] * gain;

		// Clip to int16 range
		buffer[i] = (int16_t)fmaxf(-32768.0f, fminf(32767.0f, left));
		buffer[i + 1] = (int16_t)fmaxf(-32768.0f, fminf(32767.0f, right));
	}
}

// ============================================================================
// Generic Parameter Interface
// ============================================================================

#include "../param_interface.h"

// Parameter groups
typedef enum {
	FX_FADER_GROUP_MAIN = 0,
	FX_FADER_GROUP_COUNT
} FXFaderParamGroup;

// Parameter indices
typedef enum {
	FX_FADER_PARAM_LEVEL = 0,
	FX_FADER_PARAM_COUNT
} FXFaderParamIndex;

// Parameter metadata (ALL VALUES NORMALIZED 0.0-1.0)
static const ParameterInfo fader_params[FX_FADER_PARAM_COUNT] = {
	{"Level", "dB", 1.0f, 0.0f, 1.0f, FX_FADER_GROUP_MAIN, 0}
};

static const char* group_names[FX_FADER_GROUP_COUNT] = {
	"Fader"
};

int fx_fader_get_parameter_count(void)
{
	return FX_FADER_PARAM_COUNT;
}

float fx_fader_get_parameter_value(FXFader* fx, int index)
{
	if (!fx || index < 0 || index >= FX_FADER_PARAM_COUNT) return 0.0f;

	switch (index) {
		case FX_FADER_PARAM_LEVEL:
			return fx_fader_get_level(fx);
		default:
			return 0.0f;
	}
}

void fx_fader_set_parameter_value(FXFader* fx, int index, float value)
{
	if (!fx || index < 0 || index >= FX_FADER_PARAM_COUNT) return;

	switch (index) {
		case FX_FADER_PARAM_LEVEL:
			fx_fader_set_level(fx, value);
			break;
	}
}

// Generate all metadata accessor functions using shared macro
DEFINE_PARAM_METADATA_ACCESSORS(fx_fader, fader_params, FX_FADER_PARAM_COUNT, group_names, FX_FADER_GROUP_COUNT)
