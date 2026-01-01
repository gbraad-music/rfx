#pragma once

#include "audio_analysis.h"
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Audio cache file format (RTX format - compatible with rescratch):
 * - .rtx file contains metadata in INI text format
 * - .rtxcache file contains binary waveform data
 * - Cached alongside original audio file
 * - Stores waveform frames with spectral analysis
 */

// Cache file metadata
typedef struct {
    char original_path[512];   // Path to original audio file
    int sample_rate;           // Sample rate of original
    size_t num_samples;        // Total samples in original
    int channels;              // Number of channels
    float duration;            // Duration in seconds
    float bpm;                 // Detected BPM (0 if not detected)
    size_t num_frames;         // Number of waveform frames
    size_t downsample;         // Downsample factor used
    size_t first_beat;         // First beat position in samples (beat grid offset)
} AudioCacheMetadata;

// Complete cached audio data
typedef struct {
    AudioCacheMetadata metadata;
    WaveformFrame* frames;     // Array of waveform frames
} AudioCache;

/**
 * Generate RTX file paths from audio file path
 * @param audio_path Path to audio file
 * @param rtx_path_out Output buffer for .rtx path (must be >= 512 bytes)
 * @param cache_path_out Output buffer for .rtxcache path (must be >= 512 bytes)
 */
void audio_cache_get_paths(const char* audio_path, char* rtx_path_out, char* cache_path_out);

/**
 * Check if cache file exists and is valid for the given audio file
 * @param audio_path Path to audio file
 * @param num_samples Number of samples in audio file
 * @param sample_rate Sample rate of audio file
 * @return true if valid cache exists, false otherwise
 */
bool audio_cache_is_valid(
    const char* audio_path,
    size_t num_samples,
    int sample_rate
);

/**
 * Load cached waveform data from disk
 * @param audio_path Path to original audio file
 * @param cache_out Output cache structure (caller must free cache_out->frames)
 * @return true on success, false on failure
 */
bool audio_cache_load(
    const char* audio_path,
    AudioCache* cache_out
);

/**
 * Save waveform data to cache file
 * @param audio_path Path to original audio file
 * @param metadata Cache metadata (includes first_beat offset)
 * @param frames Array of waveform frames
 * @param num_frames Number of frames
 * @return true on success, false on failure
 */
bool audio_cache_save(
    const char* audio_path,
    const AudioCacheMetadata* metadata,
    const WaveformFrame* frames,
    size_t num_frames
);

/**
 * Free cached audio data
 * @param cache Cache to free
 */
void audio_cache_free(AudioCache* cache);

#ifdef __cplusplus
}
#endif
