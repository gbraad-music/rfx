/*
 * Regroove Unified API - Compatibility Layer
 *
 * This header provides a unified API that works with either:
 * - Full libopenmpt-based engine (./engine/regroove_engine.h)
 * - Minimal embedded controller (./players/regroove_controller.h)
 *
 * Usage:
 *   #define USE_REGROOVE_ENGINE  // or USE_REGROOVE_CONTROLLER
 *   #include "regroove_unified.h"
 *
 * Then use the rg_* macros which will map to the appropriate implementation.
 *
 * Example:
 *   RegrooveHandle* handle = ...;
 *   rg_trigger_loop(handle);
 *   rg_set_loop_range(handle, 0, 0, 3, 63);
 *   int order, row;
 *   rg_get_position(handle, &order, &row);
 */

#ifndef REGROOVE_UNIFIED_H
#define REGROOVE_UNIFIED_H

#include <stdint.h>
#include <stdbool.h>

// ============================================================================
// Implementation Selection
// ============================================================================

#if defined(USE_REGROOVE_ENGINE) && defined(USE_REGROOVE_CONTROLLER)
    #error "Cannot define both USE_REGROOVE_ENGINE and USE_REGROOVE_CONTROLLER"
#endif

#if !defined(USE_REGROOVE_ENGINE) && !defined(USE_REGROOVE_CONTROLLER)
    // Default to engine if nothing specified
    #define USE_REGROOVE_ENGINE
#endif

// ============================================================================
// Include appropriate header
// ============================================================================

#ifdef USE_REGROOVE_ENGINE
    #include "engine/regroove_engine.h"

    // Type alias
    typedef Regroove RegrooveHandle;

    // Function prefix
    #define RG_FN(name) regroove_##name

#else // USE_REGROOVE_CONTROLLER
    #include "players/regroove_controller.h"

    // Type alias
    typedef RegrooveController RegrooveHandle;

    // Function prefix
    #define RG_FN(name) regroove_controller_##name

#endif

// ============================================================================
// Common Enums (both implementations now support these)
// ============================================================================

// Loop state is defined in both headers now
// Pattern mode is defined in both headers now

// ============================================================================
// Unified API Macros
// ============================================================================

// Core lifecycle
#define rg_destroy(h)                   RG_FN(destroy)(h)
#define rg_set_callbacks(h, cb, ud)     RG_FN(set_callbacks)(h, cb, ud)

// Loop control
#define rg_set_loop_range               RG_FN(set_loop_range)
#define rg_trigger_loop(h)              RG_FN(trigger_loop)(h)
#define rg_get_loop_state(h)            RG_FN(get_loop_state)(h)
#define rg_arm_loop(h)                  RG_FN(arm_loop)(h)
#define rg_disable_loop(h)              RG_FN(disable_loop)(h)

// Position control
#define rg_get_position(h, o, r)        RG_FN(get_position)(h, o, r)
#define rg_jump_immediate(h, o, r)      RG_FN(jump_immediate)(h, o, r)

// Pattern mode
#define rg_retrigger_pattern(h)         RG_FN(retrigger_pattern)(h)

// Queued commands
#define rg_queue_next_order(h)          RG_FN(queue_next_order)(h)
#define rg_queue_prev_order(h)          RG_FN(queue_prev_order)(h)

// Channel control
#define rg_toggle_channel_mute(h, ch)   RG_FN(toggle_channel_mute)(h, ch)
#define rg_queue_channel_mute(h, ch)    RG_FN(queue_channel_mute)(h, ch)
#define rg_get_channel_mute(h, ch)      RG_FN(get_channel_mute)(h, ch)
#define rg_is_channel_muted(h, ch)      RG_FN(is_channel_muted)(h, ch)

// Metadata queries
#define rg_get_num_orders(h)            RG_FN(get_num_orders)(h)
#define rg_get_num_channels(h)          RG_FN(get_num_channels)(h)
#define rg_get_song_length(h)           RG_FN(get_song_length)(h)

// ============================================================================
// Implementation-Specific Wrappers
// ============================================================================

#ifdef USE_REGROOVE_ENGINE

// Engine-specific functions that need wrappers for unified API

// Pattern mode: engine uses int, unified API uses enum
static inline void rg_set_pattern_mode(RegrooveHandle* h, RegroovePatternMode mode) {
    regroove_pattern_mode(h, (mode == RG_PATTERN_MODE_SINGLE) ? 1 : 0);
}

