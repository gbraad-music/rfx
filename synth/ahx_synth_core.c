/*
 * AHX Synthesis Core - Implementation
 *
 * Authentic AHX synthesis algorithms extracted from ahx_player.c
 * Shared by both the module player and synth plugin
 */

#include "ahx_synth_core.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define AMIGA_PAULA_PAL_CLK 3546895
#define AHX_FRAME_RATE 50
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define CLAMP(x, low, high) (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))

// MIDI note to Amiga period conversion table (based on C-2 = period 428)
static const int note_to_period_table[128] = {
    // Extended for full MIDI range
    6848, 6464, 6096, 5760, 5424, 5120, 4832, 4560, 4304, 4064, 3840, 3616,  // Octave 0
    3424, 3232, 3048, 2880, 2712, 2560, 2416, 2280, 2152, 2032, 1920, 1808,  // Octave 1
    1712, 1616, 1524, 1440, 1356, 1280, 1208, 1140, 1076, 1016,  960,  904,  // Octave 2
     856,  808,  762,  720,  678,  640,  604,  570,  538,  508,  480,  452,  // Octave 3
     428,  404,  381,  360,  339,  320,  302,  285,  269,  254,  240,  226,  // Octave 4 (C-2 = 428)
     214,  202,  190,  180,  170,  160,  151,  143,  135,  127,  120,  113,  // Octave 5
     107,  101,   95,   90,   85,   80,   75,   71,   67,   63,   60,   56,  // Octave 6
      53,   50,   47,   45,   42,   40,   37,   35,   33,   31,   30,   28,  // Octave 7
      26,   25,   23,   22,   21,   20,   18,   17,   16,   15,   15,   14,  // Octave 8
      13,   12,   11,   11,   10,   10,    9,    8,    8,    7,    7,    7,  // Octave 9
       6,    6,    5,    5,    5,    5,    4,    4  // Octave 10 (partial, MIDI ends at 127)
};

// Initialize voice
void ahx_synth_voice_init(AhxSynthVoice* voice) {
    memset(voice, 0, sizeof(AhxSynthVoice));
    memset(voice->VoiceBuffer, 0, 0x281 * sizeof(int16_t));

    // Initialize tracker components
    tracker_modulator_init(&voice->filter_mod);
    tracker_modulator_init(&voice->square_mod);
    tracker_voice_init(&voice->voice_playback);

    voice->TrackOn = true;
    voice->NoteMaxVolume = 0x40;  // Default max volume
    voice->WNRandom = 0x280;      // White noise seed
}

// Calculate ADSR deltas (AUTHENTIC AHX ALGORITHM from ahx_player.c:389)
void ahx_synth_voice_calc_adsr(AhxSynthVoice* voice, const AhxCoreInstrument* instrument) {
    voice->Instrument = instrument;

    // Calculate deltas per frame (exactly as AHX does it)
    if (instrument->Envelope.aFrames > 0) {
        voice->ADSR.aFrames = instrument->Envelope.aFrames;
        voice->ADSR.aVolume = instrument->Envelope.aVolume * 256 / voice->ADSR.aFrames;
    } else {
        voice->ADSR.aFrames = 1;
        voice->ADSR.aVolume = instrument->Envelope.aVolume * 256;
    }

    if (instrument->Envelope.dFrames > 0) {
        voice->ADSR.dFrames = instrument->Envelope.dFrames;
        voice->ADSR.dVolume = (instrument->Envelope.dVolume - instrument->Envelope.aVolume) * 256 / voice->ADSR.dFrames;
    } else {
        voice->ADSR.dFrames = 1;
        voice->ADSR.dVolume = (instrument->Envelope.dVolume - instrument->Envelope.aVolume) * 256;
    }

    voice->ADSR.sFrames = instrument->Envelope.sFrames;

    if (instrument->Envelope.rFrames > 0) {
        voice->ADSR.rFrames = instrument->Envelope.rFrames;
        voice->ADSR.rVolume = (instrument->Envelope.rVolume - instrument->Envelope.dVolume) * 256 / voice->ADSR.rFrames;
    } else {
        voice->ADSR.rFrames = 1;
        voice->ADSR.rVolume = (instrument->Envelope.rVolume - instrument->Envelope.dVolume) * 256;
    }
}

// Convert MIDI note to Amiga period
int ahx_synth_note_to_period(uint8_t note) {
    if (note >= 128) note = 127;
    return note_to_period_table[note];
}

