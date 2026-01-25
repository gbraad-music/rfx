/*
 * RGAHX Drum - AHX One-Shot Drum Synthesizer
 *
 * Uses authentic AHX synthesis for drum sounds
 * Presets are embedded directly in the binary
 */

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "../../synth/ahx_preset.h"
#include "../../synth/ahx_instrument.h"

// Embedded presets
#include "preset_kick.h"
#include "preset_snare.h"

#define MAX_VOICES 16
#define MAX_PRESETS 16

typedef struct {
    AhxInstrument instrument;
    bool active;
    uint8_t note;
} RGAHXDrumVoice;

typedef struct {
    AhxInstrumentParams* presets[MAX_PRESETS];
    int preset_count;
    RGAHXDrumVoice voices[MAX_VOICES];
    uint32_t sample_rate;
} RGAHXDrum;

// Preset mapping (MIDI note -> preset pointer)
// 36 = Kick, 38 = Snare, etc.
static const struct {
    uint8_t midi_note;
    AhxInstrumentParams* params;
} preset_map[] = {
    {36, &preset_kick_params},       // MIDI note 36 = Bass Drum 1 (C1) - kick
    {38, &preset_snare_params},      // MIDI note 38 = Snare (D1)
};
#define PRESET_MAP_SIZE (sizeof(preset_map) / sizeof(preset_map[0]))

RGAHXDrum* rgahxdrum_create(uint32_t sample_rate) {
    RGAHXDrum* drum = (RGAHXDrum*)calloc(1, sizeof(RGAHXDrum));
    if (!drum) return NULL;

    drum->sample_rate = sample_rate;

    // Copy preset pointers directly (no file loading needed!)
    drum->preset_count = PRESET_MAP_SIZE;
    for (int i = 0; i < PRESET_MAP_SIZE; i++) {
        drum->presets[i] = preset_map[i].params;
    }

    // Initialize all voices
    for (int i = 0; i < MAX_VOICES; i++) {
        ahx_instrument_init(&drum->voices[i].instrument);
        drum->voices[i].active = false;
    }

    return drum;
}

void rgahxdrum_destroy(RGAHXDrum* drum) {
    if (!drum) return;

    // Presets are statically embedded, no need to free them

    free(drum);
}

int rgahxdrum_get_preset_for_note(RGAHXDrum* drum, uint8_t midi_note) {
    for (int i = 0; i < PRESET_MAP_SIZE; i++) {
        if (preset_map[i].midi_note == midi_note) {
            return i;  // Return preset index
        }
    }
    return -1;  // Not found
}

void rgahxdrum_trigger(RGAHXDrum* drum, uint8_t midi_note, uint8_t velocity) {
    if (!drum) return;

    // Find preset for this MIDI note
    int preset_idx = rgahxdrum_get_preset_for_note(drum, midi_note);
    if (preset_idx < 0 || preset_idx >= drum->preset_count) return;

    // Find free voice
    int voice_idx = -1;
    for (int i = 0; i < MAX_VOICES; i++) {
        if (!drum->voices[i].active || !ahx_instrument_is_active(&drum->voices[i].instrument)) {
            voice_idx = i;
            break;
        }
    }

    if (voice_idx < 0) {
        // Steal oldest voice
        voice_idx = 0;
    }

    RGAHXDrumVoice* voice = &drum->voices[voice_idx];

    // Set preset parameters (directly from embedded preset)
    ahx_instrument_set_params(&voice->instrument, drum->presets[preset_idx]);

    // Trigger note (use MIDI note 0 for one-shot, as all notes in preset are fixed)
    ahx_instrument_note_on(&voice->instrument, 0, velocity, drum->sample_rate);

    voice->active = true;
    voice->note = midi_note;
}

void rgahxdrum_process(RGAHXDrum* drum, float* output, uint32_t num_samples) {
    if (!drum || !output) return;

    // Clear output buffer
    memset(output, 0, num_samples * sizeof(float));

    // Check if any voices are active - if not, return silence (no noise generation)
    bool any_active = false;
    for (int i = 0; i < MAX_VOICES; i++) {
        if (drum->voices[i].active) {
            any_active = true;
            break;
        }
    }

    if (!any_active) {
        // No voices active - return silence, don't run synthesis (prevents continuous noise)
        return;
    }

    // Mix all active voices
    float* voice_buffer = (float*)alloca(num_samples * sizeof(float));

    for (int i = 0; i < MAX_VOICES; i++) {
        if (!drum->voices[i].active) continue;

        RGAHXDrumVoice* voice = &drum->voices[i];

        // Check if voice is actually still playing
        if (!ahx_instrument_is_active(&voice->instrument)) {
            voice->active = false;
            continue;
        }

        // Process voice
        uint32_t rendered = ahx_instrument_process(&voice->instrument, voice_buffer,
                                                   num_samples, drum->sample_rate);

        // Mix into output
        for (uint32_t s = 0; s < rendered; s++) {
            output[s] += voice_buffer[s];
        }

        // Check if voice finished (will be caught at top of next iteration)
        if (!ahx_instrument_is_active(&voice->instrument)) {
            voice->active = false;
        }
    }
}
