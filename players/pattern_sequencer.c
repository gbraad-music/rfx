/*
 * Pattern Sequencer Implementation
 */

#include "pattern_sequencer.h"
#include <stdlib.h>
#include <string.h>

struct PatternSequencer {
    // Song structure
    uint16_t* pattern_order;     // Array of pattern numbers (owned)
    uint16_t order_length;       // Length of pattern_order
    uint16_t rows_per_pattern;   // Rows per pattern

    // Timing mode
    PatternSequencerMode mode;   // Tick-based (MOD/MMD) or frame-based (AHX)

    // Playback state
    bool playing;
    uint16_t current_pattern_index;  // Index in pattern_order
    uint16_t current_row;
    uint8_t tick;
    uint8_t speed;               // Ticks per row (default 6)
    uint8_t bpm;                 // Beats per minute (default 125)

    // Timing (double for precise accumulation)
    double samples_per_tick;
    double sample_accumulator;

    // Loop control
    uint16_t loop_start;
    uint16_t loop_end;
    bool looping_enabled;

    // Pattern loop (E6x effect)
    uint16_t pattern_loop_row;
    uint8_t pattern_loop_count;
    uint8_t pattern_loop_target;
    bool pattern_loop_pending;

    // Pattern delay (EEx effect)
    uint8_t pattern_delay;
    bool in_pattern_delay;

    // Position jump (B+D combination)
    bool jump_pending;
    uint16_t jump_to_pattern;
    uint16_t jump_to_row;

    // Callbacks
    PatternSequencerCallbacks callbacks;
    void* user_data;

    // Track last position for change detection
    uint16_t last_pattern_index;
    uint16_t last_row;
};

// Create sequencer
PatternSequencer* pattern_sequencer_create(void) {
    PatternSequencer* seq = (PatternSequencer*)calloc(1, sizeof(PatternSequencer));
    if (!seq) return NULL;

    // Initialize defaults
    seq->mode = PS_MODE_TICK_BASED;  // Default to tick-based (MOD/MMD)
    seq->speed = 6;
    seq->bpm = 125;
    seq->looping_enabled = true;
    seq->rows_per_pattern = 64;  // Standard MOD/tracker default
    seq->samples_per_tick = 0.0;
    seq->sample_accumulator = 0.0;

    return seq;
}

// Destroy sequencer
void pattern_sequencer_destroy(PatternSequencer* seq) {
    if (!seq) return;

    if (seq->pattern_order) {
        free(seq->pattern_order);
    }

    free(seq);
}

// Set timing mode
void pattern_sequencer_set_mode(PatternSequencer* seq, PatternSequencerMode mode) {
    if (!seq) return;
    seq->mode = mode;
}

// Set callbacks
void pattern_sequencer_set_callbacks(PatternSequencer* seq,
                                     const PatternSequencerCallbacks* callbacks,
                                     void* user_data) {
    if (!seq) return;

    if (callbacks) {
        seq->callbacks = *callbacks;
    } else {
        memset(&seq->callbacks, 0, sizeof(PatternSequencerCallbacks));
    }
    seq->user_data = user_data;
}

// Set song structure
void pattern_sequencer_set_song(PatternSequencer* seq,
                                const uint16_t* pattern_order,
                                uint16_t order_length,
                                uint16_t rows_per_pattern) {
    if (!seq) return;

    // Free old pattern order
    if (seq->pattern_order) {
        free(seq->pattern_order);
        seq->pattern_order = NULL;
    }

    // Copy pattern order
    if (pattern_order && order_length > 0) {
        seq->pattern_order = (uint16_t*)malloc(order_length * sizeof(uint16_t));
        if (seq->pattern_order) {
            memcpy(seq->pattern_order, pattern_order, order_length * sizeof(uint16_t));
            seq->order_length = order_length;
        }
    }

    seq->rows_per_pattern = rows_per_pattern;

    // Reset position
    seq->current_pattern_index = 0;
    seq->current_row = 0;
    seq->tick = 0;

    // Default loop: entire song
    seq->loop_start = 0;
    seq->loop_end = order_length > 0 ? order_length - 1 : 0;
}

