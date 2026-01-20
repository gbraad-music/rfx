/*
 * Math Compatibility Helpers
 *
 * Workarounds for MinGW static linking issues with certain math functions
 */

#ifndef FX_MATH_COMPAT_H
#define FX_MATH_COMPAT_H

#include <math.h>

// MinGW with -static flag has issues linking some math functions
// Implement them using functions that do link properly
#if defined(_WIN32) && defined(__MINGW32__)

// tan() doesn't link properly with -static, but sin() and cos() do
static inline double fx_tan(double x) {
    return sin(x) / cos(x);
}

static inline float fx_tanf(float x) {
    return sinf(x) / cosf(x);
}

#define tan fx_tan
#define tanf fx_tanf

#endif // _WIN32 && __MINGW32__

#endif // FX_MATH_COMPAT_H
