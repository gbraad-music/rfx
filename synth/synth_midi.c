/*
 * Synth MIDI Handler - Implementation
 */

#include "synth_midi.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

// MIDI A4 reference frequency (Hz)
#define MIDI_A4_FREQ 440.0f
#define MIDI_A4_NOTE 69

// Create MIDI handler
SynthMidiHandler* synth_midi_create(uint32_t num_voices, VoiceAllocStrategy strategy) {
    if (num_voices == 0) return NULL;

    SynthMidiHandler* handler = (SynthMidiHandler*)calloc(1, sizeof(SynthMidiHandler));
    if (!handler) return NULL;

    handler->voices = (SynthMidiVoice*)calloc(num_voices, sizeof(SynthMidiVoice));
    if (!handler->voices) {
        free(handler);
        return NULL;
    }

    handler->num_voices = num_voices;
    handler->strategy = strategy;
    handler->timestamp = 0;
    handler->held_count = 0;

    return handler;
}

// Destroy MIDI handler
void synth_midi_destroy(SynthMidiHandler* handler) {
    if (!handler) return;
    if (handler->voices) free(handler->voices);
    free(handler);
}

// Set voice allocation strategy
void synth_midi_set_strategy(SynthMidiHandler* handler, VoiceAllocStrategy strategy) {
    if (!handler) return;
    handler->strategy = strategy;
}

// Parse raw MIDI message
bool synth_midi_parse(const uint8_t* data, uint32_t size, SynthMidiMessage* msg) {
    if (!data || !msg || size == 0) return false;

    memset(msg, 0, sizeof(SynthMidiMessage));

    uint8_t status = data[0];

    // Handle running status (data bytes without status)
    if (status < 0x80) return false;

    msg->type = (MidiMessageType)(status & 0xF0);
    msg->channel = status & 0x0F;

    switch (msg->type) {
        case MIDI_NOTE_OFF:
        case MIDI_NOTE_ON:
        case MIDI_POLY_PRESSURE:
            if (size < 3) return false;
            msg->note = data[1] & 0x7F;
            msg->velocity = data[2] & 0x7F;
            // Note: velocity 0 on Note On is treated as Note Off by caller
            return true;

        case MIDI_CC:
            if (size < 3) return false;
            msg->cc_number = data[1] & 0x7F;
            msg->cc_value = data[2] & 0x7F;
            return true;

        case MIDI_PROGRAM_CHANGE:
        case MIDI_CHANNEL_PRESSURE:
            if (size < 2) return false;
            msg->program = data[1] & 0x7F;
            msg->pressure = data[1] & 0x7F;
            return true;

        case MIDI_PITCH_BEND:
            if (size < 3) return false;
            // Pitch bend: 14-bit value, LSB first
            // Range: 0-16383, center = 8192
            {
                uint16_t bend_value = (data[1] & 0x7F) | ((data[2] & 0x7F) << 7);
                msg->pitch_bend = (int16_t)bend_value - 8192;
            }
            return true;

        case MIDI_SYSTEM:
            // System messages not handled by this parser
            return false;

        default:
            return false;
    }
}

// Find free voice (polyphonic mode)
static int find_free_voice(SynthMidiHandler* handler) {
    // First pass: find inactive voice
    for (uint32_t i = 0; i < handler->num_voices; i++) {
        if (!handler->voices[i].active) return i;
    }

    // Second pass: steal oldest voice
    uint32_t oldest_idx = 0;
    uint32_t oldest_time = handler->voices[0].trigger_time;

    for (uint32_t i = 1; i < handler->num_voices; i++) {
        if (handler->voices[i].trigger_time < oldest_time) {
            oldest_time = handler->voices[i].trigger_time;
            oldest_idx = i;
        }
    }

    return oldest_idx;
}

// Add note to monophonic held stack
static void mono_add_note(SynthMidiHandler* handler, uint8_t note) {
    // Check if already in stack
    for (uint32_t i = 0; i < handler->held_count; i++) {
        if (handler->held_notes[i] == note) return;
    }

    // Add to stack
    if (handler->held_count < 128) {
        handler->held_notes[handler->held_count++] = note;
    }
}