// Start playback
void pattern_sequencer_start(PatternSequencer* seq) {
    if (!seq) return;

    seq->playing = true;
    seq->current_pattern_index = seq->loop_start;
    seq->current_row = 0;
    seq->tick = 0;
    seq->sample_accumulator = 0.0;

    // Clear flow control state
    seq->pattern_loop_row = 0;
    seq->pattern_loop_count = 0;
    seq->pattern_loop_pending = false;
    seq->pattern_delay = 0;
    seq->in_pattern_delay = false;
    seq->jump_pending = false;

    // Trigger initial callbacks
    if (seq->callbacks.on_pattern_change && seq->pattern_order) {
        uint16_t pattern_num = seq->pattern_order[seq->current_pattern_index];
        seq->callbacks.on_pattern_change(seq->user_data, seq->current_pattern_index, pattern_num);
    }

    seq->last_pattern_index = seq->current_pattern_index;
    seq->last_row = seq->current_row;
}

// Stop playback
void pattern_sequencer_stop(PatternSequencer* seq) {
    if (!seq) return;
    seq->playing = false;
}

// Check if playing
bool pattern_sequencer_is_playing(const PatternSequencer* seq) {
    return seq ? seq->playing : false;
}

// Set BPM
void pattern_sequencer_set_bpm(PatternSequencer* seq, uint8_t bpm) {
    if (!seq) return;
    if (bpm < 32) bpm = 32;
    if (bpm > 255) bpm = 255;
    seq->bpm = bpm;
}

// Set speed
void pattern_sequencer_set_speed(PatternSequencer* seq, uint8_t speed) {
    if (!seq) return;
    if (speed < 1) speed = 1;
    if (speed > 31) speed = 31;
    seq->speed = speed;
}

// Get BPM
uint8_t pattern_sequencer_get_bpm(const PatternSequencer* seq) {
    return seq ? seq->bpm : 125;
}

// Get speed
uint8_t pattern_sequencer_get_speed(const PatternSequencer* seq) {
    return seq ? seq->speed : 6;
}

// Set loop range
void pattern_sequencer_set_loop_range(PatternSequencer* seq,
                                      uint16_t start_index,
                                      uint16_t end_index) {
    if (!seq) return;

    seq->loop_start = start_index;
    seq->loop_end = end_index;

    // If both are 0, loop entire song
    if (start_index == 0 && end_index == 0) {
        seq->loop_end = seq->order_length > 0 ? seq->order_length - 1 : 0;
    }
}

// Set looping enabled
void pattern_sequencer_set_looping(PatternSequencer* seq, bool enabled) {
    if (!seq) return;
    seq->looping_enabled = enabled;
}

// Get position
void pattern_sequencer_get_position(const PatternSequencer* seq,
                                    uint16_t* pattern_index,
                                    uint16_t* pattern_number,
                                    uint16_t* row) {
    if (!seq) return;

    if (pattern_index) *pattern_index = seq->current_pattern_index;
    if (row) *row = seq->current_row;

    if (pattern_number) {
        if (seq->pattern_order && seq->current_pattern_index < seq->order_length) {
            *pattern_number = seq->pattern_order[seq->current_pattern_index];
        } else {
            *pattern_number = 0;
        }
    }
}

// Get song length
uint16_t pattern_sequencer_get_song_length(const PatternSequencer* seq) {
    if (!seq) return 0;
    return seq->order_length;
}

// Set position
void pattern_sequencer_set_position(PatternSequencer* seq,
                                    uint16_t pattern_index,
                                    uint16_t row) {
    if (!seq) return;

    if (pattern_index < seq->order_length) {
        seq->current_pattern_index = pattern_index;
        seq->current_row = row;
        seq->tick = 0;
        seq->sample_accumulator = 0.0;

        // Trigger pattern change callback
        if (seq->callbacks.on_pattern_change && seq->pattern_order) {
            uint16_t pattern_num = seq->pattern_order[seq->current_pattern_index];
            seq->callbacks.on_pattern_change(seq->user_data, seq->current_pattern_index, pattern_num);
        }
    }
}

