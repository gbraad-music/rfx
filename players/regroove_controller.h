#ifndef REGROOVE_CONTROLLER_H
#define REGROOVE_CONTROLLER_H

/**
 * RegrooveController - Advanced DJ/Performance Layer for Pattern Sequencer
 *
 * This layer wraps PatternSequencer to add advanced features like:
 * - Row-precise loop control with arming/triggering
 * - Command queuing (executes on pattern boundaries)
 * - Pattern mode (loop single pattern)
 * - Channel mute/solo queuing
 * - Extended callbacks
 *
 * Usage:
 *   // Basic usage (pattern_sequencer only)
 *   PatternSequencer* seq = pattern_sequencer_create();
 *   // ... normal sequencer usage
 *
 *   // Advanced usage (regroove controller)
 *   PatternSequencer* seq = pattern_sequencer_create();
 *   RegrooveController* ctrl = regroove_controller_create(seq);
 *   regroove_controller_queue_jump(ctrl, 5, 0);
 *   regroove_controller_set_loop_range_rows(ctrl, 2, 0, 5, 63);
 *   regroove_controller_arm_loop(ctrl);
 *   // Controller intercepts sequencer callbacks, adds advanced logic
 */

#include <stdint.h>
#include <stdbool.h>
#include "pattern_sequencer.h"