// Remove note from monophonic held stack
static void mono_remove_note(SynthMidiHandler* handler, uint8_t note) {
    for (uint32_t i = 0; i < handler->held_count; i++) {
        if (handler->held_notes[i] == note) {
            // Shift remaining notes down
            for (uint32_t j = i; j < handler->held_count - 1; j++) {
                handler->held_notes[j] = handler->held_notes[j + 1];
            }
            handler->held_count--;
            return;
        }
    }
}

// Allocate voice for note-on
int synth_midi_allocate_voice(SynthMidiHandler* handler, uint8_t channel, uint8_t note, uint8_t velocity) {
    if (!handler) return -1;
    if (note > 127 || velocity > 127 || channel > 15) return -1;

    int voice_idx = -1;

    switch (handler->strategy) {
        case VOICE_ALLOC_POLYPHONIC:
            voice_idx = find_free_voice(handler);
            break;

        case VOICE_ALLOC_CHANNEL_BASED:
            // Map MIDI channel to voice (SID-style: ch0→v0, ch1→v1, ch2→v2, etc.)
            voice_idx = channel % handler->num_voices;
            break;

        case VOICE_ALLOC_MONO_LAST:
        case VOICE_ALLOC_MONO_LOW:
        case VOICE_ALLOC_MONO_HIGH:
            // Monophonic modes always use voice 0
            voice_idx = 0;
            mono_add_note(handler, note);
            break;
    }

    if (voice_idx < 0 || voice_idx >= (int)handler->num_voices) return -1;

    // Activate voice
    handler->voices[voice_idx].active = true;
    handler->voices[voice_idx].note = note;
    handler->voices[voice_idx].velocity = velocity;
    handler->voices[voice_idx].channel = channel;
    handler->voices[voice_idx].trigger_time = handler->timestamp++;

    return voice_idx;
}

// Find voice(s) for note-off
int synth_midi_find_voices_for_note(const SynthMidiHandler* handler, uint8_t channel, uint8_t note, int* voices_out) {
    if (!handler || !voices_out) return 0;

    int count = 0;

    switch (handler->strategy) {
        case VOICE_ALLOC_POLYPHONIC:
            // Find all voices playing this note
            for (uint32_t i = 0; i < handler->num_voices; i++) {
                if (handler->voices[i].active && handler->voices[i].note == note) {
                    voices_out[count++] = i;
                }
            }
            break;

        case VOICE_ALLOC_CHANNEL_BASED:
            // Find voice on this channel playing this note
            {
                int voice_idx = channel % handler->num_voices;
                if (handler->voices[voice_idx].active &&
                    handler->voices[voice_idx].note == note &&
                    handler->voices[voice_idx].channel == channel) {
                    voices_out[count++] = voice_idx;
                }
            }
            break;

        case VOICE_ALLOC_MONO_LAST:
        case VOICE_ALLOC_MONO_LOW:
        case VOICE_ALLOC_MONO_HIGH:
            // Monophonic mode: always voice 0
            if (handler->voices[0].active && handler->voices[0].note == note) {
                voices_out[count++] = 0;
            }
            break;
    }

    return count;
}

// Release voice
void synth_midi_release_voice(SynthMidiHandler* handler, int voice_index) {
    if (!handler) return;
    if (voice_index < 0 || voice_index >= (int)handler->num_voices) return;

    // Remove from mono stack if applicable
    if (handler->strategy == VOICE_ALLOC_MONO_LAST ||
        handler->strategy == VOICE_ALLOC_MONO_LOW ||
        handler->strategy == VOICE_ALLOC_MONO_HIGH) {
        mono_remove_note(handler, handler->voices[voice_index].note);
    }

    handler->voices[voice_index].active = false;
    handler->voices[voice_index].note = 0;
    handler->voices[voice_index].velocity = 0;
}

// All notes off (panic)
void synth_midi_all_notes_off(SynthMidiHandler* handler) {
    if (!handler) return;

    for (uint32_t i = 0; i < handler->num_voices; i++) {
        handler->voices[i].active = false;
        handler->voices[i].note = 0;
        handler->voices[i].velocity = 0;
    }

    handler->held_count = 0;
}