// Pattern break (Dxx)
void pattern_sequencer_pattern_break(PatternSequencer* seq, uint16_t row) {
    if (!seq) return;

    seq->jump_pending = true;
    seq->jump_to_pattern = seq->current_pattern_index + 1;
    seq->jump_to_row = row;
}

// Position jump (Bxx)
void pattern_sequencer_position_jump(PatternSequencer* seq, uint16_t pattern_index) {
    if (!seq) return;

    seq->jump_pending = true;
    seq->jump_to_pattern = pattern_index;
    seq->jump_to_row = 0;
}

// Jump to position (B+D combination)
void pattern_sequencer_jump_to(PatternSequencer* seq,
                               uint16_t pattern_index,
                               uint16_t row) {
    if (!seq) return;

    seq->jump_pending = true;
    seq->jump_to_pattern = pattern_index;
    seq->jump_to_row = row;
}

// Set pattern loop start (E60)
void pattern_sequencer_set_pattern_loop_start(PatternSequencer* seq) {
    if (!seq) return;
    seq->pattern_loop_row = seq->current_row;
}

// Execute pattern loop (E6x)
void pattern_sequencer_execute_pattern_loop(PatternSequencer* seq, uint8_t count) {
    if (!seq) return;

    if (seq->pattern_loop_count == 0) {
        // First time: set up loop
        seq->pattern_loop_count = 1;
        seq->pattern_loop_target = count;
        seq->pattern_loop_pending = true;
    } else if (seq->pattern_loop_count < seq->pattern_loop_target) {
        // Continue looping
        seq->pattern_loop_count++;
        seq->pattern_loop_pending = true;
    } else {
        // Loop finished
        seq->pattern_loop_count = 0;
        seq->pattern_loop_pending = false;
    }
}

// Pattern delay (EEx)
void pattern_sequencer_pattern_delay(PatternSequencer* seq, uint8_t count) {
    if (!seq) return;

    // Only set delay if not already in a delay
    if (!seq->in_pattern_delay) {
        seq->pattern_delay = count;
    }
}

// Get current tick
uint8_t pattern_sequencer_get_current_tick(const PatternSequencer* seq) {
    return seq ? seq->tick : 0;
}

// Get samples per tick
double pattern_sequencer_get_samples_per_tick(const PatternSequencer* seq) {
    return seq ? seq->samples_per_tick : 0.0;
}

// Helper to recalculate timing (called when BPM changes or at start of buffer)
static inline void recalculate_timing(PatternSequencer* seq, uint32_t sample_rate) {
    if (seq->mode == PS_MODE_TICK_BASED) {
        // Tick-based (MOD/MMD): BPM controls timing
        // samples_per_tick = (2.5 * sample_rate) / BPM
        // This matches standard ProTracker CIA timer timing
        seq->samples_per_tick = (2.5 * sample_rate) / (double)seq->bpm;
    } else {
        // Frame-based (AHX/HVL): BPM represents frame rate in Hz
        // samples_per_tick = sample_rate / frame_rate
        // Default is 50Hz (PAL), but can be adjusted via SpeedMultiplier (50, 100, 150, 200)
        seq->samples_per_tick = sample_rate / (double)seq->bpm;
    }
}

