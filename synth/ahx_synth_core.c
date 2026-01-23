/*
 * AHX Synthesis Core - Implementation
 *
 * Authentic AHX synthesis algorithms extracted from ahx_player.c
 * Shared by both the module player and synth plugin
 */

#include "ahx_synth_core.h"
#include "ahx_waves.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifdef EMSCRIPTEN
#include <emscripten.h>
#endif

#define AMIGA_PAULA_PAL_CLK 3546895
#define AHX_FRAME_RATE 50
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define CLAMP(x, low, high) (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))

// AHX Period Table (from ahx_player.c:211) - converts note index to Amiga period
// Index 1 = lowest note (longest period), Index 60 = highest note (shortest period)
static const int AhxPeriodTable[61] = {
    0x0000, 0x0D60, 0x0CA0, 0x0BE8, 0x0B40, 0x0A98, 0x0A00, 0x0970,
    0x08E8, 0x0868, 0x07F0, 0x0780, 0x0714, 0x06B0, 0x0650, 0x05F4,
    0x05A0, 0x054C, 0x0500, 0x04B8, 0x0474, 0x0434, 0x03F8, 0x03C0,
    0x038A, 0x0358, 0x0328, 0x02FA, 0x02D0, 0x02A6, 0x0280, 0x025C,
    0x023A, 0x021A, 0x01FC, 0x01E0, 0x01C5, 0x01AC, 0x0194, 0x017D,
    0x0168, 0x0153, 0x0140, 0x012E, 0x011D, 0x010D, 0x00FE, 0x00F0,
    0x00E2, 0x00D6, 0x00CA, 0x00BE, 0x00B4, 0x00AA, 0x00A0, 0x0097,
    0x008F, 0x0087, 0x007F, 0x0078, 0x0071
};

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

// ============================================================================
// Waveform Generation - now uses authentic AHX pre-computed waveforms
// ============================================================================

// Generate waveform and populate VoiceBuffer based on waveform type, wave_length, and filter position
void ahx_synth_generate_waveform(AhxSynthVoice* voice, uint8_t waveform, uint8_t wave_length, int filter_pos) {
    // Get shared AHX waves instance (singleton, initialized on first call)
    AhxWaves* waves = ahx_waves_get();
    if (!waves) {
        memset(voice->VoiceBuffer, 0, 0x281 * sizeof(int16_t));
        return;
    }

    // Clamp filter position to valid range (32-63)
    if (filter_pos < 32) filter_pos = 32;
    if (filter_pos > 63) filter_pos = 63;

    // For triangle/sawtooth/noise: get pre-computed waveform pointer
    // For square: generate into VoiceBuffer with authentic resampling
    if (waveform == 2) {
        // Square waveform - use authentic AHX square generation with PWM
        // SquarePos comes from modulator (0-63), affects pulse width
        int16_t temp_buffer[0x281];
        int square_reverse = 0;
        ahx_waves_generate_square(waves, temp_buffer,
                                  voice->SquarePos, wave_length, filter_pos, &square_reverse);

        // Repeat square waveform to fill VoiceBuffer (matching player_set_audio logic)
        int wave_loops = (1 << (5 - wave_length)) * 5;
        int wave_size = 4 * (1 << wave_length);
        for (int i = 0; i < wave_loops; i++) {
            memcpy(&voice->VoiceBuffer[i * wave_size], temp_buffer, wave_size * sizeof(int16_t));
        }
    } else if (waveform == 3) {
        // White noise - use random offset for variation (authentic AHX)
        // Get pre-computed noise table
        const int16_t* noise_source = ahx_waves_get_waveform(waves, 3, 0, 32);
        if (noise_source) {
            // Calculate random offset into WhiteNoiseBig table (matching ahx_player.c:1173-1176)
            int offset = (voice->WNRandom & (2*0x280-1)) & ~1;

            // Copy noise from random offset
            for (int i = 0; i < 0x280; i++) {
                voice->VoiceBuffer[i] = noise_source[(offset + i) % (0x280*3)];
            }

            // Update random seed for next time (authentic AHX algorithm)
            voice->WNRandom += 2239384;
            voice->WNRandom = ((((voice->WNRandom >> 8) | (voice->WNRandom << 24)) + 782323) ^ 75) - 6735;
        } else {
            memset(voice->VoiceBuffer, 0, 0x280 * sizeof(int16_t));
        }
    } else {
        // Triangle (0) or Sawtooth (1) - get pre-computed filtered waveform
        const int16_t* waveform_source = ahx_waves_get_waveform(waves, waveform, wave_length, filter_pos);
        if (waveform_source) {
            // Repeat waveform to fill VoiceBuffer (matching player_set_audio logic)
            int wave_loops = (1 << (5 - wave_length)) * 5;
            int wave_size = 4 * (1 << wave_length);

            for (int i = 0; i < wave_loops; i++) {
                memcpy(&voice->VoiceBuffer[i * wave_size], waveform_source, wave_size * sizeof(int16_t));
            }
        } else {
            memset(voice->VoiceBuffer, 0, 0x280 * sizeof(int16_t));
        }
    }

    // Wrap-around sample for interpolation
    voice->VoiceBuffer[0x280] = voice->VoiceBuffer[0];
}

