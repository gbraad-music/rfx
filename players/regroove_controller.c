/*
 * RegrooveController - Advanced DJ/Performance Layer for Pattern Sequencer
 *
 * Provides advanced features on top of PatternSequencer:
 * - Row-precise loop control with arming/triggering
 * - Command queuing for pattern-boundary execution
 * - Pattern mode (single pattern looping)
 * - Channel mute/solo with queuing
 */

#include "regroove_controller.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define MAX_QUEUED_COMMANDS 16
#define MAX_CHANNELS 64

// Queued command structure
typedef struct {
    RegrooveCommandType type;
    uint16_t param1;  // order for jump, channel for mute/solo
    uint16_t param2;  // row for jump
} QueuedCommand;

// Internal structure
struct RegrooveController {
    // Wrapped pattern sequencer
    PatternSequencer* sequencer;

    // Loop control
    RegrooveLoopState loop_state;
    uint16_t loop_start_order;
    uint16_t loop_start_row;
    uint16_t loop_end_order;
    uint16_t loop_end_row;

    // Command queue
    QueuedCommand command_queue[MAX_QUEUED_COMMANDS];
    uint8_t queue_size;
    bool execute_on_pattern_boundary;

    // Pattern mode
    RegroovePatternMode pattern_mode;
    uint16_t pattern_mode_locked_order;

    // Channel control
    bool channel_muted[MAX_CHANNELS];
    bool channel_solo[MAX_CHANNELS];
    bool any_solo_active;

    // Current position tracking
    uint16_t current_order;
    uint16_t current_row;
    uint16_t last_order;  // For detecting pattern boundaries

    // Callbacks
    RegrooveControllerCallbacks callbacks;
    void* callback_user_data;

    // Original sequencer callbacks (we intercept these)
    PatternSequencerCallbacks original_callbacks;
    void* original_user_data;
};

// Forward declarations for internal callbacks
static void rg_on_tick(void* user_data, uint8_t tick);
static void rg_on_row(void* user_data, uint16_t pattern_index, uint16_t pattern_number, uint16_t row);
static void rg_on_pattern_change(void* user_data, uint16_t old_pattern, uint16_t new_pattern);
static bool rg_on_song_end(void* user_data);

// ============================================================================
// Core Functions
// ============================================================================

RegrooveController* regroove_controller_create(PatternSequencer* sequencer) {
    if (!sequencer) return NULL;

    RegrooveController* ctrl = calloc(1, sizeof(RegrooveController));
    if (!ctrl) return NULL;

    ctrl->sequencer = sequencer;
    ctrl->loop_state = RG_LOOP_OFF;
    ctrl->pattern_mode = RG_PATTERN_MODE_OFF;
    ctrl->queue_size = 0;
    ctrl->execute_on_pattern_boundary = true;
    ctrl->any_solo_active = false;

    // Get original callbacks from sequencer (if any)
    // We'll chain to them after our processing
    // Note: This requires pattern_sequencer to expose a way to get current callbacks
    // For now, we'll just intercept and store user data

    // Set our intercepting callbacks
    PatternSequencerCallbacks intercept_callbacks = {
        .on_tick = rg_on_tick,
        .on_row = rg_on_row,
        .on_pattern_change = rg_on_pattern_change,
        .on_song_end = rg_on_song_end
    };
    pattern_sequencer_set_callbacks(sequencer, &intercept_callbacks, ctrl);

    return ctrl;
}

void regroove_controller_destroy(RegrooveController* controller) {
    if (!controller) return;

    // Destroy wrapped sequencer
    if (controller->sequencer) {
        pattern_sequencer_destroy(controller->sequencer);
    }

    free(controller);
}

PatternSequencer* regroove_controller_get_sequencer(RegrooveController* controller) {
    return controller ? controller->sequencer : NULL;
}

