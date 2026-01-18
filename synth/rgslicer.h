/**
 * RGSlicer - Slicing Sampler Engine
 * Load, slice, and playback samples with per-slice pitch/time effects
 */

#ifndef RGSLICER_H
#define RGSLICER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Constants
#define RGSLICER_MAX_SLICES 64
#define RGSLICER_MAX_VOICES 16
#define RGSLICER_MAX_NAME 256

// Forward declaration for SampleFX (from sample_fx.h)
typedef struct SampleFX SampleFX;

// Internal slice data
typedef struct SliceData {
    uint32_t offset;           // Start position in samples
    uint32_t length;           // Length in samples
    uint32_t end;              // End position (offset + length)

    // Per-slice parameters
    float pitch_semitones;     // -12.0 to +12.0
    float time_stretch;        // 0.5 to 2.0
    float volume;              // 0.0 to 2.0
    float pan;                 // -1.0 to +1.0
    bool reverse;              // Play backwards
    bool loop;                 // Loop playback
    bool one_shot;             // Ignore note-off (play to completion)
} SliceData;

// Voice state (for polyphonic playback)
typedef struct SliceVoice {
    bool active;
    uint8_t slice_index;       // Which slice is playing
    uint8_t note;              // MIDI note that triggered
    uint8_t velocity;          // MIDI velocity

    float playback_pos;        // Current position in slice (fractional for pitch shift)
    bool reverse;              // Playback direction

    SampleFX* fx;              // Per-voice effects (UNUSED - sounds muffled)
    float volume;              // Voice volume (from velocity)
} SliceVoice;

// Main RGSlicer structure
typedef struct RGSlicer {
    // Source sample
    int16_t* sample_data;      // Loaded WAV data (mono)
    uint32_t sample_length;    // Total samples
    uint32_t sample_rate;      // Sample rate
    bool sample_loaded;

    // Slices (max 64)
    SliceData slices[RGSLICER_MAX_SLICES];
    uint8_t num_slices;

    // Note-to-slice mapping (from CUE labels or white-key default)
    // note_map[midi_note] = slice_index, or 0xFF if unmapped
    uint8_t note_map[128];
    bool use_note_map;         // True if CUE labels specified notes

    // Voice state (polyphonic playback)
    SliceVoice voices[RGSLICER_MAX_VOICES];
    uint8_t voice_allocator;

    // Global parameters
    float master_pitch;        // Global pitch offset
    float master_time;         // Global time stretch
    float master_volume;       // Global volume

    // Metadata
    char sample_name[RGSLICER_MAX_NAME];
    float bpm;                 // Tempo for random slicer (default 125)
    uint8_t root_note;         // Root MIDI note (default 60)

    // Random slice sequencer (Note 39)
    bool random_seq_active;    // Is random sequencer running?
    uint32_t random_seq_phase; // Sample counter for tempo sync
    uint32_t random_seq_interval; // Samples between triggers

    uint32_t target_sample_rate;
} RGSlicer;

// Slice detection modes
typedef enum {
    SLICE_TRANSIENT,      // Detect transients/peaks
    SLICE_ZERO_CROSSING,  // Split at zero crossings
    SLICE_FIXED_GRID,     // Equal divisions
    SLICE_BPM_SYNC        // Sync to BPM (future)
} SliceMode;

// Playback modes
typedef enum {
    PLAYBACK_ONESHOT,     // Play once
    PLAYBACK_LOOP,        // Loop slice
    PLAYBACK_GATE,        // Play while held
    PLAYBACK_REVERSE,     // Play backwards
    PLAYBACK_PINGPONG     // Forward then backward
} PlaybackMode;

// ============================================================================
// Lifecycle
// ============================================================================

/**
 * Create RGSlicer instance
 * sample_rate: Target sample rate in Hz
 */
RGSlicer* rgslicer_create(uint32_t sample_rate);

/**
 * Destroy RGSlicer instance
 */
void rgslicer_destroy(RGSlicer* slicer);

