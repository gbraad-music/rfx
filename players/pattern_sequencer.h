/*
 * Pattern Sequencer - Generic Tracker Timing/Sequencing Engine
 *
 * Extracts the common timing and pattern sequencing logic shared by:
 * - MOD (ProTracker)
 * - MMD (OctaMED)
 * - AHX/HVL
 * - Other tracker formats
 *
 * This component handles:
 * - Tick/Row/Pattern timing
 * - BPM-based timing calculation
 * - Pattern order sequencing
 * - Common pattern flow effects (break, jump, loop)
 * - Position management
 *
 * Format-specific behavior (note parsing, effects) is handled via callbacks.
 */

#ifndef PATTERN_SEQUENCER_H
#define PATTERN_SEQUENCER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct PatternSequencer PatternSequencer;

/**
 * Timing mode: controls how samples_per_tick is calculated
 */
typedef enum {
    /**
     * Tick-based mode (MOD/MMD): Uses BPM for timing
     * samples_per_tick = (2.5 * sample_rate) / bpm
     * One "tick" represents a ProTracker CIA timer period
     */
    PS_MODE_TICK_BASED,

    /**
     * Frame-based mode (AHX/HVL): Uses fixed 50Hz frame rate
     * samples_per_tick = sample_rate / 50.0
     * One "tick" represents a 50Hz PAL video frame (20ms)
     */
    PS_MODE_FRAME_BASED
} PatternSequencerMode;

/**
 * Sequencer event callbacks
 * These allow format-specific implementations to hook into the timing engine
 */
typedef struct {
    /**
     * Called once per tick (before processing row if tick == 0)
     * Use this for: continuous effects (vibrato, tremolo, portamento, etc.)
     */
    void (*on_tick)(void* user_data, uint8_t tick);

    /**
     * Called when advancing to a new row (tick 0)
     * Use this for: note triggering, effect parsing, row-based effects
     *
     * @param pattern_index Index in pattern order (not pattern number)
     * @param pattern_number Actual pattern number being played
     * @param row Row number within pattern
     */
    void (*on_row)(void* user_data, uint16_t pattern_index, uint16_t pattern_number, uint16_t row);

    /**
     * Called when changing to a new pattern
     * Use this for: pattern-level initialization, UI updates
     */
    void (*on_pattern_change)(void* user_data, uint16_t pattern_index, uint16_t pattern_number);

    /**
     * Called when song ends (reaches end of pattern order)
     * Use this for: stopping playback, triggering callbacks
     * Return true to continue (loop), false to stop
     */
    bool (*on_song_end)(void* user_data);

} PatternSequencerCallbacks;

/**
 * Create a new pattern sequencer
 * Returns NULL on allocation failure
 * Default mode: PS_MODE_TICK_BASED
 */
PatternSequencer* pattern_sequencer_create(void);

/**
 * Destroy a pattern sequencer
 */
void pattern_sequencer_destroy(PatternSequencer* seq);

/**
 * Set timing mode
 * @param mode PS_MODE_TICK_BASED (MOD/MMD) or PS_MODE_FRAME_BASED (AHX)
 *
 * IMPORTANT: Call this BEFORE pattern_sequencer_start()
 * - Tick-based: BPM controls timing, speed controls ticks per row
 * - Frame-based: Fixed 50Hz, speed (tempo) controls frames per row
 */
void pattern_sequencer_set_mode(PatternSequencer* seq, PatternSequencerMode mode);

/**
 * Set callbacks for format-specific behavior
 * All callbacks are optional (can be NULL)
 */
void pattern_sequencer_set_callbacks(PatternSequencer* seq,
                                     const PatternSequencerCallbacks* callbacks,
                                     void* user_data);

/**
 * Set song structure
 * @param pattern_order Array of pattern numbers to play (copied internally)
 * @param order_length Number of entries in pattern_order
 * @param rows_per_pattern Number of rows in each pattern (64 for MOD, variable for others)
 */
void pattern_sequencer_set_song(PatternSequencer* seq,
                                const uint16_t* pattern_order,
                                uint16_t order_length,
                                uint16_t rows_per_pattern);

/**
 * Start playback from beginning
 */
void pattern_sequencer_start(PatternSequencer* seq);

/**
 * Stop playback
 */
void pattern_sequencer_stop(PatternSequencer* seq);

/**
 * Check if currently playing
 */
bool pattern_sequencer_is_playing(const PatternSequencer* seq);

