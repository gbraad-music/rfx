#ifndef REGROOVE_H
#define REGROOVE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef struct Regroove Regroove;

// Loop state enum for compatibility with regroove_controller
typedef enum {
    RG_LOOP_OFF = 0,      // Not looping
    RG_LOOP_ARMED = 1,    // Loop armed, waiting to reach loop start
    RG_LOOP_ACTIVE = 2    // Currently looping
} RegrooveLoopState;

// Pattern mode enum for compatibility with regroove_controller
typedef enum {
    RG_PATTERN_MODE_OFF = 0,     // Normal playback through song
    RG_PATTERN_MODE_SINGLE = 1   // Loop current pattern indefinitely
} RegroovePatternMode;

// --- Optional UI callback types ---
typedef void (*RegrooveOrderCallback)(int order, int pattern, void *userdata);
typedef void (*RegrooveRowCallback)(int order, int row, void *userdata);
typedef void (*RegrooveLoopPatternCallback)(int order, int pattern, void *userdata);
typedef void (*RegrooveLoopSongCallback)(void *userdata);
typedef void (*RegroovePatternModeCallback)(int pattern_mode, int reason, void *userdata);
// reason: 0=manual toggle, 1=auto-exit due to pattern break/jump
// MIDI output callback for note events
// channel: tracker channel (0-63)
// note: tracker note number (0-119, where 48=C-4)
// instrument: instrument number (0-255)
// volume: note volume (0-64, tracker range)
// effect_cmd: effect command (0-255, e.g. 0x0F for 0Fxx)
// effect_param: effect parameter (0-255, e.g. 0xFF for 0FFF)
typedef void (*RegrooveNoteCallback)(int channel, int note, int instrument, int volume,
                                     int effect_cmd, int effect_param, void *userdata);

struct RegrooveCallbacks {
    RegrooveOrderCallback       on_order_change;
    RegrooveRowCallback         on_row_change;
    RegrooveLoopPatternCallback on_loop_pattern;
    RegrooveLoopSongCallback    on_loop_song;
    RegroovePatternModeCallback on_pattern_mode_change;
    RegrooveNoteCallback        on_note;
    void *userdata;
};

// Creation & lifetime
Regroove *regroove_create(const char *filename, double samplerate);
void regroove_destroy(Regroove *g);
void regroove_set_callbacks(Regroove *g, struct RegrooveCallbacks *cb);

// Rendering
int regroove_render_audio(Regroove *g, int16_t *buffer, int frames);

// User commands (to be called from main loop or UI)
void regroove_process_commands(Regroove *g);
void regroove_pattern_mode(Regroove *g, int on);
void regroove_queue_next_order(Regroove *g);
void regroove_queue_prev_order(Regroove *g);
void regroove_queue_order(Regroove *g, int order);
void regroove_queue_pattern(Regroove *g, int pattern);
void regroove_jump_to_order(Regroove *g, int order);
void regroove_jump_to_pattern(Regroove *g, int pattern);
void regroove_set_position_row(Regroove *g, int row);  // Set row within current order
void regroove_clear_pending_jump(Regroove *g);  // Clear any pending order/pattern jump

// Loop range system (replaces loop_till_row)
void regroove_set_loop_range(Regroove *g, int start_order, int start_row, int end_order, int end_row);
void regroove_get_loop_range(const Regroove *g, int *start_order, int *start_row, int *end_order, int *end_row);
void regroove_set_loop_start_here(Regroove *g);   // Set loop start to current position
void regroove_set_loop_end_here(Regroove *g);     // Set loop end to current position
void regroove_trigger_loop(Regroove *g);          // Jump to loop start and activate
void regroove_play_to_loop(Regroove *g);          // Toggle: OFF↔ARMED, ACTIVE→OFF
int regroove_get_loop_state(const Regroove *g);   // 0=OFF, 1=ARMED, 2=ACTIVE

void regroove_retrigger_pattern(Regroove *g);
void regroove_set_custom_loop_rows(Regroove *g, int rows);
void regroove_toggle_channel_mute(Regroove *g, int ch);
void regroove_queue_channel_mute(Regroove *g, int ch);       // Queued mute toggle
void regroove_mute_all(Regroove *g);
void regroove_unmute_all(Regroove *g);
void regroove_toggle_channel_solo(Regroove *g, int ch);
void regroove_queue_channel_solo(Regroove *g, int ch);       // Queued solo toggle
void regroove_set_channel_volume(Regroove *g, int ch, double vol);
double regroove_get_channel_volume(const Regroove* g, int ch);
void regroove_set_channel_panning(Regroove *g, int ch, double pan);
double regroove_get_channel_panning(const Regroove* g, int ch);

void regroove_set_pitch(Regroove *g, double pitch);

// Interpolation filter control
// filter: 0 = none, 1 = linear, 2 = cubic, 4 = FIR (high quality)
void regroove_set_interpolation_filter(Regroove *g, int filter);
int regroove_get_interpolation_filter(const Regroove *g);