// Get active note for monophonic mode
bool synth_midi_get_mono_note(const SynthMidiHandler* handler, uint8_t* note_out) {
    if (!handler || !note_out) return false;
    if (handler->held_count == 0) return false;

    switch (handler->strategy) {
        case VOICE_ALLOC_MONO_LAST:
            // Last note priority: most recent note
            *note_out = handler->held_notes[handler->held_count - 1];
            return true;

        case VOICE_ALLOC_MONO_LOW:
            // Lowest note priority
            {
                uint8_t lowest = handler->held_notes[0];
                for (uint32_t i = 1; i < handler->held_count; i++) {
                    if (handler->held_notes[i] < lowest) {
                        lowest = handler->held_notes[i];
                    }
                }
                *note_out = lowest;
                return true;
            }

        case VOICE_ALLOC_MONO_HIGH:
            // Highest note priority
            {
                uint8_t highest = handler->held_notes[0];
                for (uint32_t i = 1; i < handler->held_count; i++) {
                    if (handler->held_notes[i] > highest) {
                        highest = handler->held_notes[i];
                    }
                }
                *note_out = highest;
                return true;
            }

        default:
            return false;
    }
}

// Convert MIDI note to frequency (Hz)
float synth_midi_note_to_freq(uint8_t note) {
    if (note > 127) note = 127;

    // f = 440 * 2^((n-69)/12)
    // A4 (MIDI note 69) = 440 Hz
    float semitones_from_a4 = (float)note - MIDI_A4_NOTE;
    return MIDI_A4_FREQ * powf(2.0f, semitones_from_a4 / 12.0f);
}

// Convert MIDI note to period (Amiga-style)
uint32_t synth_midi_note_to_period(uint8_t note, uint32_t clock_rate) {
    if (note > 127) note = 127;

    float freq = synth_midi_note_to_freq(note);
    if (freq < 0.1f) freq = 0.1f; // Avoid division by zero

    return (uint32_t)(clock_rate / freq);
}

// Convert pitch bend to frequency multiplier
float synth_midi_pitch_bend_to_multiplier(int16_t pitch_bend, float semitone_range) {
    if (pitch_bend < -8192) pitch_bend = -8192;
    if (pitch_bend > 8191) pitch_bend = 8191;

    // Normalize to -1.0 to +1.0
    float normalized = (float)pitch_bend / 8192.0f;

    // Convert to semitones
    float semitones = normalized * semitone_range;

    // Convert to frequency multiplier: 2^(semitones/12)
    return powf(2.0f, semitones / 12.0f);
}

// Convert pitch bend to period offset (Amiga-style)
int32_t synth_midi_pitch_bend_to_period_offset(int16_t pitch_bend, float semitone_range, uint32_t base_period) {
    float multiplier = synth_midi_pitch_bend_to_multiplier(pitch_bend, semitone_range);

    // Period is inversely proportional to frequency
    // higher frequency = lower period
    // multiplier > 1.0 means higher freq, so lower period (negative offset)
    float new_period = (float)base_period / multiplier;

    return (int32_t)new_period - (int32_t)base_period;
}

// Convert MIDI velocity to linear gain
float synth_midi_velocity_to_gain(uint8_t velocity, float curve) {
    if (velocity > 127) velocity = 127;

    // Normalize to 0.0 - 1.0
    float normalized = (float)velocity / 127.0f;

    if (curve == 0.0f) {
        // Linear
        return normalized;
    } else if (curve > 0.0f) {
        // Exponential (more sensitive at low velocities)
        float exponent = 1.0f + curve * 2.0f; // curve 1.0 → exponent 3.0
        return powf(normalized, exponent);
    } else {
        // Compressed (less sensitive at low velocities)
        float exponent = 1.0f / (1.0f - curve * 0.5f); // curve -1.0 → exponent 2.0
        return powf(normalized, exponent);
    }
}

// Check if MIDI CC is a channel mode message
bool synth_midi_is_channel_mode(uint8_t cc_number) {
    return cc_number >= 120 && cc_number <= 127;
}
