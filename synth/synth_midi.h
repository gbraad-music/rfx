/*
 * Synth MIDI Handler
 *
 * Generic MIDI message parsing and voice allocation for all synth plugins.
 * Handles note on/off, CC, pitch bend, and various voice allocation strategies.
 *
 * Features:
 * - MIDI message parsing (Note On/Off, CC, Pitch Bend, etc.)
 * - Voice allocation strategies (polyphonic, channel-based, monophonic)
 * - Voice stealing (oldest, least important)
 * - Note tracking for proper note-off handling
 * - MIDI to frequency/period conversion
 * - Pitch bend processing
 *
 * Used by: AHX synths, SID synths, piano synths, SFZ players, drum machines, etc.
 */

#ifndef SYNTH_MIDI_H
#define SYNTH_MIDI_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// MIDI message types
typedef enum {
    MIDI_NOTE_OFF = 0x80,
    MIDI_NOTE_ON = 0x90,
    MIDI_POLY_PRESSURE = 0xA0,
    MIDI_CC = 0xB0,
    MIDI_PROGRAM_CHANGE = 0xC0,
    MIDI_CHANNEL_PRESSURE = 0xD0,
    MIDI_PITCH_BEND = 0xE0,
    MIDI_SYSTEM = 0xF0
} MidiMessageType;

// Common MIDI CC numbers
typedef enum {
    MIDI_CC_MODULATION = 1,
    MIDI_CC_BREATH = 2,
    MIDI_CC_VOLUME = 7,
    MIDI_CC_PAN = 10,
    MIDI_CC_EXPRESSION = 11,
    MIDI_CC_SUSTAIN_PEDAL = 64,
    MIDI_CC_PORTAMENTO = 65,
    MIDI_CC_SOSTENUTO = 66,
    MIDI_CC_SOFT_PEDAL = 67,
    MIDI_CC_FILTER_CUTOFF = 74,
    MIDI_CC_FILTER_RESONANCE = 71,
    MIDI_CC_ATTACK = 73,
    MIDI_CC_RELEASE = 72,
    MIDI_CC_ALL_SOUND_OFF = 120,
    MIDI_CC_ALL_CONTROLLERS_OFF = 121,
    MIDI_CC_ALL_NOTES_OFF = 123
} MidiCCNumber;

// Voice allocation strategies
typedef enum {
    VOICE_ALLOC_POLYPHONIC,    // Round-robin, steal oldest
    VOICE_ALLOC_CHANNEL_BASED, // MIDI channel → voice index (SID-style: ch 0→v0, ch 1→v1, ch 2→v2)
    VOICE_ALLOC_MONO_LAST,     // Monophonic, last note priority (most recent wins)
    VOICE_ALLOC_MONO_LOW,      // Monophonic, lowest note priority
    VOICE_ALLOC_MONO_HIGH      // Monophonic, highest note priority
} VoiceAllocStrategy;

// Parsed MIDI message
typedef struct {
    MidiMessageType type;
    uint8_t channel;           // 0-15
    uint8_t note;              // 0-127 (for note messages)
    uint8_t velocity;          // 0-127 (for note messages)
    uint8_t cc_number;         // 0-127 (for CC messages)
    uint8_t cc_value;          // 0-127 (for CC messages)
    int16_t pitch_bend;        // -8192 to +8191
    uint8_t program;           // 0-127 (for program change)
    uint8_t pressure;          // 0-127 (for pressure messages)
} SynthMidiMessage;

// Voice state for allocation
typedef struct {
    bool active;               // Is this voice currently playing?
    uint8_t note;              // MIDI note number
    uint8_t velocity;          // MIDI velocity
    uint8_t channel;           // MIDI channel (for channel-based allocation)
    uint32_t trigger_time;     // Timestamp for voice stealing (oldest first)
} SynthMidiVoice;

// MIDI handler state
typedef struct {
    SynthMidiVoice* voices;
    uint32_t num_voices;
    VoiceAllocStrategy strategy;
    uint32_t timestamp;        // Incremented each note-on for voice stealing

    // Monophonic mode state
    uint8_t held_notes[128];   // Stack of held notes (for mono modes)
    uint32_t held_count;       // Number of held notes
} SynthMidiHandler;

/**
 * Create MIDI handler
 * @param num_voices Number of available voices
 * @param strategy Voice allocation strategy
 * @return Handler instance, or NULL on failure
 */
SynthMidiHandler* synth_midi_create(uint32_t num_voices, VoiceAllocStrategy strategy);

/**
 * Destroy MIDI handler
 */
