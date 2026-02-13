/*
 * Example: Using the Regroove Unified API
 *
 * This example demonstrates how to write code that works with both:
 * - Full libopenmpt engine (desktop/server)
 * - Minimal embedded controller (embedded systems)
 *
 * Compile with:
 *   gcc -DUSE_REGROOVE_ENGINE regroove_unified_example.c -o example_engine
 *   gcc -DUSE_REGROOVE_CONTROLLER regroove_unified_example.c -o example_controller
 */

#include <stdio.h>
#include <stdlib.h>

// Select implementation at compile time
// Uncomment one of these, or pass via -D flag:
// #define USE_REGROOVE_ENGINE
// #define USE_REGROOVE_CONTROLLER

#include "regroove_unified.h"

// Example: DJ-style loop control using unified API
void setup_dj_loop(RegrooveHandle* handle) {
    printf("Setting up DJ loop...\n");

    // Set loop range: orders 2-5, full patterns
    rg_set_loop_range_rows(handle, 2, 0, 5, 63);

    // Arm the loop (will activate when we reach order 2)
    rg_arm_loop(handle);

    printf("Loop armed. Will activate at order 2.\n");
}

// Example: Check and display current position
void display_position(RegrooveHandle* handle) {
    int order, row;
    rg_get_position(handle, &order, &row);

    RegrooveLoopState loop_state = rg_get_loop_state(handle);
    const char* state_names[] = {"OFF", "ARMED", "ACTIVE"};

    printf("Position: Order %d, Row %d | Loop: %s\n",
           order, row, state_names[loop_state]);
}

// Example: Pattern mode control
void enable_pattern_mode(RegrooveHandle* handle) {
    printf("Enabling pattern mode (loop current pattern)...\n");
    rg_set_pattern_mode(handle, RG_PATTERN_MODE_SINGLE);
}

void disable_pattern_mode(RegrooveHandle* handle) {
    printf("Disabling pattern mode (return to song mode)...\n");
    rg_set_pattern_mode(handle, RG_PATTERN_MODE_OFF);
}

// Example: Navigation commands
void navigate_song(RegrooveHandle* handle) {
    printf("Queueing next order...\n");
    rg_queue_next_order(handle);

    printf("Current position: ");
    display_position(handle);

    // Jump immediately to a specific position
    printf("Jumping immediately to order 3, row 16...\n");
    rg_jump_immediate(handle, 3, 16);

    display_position(handle);
}

// Example: Channel mute control
void setup_channel_mutes(RegrooveHandle* handle) {
    printf("Muting channel 0...\n");
    rg_toggle_channel_mute(handle, 0);

    printf("Queueing mute toggle for channel 1 (will apply at pattern boundary)...\n");
    rg_queue_channel_mute(handle, 1);
}

// Example: Loop control workflow
void loop_control_demo(RegrooveHandle* handle) {
    printf("\n=== Loop Control Demo ===\n");

    // Check current state
    RegrooveLoopState state = rg_get_loop_state(handle);
    printf("Initial loop state: %d\n", state);

    // Set up loop range
    rg_set_loop_range_rows(handle, 1, 0, 3, 63);
    printf("Loop range set: Order 1-3, full patterns\n");

    // Arm the loop
    rg_arm_loop(handle);
    state = rg_get_loop_state(handle);
    printf("Loop state after arming: %d (ARMED)\n", state);

    // Trigger loop immediately
    rg_trigger_loop(handle);
    state = rg_get_loop_state(handle);
    printf("Loop state after triggering: %d (ACTIVE)\n", state);

    // Disable loop
    rg_disable_loop(handle);
    state = rg_get_loop_state(handle);
    printf("Loop state after disabling: %d (OFF)\n", state);
}

// Example: Song information
void display_song_info(RegrooveHandle* handle) {
    printf("\n=== Song Information ===\n");

    int num_orders = rg_get_num_orders(handle);
    int num_channels = rg_get_num_channels(handle);
    int current_pattern = rg_get_current_pattern(handle);

    printf("Song length: %d orders\n", num_orders);
    printf("Number of channels: %d\n", num_channels);
    printf("Current pattern: %d\n", current_pattern);
}

// Main example
int main(int argc, char** argv) {
    printf("=== Regroove Unified API Example ===\n");

#ifdef USE_REGROOVE_ENGINE
    printf("Using: Full libopenmpt engine\n\n");

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <module_file>\n", argv[0]);
        return 1;
    }

    // Create engine handle
    RegrooveHandle* handle = regroove_create(argv[1], 48000.0);
    if (!handle) {
        fprintf(stderr, "Failed to load module: %s\n", argv[1]);
        return 1;
    }

#else // USE_REGROOVE_CONTROLLER
    printf("Using: Minimal embedded controller\n\n");

    // For controller, you'd need to create a PatternSequencer first
    // This is just a stub for demonstration
    printf("Note: Controller requires a PatternSequencer backend.\n");
    printf("This example shows API compatibility only.\n\n");

    // PatternSequencer* seq = create_mod_player(...);
    // RegrooveHandle* handle = regroove_controller_create(seq);

    // For this example, we'll just return
    printf("Example would continue here with actual player implementation.\n");
    return 0;
#endif

    // All of the following code works with BOTH implementations!

    display_song_info(handle);
    display_position(handle);

    loop_control_demo(handle);

    setup_dj_loop(handle);
    display_position(handle);

    enable_pattern_mode(handle);
    rg_retrigger_pattern(handle);

    navigate_song(handle);

    setup_channel_mutes(handle);

    // Cleanup
    printf("\nCleaning up...\n");
    rg_destroy(handle);

    printf("Done.\n");
    return 0;
}
