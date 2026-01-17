/*
 * SID Synthesizer - MIDI CC Mapping
 * Compatible with MIDIbox SID V2
 *
 * This provides a MIDI CC interface to the SID synth, making it
 * compatible with controllers, DAWs, and the MIDIbox/SammichSID ecosystem.
 *
 * Copyright (C) 2024
 * SPDX-License-Identifier: ISC
 */

#ifndef SYNTH_SID_CC_H
#define SYNTH_SID_CC_H

#include "synth_sid.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// MIDI CC Mapping (MIDIbox SID V2 Compatible)
// ============================================================================

/*
 * Standard MIDI Controllers (Global)
 */
#define SID_CC_BANK_SELECT      0    // Bank change (0-7)
#define SID_CC_MODULATION       1    // Modulation wheel
#define SID_CC_FILTER_CUTOFF    4    // Filter cutoff frequency (0-127)
#define SID_CC_FILTER_RESONANCE 5    // Filter resonance (0-127)
#define SID_CC_VOLUME           7    // Volume control (0-127)
#define SID_CC_PAN              10   // Pan (not applicable to mono SID, reserved)

/*
 * Oscillator Control (Per Voice - Voice 1 shown, add 32 for Voice 2, 64 for Voice 3)
 */
#define SID_CC_VOICE1_PHASE_OFFSET  2    // Phase offset adjustment
#define SID_CC_VOICE1_DETUNE        3    // Detune parameter

/*
 * Waveform Selection (Per Voice)
 * Bits: 0=Triangle, 1=Saw, 2=Pulse, 3=Noise
 */
#define SID_CC_VOICE1_WAVEFORM      20   // Voice 1 waveform (bitfield 0-15)
#define SID_CC_VOICE2_WAVEFORM      21   // Voice 2 waveform (bitfield 0-15)
#define SID_CC_VOICE3_WAVEFORM      22   // Voice 3 waveform (bitfield 0-15)

/*
 * Pitch Control (Per Voice)
 * Transpose: 0-63 = negative, 64 = off, 65-127 = positive
 */
#define SID_CC_VOICE1_TRANSPOSE     24   // Voice 1 transpose (-63 to +64 semitones)
#define SID_CC_VOICE2_TRANSPOSE     25   // Voice 2 transpose
#define SID_CC_VOICE3_TRANSPOSE     26   // Voice 3 transpose

#define SID_CC_VOICE1_FINETUNE      28   // Voice 1 fine tune
#define SID_CC_VOICE2_FINETUNE      29   // Voice 2 fine tune
#define SID_CC_VOICE3_FINETUNE      30   // Voice 3 fine tune

/*
 * Pulse Width (Per Voice)
 */
#define SID_CC_VOICE1_PULSEWIDTH    31   // Voice 1 pulse width (0-127)
#define SID_CC_VOICE2_PULSEWIDTH    32   // Voice 2 pulse width
#define SID_CC_VOICE3_PULSEWIDTH    33   // Voice 3 pulse width

/*
 * ADSR Envelope (Per Voice)
 * Values are 0-127 (internally converted to 0-15 for SID)
 */
#define SID_CC_VOICE1_ATTACK        42   // Voice 1 attack time
#define SID_CC_VOICE1_DECAY         43   // Voice 1 decay time
#define SID_CC_VOICE1_SUSTAIN       44   // Voice 1 sustain level
#define SID_CC_VOICE1_RELEASE       45   // Voice 1 release time

#define SID_CC_VOICE2_ATTACK        46   // Voice 2 attack time
#define SID_CC_VOICE2_DECAY         47   // Voice 2 decay time
#define SID_CC_VOICE2_SUSTAIN       48   // Voice 2 sustain level
#define SID_CC_VOICE2_RELEASE       49   // Voice 2 release time

#define SID_CC_VOICE3_ATTACK        50   // Voice 3 attack time
#define SID_CC_VOICE3_DECAY         51   // Voice 3 decay time
#define SID_CC_VOICE3_SUSTAIN       52   // Voice 3 sustain level
#define SID_CC_VOICE3_RELEASE       53   // Voice 3 release time

/*
 * Filter Mode
 */
#define SID_CC_FILTER_MODE          54   // 0=Off, 1=LP, 2=BP, 3=HP

/*
 * Voice Routing to Filter
 */
#define SID_CC_FILTER_VOICE1        55   // Voice 1 to filter (0=bypass, 127=through)
#define SID_CC_FILTER_VOICE2        56   // Voice 2 to filter
#define SID_CC_FILTER_VOICE3        57   // Voice 3 to filter

/*
 * Modulation Controls
 */
#define SID_CC_VOICE1_RING_MOD      58   // Voice 1 ring modulation (0=off, 127=on)
#define SID_CC_VOICE1_SYNC          59   // Voice 1 hard sync (0=off, 127=on)

#define SID_CC_VOICE2_RING_MOD      60   // Voice 2 ring modulation
#define SID_CC_VOICE2_SYNC          61   // Voice 2 hard sync

#define SID_CC_VOICE3_RING_MOD      62   // Voice 3 ring modulation
#define SID_CC_VOICE3_SYNC          63   // Voice 3 hard sync

/*
 * Standard MIDI Controllers
 */
#define SID_CC_SUSTAIN_PEDAL        64   // Sustain pedal (hold)
#define SID_CC_PORTAMENTO           65   // Portamento on/off
#define SID_CC_PORTAMENTO_TIME      5    // Portamento time (using CC 5 alt function)

/*
 * LFO Controls
 */
#define SID_CC_LFO1_RATE            70   // LFO1 frequency (0-127)
#define SID_CC_LFO1_WAVEFORM        71   // LFO1 waveform (0-5)
#define SID_CC_LFO1_TO_PITCH        72   // LFO1 → Pitch depth
#define SID_CC_LFO2_RATE            73   // LFO2 frequency (0-127)
#define SID_CC_LFO2_WAVEFORM        74   // LFO2 waveform (0-5)
#define SID_CC_LFO2_TO_FILTER       75   // LFO2 → Filter depth
#define SID_CC_LFO2_TO_PW           76   // LFO2 → PW depth

/*
 * All Sound Off / All Notes Off
 */
#define SID_CC_ALL_SOUND_OFF        120  // Silence all voices immediately
#define SID_CC_ALL_NOTES_OFF        123  // Release all notes

// ============================================================================
// CC Handler
// ============================================================================

/**
 * Process a MIDI CC message
 * @param sid SID synthesizer instance
 * @param cc CC number (0-127)
 * @param value CC value (0-127)
 */
void synth_sid_handle_cc(SynthSID* sid, uint8_t cc, uint8_t value);

/**
 * Process a MIDI pitch bend message
 * @param sid SID synthesizer instance
 * @param voice Voice number (0-2)
 * @param value Pitch bend value (0-16383, center=8192)
 */
void synth_sid_handle_pitch_bend_midi(SynthSID* sid, uint8_t voice, uint16_t value);

#ifdef __cplusplus
}
#endif

#endif // SYNTH_SID_CC_H