// Trigger note on
void ahx_synth_voice_note_on(AhxSynthVoice* voice, uint8_t note, uint8_t velocity, uint32_t sample_rate) {
    if (!voice || !voice->Instrument) return;

    voice->Released = false;
    voice->TrackOn = true;
    voice->samples_per_frame = sample_rate / AHX_FRAME_RATE;
    voice->samples_in_frame = 0;

    // Convert MIDI note to Amiga period
    voice->InstrPeriod = ahx_synth_note_to_period(note);
    voice->VoicePeriod = voice->InstrPeriod;

    // Set velocity as volume
    voice->NoteMaxVolume = (velocity * 64) / 127;
    if (voice->NoteMaxVolume > 64) voice->NoteMaxVolume = 64;

    // Reset ADSR to attack phase
    voice->ADSRVolume = 0;

    // Reset vibrato
    voice->VibratoDelay = voice->Instrument->VibratoDelay;
    voice->VibratoCurrent = 0;
    voice->VibratoPeriod = 0;
    voice->VibratoDepth = voice->Instrument->VibratoDepth;
    voice->VibratoSpeed = voice->Instrument->VibratoSpeed;

    // Setup filter modulation
    if (voice->Instrument->FilterLowerLimit != voice->Instrument->FilterUpperLimit) {
        tracker_modulator_set_active(&voice->filter_mod, true);
        tracker_modulator_set_limits(&voice->filter_mod,
            voice->Instrument->FilterLowerLimit,
            voice->Instrument->FilterUpperLimit);
        tracker_modulator_set_speed(&voice->filter_mod, voice->Instrument->FilterSpeed);
        tracker_modulator_init(&voice->filter_mod);  // Reset to start
    } else {
        tracker_modulator_set_active(&voice->filter_mod, false);
    }

    // Setup PWM modulation
    if (voice->Instrument->SquareLowerLimit != voice->Instrument->SquareUpperLimit) {
        tracker_modulator_set_active(&voice->square_mod, true);
        tracker_modulator_set_limits(&voice->square_mod,
            voice->Instrument->SquareLowerLimit,
            voice->Instrument->SquareUpperLimit);
        tracker_modulator_set_speed(&voice->square_mod, voice->Instrument->SquareSpeed);
        tracker_modulator_init(&voice->square_mod);  // Reset to start
    } else {
        tracker_modulator_set_active(&voice->square_mod, false);
    }

    // Setup hard cut release
    voice->HardCutRelease = voice->Instrument->HardCutRelease;
    voice->HardCutReleaseF = voice->Instrument->HardCutReleaseFrames;
    voice->NoteCutOn = 0;
    voice->NoteCutWait = 0;

    // Set waveform and wave length
    voice->Waveform = 1;  // Default to sawtooth (AHX default)
    voice->WaveLength = voice->Instrument->WaveLength;

    // Setup voice playback with period
    tracker_voice_set_period(&voice->voice_playback, voice->VoicePeriod, AMIGA_PAULA_PAL_CLK, sample_rate);
}

// Trigger note off
void ahx_synth_voice_note_off(AhxSynthVoice* voice) {
    if (!voice) return;

    voice->Released = true;

    if (voice->Instrument && voice->Instrument->HardCutRelease) {
        // Hard cut release: immediate cut with short fade
        voice->NoteCutOn = 1;
        voice->NoteCutWait = 0;
    }
    // Normal release happens in process_frame via ADSR
}

