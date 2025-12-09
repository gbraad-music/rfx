/*
 * Common interface for Regroove Effects
 *
 * Each effect follows this pattern:
 * - Create/destroy lifecycle
 * - Process function (float or int16_t)
 * - Normalized parameters (0.0-1.0)
 * - Enabled/disabled state
 */

#ifndef FX_COMMON_H
#define FX_COMMON_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Common parameter type (normalized 0.0-1.0)
typedef float fx_param_t;

// Common sample types
typedef float fx_sample_t;
typedef int16_t fx_sample_i16_t;

#ifdef __cplusplus
}
#endif

#endif // FX_COMMON_H