// Initialize voice
void ahx_synth_voice_init(AhxSynthVoice* voice) {
    memset(voice, 0, sizeof(AhxSynthVoice));
    memset(voice->VoiceBuffer, 0, 0x281 * sizeof(int16_t));

    // Initialize tracker components
    tracker_modulator_init(&voice->filter_mod);
    tracker_modulator_init(&voice->square_mod);
    tracker_voice_init(&voice->voice_playback);

    // Set tracker voice volume to 1 (we apply volume separately via VoiceVolume)
    tracker_voice_set_volume(&voice->voice_playback, 1);

    voice->TrackOn = true;
    voice->NoteMaxVolume = 0x40;  // Default max volume
    voice->WNRandom = 0x280;      // White noise seed
    voice->SpeedMultiplier = 1;   // Default speed (no multiplier)
    voice->PListActive = false;   // No PList by default
    voice->FilterPos = 32;        // Default filter position (middle)
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

// Convert MIDI note to AHX note index (1-60 range for PeriodTable)
int ahx_synth_note_to_period(uint8_t note) {
    // Map MIDI notes to AHX note range (1-60)
    // MIDI 24 (C1) -> AHX 1 (lowest)
    // MIDI 83 (B5) -> AHX 60 (highest)
    int ahx_note = (int)note - 23;  // Offset to align with AHX range

    // Clamp to valid AHX note range
    if (ahx_note < 1) ahx_note = 1;
    if (ahx_note > 60) ahx_note = 60;

    return ahx_note;  // Return AHX note INDEX (not period!)
}

// Get Amiga period for AHX note index
int ahx_synth_get_period_for_note(int ahx_note) {
    // Clamp to valid range
    if (ahx_note < 1) ahx_note = 1;
    if (ahx_note > 60) ahx_note = 60;

    return AhxPeriodTable[ahx_note];
}

// Trigger note on
void ahx_synth_voice_note_on(AhxSynthVoice* voice, uint8_t note, uint8_t velocity, uint32_t sample_rate) {
    if (!voice || !voice->Instrument) return;

    voice->Released = false;
    voice->TrackOn = true;
    // Apply SpeedMultiplier to frame rate (higher multiplier = more frames per second)
    int speed_mult = (voice->SpeedMultiplier > 0) ? voice->SpeedMultiplier : 1;
    voice->samples_per_frame = sample_rate / AHX_FRAME_RATE / speed_mult;
    voice->samples_in_frame = 0;

#ifdef EMSCRIPTEN
    emscripten_log(EM_LOG_CONSOLE, "[AHX] Note on: sample_rate=%d, SpeedMult=%d, samples_per_frame=%d",
        sample_rate, speed_mult, voice->samples_per_frame);
#endif

    // Reset frame counter for debugging
    voice->debug_frame_count = 0;

    // Convert MIDI note to AHX note index (1-60) - NOT period!
    voice->InstrPeriod = ahx_synth_note_to_period(note);  // Stores AHX note INDEX
    voice->PlantPeriod = 1;  // Signal that period needs calculation

    // VoicePeriod will be calculated in first process_frame() from PeriodTable lookup
    voice->VoicePeriod = AhxPeriodTable[voice->InstrPeriod];

    // Set velocity as volume
    voice->NoteMaxVolume = (velocity * 64) / 127;
    if (voice->NoteMaxVolume > 64) voice->NoteMaxVolume = 64;

    // Reset ADSR to attack phase
    voice->ADSRVolume = 0;

    // CRITICAL: Recalculate ADSR deltas to reset frame counters
#ifdef EMSCRIPTEN
    if (!voice->Instrument) {
        emscripten_log(EM_LOG_ERROR, "[AHX] ERROR: Instrument pointer is NULL!");
    } else {
        emscripten_log(EM_LOG_CONSOLE, "[AHX] Instrument envelope BEFORE calc: a=%d(%d) d=%d(%d) s=%d r=%d(%d)",
            voice->Instrument->Envelope.aFrames, voice->Instrument->Envelope.aVolume,
            voice->Instrument->Envelope.dFrames, voice->Instrument->Envelope.dVolume,
            voice->Instrument->Envelope.sFrames,
            voice->Instrument->Envelope.rFrames, voice->Instrument->Envelope.rVolume);
    }
#endif

    ahx_synth_voice_calc_adsr(voice, voice->Instrument);

#ifdef EMSCRIPTEN
    emscripten_log(EM_LOG_CONSOLE, "[AHX] Calculated ADSR runtime AFTER calc: a=%d d=%d s=%d r=%d",
        voice->ADSR.aFrames, voice->ADSR.dFrames, voice->ADSR.sFrames, voice->ADSR.rFrames);
#endif

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

    // Initialize modulator wait counters (authentic AHX timing)
    // Filter uses Speed-3 (min 1), Square uses Speed directly
    voice->FilterWait = voice->Instrument->FilterSpeed - 3;
    if (voice->FilterWait < 1) voice->FilterWait = 1;
    voice->SquareWait = voice->Instrument->SquareSpeed;

    // Setup hard cut release
    voice->HardCutRelease = voice->Instrument->HardCutRelease;
    voice->HardCutReleaseF = voice->Instrument->HardCutReleaseFrames;
    voice->NoteCutOn = 0;
    voice->NoteCutWait = 0;

    // Set waveform and wave length from instrument
    voice->Waveform = voice->Instrument->Waveform;
    voice->WaveLength = voice->Instrument->WaveLength;

#ifdef EMSCRIPTEN
    emscripten_log(EM_LOG_CONSOLE, "[AHX] Generating waveform: type=%d, length=%d", voice->Waveform, voice->WaveLength);
    emscripten_log(EM_LOG_CONSOLE, "[AHX] MIDI note=%d, AHX note=%d, Period=%d", note, voice->InstrPeriod, voice->VoicePeriod);
#endif

    // Generate waveform and populate VoiceBuffer
    ahx_synth_generate_waveform(voice, voice->Waveform, voice->WaveLength, voice->FilterPos);

    // Setup voice playback with period and waveform buffer
    tracker_voice_set_period(&voice->voice_playback, voice->VoicePeriod, AMIGA_PAULA_PAL_CLK, sample_rate);
    tracker_voice_set_waveform_16bit(&voice->voice_playback, voice->VoiceBuffer, 0x280);

    // Set tracker voice volume to 1 (we apply volume separately via VoiceVolume)
    tracker_voice_set_volume(&voice->voice_playback, 1);

    // CRITICAL: Reset playback position to start of waveform
    tracker_voice_reset_position(&voice->voice_playback);
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

    static int frame_count = 0;
    if (frame_count < 50) {  // Log first 50 frames
#ifdef EMSCRIPTEN
        emscripten_log(EM_LOG_CONSOLE, "[Frame %d] ADSR: a=%d d=%d s=%d r=%d, vol=%d, TrackOn=%d",
            frame_count++, voice->ADSR.aFrames, voice->ADSR.dFrames, voice->ADSR.sFrames, voice->ADSR.rFrames,
            voice->ADSRVolume >> 8, voice->TrackOn);
#endif
    }

    // Hard cut release processing
    if (voice->HardCutRelease && voice->NoteCutOn) {
        if (voice->NoteCutWait <= 0) {
            voice->NoteCutOn = 0;
            // Recalculate release for hard cut
            int target_vol = voice->Instrument->Envelope.rVolume;
            int frames = voice->HardCutReleaseF ? voice->HardCutReleaseF : 1;

            // Force immediate transition to release phase
            voice->ADSR.aFrames = 0;
            voice->ADSR.dFrames = 0;
            voice->ADSR.sFrames = 0;
            voice->ADSR.rFrames = frames;
            voice->ADSR.rVolume = -(voice->ADSRVolume - (target_vol << 8)) / frames;
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
        if (--voice->ADSR.dFrames <= 0) {
            voice->ADSRVolume = voice->Instrument->Envelope.dVolume << 8;
            // If sustain_frames=0, skip to release immediately (for percussion)
            if (voice->Instrument->Envelope.sFrames == 0) {
                // Recalculate release from decay volume to release volume
                if (voice->Instrument->Envelope.rFrames > 0) {
                    voice->ADSR.rFrames = voice->Instrument->Envelope.rFrames;
                    voice->ADSR.rVolume = (voice->Instrument->Envelope.rVolume - voice->Instrument->Envelope.dVolume) * 256 / voice->ADSR.rFrames;
                } else {
                    voice->ADSR.rFrames = 1;
                    voice->ADSR.rVolume = (voice->Instrument->Envelope.rVolume - voice->Instrument->Envelope.dVolume) * 256;
                }
            }
        }
    } else if (voice->ADSR.sFrames) {
        // Sustain phase - count down frames
        if (--voice->ADSR.sFrames <= 0) {
            // Sustain finished - reset release phase from instrument
            voice->ADSR.sFrames = 0;

            // Recalculate release from current decay volume to release volume
            if (voice->Instrument->Envelope.rFrames > 0) {
                voice->ADSR.rFrames = voice->Instrument->Envelope.rFrames;
                voice->ADSR.rVolume = (voice->Instrument->Envelope.rVolume - voice->Instrument->Envelope.dVolume) * 256 / voice->ADSR.rFrames;
            } else {
                voice->ADSR.rFrames = 1;
                voice->ADSR.rVolume = (voice->Instrument->Envelope.rVolume - voice->Instrument->Envelope.dVolume) * 256;
            }
        }
    } else if (voice->ADSR.rFrames) {
        voice->ADSRVolume += voice->ADSR.rVolume;
        if (--voice->ADSR.rFrames <= 0) {
            voice->ADSRVolume = voice->Instrument->Envelope.rVolume << 8;
            // Release finished - stop voice UNLESS PList is still active
            if (!voice->PListActive) {
                voice->TrackOn = false;
            }
        }
    } else {
        // All ADSR stages complete - stop voice UNLESS PList is still active
        if (!voice->PListActive) {
            voice->TrackOn = false;
        }
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

    // Update filter modulation (authentic AHX timing - wait counter)
    if (tracker_modulator_is_active(&voice->filter_mod) && --voice->FilterWait <= 0) {
        // Authentic AHX filter behavior: Speed < 4 updates multiple times
        int f_max = (voice->Instrument->FilterSpeed < 4) ? (5 - voice->Instrument->FilterSpeed) : 1;
        for (int i = 0; i < f_max; i++) {
            tracker_modulator_update(&voice->filter_mod);
        }

        // Clamp filter position to valid range
        int filter_pos = tracker_modulator_get_position(&voice->filter_mod);
        if (filter_pos < 1) {
            filter_pos = 1;
            tracker_modulator_set_position(&voice->filter_mod, filter_pos);
        }
        if (filter_pos > 63) {
            filter_pos = 63;
            tracker_modulator_set_position(&voice->filter_mod, filter_pos);
        }

        // Sync to voice FilterPos (used for waveform generation)
        voice->FilterPos = filter_pos;

        // Reset wait counter
        voice->FilterWait = voice->Instrument->FilterSpeed - 3;
        if (voice->FilterWait < 1) voice->FilterWait = 1;

        // Mark for waveform regeneration
        voice->NewWaveform = 1;
    }

    // Update PWM modulation (authentic AHX timing - wait counter)
    if (voice->Waveform == 2 && tracker_modulator_is_active(&voice->square_mod)) {
        if (--voice->SquareWait <= 0) {
            tracker_modulator_update(&voice->square_mod);
            voice->SquareWait = voice->Instrument->SquareSpeed;
            voice->NewWaveform = 1;
        }
    }

    // Process waveform changes (AUTHENTIC AHX ALGORITHM from ahx_player.c:1204)
    if (voice->NewWaveform) {
        ahx_synth_generate_waveform(voice, voice->Waveform, voice->WaveLength, voice->FilterPos);

        // Update voice playback with new waveform buffer (16-bit)
        tracker_voice_set_waveform_16bit(&voice->voice_playback, voice->VoiceBuffer, 0x280);

        voice->NewWaveform = 0;
    }

    // Calculate final voice volume
    int adsr_vol = voice->ADSRVolume >> 8;  // Convert from fixed point (8-bit)
    voice->VoiceVolume = (voice->NoteMaxVolume * adsr_vol * voice->Instrument->Volume) >> 12;
    if (voice->VoiceVolume > 64) voice->VoiceVolume = 64;
    if (voice->VoiceVolume < 0) voice->VoiceVolume = 0;

    // Calculate final period with vibrato (AUTHENTIC AHX from ahx_player.c:1228-1261)
    if (voice->PlantPeriod) {
        voice->PlantPeriod = 0;

        // Calculate audio period from instrument note index (ahx_player.c:1228-1233)
        int audio_period = voice->InstrPeriod;

        // Note: In full AHX player, this would add Transpose + TrackPeriod
        // For synth mode we skip transpose since we're using MIDI directly

        // Clamp to valid note range
        if (audio_period > 60) audio_period = 60;
        if (audio_period < 0) audio_period = 0;

        // Look up actual Amiga period from table
        audio_period = AhxPeriodTable[audio_period];

        // Add portamento and vibrato
        voice->VoicePeriod = audio_period + voice->VibratoPeriod;

        // Clamp to valid period range
        if (voice->VoicePeriod < 113) voice->VoicePeriod = 113;
        if (voice->VoicePeriod > 6848) voice->VoicePeriod = 6848;
    }
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

        // Get raw sample from voice playback (without volume/panning applied)
        int32_t sample = tracker_voice_get_sample(&voice->voice_playback);

        // Apply volume and convert to float
        // Reduce output to 50% to prevent clipping when mixing 4 voices
        float sample_f = sample / 32768.0f;
        float vol = (voice->VoiceVolume / 64.0f) * 0.5f;
        output[i] = sample_f * vol;

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
