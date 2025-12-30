/*
 * Regroove DJ Crossfader Effect
 * Blends between two stereo inputs
 */

#include <math.h>
#include <stdlib.h>
#include "fx_common.h"
#include "fx_crossfader.h"

struct FXCrossfader {
	int enabled;
	float position;       // 0.0 = all A, 1.0 = all B
	float curve;          // 0.0 = linear, 1.0 = sharp cut
	float smooth_pos;     // Smoothed position
};

FXCrossfader* fx_crossfader_create(void)
{
	FXCrossfader* fx = (FXCrossfader*)calloc(1, sizeof(FXCrossfader));
	if (!fx) return NULL;
	fx->enabled = 1;
	fx->position = 0.5f;     // Center position
	fx->curve = 0.0f;        // Linear crossfade
	fx->smooth_pos = 0.5f;
	return fx;
}

void fx_crossfader_destroy(FXCrossfader* fx)
{
	if (fx) free(fx);
}

void fx_crossfader_reset(FXCrossfader* fx)
{
	if (!fx) return;
	fx->smooth_pos = fx->position;
}

void fx_crossfader_set_enabled(FXCrossfader* fx, int enabled)
{
	if (fx) fx->enabled = enabled;
}

void fx_crossfader_set_position(FXCrossfader* fx, float position)
{
	if (fx) fx->position = fmaxf(0.0f, fminf(1.0f, position));
}

void fx_crossfader_set_curve(FXCrossfader* fx, float curve)
{
	if (fx) fx->curve = fmaxf(0.0f, fminf(1.0f, curve));
}

int fx_crossfader_get_enabled(FXCrossfader* fx)
{
	return fx ? fx->enabled : 0;
}

float fx_crossfader_get_position(FXCrossfader* fx)
{
	return fx ? fx->position : 0.5f;
}

float fx_crossfader_get_curve(FXCrossfader* fx)
{
	return fx ? fx->curve : 0.0f;
}

void fx_crossfader_process_frame(FXCrossfader* fx,
                                  float in_a_left, float in_a_right,
                                  float in_b_left, float in_b_right,
                                  float* out_left, float* out_right,
                                  int sample_rate)
{
	(void)sample_rate;
	if (!fx || !out_left || !out_right) return;

	if (!fx->enabled) {
		// Pass through A when disabled
		*out_left = in_a_left;
		*out_right = in_a_right;
		return;
	}

	// Smooth the position to prevent zipper noise
	const float smoothing = 0.001f;
	fx->smooth_pos += (fx->position - fx->smooth_pos) * smoothing;

	float pos = fx->smooth_pos;

	// Apply crossfade curve
	// Linear: gain_a = 1-pos, gain_b = pos
	// Sharp: apply power function to make center cut sharper
	float gain_a, gain_b;

	if (fx->curve > 0.0f) {
		// Apply curve (power function)
		// At curve=1.0, use x^3 for sharp transition
		float power = 1.0f + fx->curve * 2.0f;  // 1.0 to 3.0
		gain_a = powf(1.0f - pos, power);
		gain_b = powf(pos, power);
	} else {
		// Linear crossfade
		gain_a = 1.0f - pos;
		gain_b = pos;
	}

	// Mix the signals
	*out_left = in_a_left * gain_a + in_b_left * gain_b;
	*out_right = in_a_right * gain_a + in_b_right * gain_b;
}

// ============================================================================
// Generic Parameter Interface
// ============================================================================

#include "../param_interface.h"

// Parameter groups
typedef enum {
	FX_CROSSFADER_GROUP_MAIN = 0,
	FX_CROSSFADER_GROUP_COUNT
} FXCrossfaderParamGroup;

// Parameter indices
typedef enum {
	FX_CROSSFADER_PARAM_POSITION = 0,
	FX_CROSSFADER_PARAM_CURVE,
	FX_CROSSFADER_PARAM_COUNT
} FXCrossfaderParamIndex;

// Parameter metadata (ALL VALUES NORMALIZED 0.0-1.0)
static const ParameterInfo crossfader_params[FX_CROSSFADER_PARAM_COUNT] = {
	{"Position", "", 0.5f, 0.0f, 1.0f, FX_CROSSFADER_GROUP_MAIN, 0},
	{"Curve", "", 0.0f, 0.0f, 1.0f, FX_CROSSFADER_GROUP_MAIN, 0}
};

static const char* group_names[FX_CROSSFADER_GROUP_COUNT] = {
	"Crossfader"
};

int fx_crossfader_get_parameter_count(void)
{
	return FX_CROSSFADER_PARAM_COUNT;
}

float fx_crossfader_get_parameter_value(FXCrossfader* fx, int index)
{
	if (!fx || index < 0 || index >= FX_CROSSFADER_PARAM_COUNT) return 0.0f;

	switch (index) {
		case FX_CROSSFADER_PARAM_POSITION:
			return fx_crossfader_get_position(fx);
		case FX_CROSSFADER_PARAM_CURVE:
			return fx_crossfader_get_curve(fx);
		default:
			return 0.0f;
	}
}

void fx_crossfader_set_parameter_value(FXCrossfader* fx, int index, float value)
{
	if (!fx || index < 0 || index >= FX_CROSSFADER_PARAM_COUNT) return;

	switch (index) {
		case FX_CROSSFADER_PARAM_POSITION:
			fx_crossfader_set_position(fx, value);
			break;
		case FX_CROSSFADER_PARAM_CURVE:
			fx_crossfader_set_curve(fx, value);
			break;
	}
}

// Generate all metadata accessor functions using shared macro
DEFINE_PARAM_METADATA_ACCESSORS(fx_crossfader, crossfader_params, FX_CROSSFADER_PARAM_COUNT, group_names, FX_CROSSFADER_GROUP_COUNT)