static inline RegroovePatternMode rg_get_pattern_mode(const RegrooveHandle* h) {
    return regroove_get_pattern_mode(h) ? RG_PATTERN_MODE_SINGLE : RG_PATTERN_MODE_OFF;
}

// Loop range: both now support both names via aliases
static inline void rg_set_loop_range_rows(RegrooveHandle* h, int start_order, int start_row,
                                           int end_order, int end_row) {
    regroove_set_loop_range_rows(h, start_order, start_row, end_order, end_row);
}

static inline void rg_get_loop_range_rows(const RegrooveHandle* h, int* start_order, int* start_row,
                                           int* end_order, int* end_row) {
    regroove_get_loop_range_rows(h, start_order, start_row, end_order, end_row);
}

// Queued jump: engine uses queue_order
static inline void rg_queue_jump(RegrooveHandle* h, int order, int row) {
    regroove_queue_order(h, order);
    // Note: engine queue_order doesn't support row parameter currently
    (void)row; // Suppress warning
}

// Get current pattern: engine has direct function
static inline int rg_get_current_pattern(const RegrooveHandle* h) {
    return regroove_get_current_pattern(h);
}

// Get current order/row: engine has separate functions
static inline int rg_get_current_order(const RegrooveHandle* h) {
    return regroove_get_current_order(h);
}

static inline int rg_get_current_row(const RegrooveHandle* h) {
    return regroove_get_current_row(h);
}

#else // USE_REGROOVE_CONTROLLER

// Controller-specific functions that need wrappers for unified API

// Pattern mode: controller uses enum directly
static inline void rg_set_pattern_mode(RegrooveHandle* h, RegroovePatternMode mode) {
    regroove_controller_set_pattern_mode(h, mode);
}

static inline RegroovePatternMode rg_get_pattern_mode(const RegrooveHandle* h) {
    return regroove_controller_get_pattern_mode(h);
}

// Loop range: both now support both names via aliases
static inline void rg_set_loop_range_rows(RegrooveHandle* h, uint16_t start_order, uint16_t start_row,
                                           uint16_t end_order, uint16_t end_row) {
    regroove_controller_set_loop_range_rows(h, start_order, start_row, end_order, end_row);
}

static inline void rg_get_loop_range_rows(const RegrooveHandle* h, uint16_t* start_order, uint16_t* start_row,
                                           uint16_t* end_order, uint16_t* end_row) {
    regroove_controller_get_loop_range_rows(h, start_order, start_row, end_order, end_row);
}

// Queued jump: controller has direct function
static inline void rg_queue_jump(RegrooveHandle* h, uint16_t order, uint16_t row) {
    regroove_controller_queue_jump(h, order, row);
}

// Get current pattern: controller has direct function
static inline uint16_t rg_get_current_pattern(const RegrooveHandle* h) {
    return regroove_controller_get_current_pattern(h);
}

// Get current order/row: controller uses get_position
static inline uint16_t rg_get_current_order(const RegrooveHandle* h) {
    uint16_t order, row;
    regroove_controller_get_position(h, &order, &row);
    return order;
}

static inline uint16_t rg_get_current_row(const RegrooveHandle* h) {
    uint16_t order, row;
    regroove_controller_get_position(h, &order, &row);
    return row;
}

#endif

// ============================================================================
// Compatibility Notes
// ============================================================================

/*
 * The following features are ENGINE-ONLY (not available in controller):
 * - regroove_render_audio() - audio rendering
 * - regroove_set_pitch() - pitch control
 * - regroove_set_interpolation_filter() - audio quality settings
 * - regroove_set_stereo_separation()
 * - regroove_set_channel_volume() / get
 * - regroove_set_channel_panning() / get
 * - regroove_get_current_bpm() / get_effective_bpm()
 * - regroove_get_pattern_cell() - pattern data access
 * - regroove_get_instrument_name() / get_sample_name()
 *
 * The following features are CONTROLLER-ONLY:
 * - regroove_controller_get_sequencer() - access underlying sequencer
 * - Extended callback system (on_loop_state_change, on_command_executed, etc.)
 * - regroove_controller_clear_queue() - clear queued commands
 *
 * When using the unified API, only use the common subset documented above.
 * For implementation-specific features, use the native API directly.
 */

#endif // REGROOVE_UNIFIED_H