// Process one frame (AUTHENTIC AHX ALGORITHM from ahx_player.c:901)
void ahx_synth_voice_process_frame(AhxSynthVoice* voice) {
    if (!voice || !voice->TrackOn || !voice->Instrument) return;

    // Hard cut release processing
    if (voice->HardCutRelease && voice->NoteCutOn) {
        if (voice->NoteCutWait <= 0) {
            voice->NoteCutOn = 0;
            // Recalculate release for hard cut
            int target_vol = voice->Instrument->Envelope.rVolume;
            int frames = voice->HardCutReleaseF ? voice->HardCutReleaseF : 1;
            voice->ADSR.rVolume = -(voice->ADSRVolume - (target_vol << 8)) / frames;
            voice->ADSR.rFrames = frames;
            voice->ADSR.aFrames = voice->ADSR.dFrames = voice->ADSR.sFrames = 0;
        } else {
            voice->NoteCutWait--;
        }
    }

    // ADSR envelope (EXACT ALGORITHM from ahx_player.c:948-962)
    if (voice->ADSR.aFrames) {
        voice->ADSRVolume += voice->ADSR.aVolume;
        if (--voice->ADSR.aFrames <= 0)
            voice->ADSRVolume = voice->Instrument->Envelope.aVolume << 8;
    } else if (voice->ADSR.dFrames) {
        voice->ADSRVolume += voice->ADSR.dVolume;
        if (--voice->ADSR.dFrames <= 0)
            voice->ADSRVolume = voice->Instrument->Envelope.dVolume << 8;
    } else if (voice->ADSR.sFrames) {
        voice->ADSR.sFrames--;
        // After sustain, go to release if note was released
        if (voice->ADSR.sFrames == 0 && voice->Released) {
            // Transition to release
        }
    } else if (voice->ADSR.rFrames) {
        voice->ADSRVolume += voice->ADSR.rVolume;
        if (--voice->ADSR.rFrames <= 0) {
            voice->ADSRVolume = voice->Instrument->Envelope.rVolume << 8;
            // Voice finished
            if (voice->Instrument->Envelope.rVolume == 0) {
                voice->TrackOn = false;
            }
        }
    } else if (voice->Released) {
        // Released but no release frames - go to release immediately
        voice->TrackOn = false;
    }

    // Vibrato
    if (voice->VibratoDelay > 0) {
        voice->VibratoDelay--;
    } else {
        voice->VibratoCurrent = (voice->VibratoCurrent + voice->VibratoSpeed) & 0xFF;
        // Sine-based vibrato (simplified)
        int vib_table_value = (int)(sinf(voice->VibratoCurrent * 3.14159f / 128.0f) * voice->VibratoDepth * 8);
        voice->VibratoPeriod = vib_table_value;
    }

    // Update filter modulation
    if (tracker_modulator_is_active(&voice->filter_mod)) {
        tracker_modulator_update(&voice->filter_mod);
    }

    // Update PWM modulation
    if (tracker_modulator_is_active(&voice->square_mod)) {
        tracker_modulator_update(&voice->square_mod);
    }

    // Calculate final voice volume
    int adsr_vol = voice->ADSRVolume >> 8;  // Convert from fixed point (8-bit)
    voice->VoiceVolume = (voice->NoteMaxVolume * adsr_vol * voice->Instrument->Volume) >> 12;
    if (voice->VoiceVolume > 64) voice->VoiceVolume = 64;
    if (voice->VoiceVolume < 0) voice->VoiceVolume = 0;

    // Calculate final period with vibrato
    voice->VoicePeriod = voice->InstrPeriod + voice->VibratoPeriod;
    if (voice->VoicePeriod < 113) voice->VoicePeriod = 113;  // Min period
    if (voice->VoicePeriod > 6848) voice->VoicePeriod = 6848;  // Max period
}

// Process audio samples
uint32_t ahx_synth_voice_process(AhxSynthVoice* voice, float* output, uint32_t num_samples, uint32_t sample_rate) {
    if (!voice || !output || !voice->TrackOn) {
        if (output) {
            memset(output, 0, num_samples * sizeof(float));
        }
        return 0;
    }

    for (uint32_t i = 0; i < num_samples; i++) {
        // Process frame timing (50Hz)
        if (voice->samples_in_frame >= voice->samples_per_frame) {
            ahx_synth_voice_process_frame(voice);
            voice->samples_in_frame = 0;

            // Update voice period if changed
            tracker_voice_set_period(&voice->voice_playback, voice->VoicePeriod, AMIGA_PAULA_PAL_CLK, sample_rate);
        }
        voice->samples_in_frame++;

        // Get sample from voice playback
        int32_t left, right;
        tracker_voice_get_stereo_sample(&voice->voice_playback, &left, &right);

        // Apply volume and convert to float
        float sample = ((float)(left + right) / 2.0f) * (voice->VoiceVolume / 64.0f) / 32768.0f;
        output[i] = sample;

        if (!voice->TrackOn) {
            // Voice stopped - clear remaining buffer
            memset(&output[i + 1], 0, (num_samples - i - 1) * sizeof(float));
            return i + 1;
        }
    }

    return num_samples;
}

// Check if voice is active
bool ahx_synth_voice_is_active(const AhxSynthVoice* voice) {
    return voice && voice->TrackOn;
}

// Reset voice
void ahx_synth_voice_reset(AhxSynthVoice* voice) {
    if (!voice) return;

    const AhxCoreInstrument* inst = voice->Instrument;
    ahx_synth_voice_init(voice);
    voice->Instrument = inst;
}
