#pragma once
/**
 * MOD File Player - ProTracker Module Player
 *
 * Features:
 * - Standard 4-channel ProTracker MOD file playback
 * - Pattern loop control (stay within pattern range)
 * - Per-channel mute, volume, and panning
 * - Support for common MOD effects
 */

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MOD_MAX_CHANNELS 4
#define MOD_MAX_PATTERNS 128
#define MOD_MAX_SAMPLES 31
#define MOD_PATTERN_ROWS 64
#define MOD_TITLE_LENGTH 20
#define MOD_SAMPLE_NAME_LENGTH 22

typedef struct ModPlayer ModPlayer;

/**
 * MOD sample structure
 */
typedef struct {
    char name[MOD_SAMPLE_NAME_LENGTH + 1];
    uint32_t length;          // Length in words (2 bytes)
    int8_t finetune;          // Finetune value (-8 to +7)
    uint8_t volume;           // Default volume (0-64)
    uint32_t repeat_start;    // Loop start in words
    uint32_t repeat_length;   // Loop length in words
    int8_t* data;             // Sample data (8-bit signed)
} ModSample;

/**
 * MOD note/pattern data
 */
typedef struct {
    uint8_t sample;           // Sample number (0-31, 0 = no sample)
    uint16_t period;          // Period value (113-856 for notes)
    uint8_t effect;           // Effect type (0x0-0xF)
    uint8_t effect_param;     // Effect parameter
} ModNote;

/**
 * Channel playback state
 */
typedef struct {
    // Sample playback
    const ModSample* sample;
    float position;           // Current position in sample (fractional)
    float increment;          // Sample increment per output sample

    // Note state
    uint16_t period;          // Current period
    uint8_t volume;           // Current volume (0-64)
    int8_t finetune;          // Current finetune

    // Effects state
    uint8_t effect;
    uint8_t effect_param;
    uint16_t portamento_target;
    uint8_t vibrato_pos;
    uint8_t vibrato_speed;
    uint8_t vibrato_depth;
    uint8_t tremolo_pos;
    uint8_t tremolo_speed;
    uint8_t tremolo_depth;
    uint8_t retrigger_count;
    uint8_t note_delay_ticks;

    // Effect memory (for effects that use 00 parameter)
    uint8_t last_portamento_up;
    uint8_t last_portamento_down;
    uint8_t last_tone_portamento;
    uint8_t last_volume_slide;
    uint8_t last_sample_offset;

    // User controls
    bool muted;               // Channel muted by user
    float user_volume;        // User volume control (0.0-1.0)
    float panning;            // Panning (-1.0 = left, 0 = center, 1.0 = right)
} ModChannel;

/**
 * Create a new MOD player instance
 * Returns NULL on allocation failure
 */
ModPlayer* mod_player_create(void);

/**
 * Destroy a MOD player instance
 */
void mod_player_destroy(ModPlayer* player);

/**
 * Load a MOD file from memory
 * data: Pointer to MOD file data
 * size: Size of MOD file in bytes
 * Returns true on success, false on parse error
 */
bool mod_player_load(ModPlayer* player, const uint8_t* data, uint32_t size);

/**
 * Start playback
 */
void mod_player_start(ModPlayer* player);

/**
 * Stop playback
 */
void mod_player_stop(ModPlayer* player);

/**
 * Check if player is currently playing
 */
bool mod_player_is_playing(const ModPlayer* player);

/**
 * Set loop range (pattern indices)
 * start_pattern: First pattern to play (0-127)
 * end_pattern: Last pattern to play (0-127)
 * If start == end, only that pattern will loop
 */
void mod_player_set_loop_range(ModPlayer* player, uint8_t start_pattern, uint8_t end_pattern);

/**
 * Get current position
 */
void mod_player_get_position(const ModPlayer* player, uint8_t* pattern, uint8_t* row);

/**
 * Set position (jump to specific pattern/row)
 */
void mod_player_set_position(ModPlayer* player, uint8_t pattern, uint8_t row);

/**
 * Set BPM (125 is standard)
 */
void mod_player_set_bpm(ModPlayer* player, uint8_t bpm);

/**
 * Set speed (6 is standard, affects ticks per row)
 */
void mod_player_set_speed(ModPlayer* player, uint8_t speed);

/**
 * Channel control - mute/unmute
 */
void mod_player_set_channel_mute(ModPlayer* player, uint8_t channel, bool muted);

/**
 * Channel control - volume (0.0 to 1.0)
 */
void mod_player_set_channel_volume(ModPlayer* player, uint8_t channel, float volume);

/**
 * Channel control - panning (-1.0 = left, 0 = center, 1.0 = right)
 */
void mod_player_set_channel_panning(ModPlayer* player, uint8_t channel, float panning);

/**
 * Get channel state (for UI display)
 */
bool mod_player_get_channel_mute(const ModPlayer* player, uint8_t channel);
float mod_player_get_channel_volume(const ModPlayer* player, uint8_t channel);
float mod_player_get_channel_panning(const ModPlayer* player, uint8_t channel);

/**
 * Process stereo output
 * left/right: Output buffers
 * frames: Number of frames to process
 * sample_rate: Output sample rate (typically 48000)
 */
void mod_player_process(ModPlayer* player, float* left, float* right, uint32_t frames, uint32_t sample_rate);

/**
 * Get MOD title
 */
const char* mod_player_get_title(const ModPlayer* player);

/**
 * Get number of patterns in song
 */
uint8_t mod_player_get_song_length(const ModPlayer* player);

#ifdef __cplusplus
}
#endif