// Main process function
void pattern_sequencer_process(PatternSequencer* seq,
                               uint32_t frames,
                               uint32_t sample_rate) {
    if (!seq || !seq->playing || !seq->pattern_order || seq->order_length == 0) {
        return;
    }

    // Calculate samples per tick at start of buffer
    // NOTE: This will be recalculated mid-buffer if BPM changes (see pattern_sequencer_set_bpm)
    recalculate_timing(seq, sample_rate);

    for (uint32_t frame = 0; frame < frames; frame++) {
        // Check if it's time for a tick
        if (seq->sample_accumulator >= seq->samples_per_tick) {
            seq->sample_accumulator -= seq->samples_per_tick;

            // Handle pattern loop jump
            if (seq->pattern_loop_pending) {
                seq->pattern_loop_pending = false;
                seq->current_row = seq->pattern_loop_row;
                seq->tick = 0;
                // Don't continue to normal row advance
                goto tick_done;
            }

            // Tick callback (called every tick)
            if (seq->callbacks.on_tick) {
                seq->callbacks.on_tick(seq->user_data, seq->tick);
            }

            seq->tick++;

            // Check if it's time to advance to next row
            if (seq->tick >= seq->speed) {
                seq->tick = 0;

                // Handle pattern delay
                if (seq->pattern_delay > 0) {
                    seq->pattern_delay--;
                    seq->in_pattern_delay = true;
                    // Process same row again, but don't trigger on_row
                    goto tick_done;
                }
                seq->in_pattern_delay = false;

                // Row callback (called when advancing to new row)
                if (seq->callbacks.on_row && seq->pattern_order) {
                    uint16_t pattern_num = seq->pattern_order[seq->current_pattern_index];
                    seq->callbacks.on_row(seq->user_data,
                                         seq->current_pattern_index,
                                         pattern_num,
                                         seq->current_row);
                }

                // Handle pending jump (B or D effect)
                if (seq->jump_pending) {
                    seq->jump_pending = false;

                    // Clamp to valid range
                    if (seq->jump_to_pattern >= seq->order_length) {
                        seq->jump_to_pattern = 0;
                    }

                    seq->current_pattern_index = seq->jump_to_pattern;
                    seq->current_row = seq->jump_to_row;

                    // Clear pattern loop on jump
                    seq->pattern_loop_row = 0;
                    seq->pattern_loop_count = 0;

                    // Trigger pattern change callback
                    if (seq->callbacks.on_pattern_change && seq->pattern_order) {
                        uint16_t pattern_num = seq->pattern_order[seq->current_pattern_index];
                        seq->callbacks.on_pattern_change(seq->user_data,
                                                         seq->current_pattern_index,
                                                         pattern_num);
                    }
                } else {
                    // Normal row advance
                    seq->current_row++;

                    // Check if we've reached end of pattern
                    if (seq->current_row >= seq->rows_per_pattern) {
                        seq->current_row = 0;
                        seq->current_pattern_index++;

                        // Clear pattern loop when advancing to new pattern
                        seq->pattern_loop_row = 0;
                        seq->pattern_loop_count = 0;

                        // Check if we've reached end of song order
                        if (seq->current_pattern_index > seq->loop_end) {
                            // Song ended
                            bool should_continue = true;

                            if (seq->callbacks.on_song_end) {
                                should_continue = seq->callbacks.on_song_end(seq->user_data);
                            }

                            if (should_continue && seq->looping_enabled) {
                                // Loop back to start
                                seq->current_pattern_index = seq->loop_start;
                            } else {
                                // Stop playback
                                seq->playing = false;
                                return;
                            }
                        }

                        // Trigger pattern change callback
                        if (seq->callbacks.on_pattern_change && seq->pattern_order) {
                            uint16_t pattern_num = seq->pattern_order[seq->current_pattern_index];
                            seq->callbacks.on_pattern_change(seq->user_data,
                                                             seq->current_pattern_index,
                                                             pattern_num);
                        }
                    }
                }
            }

tick_done:
            (void)0; // Empty statement for label
        }

        // Increment sample accumulator AFTER tick processing
        // This matches the original MOD player timing and prevents systematic drift
        seq->sample_accumulator += 1.0;
    }

    // Update last position
    seq->last_pattern_index = seq->current_pattern_index;
    seq->last_row = seq->current_row;
}

// Update timing (call once per buffer for efficiency)
void pattern_sequencer_update_timing(PatternSequencer* seq, uint32_t sample_rate) {
    if (!seq) return;
    recalculate_timing(seq, sample_rate);
}

