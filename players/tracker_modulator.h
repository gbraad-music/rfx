/*
 * Generic Tracker Modulator
 *
 * Frame-based parameter modulation used in tracker formats for:
 * - Filter sweeps
 * - PWM (pulse width modulation)
 * - Pitch sweeps
 * - Volume envelopes
 * - Any other time-varying parameter
 *
 * Based on patterns from AHX/HVL, ProTracker, FastTracker, etc.
 */

#ifndef TRACKER_MODULATOR_H
#define TRACKER_MODULATOR_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int32_t position;      // Current position
    int32_t sign;          // Direction: +1 or -1
    int32_t speed;         // Speed of change per frame
    int32_t lower_limit;   // Lower boundary
    int32_t upper_limit;   // Upper boundary
    bool active;           // Is modulation enabled?
    bool init_pending;     // Needs initialization?
    bool sliding_in;       // Currently sliding to first limit?
} TrackerModulator;

/**
 * Initialize modulator to default state
 */
void tracker_modulator_init(TrackerModulator* mod);

/**
 * Set modulation limits
 * If lower > upper, they will be swapped automatically
 */
void tracker_modulator_set_limits(TrackerModulator* mod, int32_t lower, int32_t upper);

/**
 * Set modulation speed
 */
void tracker_modulator_set_speed(TrackerModulator* mod, int32_t speed);

/**
 * Set current position directly
 */
void tracker_modulator_set_position(TrackerModulator* mod, int32_t position);

/**
 * Enable/disable modulation
 * When enabled, will initialize on next update
 */
void tracker_modulator_set_active(TrackerModulator* mod, bool active);

/**
 * Set direction explicitly (+1 or -1)
 */
void tracker_modulator_set_direction(TrackerModulator* mod, int32_t sign);

/**
 * Update modulation (call once per frame, e.g. 50Hz for PAL)
 * Returns true if position changed
 */
bool tracker_modulator_update(TrackerModulator* mod);

/**
 * Get current position
 */
int32_t tracker_modulator_get_position(const TrackerModulator* mod);

/**
 * Check if modulator is active
 */
bool tracker_modulator_is_active(const TrackerModulator* mod);

#ifdef __cplusplus
}
#endif

#endif // TRACKER_MODULATOR_H
