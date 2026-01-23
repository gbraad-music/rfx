/*
 * AHX Waves - Shared Waveform Tables
 *
 * Pre-computed waveform tables with authentic AHX filter modulation
 * Used by both ahx_player and ahx_instrument for consistent sound
 */

#ifndef AHX_WAVES_H
#define AHX_WAVES_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// AHX waveform tables with 31 filtered variations
typedef struct {
    int16_t LowPasses[(0xfc+0xfc+0x80*0x1f+0x80+3*0x280)*31];  // 31 lowpass filtered versions
    int16_t Triangle04[0x04], Triangle08[0x08], Triangle10[0x10], Triangle20[0x20], Triangle40[0x40], Triangle80[0x80];
    int16_t Sawtooth04[0x04], Sawtooth08[0x08], Sawtooth10[0x10], Sawtooth20[0x20], Sawtooth40[0x40], Sawtooth80[0x80];
    int16_t Squares[0x80*0x20];
    int16_t WhiteNoiseBig[0x280*3];
    int16_t HighPasses[(0xfc+0xfc+0x80*0x1f+0x80+3*0x280)*31];  // 31 highpass filtered versions
} AhxWaves;

/**
 * Get shared waves instance (singleton)
 * Automatically initializes on first call
 */
AhxWaves* ahx_waves_get(void);

/**
 * Get waveform pointer for playback (triangle, sawtooth, noise)
 * @param waveform Waveform type (0=triangle, 1=sawtooth, 3=noise) - NOT for square (2)
 * @param wave_length Wave length parameter (0-5) - ignored for noise
 * @param filter_pos Filter position (32-63) - selects harmonic content
 * @return Pointer to waveform data (NULL if invalid parameters)
 */
const int16_t* ahx_waves_get_waveform(AhxWaves* waves, uint8_t waveform, uint8_t wave_length, int filter_pos);

/**
 * Generate square waveform into buffer (authentic AHX resampling)
 * @param waves Waves instance
 * @param output_buffer Buffer to write square wave (must be at least 0x280 samples)
 * @param square_pos Square modulator position (0-63)
 * @param wave_length Wave length (0-5) affects resampling
 * @param filter_pos Filter position (32-63)
 * @param square_reverse Output parameter - set to 1 if waveform is reversed
 */
void ahx_waves_generate_square(AhxWaves* waves, int16_t* output_buffer, int square_pos,
                                uint8_t wave_length, int filter_pos, int* square_reverse);

/**
 * Release shared waves instance (call at shutdown)
 */
void ahx_waves_release(void);

#ifdef __cplusplus
}
#endif

#endif // AHX_WAVES_H