void regroove_controller_set_callbacks(RegrooveController* controller,
                                      const RegrooveControllerCallbacks* callbacks,
                                      void* user_data) {
    if (!controller) return;

    if (callbacks) {
        controller->callbacks = *callbacks;
    } else {
        memset(&controller->callbacks, 0, sizeof(RegrooveControllerCallbacks));
    }
    controller->callback_user_data = user_data;
}

// ============================================================================
// Internal Callback Implementations
// ============================================================================

static void rg_on_tick(void* user_data, uint8_t tick) {
    RegrooveController* ctrl = (RegrooveController*)user_data;

    // Chain to original callback if it exists
    if (ctrl->original_callbacks.on_tick) {
        ctrl->original_callbacks.on_tick(ctrl->original_user_data, tick);
    }
}

static void execute_queued_commands(RegrooveController* ctrl) {
    for (uint8_t i = 0; i < ctrl->queue_size; i++) {
        QueuedCommand* cmd = &ctrl->command_queue[i];

        switch (cmd->type) {
            case RG_CMD_JUMP_TO_ORDER:
                pattern_sequencer_set_position(ctrl->sequencer, cmd->param1, cmd->param2);
                ctrl->current_order = cmd->param1;
                ctrl->current_row = cmd->param2;
                break;

            case RG_CMD_NEXT_ORDER: {
                uint16_t song_length = pattern_sequencer_get_song_length(ctrl->sequencer);
                uint16_t next_order = (ctrl->current_order + 1) % song_length;
                pattern_sequencer_set_position(ctrl->sequencer, next_order, 0);
                ctrl->current_order = next_order;
                ctrl->current_row = 0;
                break;
            }

            case RG_CMD_PREV_ORDER: {
                uint16_t song_length = pattern_sequencer_get_song_length(ctrl->sequencer);
                uint16_t prev_order = (ctrl->current_order == 0) ? (song_length - 1) : (ctrl->current_order - 1);
                pattern_sequencer_set_position(ctrl->sequencer, prev_order, 0);
                ctrl->current_order = prev_order;
                ctrl->current_row = 0;
                break;
            }

            case RG_CMD_RETRIGGER_PATTERN:
                pattern_sequencer_set_position(ctrl->sequencer, ctrl->current_order, 0);
                ctrl->current_row = 0;
                break;

            case RG_CMD_TOGGLE_CHANNEL_MUTE:
                if (cmd->param1 < MAX_CHANNELS) {
                    ctrl->channel_muted[cmd->param1] = !ctrl->channel_muted[cmd->param1];
                }
                break;

            case RG_CMD_SET_CHANNEL_SOLO:
                if (cmd->param1 < MAX_CHANNELS) {
                    ctrl->channel_solo[cmd->param1] = cmd->param2;
                    // Update any_solo_active flag
                    ctrl->any_solo_active = false;
                    for (int ch = 0; ch < MAX_CHANNELS; ch++) {
                        if (ctrl->channel_solo[ch]) {
                            ctrl->any_solo_active = true;
                            break;
                        }
                    }
                }
                break;

            default:
                break;
        }

        // Callback for command execution
        if (ctrl->callbacks.on_command_executed) {
            ctrl->callbacks.on_command_executed(ctrl->callback_user_data, cmd->type);
        }
    }

    // Clear queue
    ctrl->queue_size = 0;
}

