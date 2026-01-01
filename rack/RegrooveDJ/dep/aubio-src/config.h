/* Minimal aubio config.h for VCV Rack plugin compilation */
#ifndef AUBIO_CONFIG_H
#define AUBIO_CONFIG_H

/* Include standard headers first */
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <stdarg.h>
#include <math.h>

/* Package info */
#define PACKAGE "aubio"
#define VERSION "0.4.9"

/* Use float samples (not double) */
#define HAVE_AUBIO_DOUBLE 0

/* Math functions */
#define HAVE_C99_VARARGS_MACROS 1
#define HAVE_MATH_H 1

/* Standard headers */
#define HAVE_STDLIB_H 1
#define HAVE_STDIO_H 1
#define HAVE_STRING_H 1
#define HAVE_LIMITS_H 1
#define HAVE_STDARG_H 1

/* Disable external dependencies */
/* We only need FFT and tempo detection, no audio I/O */
/* Leave undefined - don't define to 0, as #if defined() checks for existence */
/* #undef HAVE_SNDFILE */
/* #undef HAVE_AVCODEC */
/* #undef HAVE_SAMPLERATE */
/* #undef HAVE_JACK */
/* #undef HAVE_FFTW3 */
/* #undef HAVE_FFTW3F */

/* Use built-in FFT (ooura) instead of external libraries */
/* Don't define HAVE_FFTW3 at all - aubio will use built-in FFT */

/* Platform detection */
#ifdef _WIN32
#define HAVE_WIN_HACKS 1
#endif

#endif /* AUBIO_CONFIG_H */
