#pragma once

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Spectral band energy distribution
typedef struct {
    float low;    // Low frequency energy (0-200 Hz)
    float mid;    // Mid frequency energy (200-2000 Hz)
    float high;   // High frequency energy (2000+ Hz)
} SpectralBands;

// Single waveform visualization frame
typedef struct {
    float amplitude;      // Normalized amplitude (-1.0 to 1.0)
    SpectralBands bands;  // Frequency band energies
} WaveformFrame;

// Analysis parameters
#define AUBIO_WIN_SIZE 2048
#define AUBIO_HOP_SIZE 512
#define WAVEFORM_DOWNSAMPLE 512

// Progress callback for long operations
typedef void (*ProgressCallback)(float progress, void* user_data);

/**
 * Analyze audio data and generate waveform with spectral information
 *
 * @param audio_data    Float array of mono audio samples
 * @param num_samples   Number of samples in audio_data
 * @param sample_rate   Sample rate of the audio
 * @param waveform_out  Output array for waveform frames (caller allocates)
 * @param num_frames    Number of waveform frames to generate
 * @param progress_cb   Optional progress callback
 * @param user_data     User data for progress callback
 * @return true on success, false on failure
 */
bool analyze_audio_waveform(
    const float* audio_data,
    size_t num_samples,
    int sample_rate,
    WaveformFrame* waveform_out,
    size_t num_frames,
    ProgressCallback progress_cb,
    void* user_data
);

/**
 * Detect BPM (beats per minute) of audio
 *
 * @param audio_data    Float array of mono audio samples
 * @param num_samples   Number of samples in audio_data
 * @param sample_rate   Sample rate of the audio
 * @return Detected BPM, or 0.0 if detection failed
 */
float detect_bpm(
    const float* audio_data,
    size_t num_samples,
    int sample_rate
);

#ifdef __cplusplus
}
#endif