static void rg_on_row(void* user_data, uint16_t pattern_index, uint16_t pattern_number, uint16_t row) {
    RegrooveController* ctrl = (RegrooveController*)user_data;

    // Update position tracking
    ctrl->current_order = pattern_index;
    ctrl->current_row = row;

    // Detect pattern boundary (order changed)
    bool pattern_boundary = (pattern_index != ctrl->last_order);
    if (pattern_boundary) {
        ctrl->last_order = pattern_index;

        // Execute queued commands on pattern boundary
        if (ctrl->execute_on_pattern_boundary && ctrl->queue_size > 0) {
            execute_queued_commands(ctrl);
        }
    }

    // Handle pattern mode
    if (ctrl->pattern_mode == RG_PATTERN_MODE_SINGLE) {
        // Lock to current pattern - check if we've moved to a different order
        if (pattern_index != ctrl->pattern_mode_locked_order) {
            // Jump back to start of locked pattern
            pattern_sequencer_set_position(ctrl->sequencer, ctrl->pattern_mode_locked_order, 0);
            ctrl->current_order = ctrl->pattern_mode_locked_order;
            ctrl->current_row = 0;
            return; // Don't process further this frame
        }
    }

    // Handle loop arming/triggering
    if (ctrl->loop_state == RG_LOOP_ARMED) {
        // Check if we've reached the loop start point
        if (pattern_index == ctrl->loop_start_order && row == ctrl->loop_start_row) {
            // Activate loop
            RegrooveLoopState old_state = ctrl->loop_state;
            ctrl->loop_state = RG_LOOP_ACTIVE;

            if (ctrl->callbacks.on_loop_state_change) {
                ctrl->callbacks.on_loop_state_change(ctrl->callback_user_data, old_state, RG_LOOP_ACTIVE);
            }
            if (ctrl->callbacks.on_loop_trigger) {
                ctrl->callbacks.on_loop_trigger(ctrl->callback_user_data, pattern_index, row);
            }
        }
    }

    // Handle active looping (row-precise)
    if (ctrl->loop_state == RG_LOOP_ACTIVE) {
        // Check if we've reached the loop end point
        if (pattern_index == ctrl->loop_end_order && row == ctrl->loop_end_row) {
            // Jump back to loop start
            pattern_sequencer_set_position(ctrl->sequencer, ctrl->loop_start_order, ctrl->loop_start_row);
            ctrl->current_order = ctrl->loop_start_order;
            ctrl->current_row = ctrl->loop_start_row;
            return; // Don't process further this frame
        }
    }

    // Callback for note event (can be used for visualization)
    // This fires on every row
    if (ctrl->callbacks.on_note) {
        // We don't know the channel here, so we'll call for all channels
        // The player can filter based on actual note data
        ctrl->callbacks.on_note(ctrl->callback_user_data, 0, pattern_index, row);
    }

    // Chain to original callback if it exists
    if (ctrl->original_callbacks.on_row) {
        ctrl->original_callbacks.on_row(ctrl->original_user_data, pattern_index, pattern_number, row);
    }
}

static void rg_on_pattern_change(void* user_data, uint16_t old_pattern, uint16_t new_pattern) {
    RegrooveController* ctrl = (RegrooveController*)user_data;

    // Chain to original callback if it exists
    if (ctrl->original_callbacks.on_pattern_change) {
        ctrl->original_callbacks.on_pattern_change(ctrl->original_user_data, old_pattern, new_pattern);
    }
}

static bool rg_on_song_end(void* user_data) {
    RegrooveController* ctrl = (RegrooveController*)user_data;

    // Chain to original callback if it exists
    if (ctrl->original_callbacks.on_song_end) {
        return ctrl->original_callbacks.on_song_end(ctrl->original_user_data);
    }

    return true;  // Default: continue looping
}

// ============================================================================
// Process Function
// ============================================================================

void regroove_controller_process(RegrooveController* controller,
                                uint32_t frames,
                                double sample_rate) {
    if (!controller || !controller->sequencer) return;

    // Delegate to wrapped sequencer (our callbacks will be called)
    pattern_sequencer_process(controller->sequencer, frames, sample_rate);
}

// ============================================================================
// Advanced Loop Control
// ============================================================================

void regroove_controller_set_loop_range_rows(RegrooveController* controller,
                                             uint16_t start_order, uint16_t start_row,
                                             uint16_t end_order, uint16_t end_row) {
    if (!controller) return;

    controller->loop_start_order = start_order;
    controller->loop_start_row = start_row;
    controller->loop_end_order = end_order;
    controller->loop_end_row = end_row;

    // Also update underlying sequencer's loop range (order-level only)
    pattern_sequencer_set_loop_range(controller->sequencer, start_order, end_order);
}