// Audio quality settings
// stereo_separation: 0-200 (0=mono, 100=default, 200=extra wide)
void regroove_set_stereo_separation(Regroove *g, int separation);
int regroove_get_stereo_separation(const Regroove *g);

// dither: 0=none, 1=default, 2=rectangular 0.5bit, 3=rectangular 1bit with noise shaping
void regroove_set_dither(Regroove *g, int dither);
int regroove_get_dither(const Regroove *g);

// Amiga resampler (only affects 4-channel Amiga modules)
// enabled: 0=disabled, 1=enabled
void regroove_set_amiga_resampler(Regroove *g, int enabled);
int regroove_get_amiga_resampler(const Regroove *g);

// filter_type: 0=auto, 1=a500, 2=a1200, 3=unfiltered
void regroove_set_amiga_filter_type(Regroove *g, int filter_type);
int regroove_get_amiga_filter_type(const Regroove *g);

// State queries
int regroove_get_num_orders(const Regroove *g);
int regroove_get_num_patterns(const Regroove *g);
int regroove_get_order_pattern(const Regroove *g, int order);
int regroove_get_current_order(const Regroove *g);
int regroove_get_current_pattern(const Regroove *g);
int regroove_get_current_row(const Regroove *g);
int regroove_get_num_channels(const Regroove *g);
double regroove_get_pitch(const Regroove *g);
int regroove_is_channel_muted(const Regroove *g, int ch);
int regroove_has_pending_mute_changes(const Regroove *g);
int regroove_get_pending_channel_mute(const Regroove *g, int ch);
int regroove_get_queued_action_for_channel(const Regroove *g, int ch);  // 0=none, 1=mute, 2=solo
int regroove_get_queued_jump_type(const Regroove *g);  // 0=none, 1=next, 2=prev, 3=order, 4=pattern
int regroove_get_queued_order(const Regroove *g);  // Returns the queued order number (-1 if none)
int regroove_get_pattern_mode(const Regroove *g);
int regroove_get_custom_loop_rows(const Regroove *g);
int regroove_get_full_pattern_rows(const Regroove *g);
int regroove_get_pattern_num_rows(const Regroove *g, int pattern);  // Get row count for specific pattern
double regroove_get_current_bpm(const Regroove *g);  // Get module's base BPM (before pitch adjustment)
double regroove_get_effective_bpm(const Regroove *g);  // Get effective playback BPM (after pitch adjustment)
int regroove_get_current_speed(const Regroove *g);  // Get current speed (ticks per row)

// Get formatted pattern cell data (note, instrument, volume, effects)
// Returns 0 on success, -1 on error
// buffer should be at least 32 bytes
int regroove_get_pattern_cell(const Regroove *g, int pattern, int row, int channel, char *buffer, size_t buffer_size);

// Get number of instruments in module
int regroove_get_num_instruments(const Regroove *g);

// Get instrument name by index
// Returns NULL if instrument doesn't exist or has no name
const char* regroove_get_instrument_name(const Regroove *g, int index);

// Get number of samples in module
int regroove_get_num_samples(const Regroove *g);

// Get sample name by index
// Returns NULL if sample doesn't exist or has no name
const char* regroove_get_sample_name(const Regroove *g, int index);

// ============================================================================
// Compatibility wrappers for unified API with regroove_controller
// ============================================================================

// Unified position getter (compatible with regroove_controller API)
static inline void regroove_get_position(const Regroove *g, int *order, int *row) {
    if (!g) return;
    if (order) *order = regroove_get_current_order(g);
    if (row) *row = regroove_get_current_row(g);
}

// Unified immediate jump (compatible with regroove_controller API)
static inline void regroove_jump_immediate(Regroove *g, int order, int row) {
    if (!g) return;
    regroove_jump_to_order(g, order);
    regroove_set_position_row(g, row);
}

// Explicit loop control (compatible with regroove_controller API)
static inline void regroove_arm_loop(Regroove *g) {
    if (!g) return;
    if (regroove_get_loop_state(g) == RG_LOOP_OFF) {
        regroove_play_to_loop(g);  // OFF -> ARMED
    }
}

static inline void regroove_disable_loop(Regroove *g) {
    if (!g) return;
    if (regroove_get_loop_state(g) != RG_LOOP_OFF) {
        regroove_play_to_loop(g);  // ARMED or ACTIVE -> OFF
    }
}

// Naming consistency aliases for compatibility with controller API

// Loop range functions (_rows suffix)
#define regroove_set_loop_range_rows regroove_set_loop_range
#define regroove_get_loop_range_rows regroove_get_loop_range

// Channel mute query (controller uses get_channel_mute, engine uses is_channel_muted)
#define regroove_get_channel_mute regroove_is_channel_muted

#ifdef __cplusplus
}
#endif

#endif // REGROOVE_H