#ifdef __cplusplus
extern "C" {
#endif

// Opaque controller structure
typedef struct RegrooveController RegrooveController;

// Loop state
typedef enum {
    RG_LOOP_OFF = 0,      // Not looping
    RG_LOOP_ARMED,        // Loop armed, waiting for trigger point
    RG_LOOP_ACTIVE        // Currently looping
} RegrooveLoopState;

// Pattern mode
typedef enum {
    RG_PATTERN_MODE_OFF = 0,     // Normal playback through song
    RG_PATTERN_MODE_SINGLE,      // Loop current pattern indefinitely
    RG_PATTERN_MODE_CHAIN        // Loop pattern range (future)
} RegroovePatternMode;

// Queued command types
typedef enum {
    RG_CMD_NONE = 0,
    RG_CMD_JUMP_TO_ORDER,        // Jump to specific order+row
    RG_CMD_NEXT_ORDER,           // Jump to next order
    RG_CMD_PREV_ORDER,           // Jump to previous order
    RG_CMD_RETRIGGER_PATTERN,    // Jump to start of current pattern
    RG_CMD_TOGGLE_CHANNEL_MUTE,  // Toggle channel mute
    RG_CMD_SET_CHANNEL_SOLO      // Set channel solo
} RegrooveCommandType;

// Extended callbacks (in addition to PatternSequencer callbacks)
typedef struct {
    // Called when loop state changes
    void (*on_loop_state_change)(void* user_data, RegrooveLoopState old_state, RegrooveLoopState new_state);

    // Called when loop trigger point is reached
    void (*on_loop_trigger)(void* user_data, uint16_t order, uint16_t row);

    // Called when pattern mode changes
    void (*on_pattern_mode_change)(void* user_data, RegroovePatternMode mode);

    // Called when a queued command executes
    void (*on_command_executed)(void* user_data, RegrooveCommandType command);

    // Called on every note trigger (for visualization/sync)
    void (*on_note)(void* user_data, uint8_t channel, uint16_t order, uint16_t row);
} RegrooveControllerCallbacks;

// ============================================================================
// Core Functions
// ============================================================================

/**
 * Create a new RegrooveController wrapping a PatternSequencer
 * The controller takes ownership of the sequencer and will destroy it
 * when the controller is destroyed.
 */
RegrooveController* regroove_controller_create(PatternSequencer* sequencer);

/**
 * Destroy the controller and its wrapped sequencer
 */
void regroove_controller_destroy(RegrooveController* controller);

/**
 * Get the underlying PatternSequencer (for direct access if needed)
 */
PatternSequencer* regroove_controller_get_sequencer(RegrooveController* controller);

/**
 * Set extended callbacks
 */
void regroove_controller_set_callbacks(RegrooveController* controller,
                                      const RegrooveControllerCallbacks* callbacks,
                                      void* user_data);

/**
 * Process timing and execute queued commands
 * This should be called instead of pattern_sequencer_process()
 */
void regroove_controller_process(RegrooveController* controller,
                                uint32_t frames,
                                double sample_rate);

// ============================================================================
// Advanced Loop Control
// ============================================================================

/**
 * Set loop range with row precision
 * @param start_order Start order (pattern index)
 * @param start_row Start row within start order
 * @param end_order End order (pattern index)
 * @param end_row End row within end order
 */
void regroove_controller_set_loop_range_rows(RegrooveController* controller,
                                             uint16_t start_order, uint16_t start_row,
                                             uint16_t end_order, uint16_t end_row);

/**
 * Get current loop range with row precision
 */
void regroove_controller_get_loop_range_rows(const RegrooveController* controller,
                                             uint16_t* start_order, uint16_t* start_row,
                                             uint16_t* end_order, uint16_t* end_row);

/**
 * Arm loop trigger - next time we reach the loop start point, activate looping
 * This enables the "play-to-loop" workflow
 */
void regroove_controller_arm_loop(RegrooveController* controller);

/**
 * Trigger loop immediately - jump to loop start and activate looping
 */
void regroove_controller_trigger_loop(RegrooveController* controller);

/**
 * Disable loop - stop looping and return to normal playback
 */
void regroove_controller_disable_loop(RegrooveController* controller);

/**
 * Get current loop state
 */
RegrooveLoopState regroove_controller_get_loop_state(const RegrooveController* controller);

// ============================================================================
// Queued Commands (execute on pattern boundary)
// ============================================================================

/**
 * Queue a jump to specific order+row (executes on next pattern boundary)
 */
void regroove_controller_queue_jump(RegrooveController* controller,
                                   uint16_t order, uint16_t row);

/**
 * Queue jump to next order (executes on next pattern boundary)
 */
void regroove_controller_queue_next_order(RegrooveController* controller);

/**
 * Queue jump to previous order (executes on next pattern boundary)
 */
void regroove_controller_queue_prev_order(RegrooveController* controller);

/**
 * Queue pattern retrigger - jump to start of current pattern
 */
void regroove_controller_queue_retrigger_pattern(RegrooveController* controller);

/**
 * Immediate jump (not queued) - executes immediately
 */
void regroove_controller_jump_immediate(RegrooveController* controller,
                                       uint16_t order, uint16_t row);

/**
 * Clear all queued commands
 */
void regroove_controller_clear_queue(RegrooveController* controller);

// ============================================================================
// Pattern Mode
// ============================================================================

/**
 * Set pattern mode
 * @param mode Pattern playback mode
 */
void regroove_controller_set_pattern_mode(RegrooveController* controller,
                                         RegroovePatternMode mode);

/**
 * Get current pattern mode
 */
RegroovePatternMode regroove_controller_get_pattern_mode(const RegrooveController* controller);

/**
 * Retrigger current pattern (immediate)
 */
void regroove_controller_retrigger_pattern(RegrooveController* controller);

// ============================================================================
// Channel Control (requires integration with player)
// ============================================================================

/**
 * Queue channel mute toggle (executes on next pattern boundary)
 * NOTE: This requires the player to check mute state and apply it
 */
void regroove_controller_queue_channel_mute(RegrooveController* controller, uint8_t channel);

/**
 * Queue channel solo (executes on next pattern boundary)
 * NOTE: This requires the player to check solo state and apply it
 */
void regroove_controller_queue_channel_solo(RegrooveController* controller, uint8_t channel);

/**
 * Toggle channel mute immediately
 */
void regroove_controller_toggle_channel_mute(RegrooveController* controller, uint8_t channel);

/**
 * Set channel solo immediately
 */
void regroove_controller_set_channel_solo(RegrooveController* controller, uint8_t channel, bool solo);

/**
 * Get channel mute state
 */
bool regroove_controller_get_channel_mute(const RegrooveController* controller, uint8_t channel);

/**
 * Get channel solo state
 */
bool regroove_controller_get_channel_solo(const RegrooveController* controller, uint8_t channel);

/**
 * Clear all solo states
 */
void regroove_controller_clear_all_solo(RegrooveController* controller);

// ============================================================================
// Position/State Queries
// ============================================================================

/**
 * Get current playback position with row precision
 */
void regroove_controller_get_position(const RegrooveController* controller,
                                     uint16_t* order, uint16_t* row);

/**
 * Get song length (number of orders)
 */
uint16_t regroove_controller_get_song_length(const RegrooveController* controller);

/**
 * Get rows per pattern for a given order
 */
uint16_t regroove_controller_get_rows_per_pattern(const RegrooveController* controller, uint16_t order);

#ifdef __cplusplus
}
#endif

#endif // REGROOVE_CONTROLLER_H
