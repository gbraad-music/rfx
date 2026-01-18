/**
 * Shared Granular Processing Functions
 * Used by both fx_pitchshift.c and sample_fx.c
 */

#ifndef FX_GRANULAR_SHARED_H
#define FX_GRANULAR_SHARED_H

#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/**
 * Initialize a Hann window
 * window: Output buffer
 * size: Window size in samples
 */
static inline void granular_init_hann_window(float* window, int size) {
    for (int i = 0; i < size; i++) {
        window[i] = 0.5f * (1.0f - cosf(2.0f * (float)M_PI * i / (size - 1)));
    }
}

/**
 * Wrap position into circular buffer range
 * pos: Position (can be negative or > buffer_size)
 * buffer_size: Size of circular buffer
 * Returns: Wrapped position [0, buffer_size)
 */
static inline float granular_wrap_position(float pos, int buffer_size) {
    while (pos < 0.0f)
        pos += (float)buffer_size;
    while (pos >= (float)buffer_size)
        pos -= (float)buffer_size;
    return pos;
}

/**
 * Read from circular buffer with linear interpolation (float)
 * buffer: Circular buffer (float)
 * position: Read position (fractional)
 * buffer_size: Size of buffer
 * Returns: Interpolated sample value
 */
static inline float granular_read_float(const float* buffer, float position, int buffer_size) {
    position = granular_wrap_position(position, buffer_size);

    int idx0 = (int)position;
    float frac = position - (float)idx0;
    int idx1 = idx0 + 1;
    if (idx1 >= buffer_size) idx1 = 0;

    return buffer[idx0] * (1.0f - frac) + buffer[idx1] * frac;
}

/**
 * Read from circular buffer with linear interpolation (int16)
 * buffer: Circular buffer (int16)
 * position: Read position (fractional)
 * buffer_size: Size of buffer
 * Returns: Interpolated sample value (float)
 */
static inline float granular_read_int16(const int16_t* buffer, float position, int buffer_size) {
    position = granular_wrap_position(position, buffer_size);

    int idx0 = (int)position;
    float frac = position - (float)idx0;
    int idx1 = idx0 + 1;
    if (idx1 >= buffer_size) idx1 = 0;

    return (float)buffer[idx0] * (1.0f - frac) + (float)buffer[idx1] * frac;
}

#endif // FX_GRANULAR_SHARED_H