/**
 * Reset all voices and playback state
 */
void rgslicer_reset(RGSlicer* slicer);

// ============================================================================
// Sample Loading
// ============================================================================

/**
 * Load WAV file from path
 * Returns: 1 on success, 0 on failure
 */
int rgslicer_load_sample(RGSlicer* slicer, const char* wav_path);

/**
 * Load WAV from memory
 * data: PCM data (int16, mono)
 * num_samples: Number of samples
 * sample_rate: Sample rate of source
 * Returns: 1 on success, 0 on failure
 */
int rgslicer_load_sample_memory(RGSlicer* slicer, int16_t* data, uint32_t num_samples, uint32_t sample_rate);

/**
 * Unload current sample
 */
void rgslicer_unload_sample(RGSlicer* slicer);

/**
 * Check if sample is loaded
 */
bool rgslicer_has_sample(RGSlicer* slicer);

// ============================================================================
// Slicing
// ============================================================================

/**
 * Auto-detect and create slices
 * mode: Detection algorithm
 * num_slices: Desired number of slices (max 64)
 * sensitivity: Detection sensitivity 0.0-1.0 (for transient mode)
 * Returns: Number of slices created
 */
uint8_t rgslicer_auto_slice(RGSlicer* slicer, SliceMode mode, uint8_t num_slices, float sensitivity);

/**
 * Manually add slice point
 * offset: Sample offset where slice starts
 * Returns: Slice index or -1 on failure
 */
int rgslicer_add_slice(RGSlicer* slicer, uint32_t offset);

/**
 * Remove slice by index
 */
void rgslicer_remove_slice(RGSlicer* slicer, uint8_t slice_index);

/**
 * Move slice point
 */
void rgslicer_move_slice(RGSlicer* slicer, uint8_t slice_index, uint32_t new_offset);

/**
 * Clear all slices
 */
void rgslicer_clear_slices(RGSlicer* slicer);

/**
 * Get number of slices
 */
uint8_t rgslicer_get_num_slices(RGSlicer* slicer);

// ============================================================================
// Per-Slice Parameters
// ============================================================================

/**
 * Set slice pitch shift in semitones
 * slice_index: Slice index (0-63)
 * semitones: -12.0 to +12.0
 */
void rgslicer_set_slice_pitch(RGSlicer* slicer, uint8_t slice_index, float semitones);

/**
 * Set slice time-stretch ratio
 * ratio: 0.5 to 2.0 (1.0 = normal speed)
 */
void rgslicer_set_slice_time(RGSlicer* slicer, uint8_t slice_index, float ratio);

/**
 * Set slice volume
 * volume: 0.0 to 2.0 (1.0 = 100%)
 */
void rgslicer_set_slice_volume(RGSlicer* slicer, uint8_t slice_index, float volume);

/**
 * Set slice pan
 * pan: -1.0 to +1.0 (0.0 = center)
 */
void rgslicer_set_slice_pan(RGSlicer* slicer, uint8_t slice_index, float pan);

/**
 * Set slice reverse playback
 */
void rgslicer_set_slice_reverse(RGSlicer* slicer, uint8_t slice_index, bool reverse);

/**
 * Set slice loop mode
 */
void rgslicer_set_slice_loop(RGSlicer* slicer, uint8_t slice_index, bool loop);

/**
 * Set slice one-shot mode (ignores note-off)
 * WARNING: one_shot + loop will consume voices until stolen!
 */
void rgslicer_set_slice_one_shot(RGSlicer* slicer, uint8_t slice_index, bool one_shot);

/**
 * Get slice parameters
 */
float rgslicer_get_slice_pitch(RGSlicer* slicer, uint8_t slice_index);
float rgslicer_get_slice_time(RGSlicer* slicer, uint8_t slice_index);
float rgslicer_get_slice_volume(RGSlicer* slicer, uint8_t slice_index);
float rgslicer_get_slice_pan(RGSlicer* slicer, uint8_t slice_index);

/**
 * Get slice offset and length
 */
