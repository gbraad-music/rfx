#pragma once
/**
 * Simple SFZ Parser - Parses basic SFZ files for sample mapping
 *
 * Supports:
 * - Basic region definitions
 * - sample= (WAV file path)
 * - key= or lokey=/hikey= (MIDI note mapping)
 * - pitch_keycenter= (root note for pitch shifting)
 * - offset=/end= (sample slicing)
 * - lovel=/hivel= (velocity layers)
 * - pan= (panning)
 */

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SFZ_MAX_REGIONS 256
#define SFZ_MAX_PATH 256

typedef struct {
    char sample_path[SFZ_MAX_PATH];  // Path to WAV file
    uint8_t lokey;                   // Lowest MIDI note (0-127)
    uint8_t hikey;                   // Highest MIDI note (0-127)
    uint8_t pitch_keycenter;         // Root note for pitch calculation (default 60)
    uint8_t lovel;                   // Lowest velocity (0-127, default 0)
    uint8_t hivel;                   // Highest velocity (0-127, default 127)
    uint32_t offset;                 // Sample start offset in samples (default 0)
    uint32_t end;                    // Sample end position in samples (default 0 = full sample)
    float pan;                       // Panning -100 to +100 (default 0)
    bool loop_mode;                  // True for looping (default true)

    // Loaded sample data (filled by loader, not parser)
    int16_t* sample_data;            // Actual sample data
    uint32_t sample_length;          // Total length in samples
    uint32_t sample_rate;            // Sample rate (e.g., 44100)
} SFZRegion;

typedef struct {
    SFZRegion regions[SFZ_MAX_REGIONS];
    uint32_t num_regions;
    char base_dir[SFZ_MAX_PATH];     // Directory where SFZ file is located
} SFZData;

/**
 * Parse an SFZ file
 * filepath: Path to the .sfz file
 * Returns allocated SFZData structure, or NULL on error
 */
SFZData* sfz_parse(const char* filepath);

/**
 * Load WAV files referenced in the SFZ
 * sfz: Parsed SFZ data
 * Returns true on success
 */
bool sfz_load_samples(SFZData* sfz);

/**
 * Find the best matching region for a note and velocity
 * Returns NULL if no region matches
 */
const SFZRegion* sfz_find_region(const SFZData* sfz, uint8_t note, uint8_t velocity);

/**
 * Free SFZ data and loaded samples
 */
void sfz_free(SFZData* sfz);

#ifdef __cplusplus
}
#endif