void synth_midi_destroy(SynthMidiHandler* handler);

/**
 * Set voice allocation strategy
 */
void synth_midi_set_strategy(SynthMidiHandler* handler, VoiceAllocStrategy strategy);

/**
 * Parse raw MIDI message
 * @param data Raw MIDI bytes (up to 3 bytes)
 * @param size Number of bytes
 * @param msg Output parsed message
 * @return true if valid message parsed
 */
bool synth_midi_parse(const uint8_t* data, uint32_t size, SynthMidiMessage* msg);

/**
 * Allocate voice for note-on
 *
 * Behavior depends on allocation strategy:
 * - POLYPHONIC: Round-robin allocation, steals oldest if all busy
 * - CHANNEL_BASED: Maps MIDI channel to voice (channel % num_voices)
 * - MONO_*: Returns voice 0, manages note stack
 *
 * @param handler MIDI handler
 * @param channel MIDI channel (0-15)
 * @param note MIDI note number
 * @param velocity MIDI velocity
 * @return Voice index, or -1 if no voice available
 */
int synth_midi_allocate_voice(SynthMidiHandler* handler, uint8_t channel, uint8_t note, uint8_t velocity);

/**
 * Find voice(s) for note-off
 *
 * @param handler MIDI handler
 * @param channel MIDI channel (0-15)
 * @param note MIDI note number
 * @param voices_out Array to store voice indices (must be size >= num_voices)
 * @return Number of voices found
 */
int synth_midi_find_voices_for_note(const SynthMidiHandler* handler, uint8_t channel, uint8_t note, int* voices_out);

/**
 * Release voice
 *
 * @param handler MIDI handler
 * @param voice_index Voice index to release
 */
void synth_midi_release_voice(SynthMidiHandler* handler, int voice_index);

/**
 * All notes off (panic)
 * Releases all active voices
 */
void synth_midi_all_notes_off(SynthMidiHandler* handler);

/**
 * Get active note for monophonic mode
 * Returns the note that should be playing based on held notes and priority
 *
 * @param handler MIDI handler
 * @param note_out Output: MIDI note to play
 * @return true if a note should be playing, false if all notes released
 */
bool synth_midi_get_mono_note(const SynthMidiHandler* handler, uint8_t* note_out);

/**
 * Convert MIDI note to frequency (Hz)
 * Uses standard equal temperament tuning (A4 = 440 Hz)
 *
 * @param note MIDI note number (0-127)
 * @return Frequency in Hz
 */
float synth_midi_note_to_freq(uint8_t note);

/**
 * Convert MIDI note to period (Amiga-style)
 *
 * @param note MIDI note number (0-127)
 * @param clock_rate Paula clock rate (e.g. 3546895 for PAL, 3579545 for NTSC)
 * @return Period value for Amiga hardware
 */
uint32_t synth_midi_note_to_period(uint8_t note, uint32_t clock_rate);

/**
 * Convert pitch bend value to frequency multiplier
 *
 * @param pitch_bend MIDI pitch bend value (-8192 to +8191, 0 = no bend)
 * @param semitone_range Pitch bend range in semitones (typically 2 or 12)
 * @return Frequency multiplier (1.0 = no bend, 2.0 = +1 octave, 0.5 = -1 octave)
 */
float synth_midi_pitch_bend_to_multiplier(int16_t pitch_bend, float semitone_range);

/**
 * Convert pitch bend to period offset (Amiga-style)
 *
 * @param pitch_bend MIDI pitch bend value (-8192 to +8191)
 * @param semitone_range Pitch bend range in semitones
 * @param base_period Base period value
 * @return Period offset to add/subtract from base period
 */
int32_t synth_midi_pitch_bend_to_period_offset(int16_t pitch_bend, float semitone_range, uint32_t base_period);

/**
 * Convert MIDI velocity to linear gain (0.0 to 1.0)
 * Applies velocity curve for natural response
 *
 * @param velocity MIDI velocity (0-127)
 * @param curve Velocity curve amount (0.0 = linear, 1.0 = exponential, -1.0 = compressed)
 * @return Gain value (0.0 to 1.0)
 */
float synth_midi_velocity_to_gain(uint8_t velocity, float curve);

/**
 * Check if MIDI CC is a channel mode message
 * (All Notes Off, All Sound Off, etc.)
 *
 * @param cc_number MIDI CC number
 * @return true if channel mode message
 */
bool synth_midi_is_channel_mode(uint8_t cc_number);

#ifdef __cplusplus
}
#endif

#endif // SYNTH_MIDI_H