uint32_t rgslicer_get_slice_offset(RGSlicer* slicer, uint8_t slice_index);
uint32_t rgslicer_get_slice_length(RGSlicer* slicer, uint8_t slice_index);

// ============================================================================
// Global Parameters
// ============================================================================

/**
 * Set global pitch shift (affects all slices)
 */
void rgslicer_set_global_pitch(RGSlicer* slicer, float semitones);

/**
 * Set global time-stretch (affects all slices)
 */
void rgslicer_set_global_time(RGSlicer* slicer, float ratio);

/**
 * Set global volume
 */
void rgslicer_set_global_volume(RGSlicer* slicer, float volume);

float rgslicer_get_global_pitch(RGSlicer* slicer);
float rgslicer_get_global_time(RGSlicer* slicer);
float rgslicer_get_global_volume(RGSlicer* slicer);

/**
 * Set BPM for random slice sequencer (Note 39)
 */
void rgslicer_set_bpm(RGSlicer* slicer, float bpm);
float rgslicer_get_bpm(RGSlicer* slicer);

// ============================================================================
// MIDI / Playback
// ============================================================================

/**
 * Trigger slice via MIDI note
 * note: MIDI note (36-99 maps to slices 0-63)
 * velocity: MIDI velocity (0-127)
 */
void rgslicer_note_on(RGSlicer* slicer, uint8_t note, uint8_t velocity);

/**
 * Stop slice via MIDI note
 */
void rgslicer_note_off(RGSlicer* slicer, uint8_t note);

/**
 * Stop all playing slices
 */
void rgslicer_all_notes_off(RGSlicer* slicer);

/**
 * Process audio output
 * buffer: Output buffer (stereo interleaved float32)
 * frames: Number of frames to process
 */
void rgslicer_process_f32(RGSlicer* slicer, float* buffer, uint32_t frames);

// ============================================================================
// SFZ Import/Export
// ============================================================================

/**
 * Import SFZ file with slice definitions
 * Returns: Number of slices loaded, or 0 on failure
 */
int rgslicer_import_sfz(RGSlicer* slicer, const char* sfz_path);

/**
 * Export current slices to SFZ file
 * sfz_path: Output SFZ file path
 * wav_path: WAV file path (can be relative)
 * Returns: 1 on success, 0 on failure
 */
int rgslicer_export_sfz(RGSlicer* slicer, const char* sfz_path, const char* wav_path);

// ============================================================================
// WAV+CUE Export
// ============================================================================

/**
 * Export current slices as WAV file with embedded CUE points
 *
 * Creates a WAV file containing the loaded sample data with embedded
 * CUE chunk markers at slice positions. CUE points are labeled with
 * MIDI note numbers (36-99) for direct compatibility with Drumlogue.
 *
 * output_path: Output WAV file path
 * Returns: 1 on success, 0 on failure
 */
int rgslicer_export_wav_cue(RGSlicer* slicer, const char* output_path);

// ============================================================================
// Preset Management (JSON)
// ============================================================================

/**
 * Save preset to JSON file
 */
int rgslicer_save_preset(RGSlicer* slicer, const char* preset_path);

/**
 * Load preset from JSON file
 */
int rgslicer_load_preset(RGSlicer* slicer, const char* preset_path);

// ============================================================================
// Metadata
// ============================================================================

/**
 * Set sample name/description
 */
void rgslicer_set_name(RGSlicer* slicer, const char* name);

/**
 * Set BPM (for tempo-sync features)
 */
void rgslicer_set_bpm(RGSlicer* slicer, float bpm);

/**
 * Set root MIDI note
 */
void rgslicer_set_root_note(RGSlicer* slicer, uint8_t note);

const char* rgslicer_get_name(RGSlicer* slicer);
float rgslicer_get_bpm(RGSlicer* slicer);
uint8_t rgslicer_get_root_note(RGSlicer* slicer);

#ifdef __cplusplus
}
#endif

#endif // RGSLICER_H