// Process a single sample (optimized for per-sample loops)
void pattern_sequencer_process_sample(PatternSequencer* seq) {
    if (!seq || !seq->playing || !seq->pattern_order || seq->order_length == 0) {
        return;
    }

    // Check if it's time for a tick
    if (seq->sample_accumulator >= seq->samples_per_tick) {
        seq->sample_accumulator -= seq->samples_per_tick;

        // Handle pattern loop jump
        if (seq->pattern_loop_pending) {
            seq->pattern_loop_pending = false;
            seq->current_row = seq->pattern_loop_row;
            seq->tick = 0;
            // Don't continue to normal row advance
            goto tick_done;
        }

        // Tick callback (called every tick)
        if (seq->callbacks.on_tick) {
            seq->callbacks.on_tick(seq->user_data, seq->tick);
        }

        seq->tick++;

        // Check if it's time to advance to next row
        if (seq->tick >= seq->speed) {
            seq->tick = 0;

            // Handle pattern delay
            if (seq->pattern_delay > 0) {
                seq->pattern_delay--;
                seq->in_pattern_delay = true;
                // Process same row again, but don't trigger on_row
                goto tick_done;
            }
            seq->in_pattern_delay = false;

            // Row callback (called when advancing to new row)
            if (seq->callbacks.on_row && seq->pattern_order) {
                uint16_t pattern_num = seq->pattern_order[seq->current_pattern_index];
                seq->callbacks.on_row(seq->user_data,
                                     seq->current_pattern_index,
                                     pattern_num,
                                     seq->current_row);
            }

            // Handle pending jump (B or D effect)
            if (seq->jump_pending) {
                seq->jump_pending = false;

                // Clamp to valid range
                if (seq->jump_to_pattern >= seq->order_length) {
                    seq->jump_to_pattern = 0;
                }

                seq->current_pattern_index = seq->jump_to_pattern;
                seq->current_row = seq->jump_to_row;

                // Clear pattern loop on jump
                seq->pattern_loop_row = 0;
                seq->pattern_loop_count = 0;

                // Trigger pattern change callback
                if (seq->callbacks.on_pattern_change && seq->pattern_order) {
                    uint16_t pattern_num = seq->pattern_order[seq->current_pattern_index];
                    seq->callbacks.on_pattern_change(seq->user_data,
                                                     seq->current_pattern_index,
                                                     pattern_num);
                }
            } else {
                // Normal row advance
                seq->current_row++;

                // Check if we've reached end of pattern
                if (seq->current_row >= seq->rows_per_pattern) {
                    seq->current_row = 0;
                    seq->current_pattern_index++;

                    // Clear pattern loop when advancing to new pattern
                    seq->pattern_loop_row = 0;
                    seq->pattern_loop_count = 0;

                    // Check if we've reached end of song order
                    if (seq->current_pattern_index > seq->loop_end) {
                        // Song ended
                        bool should_continue = true;

                        if (seq->callbacks.on_song_end) {
                            should_continue = seq->callbacks.on_song_end(seq->user_data);
                        }

                        if (should_continue && seq->looping_enabled) {
                            // Loop back to start
                            seq->current_pattern_index = seq->loop_start;
                        } else {
                            // Stop playback
                            seq->playing = false;
                            return;
                        }
                    }

                    // Trigger pattern change callback
                    if (seq->callbacks.on_pattern_change && seq->pattern_order) {
                        uint16_t pattern_num = seq->pattern_order[seq->current_pattern_index];
                        seq->callbacks.on_pattern_change(seq->user_data,
                                                         seq->current_pattern_index,
                                                         pattern_num);
                    }
                }
            }
        }

tick_done:
        (void)0; // Empty statement for label
    }

    // Increment sample accumulator AFTER tick processing
    seq->sample_accumulator += 1.0;

    // Update last position
    seq->last_pattern_index = seq->current_pattern_index;
    seq->last_row = seq->current_row;
}