void regroove_controller_get_loop_range_rows(const RegrooveController* controller,
                                             uint16_t* start_order, uint16_t* start_row,
                                             uint16_t* end_order, uint16_t* end_row) {
    if (!controller) return;

    if (start_order) *start_order = controller->loop_start_order;
    if (start_row) *start_row = controller->loop_start_row;
    if (end_order) *end_order = controller->loop_end_order;
    if (end_row) *end_row = controller->loop_end_row;
}

void regroove_controller_arm_loop(RegrooveController* controller) {
    if (!controller) return;

    RegrooveLoopState old_state = controller->loop_state;
    controller->loop_state = RG_LOOP_ARMED;

    if (controller->callbacks.on_loop_state_change) {
        controller->callbacks.on_loop_state_change(controller->callback_user_data, old_state, RG_LOOP_ARMED);
    }
}

void regroove_controller_trigger_loop(RegrooveController* controller) {
    if (!controller) return;

    // Jump to loop start immediately
    pattern_sequencer_set_position(controller->sequencer,
                                   controller->loop_start_order,
                                   controller->loop_start_row);
    controller->current_order = controller->loop_start_order;
    controller->current_row = controller->loop_start_row;

    // Activate loop
    RegrooveLoopState old_state = controller->loop_state;
    controller->loop_state = RG_LOOP_ACTIVE;

    if (controller->callbacks.on_loop_state_change) {
        controller->callbacks.on_loop_state_change(controller->callback_user_data, old_state, RG_LOOP_ACTIVE);
    }
    if (controller->callbacks.on_loop_trigger) {
        controller->callbacks.on_loop_trigger(controller->callback_user_data,
                                             controller->loop_start_order,
                                             controller->loop_start_row);
    }
}

void regroove_controller_disable_loop(RegrooveController* controller) {
    if (!controller) return;

    RegrooveLoopState old_state = controller->loop_state;
    controller->loop_state = RG_LOOP_OFF;

    if (controller->callbacks.on_loop_state_change) {
        controller->callbacks.on_loop_state_change(controller->callback_user_data, old_state, RG_LOOP_OFF);
    }
}

RegrooveLoopState regroove_controller_get_loop_state(const RegrooveController* controller) {
    return controller ? controller->loop_state : RG_LOOP_OFF;
}

// ============================================================================
// Queued Commands
// ============================================================================

static void queue_command(RegrooveController* controller, RegrooveCommandType type,
                         uint16_t param1, uint16_t param2) {
    if (!controller || controller->queue_size >= MAX_QUEUED_COMMANDS) return;

    QueuedCommand* cmd = &controller->command_queue[controller->queue_size++];
    cmd->type = type;
    cmd->param1 = param1;
    cmd->param2 = param2;
}

void regroove_controller_queue_jump(RegrooveController* controller,
                                   uint16_t order, uint16_t row) {
    queue_command(controller, RG_CMD_JUMP_TO_ORDER, order, row);
}

void regroove_controller_queue_next_order(RegrooveController* controller) {
    queue_command(controller, RG_CMD_NEXT_ORDER, 0, 0);
}

void regroove_controller_queue_prev_order(RegrooveController* controller) {
    queue_command(controller, RG_CMD_PREV_ORDER, 0, 0);
}

void regroove_controller_queue_retrigger_pattern(RegrooveController* controller) {
    queue_command(controller, RG_CMD_RETRIGGER_PATTERN, 0, 0);
}

void regroove_controller_jump_immediate(RegrooveController* controller,
                                       uint16_t order, uint16_t row) {
    if (!controller) return;

    pattern_sequencer_set_position(controller->sequencer, order, row);
    controller->current_order = order;
    controller->current_row = row;
}