/**
 * Set BPM (beats per minute)
 * Standard range: 32-255, default 125
 * Affects tick duration: samples_per_tick = (2.5 * sample_rate) / bpm
 */
void pattern_sequencer_set_bpm(PatternSequencer* seq, uint8_t bpm);

/**
 * Set speed (ticks per row)
 * Standard range: 1-31, default 6
 * Does NOT affect tick duration, only how many ticks before advancing row
 */
void pattern_sequencer_set_speed(PatternSequencer* seq, uint8_t speed);

/**
 * Get current BPM
 */
uint8_t pattern_sequencer_get_bpm(const PatternSequencer* seq);

/**
 * Get current speed
 */
uint8_t pattern_sequencer_get_speed(const PatternSequencer* seq);

/**
 * Set loop range (pattern order indices)
 * @param start_index First pattern order to play (0-based)
 * @param end_index Last pattern order to play (0-based)
 * Set both to 0 to loop entire song
 */
void pattern_sequencer_set_loop_range(PatternSequencer* seq,
                                      uint16_t start_index,
                                      uint16_t end_index);

/**
 * Enable/disable looping
 * If disabled, playback stops at end instead of looping
 */
void pattern_sequencer_set_looping(PatternSequencer* seq, bool enabled);

/**
 * Get current position
 * @param pattern_index [out] Current index in pattern order
 * @param pattern_number [out] Actual pattern number being played
 * @param row [out] Current row within pattern
 */
void pattern_sequencer_get_position(const PatternSequencer* seq,
                                    uint16_t* pattern_index,
                                    uint16_t* pattern_number,
                                    uint16_t* row);

/**
 * Get song length (number of patterns in song order)
 */
uint16_t pattern_sequencer_get_song_length(const PatternSequencer* seq);

/**
 * Jump to specific position
 * @param pattern_index Index in pattern order (NOT pattern number)
 * @param row Row within pattern
 */
void pattern_sequencer_set_position(PatternSequencer* seq,
                                    uint16_t pattern_index,
                                    uint16_t row);

/**
 * Pattern flow control - Pattern Break (Dxx effect)
 * Jumps to next pattern at specified row
 * @param row Row to jump to in next pattern (0-63 for MOD)
 */
void pattern_sequencer_pattern_break(PatternSequencer* seq, uint16_t row);

/**
 * Pattern flow control - Position Jump (Bxx effect)
 * Jumps to specific pattern in song order
 * @param pattern_index Index in pattern order to jump to
 */
void pattern_sequencer_position_jump(PatternSequencer* seq, uint16_t pattern_index);

/**
 * Pattern flow control - Position Jump + Pattern Break (B+D combination)
 * Jumps to specific pattern at specific row
 * @param pattern_index Index in pattern order
 * @param row Row within pattern
 */
void pattern_sequencer_jump_to(PatternSequencer* seq,
                               uint16_t pattern_index,
                               uint16_t row);

/**
 * Pattern loop control - Set loop start (E60 effect)
 * Marks current row as loop start point
 */
void pattern_sequencer_set_pattern_loop_start(PatternSequencer* seq);

/**
 * Pattern loop control - Execute loop (E6x effect)
 * Loops back to loop start point x times
 * @param count Number of times to loop (0 = infinite in some formats)
 */
void pattern_sequencer_execute_pattern_loop(PatternSequencer* seq, uint8_t count);

/**
 * Pattern delay - Repeat current row (EEx effect)
 * @param count Number of extra times to process current row
 */
void pattern_sequencer_pattern_delay(PatternSequencer* seq, uint8_t count);

/**
 * Process timing and trigger callbacks
 * Call this from your audio processing loop
 *
 * @param frames Number of audio frames to process
 * @param sample_rate Output sample rate (e.g., 48000)
 *
 * This handles:
 * - Incrementing sample accumulator
 * - Triggering ticks when accumulator >= samples_per_tick
 * - Advancing rows when tick reaches speed
 * - Advancing patterns when row reaches end
 * - Calling appropriate callbacks
 */
void pattern_sequencer_process(PatternSequencer* seq,
                               uint32_t frames,
                               uint32_t sample_rate);

/**
 * Get timing information (for visualization/debugging)
 */
uint8_t pattern_sequencer_get_current_tick(const PatternSequencer* seq);
double pattern_sequencer_get_samples_per_tick(const PatternSequencer* seq);

#ifdef __cplusplus
}
#endif

#endif // PATTERN_SEQUENCER_H
