/*
 * SID Synthesizer - MIDI CC Implementation
 * Compatible with MIDIbox SID V2
 */

#include "synth_sid_cc.h"
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ============================================================================
// CC Handler Implementation
// ============================================================================

void synth_sid_handle_cc(SynthSID* sid, uint8_t cc, uint8_t value) {
    if (!sid) return;

    // Normalize CC value to 0.0-1.0
    float normalized = value / 127.0f;

    switch (cc) {
        // ====================================================================
        // Global Parameters
        // ====================================================================

        case SID_CC_VOLUME:
            synth_sid_set_volume(sid, normalized);
            break;

        case SID_CC_FILTER_CUTOFF:
            synth_sid_set_filter_cutoff(sid, normalized);
            break;

        case SID_CC_FILTER_RESONANCE:
            synth_sid_set_filter_resonance(sid, normalized);
            break;

        case SID_CC_FILTER_MODE:
            if (value == 0) {
                synth_sid_set_filter_mode(sid, SID_FILTER_OFF);
            } else if (value == 1) {
                synth_sid_set_filter_mode(sid, SID_FILTER_LP);
            } else if (value == 2) {
                synth_sid_set_filter_mode(sid, SID_FILTER_BP);
            } else if (value == 3) {
                synth_sid_set_filter_mode(sid, SID_FILTER_HP);
            }
            break;

        // ====================================================================
        // Voice 1 Parameters
        // ====================================================================

        case SID_CC_VOICE1_WAVEFORM: {
            uint8_t waveform = 0;
            if (value & 0x01) waveform |= SID_WAVE_TRIANGLE;
            if (value & 0x02) waveform |= SID_WAVE_SAWTOOTH;
            if (value & 0x04) waveform |= SID_WAVE_PULSE;
            if (value & 0x08) waveform |= SID_WAVE_NOISE;
            synth_sid_set_waveform(sid, 0, waveform);
            break;
        }

        case SID_CC_VOICE1_PULSEWIDTH:
            synth_sid_set_pulse_width(sid, 0, normalized);
            break;

        case SID_CC_VOICE1_ATTACK:
            synth_sid_set_attack(sid, 0, normalized);
            break;

        case SID_CC_VOICE1_DECAY:
            synth_sid_set_decay(sid, 0, normalized);
            break;

        case SID_CC_VOICE1_SUSTAIN:
            synth_sid_set_sustain(sid, 0, normalized);
            break;

        case SID_CC_VOICE1_RELEASE:
            synth_sid_set_release(sid, 0, normalized);
            break;

        case SID_CC_VOICE1_RING_MOD:
            synth_sid_set_ring_mod(sid, 0, value >= 64);
            break;

        case SID_CC_VOICE1_SYNC:
            synth_sid_set_sync(sid, 0, value >= 64);
            break;

        case SID_CC_FILTER_VOICE1:
            synth_sid_set_filter_voice(sid, 0, value >= 64);
            break;

        // ====================================================================
        // Voice 2 Parameters
        // ====================================================================

        case SID_CC_VOICE2_WAVEFORM: {
            uint8_t waveform = 0;
            if (value & 0x01) waveform |= SID_WAVE_TRIANGLE;
            if (value & 0x02) waveform |= SID_WAVE_SAWTOOTH;
            if (value & 0x04) waveform |= SID_WAVE_PULSE;
            if (value & 0x08) waveform |= SID_WAVE_NOISE;
            synth_sid_set_waveform(sid, 1, waveform);
            break;
        }

        case SID_CC_VOICE2_PULSEWIDTH:
            synth_sid_set_pulse_width(sid, 1, normalized);
            break;

        case SID_CC_VOICE2_ATTACK:
            synth_sid_set_attack(sid, 1, normalized);
            break;

        case SID_CC_VOICE2_DECAY:
            synth_sid_set_decay(sid, 1, normalized);
            break;

        case SID_CC_VOICE2_SUSTAIN:
            synth_sid_set_sustain(sid, 1, normalized);
            break;

        case SID_CC_VOICE2_RELEASE:
            synth_sid_set_release(sid, 1, normalized);
            break;

        case SID_CC_VOICE2_RING_MOD:
            synth_sid_set_ring_mod(sid, 1, value >= 64);
            break;

        case SID_CC_VOICE2_SYNC:
            synth_sid_set_sync(sid, 1, value >= 64);
            break;

        case SID_CC_FILTER_VOICE2:
            synth_sid_set_filter_voice(sid, 1, value >= 64);
            break;

        // ====================================================================
        // Voice 3 Parameters
        // ====================================================================

        case SID_CC_VOICE3_WAVEFORM: {
            uint8_t waveform = 0;
            if (value & 0x01) waveform |= SID_WAVE_TRIANGLE;
            if (value & 0x02) waveform |= SID_WAVE_SAWTOOTH;
            if (value & 0x04) waveform |= SID_WAVE_PULSE;
            if (value & 0x08) waveform |= SID_WAVE_NOISE;
            synth_sid_set_waveform(sid, 2, waveform);
            break;
        }

        case SID_CC_VOICE3_PULSEWIDTH:
            synth_sid_set_pulse_width(sid, 2, normalized);
            break;

        case SID_CC_VOICE3_ATTACK:
            synth_sid_set_attack(sid, 2, normalized);
            break;

        case SID_CC_VOICE3_DECAY:
            synth_sid_set_decay(sid, 2, normalized);
            break;

        case SID_CC_VOICE3_SUSTAIN:
            synth_sid_set_sustain(sid, 2, normalized);
            break;

        case SID_CC_VOICE3_RELEASE:
            synth_sid_set_release(sid, 2, normalized);
            break;

        case SID_CC_VOICE3_RING_MOD:
            synth_sid_set_ring_mod(sid, 2, value >= 64);
            break;

        case SID_CC_VOICE3_SYNC:
            synth_sid_set_sync(sid, 2, value >= 64);
            break;

        case SID_CC_FILTER_VOICE3:
            synth_sid_set_filter_voice(sid, 2, value >= 64);
            break;

        // ====================================================================
        // System Controllers
        // ====================================================================

        case SID_CC_ALL_SOUND_OFF:
        case SID_CC_ALL_NOTES_OFF:
            synth_sid_all_notes_off(sid);
            break;

        default:
            // Unknown or unimplemented CC, silently ignore
            break;
    }
}

void synth_sid_handle_pitch_bend_midi(SynthSID* sid, uint8_t voice, uint16_t value) {
    if (!sid || voice >= 3) return;

    // MIDI pitch bend: 0-16383, center at 8192
    // Convert to -1.0 to +1.0 (±12 semitones)
    float bend = ((float)value - 8192.0f) / 8192.0f;

    synth_sid_set_pitch_bend(sid, voice, bend);
}