void regroove_controller_clear_queue(RegrooveController* controller) {
    if (!controller) return;
    controller->queue_size = 0;
}

// ============================================================================
// Pattern Mode
// ============================================================================

void regroove_controller_set_pattern_mode(RegrooveController* controller,
                                         RegroovePatternMode mode) {
    if (!controller) return;

    RegroovePatternMode old_mode = controller->pattern_mode;
    controller->pattern_mode = mode;

    if (mode == RG_PATTERN_MODE_SINGLE) {
        // Lock to current pattern
        controller->pattern_mode_locked_order = controller->current_order;
    }

    if (controller->callbacks.on_pattern_mode_change) {
        controller->callbacks.on_pattern_mode_change(controller->callback_user_data, mode);
    }
}

RegroovePatternMode regroove_controller_get_pattern_mode(const RegrooveController* controller) {
    return controller ? controller->pattern_mode : RG_PATTERN_MODE_OFF;
}

void regroove_controller_retrigger_pattern(RegrooveController* controller) {
    if (!controller) return;

    pattern_sequencer_set_position(controller->sequencer, controller->current_order, 0);
    controller->current_row = 0;
}

// ============================================================================
// Channel Control
// ============================================================================

void regroove_controller_queue_channel_mute(RegrooveController* controller, uint8_t channel) {
    queue_command(controller, RG_CMD_TOGGLE_CHANNEL_MUTE, channel, 0);
}

void regroove_controller_queue_channel_solo(RegrooveController* controller, uint8_t channel) {
    queue_command(controller, RG_CMD_SET_CHANNEL_SOLO, channel, 1);
}

void regroove_controller_toggle_channel_mute(RegrooveController* controller, uint8_t channel) {
    if (!controller || channel >= MAX_CHANNELS) return;
    controller->channel_muted[channel] = !controller->channel_muted[channel];
}

void regroove_controller_set_channel_solo(RegrooveController* controller, uint8_t channel, bool solo) {
    if (!controller || channel >= MAX_CHANNELS) return;

    controller->channel_solo[channel] = solo;

    // Update any_solo_active flag
    controller->any_solo_active = false;
    for (int ch = 0; ch < MAX_CHANNELS; ch++) {
        if (controller->channel_solo[ch]) {
            controller->any_solo_active = true;
            break;
        }
    }
}

bool regroove_controller_get_channel_mute(const RegrooveController* controller, uint8_t channel) {
    if (!controller || channel >= MAX_CHANNELS) return false;

    // If any solo is active, non-solo channels are effectively muted
    if (controller->any_solo_active) {
        return !controller->channel_solo[channel];
    }

    return controller->channel_muted[channel];
}

bool regroove_controller_get_channel_solo(const RegrooveController* controller, uint8_t channel) {
    if (!controller || channel >= MAX_CHANNELS) return false;
    return controller->channel_solo[channel];
}

void regroove_controller_clear_all_solo(RegrooveController* controller) {
    if (!controller) return;

    memset(controller->channel_solo, 0, sizeof(controller->channel_solo));
    controller->any_solo_active = false;
}

// ============================================================================
// Position/State Queries
// ============================================================================

void regroove_controller_get_position(const RegrooveController* controller,
                                     uint16_t* order, uint16_t* row) {
    if (!controller) return;

    if (order) *order = controller->current_order;
    if (row) *row = controller->current_row;
}

uint16_t regroove_controller_get_song_length(const RegrooveController* controller) {
    if (!controller || !controller->sequencer) return 0;
    return pattern_sequencer_get_song_length(controller->sequencer);
}

uint16_t regroove_controller_get_rows_per_pattern(const RegrooveController* controller, uint16_t order) {
    if (!controller || !controller->sequencer) return 0;
    // PatternSequencer doesn't expose rows per pattern for individual orders
    // This would need to be added or queried from the player
    // For now, return the default rows_per_pattern
    // TODO: Add API to pattern_sequencer to query this
    return 64; // Default assumption
}
