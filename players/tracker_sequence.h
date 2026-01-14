/*
 * Generic Tracker Sequence
 *
 * Frame-based command/parameter sequences used in tracker formats for:
 * - Arpeggio (AHX PList, ProTracker arpeggio)
 * - Instrument envelopes
 * - Waveform sequences
 * - Parameter automation
 *
 * Based on patterns from AHX PList, IT instrument sequences, etc.
 */

#ifndef TRACKER_SEQUENCE_H
#define TRACKER_SEQUENCE_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Generic sequence entry - can be specialized per use case
// NOTE: Field order matches AhxPListEntry for direct casting
typedef struct {
    int32_t note;          // Note/pitch parameter
    int32_t fixed;         // Fixed note (don't apply transpose)
    int32_t waveform;      // Waveform/timbre parameter
    int32_t fx[2];         // Effect commands
    int32_t fx_param[2];   // Effect parameters
} TrackerSequenceEntry;

typedef struct {
    int32_t speed;         // Frames per step
    int32_t length;        // Number of entries
    TrackerSequenceEntry* entries;

    // Playback state
    int32_t current;       // Current step index
    int32_t wait;          // Frames until next step
    bool active;           // Is sequence playing?
} TrackerSequence;

/**
 * Initialize sequence
 */
void tracker_sequence_init(TrackerSequence* seq);

/**
 * Allocate and set sequence data
 * Takes ownership of entries pointer
 */
void tracker_sequence_set_data(TrackerSequence* seq,
                               TrackerSequenceEntry* entries,
                               int32_t length,
                               int32_t speed);

/**
 * Free sequence data
 */
void tracker_sequence_free(TrackerSequence* seq);

/**
 * Start/restart sequence playback
 */
void tracker_sequence_start(TrackerSequence* seq);

/**
 * Stop sequence playback
 */
void tracker_sequence_stop(TrackerSequence* seq);

/**
 * Jump to specific step
 */
void tracker_sequence_jump(TrackerSequence* seq, int32_t step);

/**
 * Set playback speed (frames per step)
 */
void tracker_sequence_set_speed(TrackerSequence* seq, int32_t speed);

/**
 * Update sequence (call once per frame)
 * Returns pointer to current entry if step changed, NULL otherwise
 */
const TrackerSequenceEntry* tracker_sequence_update(TrackerSequence* seq);

/**
 * Get current entry (or NULL if inactive/empty)
 */
const TrackerSequenceEntry* tracker_sequence_get_current(const TrackerSequence* seq);

/**
 * Check if sequence is active
 */
bool tracker_sequence_is_active(const TrackerSequence* seq);

/**
 * Check if sequence has finished (reached end)
 */
bool tracker_sequence_is_finished(const TrackerSequence* seq);

#ifdef __cplusplus
}
#endif

#endif // TRACKER_SEQUENCE_H
