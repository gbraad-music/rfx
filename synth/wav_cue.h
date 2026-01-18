/**
 * WAV CUE Chunk Support
 *
 * Writes and reads CUE point chunks in WAV files for sample slicing.
 * CUE points are labeled with MIDI note numbers (36-99) for keyboard mapping.
 * Loop points use "N-loop" format for sustain/decay behavior.
 */

#ifndef WAV_CUE_H
#define WAV_CUE_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_CUE_POINTS 128

// CUE point data
typedef struct {
    uint32_t position;      // Sample offset where CUE point is located
    char label[32];         // Label (e.g., "36", "37", "36-loop")
    uint32_t cue_id;        // Unique ID for this CUE point
} WAVCuePoint;

// CUE chunk data
typedef struct {
    WAVCuePoint points[MAX_CUE_POINTS];
    uint32_t num_points;
} WAVCueData;

/**
 * Read CUE points from WAV file
 * Returns NULL if no CUE chunk found or on error
 */
WAVCueData* wav_cue_read(const char* wav_path);

/**
 * Write WAV file with CUE points embedded
 *
 * sample_data: PCM data (int16, mono)
 * num_samples: Total samples
 * sample_rate: Sample rate (e.g., 48000)
 * cue_data: CUE points to embed
 * output_path: Where to save WAV+CUE file
 *
 * Returns: 1 on success, 0 on failure
 */
int wav_cue_write(const int16_t* sample_data, uint32_t num_samples,
                  uint32_t sample_rate, const WAVCueData* cue_data,
                  const char* output_path);

/**
 * Create CUE data from slice offsets
 *
 * Generates CUE points labeled with MIDI notes (36-99)
 * Optionally adds loop points if loop_offsets is provided
 *
 * slice_offsets: Array of sample offsets where slices start
 * num_slices: Number of slices (max 64)
 * loop_offsets: Optional array of loop point offsets (NULL if no loops)
 * start_note: Starting MIDI note (default 36 for C1)
 */
WAVCueData* wav_cue_create_from_slices(const uint32_t* slice_offsets,
                                        uint8_t num_slices,
                                        const uint32_t* loop_offsets,
                                        uint8_t start_note);

/**
 * Parse CUE points to extract slice offsets
 *
 * Reads CUE labels and extracts MIDI note-based slices
 * Ignores loop points (those with "-loop" suffix)
 *
 * cue_data: CUE data read from WAV
 * slice_offsets: Output array of slice offsets (caller allocates)
 * max_slices: Maximum slices to extract (typically 64)
 *
 * Returns: Number of slices extracted
 */
uint8_t wav_cue_extract_slices(const WAVCueData* cue_data,
                                uint32_t* slice_offsets,
                                uint8_t max_slices);

/**
 * Free CUE data
 */
void wav_cue_free(WAVCueData* cue_data);

#ifdef __cplusplus
}
#endif

#endif // WAV_CUE_H
