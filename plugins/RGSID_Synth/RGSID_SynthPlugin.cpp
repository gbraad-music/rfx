#include "DistrhoPlugin.hpp"
#include "../../synth/synth_sid.h"
#include "../../synth/synth_sid_cc.h"
#include <cstring>
#include <cmath>

START_NAMESPACE_DISTRHO

class RGSID_SynthPlugin : public Plugin
{
public:
    RGSID_SynthPlugin()
        : Plugin(kParameterCount, 0, 0)
    {
        // Create SID synthesizer
        fSID = synth_sid_create(getSampleRate());

        // Initialize feedback prevention flag
        fUpdatingFromCC = false;

        // Initialize parameter values to defaults
        // Voice 1
        fVoice1Waveform = SID_WAVE_PULSE;
        fVoice1PulseWidth = 0.5f;
        fVoice1Attack = 0.0f;
        fVoice1Decay = 0.5f;
        fVoice1Sustain = 0.7f;
        fVoice1Release = 0.3f;
        fVoice1RingMod = 0.0f;
        fVoice1Sync = 0.0f;

        // Voice 2
        fVoice2Waveform = SID_WAVE_PULSE;
        fVoice2PulseWidth = 0.5f;
        fVoice2Attack = 0.0f;
        fVoice2Decay = 0.5f;
        fVoice2Sustain = 0.7f;
        fVoice2Release = 0.3f;
        fVoice2RingMod = 0.0f;
        fVoice2Sync = 0.0f;

        // Voice 3
        fVoice3Waveform = SID_WAVE_PULSE;
        fVoice3PulseWidth = 0.5f;
        fVoice3Attack = 0.0f;
        fVoice3Decay = 0.5f;
        fVoice3Sustain = 0.7f;
        fVoice3Release = 0.3f;
        fVoice3RingMod = 0.0f;
        fVoice3Sync = 0.0f;

        // Filter
        fFilterMode = SID_FILTER_LP;
        fFilterCutoff = 0.5f;
        fFilterResonance = 0.0f;
        fFilterVoice1 = 0.0f;
        fFilterVoice2 = 0.0f;
        fFilterVoice3 = 0.0f;

        // Global
        fVolume = 0.7f;

        // Apply initial parameters to SID
        if (fSID) {
            applyParametersToSID();
        }
    }

    ~RGSID_SynthPlugin() override
    {
        if (fSID) {
            synth_sid_destroy(fSID);
        }
    }

protected:
    const char* getLabel() const override { return RGSID_DISPLAY_NAME; }
    const char* getDescription() const override { return RGSID_DESCRIPTION; }
    const char* getMaker() const override { return "Regroove"; }
    const char* getHomePage() const override { return "https://music.gbraad.nl/regrooved/"; }
    const char* getLicense() const override { return "GPL-3.0"; }
    uint32_t getVersion() const override { return d_version(1, 0, 0); }
    int64_t getUniqueId() const override { return d_cconst('R', 'G', 'S', 'I'); }

    void initParameter(uint32_t index, Parameter& param) override
    {
        param.hints = kParameterIsAutomatable;
        param.ranges.min = 0.0f;
        param.ranges.max = 1.0f;

        switch (index) {
        // Voice 1
        case kParameterVoice1Waveform:
            param.name = "V1 Waveform";
            param.symbol = "v1_wave";
            param.ranges.min = 0.0f;
            param.ranges.max = 15.0f;
            param.ranges.def = SID_WAVE_PULSE;
            param.hints |= kParameterIsInteger;
            break;
        case kParameterVoice1PulseWidth:
            param.name = "V1 Pulse Width";
            param.symbol = "v1_pw";
            param.ranges.def = 0.5f;
            break;
        case kParameterVoice1Attack:
            param.name = "V1 Attack";
            param.symbol = "v1_attack";
            param.ranges.def = 0.0f;
            break;
        case kParameterVoice1Decay:
            param.name = "V1 Decay";
            param.symbol = "v1_decay";
            param.ranges.def = 0.5f;
            break;
        case kParameterVoice1Sustain:
            param.name = "V1 Sustain";
            param.symbol = "v1_sustain";
            param.ranges.def = 0.7f;
            break;
        case kParameterVoice1Release:
            param.name = "V1 Release";
            param.symbol = "v1_release";
            param.ranges.def = 0.3f;
            break;
        case kParameterVoice1RingMod:
            param.name = "V1 Ring Mod";
            param.symbol = "v1_ring";
            param.ranges.def = 0.0f;
            param.hints |= kParameterIsBoolean;
            break;
        case kParameterVoice1Sync:
            param.name = "V1 Sync";
            param.symbol = "v1_sync";
            param.ranges.def = 0.0f;
            param.hints |= kParameterIsBoolean;
            break;

        // Voice 2
        case kParameterVoice2Waveform:
            param.name = "V2 Waveform";
            param.symbol = "v2_wave";
            param.ranges.min = 0.0f;
            param.ranges.max = 15.0f;
            param.ranges.def = SID_WAVE_PULSE;
            param.hints |= kParameterIsInteger;
            break;
        case kParameterVoice2PulseWidth:
            param.name = "V2 Pulse Width";
            param.symbol = "v2_pw";
            param.ranges.def = 0.5f;
            break;
        case kParameterVoice2Attack:
            param.name = "V2 Attack";
            param.symbol = "v2_attack";
            param.ranges.def = 0.0f;
            break;
        case kParameterVoice2Decay:
            param.name = "V2 Decay";
            param.symbol = "v2_decay";
            param.ranges.def = 0.5f;
            break;
        case kParameterVoice2Sustain:
            param.name = "V2 Sustain";
            param.symbol = "v2_sustain";
            param.ranges.def = 0.7f;
            break;
        case kParameterVoice2Release:
            param.name = "V2 Release";
            param.symbol = "v2_release";
            param.ranges.def = 0.3f;
            break;
        case kParameterVoice2RingMod:
            param.name = "V2 Ring Mod";
            param.symbol = "v2_ring";
            param.ranges.def = 0.0f;
            param.hints |= kParameterIsBoolean;
            break;
        case kParameterVoice2Sync:
            param.name = "V2 Sync";
            param.symbol = "v2_sync";
            param.ranges.def = 0.0f;
            param.hints |= kParameterIsBoolean;
            break;

        // Voice 3
        case kParameterVoice3Waveform:
            param.name = "V3 Waveform";
            param.symbol = "v3_wave";
            param.ranges.min = 0.0f;
            param.ranges.max = 15.0f;
            param.ranges.def = SID_WAVE_PULSE;
            param.hints |= kParameterIsInteger;
            break;
        case kParameterVoice3PulseWidth:
            param.name = "V3 Pulse Width";
            param.symbol = "v3_pw";
            param.ranges.def = 0.5f;
            break;
        case kParameterVoice3Attack:
            param.name = "V3 Attack";
            param.symbol = "v3_attack";
            param.ranges.def = 0.0f;
            break;
        case kParameterVoice3Decay:
            param.name = "V3 Decay";
            param.symbol = "v3_decay";
            param.ranges.def = 0.5f;
            break;
        case kParameterVoice3Sustain:
            param.name = "V3 Sustain";
            param.symbol = "v3_sustain";
            param.ranges.def = 0.7f;
            break;
        case kParameterVoice3Release:
            param.name = "V3 Release";
            param.symbol = "v3_release";
            param.ranges.def = 0.3f;
            break;
        case kParameterVoice3RingMod:
            param.name = "V3 Ring Mod";
            param.symbol = "v3_ring";
            param.ranges.def = 0.0f;
            param.hints |= kParameterIsBoolean;
            break;
        case kParameterVoice3Sync:
            param.name = "V3 Sync";
            param.symbol = "v3_sync";
            param.ranges.def = 0.0f;
            param.hints |= kParameterIsBoolean;
            break;

        // Filter
        case kParameterFilterMode:
            param.name = "Filter Mode";
            param.symbol = "flt_mode";
            param.ranges.min = 0.0f;
            param.ranges.max = 3.0f;
            param.ranges.def = SID_FILTER_LP;
            param.hints |= kParameterIsInteger;
            break;
        case kParameterFilterCutoff:
            param.name = "Filter Cutoff";
            param.symbol = "flt_cutoff";
            param.ranges.def = 0.5f;
            break;
        case kParameterFilterResonance:
            param.name = "Filter Resonance";
            param.symbol = "flt_res";
            param.ranges.def = 0.0f;
            break;
        case kParameterFilterVoice1:
            param.name = "Filter Voice 1";
            param.symbol = "flt_v1";
            param.ranges.def = 0.0f;
            param.hints |= kParameterIsBoolean;
            break;
        case kParameterFilterVoice2:
            param.name = "Filter Voice 2";
            param.symbol = "flt_v2";
            param.ranges.def = 0.0f;
            param.hints |= kParameterIsBoolean;
            break;
        case kParameterFilterVoice3:
            param.name = "Filter Voice 3";
            param.symbol = "flt_v3";
            param.ranges.def = 0.0f;
            param.hints |= kParameterIsBoolean;
            break;

        // Global
        case kParameterVolume:
            param.name = "Volume";
            param.symbol = "volume";
            param.ranges.def = 0.7f;
            break;
        }
    }

    float getParameterValue(uint32_t index) const override
    {
        switch (index) {
        // Voice 1
        case kParameterVoice1Waveform: return fVoice1Waveform;
        case kParameterVoice1PulseWidth: return fVoice1PulseWidth;
        case kParameterVoice1Attack: return fVoice1Attack;
        case kParameterVoice1Decay: return fVoice1Decay;
        case kParameterVoice1Sustain: return fVoice1Sustain;
        case kParameterVoice1Release: return fVoice1Release;
        case kParameterVoice1RingMod: return fVoice1RingMod;
        case kParameterVoice1Sync: return fVoice1Sync;

        // Voice 2
        case kParameterVoice2Waveform: return fVoice2Waveform;
        case kParameterVoice2PulseWidth: return fVoice2PulseWidth;
        case kParameterVoice2Attack: return fVoice2Attack;
        case kParameterVoice2Decay: return fVoice2Decay;
        case kParameterVoice2Sustain: return fVoice2Sustain;
        case kParameterVoice2Release: return fVoice2Release;
        case kParameterVoice2RingMod: return fVoice2RingMod;
        case kParameterVoice2Sync: return fVoice2Sync;

        // Voice 3
        case kParameterVoice3Waveform: return fVoice3Waveform;
        case kParameterVoice3PulseWidth: return fVoice3PulseWidth;
        case kParameterVoice3Attack: return fVoice3Attack;
        case kParameterVoice3Decay: return fVoice3Decay;
        case kParameterVoice3Sustain: return fVoice3Sustain;
        case kParameterVoice3Release: return fVoice3Release;
        case kParameterVoice3RingMod: return fVoice3RingMod;
        case kParameterVoice3Sync: return fVoice3Sync;

        // Filter
        case kParameterFilterMode: return fFilterMode;
        case kParameterFilterCutoff: return fFilterCutoff;
        case kParameterFilterResonance: return fFilterResonance;
        case kParameterFilterVoice1: return fFilterVoice1;
        case kParameterFilterVoice2: return fFilterVoice2;
        case kParameterFilterVoice3: return fFilterVoice3;

        // Global
        case kParameterVolume: return fVolume;

        default: return 0.0f;
        }
    }

    void setParameterValue(uint32_t index, float value) override
    {
        if (!fSID) return;

        switch (index) {
        // Voice 1
        case kParameterVoice1Waveform:
            fVoice1Waveform = value;
            synth_sid_set_waveform(fSID, 0, (uint8_t)value);
            if (!fUpdatingFromCC) sendParameterAsCC(index, value);
            break;
        case kParameterVoice1PulseWidth:
            fVoice1PulseWidth = value;
            synth_sid_set_pulse_width(fSID, 0, value);
            if (!fUpdatingFromCC) sendParameterAsCC(index, value);
            break;
        case kParameterVoice1Attack:
            fVoice1Attack = value;
            synth_sid_set_attack(fSID, 0, value);
            if (!fUpdatingFromCC) sendParameterAsCC(index, value);
            break;
        case kParameterVoice1Decay:
            fVoice1Decay = value;
            synth_sid_set_decay(fSID, 0, value);
            if (!fUpdatingFromCC) sendParameterAsCC(index, value);
            break;
        case kParameterVoice1Sustain:
            fVoice1Sustain = value;
            synth_sid_set_sustain(fSID, 0, value);
            if (!fUpdatingFromCC) sendParameterAsCC(index, value);
            break;
        case kParameterVoice1Release:
            fVoice1Release = value;
            synth_sid_set_release(fSID, 0, value);
            if (!fUpdatingFromCC) sendParameterAsCC(index, value);
            break;
        case kParameterVoice1RingMod:
            fVoice1RingMod = value;
            synth_sid_set_ring_mod(fSID, 0, value > 0.5f);
            if (!fUpdatingFromCC) sendParameterAsCC(index, value);
            break;
        case kParameterVoice1Sync:
            fVoice1Sync = value;
            synth_sid_set_sync(fSID, 0, value > 0.5f);
            if (!fUpdatingFromCC) sendParameterAsCC(index, value);
            break;

        // Voice 2
        case kParameterVoice2Waveform:
            fVoice2Waveform = value;
            synth_sid_set_waveform(fSID, 1, (uint8_t)value);
            if (!fUpdatingFromCC) sendParameterAsCC(index, value);
            break;
        case kParameterVoice2PulseWidth:
            fVoice2PulseWidth = value;
            synth_sid_set_pulse_width(fSID, 1, value);
            if (!fUpdatingFromCC) sendParameterAsCC(index, value);
            break;
        case kParameterVoice2Attack:
            fVoice2Attack = value;
            synth_sid_set_attack(fSID, 1, value);
            if (!fUpdatingFromCC) sendParameterAsCC(index, value);
            break;
        case kParameterVoice2Decay:
            fVoice2Decay = value;
            synth_sid_set_decay(fSID, 1, value);
            if (!fUpdatingFromCC) sendParameterAsCC(index, value);
            break;
        case kParameterVoice2Sustain:
            fVoice2Sustain = value;
            synth_sid_set_sustain(fSID, 1, value);
            if (!fUpdatingFromCC) sendParameterAsCC(index, value);
            break;
        case kParameterVoice2Release:
            fVoice2Release = value;
            synth_sid_set_release(fSID, 1, value);
            if (!fUpdatingFromCC) sendParameterAsCC(index, value);
            break;
        case kParameterVoice2RingMod:
            fVoice2RingMod = value;
            synth_sid_set_ring_mod(fSID, 1, value > 0.5f);
            if (!fUpdatingFromCC) sendParameterAsCC(index, value);
            break;
        case kParameterVoice2Sync:
            fVoice2Sync = value;
            synth_sid_set_sync(fSID, 1, value > 0.5f);
            if (!fUpdatingFromCC) sendParameterAsCC(index, value);
            break;

        // Voice 3
        case kParameterVoice3Waveform:
            fVoice3Waveform = value;
            synth_sid_set_waveform(fSID, 2, (uint8_t)value);
            if (!fUpdatingFromCC) sendParameterAsCC(index, value);
            break;
        case kParameterVoice3PulseWidth:
            fVoice3PulseWidth = value;
            synth_sid_set_pulse_width(fSID, 2, value);
            if (!fUpdatingFromCC) sendParameterAsCC(index, value);
            break;
        case kParameterVoice3Attack:
            fVoice3Attack = value;
            synth_sid_set_attack(fSID, 2, value);
            if (!fUpdatingFromCC) sendParameterAsCC(index, value);
            break;
        case kParameterVoice3Decay:
            fVoice3Decay = value;
            synth_sid_set_decay(fSID, 2, value);
            if (!fUpdatingFromCC) sendParameterAsCC(index, value);
            break;
        case kParameterVoice3Sustain:
            fVoice3Sustain = value;
            synth_sid_set_sustain(fSID, 2, value);
            if (!fUpdatingFromCC) sendParameterAsCC(index, value);
            break;
        case kParameterVoice3Release:
            fVoice3Release = value;
            synth_sid_set_release(fSID, 2, value);
            if (!fUpdatingFromCC) sendParameterAsCC(index, value);
            break;
        case kParameterVoice3RingMod:
            fVoice3RingMod = value;
            synth_sid_set_ring_mod(fSID, 2, value > 0.5f);
            if (!fUpdatingFromCC) sendParameterAsCC(index, value);
            break;
        case kParameterVoice3Sync:
            fVoice3Sync = value;
            synth_sid_set_sync(fSID, 2, value > 0.5f);
            if (!fUpdatingFromCC) sendParameterAsCC(index, value);
            break;

        // Filter
        case kParameterFilterMode:
            fFilterMode = value;
            synth_sid_set_filter_mode(fSID, (SIDFilterMode)(int)value);
            if (!fUpdatingFromCC) sendParameterAsCC(index, value);
            break;
        case kParameterFilterCutoff:
            fFilterCutoff = value;
            synth_sid_set_filter_cutoff(fSID, value);
            if (!fUpdatingFromCC) sendParameterAsCC(index, value);
            break;
        case kParameterFilterResonance:
            fFilterResonance = value;
            synth_sid_set_filter_resonance(fSID, value);
            if (!fUpdatingFromCC) sendParameterAsCC(index, value);
            break;
        case kParameterFilterVoice1:
            fFilterVoice1 = value;
            synth_sid_set_filter_voice(fSID, 0, value > 0.5f);
            if (!fUpdatingFromCC) sendParameterAsCC(index, value);
            break;
        case kParameterFilterVoice2:
            fFilterVoice2 = value;
            synth_sid_set_filter_voice(fSID, 1, value > 0.5f);
            if (!fUpdatingFromCC) sendParameterAsCC(index, value);
            break;
        case kParameterFilterVoice3:
            fFilterVoice3 = value;
            synth_sid_set_filter_voice(fSID, 2, value > 0.5f);
            if (!fUpdatingFromCC) sendParameterAsCC(index, value);
            break;

        // Global
        case kParameterVolume:
            fVolume = value;
            synth_sid_set_volume(fSID, value);
            if (!fUpdatingFromCC) sendParameterAsCC(index, value);
            break;
        }
    }

    void run(const float**, float** outputs, uint32_t frames,
             const MidiEvent* midiEvents, uint32_t midiEventCount) override
    {
        if (!fSID)
            return;

        float* outL = outputs[0];
        float* outR = outputs[1];

        // Process MIDI events
        for (uint32_t i = 0; i < midiEventCount; i++) {
            const MidiEvent& event = midiEvents[i];

            if (event.size > 3)
                continue;

            const uint8_t* data = event.data;
            const uint8_t status = data[0] & 0xF0;
            const uint8_t channel = data[0] & 0x0F;

            // Route MIDI channels to voices
            // Channel 0 (MIDI 1) → Voice 0
            // Channel 1 (MIDI 2) → Voice 1
            // Channel 2 (MIDI 3) → Voice 2
            // Channels 9-15 → Voice 0 (omni fallback)
            uint8_t voice = 0;
            if (channel < 3) {
                voice = channel;
            }

            switch (status) {
                case 0x90: // Note On
                    if (data[2] > 0) {
                        synth_sid_note_on(fSID, voice, data[1], data[2]);
                    } else {
                        synth_sid_note_off(fSID, voice);
                    }
                    break;

                case 0x80: // Note Off
                    synth_sid_note_off(fSID, voice);
                    break;

                case 0xB0: // Control Change
                    if (event.size >= 3) {
                        // Update parameter from CC (this will sync UI)
                        updateParameterFromCC(data[1], data[2]);
                        // Also send to synth (in case CC doesn't map to a parameter)
                        synth_sid_handle_cc(fSID, data[1], data[2]);
                    }
                    break;

                case 0xE0: // Pitch Bend
                    if (event.size >= 3) {
                        uint16_t bend_value = data[1] | (data[2] << 7);
                        synth_sid_handle_pitch_bend_midi(fSID, voice, bend_value);
                    }
                    break;
            }
        }

        // Process audio (interleaved buffer)
        float interleavedBuffer[frames * 2];
        synth_sid_process_f32(fSID, interleavedBuffer, frames, getSampleRate());

        // De-interleave to separate L/R outputs
        for (uint32_t i = 0; i < frames; i++) {
            outL[i] = interleavedBuffer[i * 2 + 0];
            outR[i] = interleavedBuffer[i * 2 + 1];
        }
    }

    void sampleRateChanged(double newSampleRate) override
    {
        // Recreate SID with new sample rate
        if (fSID) {
            synth_sid_destroy(fSID);
        }
        fSID = synth_sid_create(newSampleRate);

        // Reapply parameters
        if (fSID) {
            applyParametersToSID();
        }
    }

private:
    /**
     * Map parameter index to MIDI CC number
     * Returns 0xFF if parameter has no CC mapping
     */
    uint8_t parameterToCC(uint32_t param) const
    {
        switch (param) {
        // Voice 1
        case kParameterVoice1Waveform:   return SID_CC_VOICE1_WAVEFORM;
        case kParameterVoice1PulseWidth: return SID_CC_VOICE1_PULSEWIDTH;
        case kParameterVoice1Attack:     return SID_CC_VOICE1_ATTACK;
        case kParameterVoice1Decay:      return SID_CC_VOICE1_DECAY;
        case kParameterVoice1Sustain:    return SID_CC_VOICE1_SUSTAIN;
        case kParameterVoice1Release:    return SID_CC_VOICE1_RELEASE;
        case kParameterVoice1RingMod:    return SID_CC_VOICE1_RING_MOD;
        case kParameterVoice1Sync:       return SID_CC_VOICE1_SYNC;

        // Voice 2
        case kParameterVoice2Waveform:   return SID_CC_VOICE2_WAVEFORM;
        case kParameterVoice2PulseWidth: return SID_CC_VOICE2_PULSEWIDTH;
        case kParameterVoice2Attack:     return SID_CC_VOICE2_ATTACK;
        case kParameterVoice2Decay:      return SID_CC_VOICE2_DECAY;
        case kParameterVoice2Sustain:    return SID_CC_VOICE2_SUSTAIN;
        case kParameterVoice2Release:    return SID_CC_VOICE2_RELEASE;
        case kParameterVoice2RingMod:    return SID_CC_VOICE2_RING_MOD;
        case kParameterVoice2Sync:       return SID_CC_VOICE2_SYNC;

        // Voice 3
        case kParameterVoice3Waveform:   return SID_CC_VOICE3_WAVEFORM;
        case kParameterVoice3PulseWidth: return SID_CC_VOICE3_PULSEWIDTH;
        case kParameterVoice3Attack:     return SID_CC_VOICE3_ATTACK;
        case kParameterVoice3Decay:      return SID_CC_VOICE3_DECAY;
        case kParameterVoice3Sustain:    return SID_CC_VOICE3_SUSTAIN;
        case kParameterVoice3Release:    return SID_CC_VOICE3_RELEASE;
        case kParameterVoice3RingMod:    return SID_CC_VOICE3_RING_MOD;
        case kParameterVoice3Sync:       return SID_CC_VOICE3_SYNC;

        // Filter
        case kParameterFilterMode:       return SID_CC_FILTER_MODE;
        case kParameterFilterCutoff:     return SID_CC_FILTER_CUTOFF;
        case kParameterFilterResonance:  return SID_CC_FILTER_RESONANCE;
        case kParameterFilterVoice1:     return SID_CC_FILTER_VOICE1;
        case kParameterFilterVoice2:     return SID_CC_FILTER_VOICE2;
        case kParameterFilterVoice3:     return SID_CC_FILTER_VOICE3;

        // Global
        case kParameterVolume:           return SID_CC_VOLUME;

        default: return 0xFF;  // No mapping
        }
    }

    /**
     * Convert parameter value (0.0-1.0 or discrete) to MIDI CC value (0-127)
     */
    uint8_t parameterValueToCC(uint32_t param, float value) const
    {
        switch (param) {
        // Waveform: 0-15 → 0-15 (discrete values)
        case kParameterVoice1Waveform:
        case kParameterVoice2Waveform:
        case kParameterVoice3Waveform:
            return (uint8_t)value;

        // Filter mode: 0-3 → 0-3 (discrete values)
        case kParameterFilterMode:
            return (uint8_t)value;

        // Boolean parameters: 0/1 → 0/127
        case kParameterVoice1RingMod:
        case kParameterVoice1Sync:
        case kParameterVoice2RingMod:
        case kParameterVoice2Sync:
        case kParameterVoice3RingMod:
        case kParameterVoice3Sync:
        case kParameterFilterVoice1:
        case kParameterFilterVoice2:
        case kParameterFilterVoice3:
            return (value > 0.5f) ? 127 : 0;

        // Continuous parameters: 0.0-1.0 → 0-127
        default:
            return (uint8_t)(value * 127.0f);
        }
    }

    /**
     * Send parameter change as MIDI CC
     */
    void sendParameterAsCC(uint32_t param, float value)
    {
        uint8_t cc = parameterToCC(param);
        if (cc == 0xFF)
            return;  // No CC mapping

        uint8_t ccValue = parameterValueToCC(param, value);

        // Send CC on MIDI channel 1 (0x00)
        MidiEvent midiEvent;
        midiEvent.frame = 0;
        midiEvent.size = 3;
        midiEvent.data[0] = 0xB0;  // CC on channel 1
        midiEvent.data[1] = cc;
        midiEvent.data[2] = ccValue;

        writeMidiEvent(midiEvent);
    }

    /**
     * Update parameter from incoming MIDI CC
     * Returns true if parameter was updated
     */
    bool updateParameterFromCC(uint8_t cc, uint8_t value)
    {
        uint32_t param = 0xFFFFFFFF;
        float paramValue = 0.0f;

        // Map CC to parameter
        switch (cc) {
        // Voice 1
        case SID_CC_VOICE1_WAVEFORM:
            param = kParameterVoice1Waveform;
            paramValue = (float)value;  // 0-15
            break;
        case SID_CC_VOICE1_PULSEWIDTH:
            param = kParameterVoice1PulseWidth;
            paramValue = value / 127.0f;
            break;
        case SID_CC_VOICE1_ATTACK:
            param = kParameterVoice1Attack;
            paramValue = value / 127.0f;
            break;
        case SID_CC_VOICE1_DECAY:
            param = kParameterVoice1Decay;
            paramValue = value / 127.0f;
            break;
        case SID_CC_VOICE1_SUSTAIN:
            param = kParameterVoice1Sustain;
            paramValue = value / 127.0f;
            break;
        case SID_CC_VOICE1_RELEASE:
            param = kParameterVoice1Release;
            paramValue = value / 127.0f;
            break;
        case SID_CC_VOICE1_RING_MOD:
            param = kParameterVoice1RingMod;
            paramValue = (value >= 64) ? 1.0f : 0.0f;
            break;
        case SID_CC_VOICE1_SYNC:
            param = kParameterVoice1Sync;
            paramValue = (value >= 64) ? 1.0f : 0.0f;
            break;

        // Voice 2
        case SID_CC_VOICE2_WAVEFORM:
            param = kParameterVoice2Waveform;
            paramValue = (float)value;
            break;
        case SID_CC_VOICE2_PULSEWIDTH:
            param = kParameterVoice2PulseWidth;
            paramValue = value / 127.0f;
            break;
        case SID_CC_VOICE2_ATTACK:
            param = kParameterVoice2Attack;
            paramValue = value / 127.0f;
            break;
        case SID_CC_VOICE2_DECAY:
            param = kParameterVoice2Decay;
            paramValue = value / 127.0f;
            break;
        case SID_CC_VOICE2_SUSTAIN:
            param = kParameterVoice2Sustain;
            paramValue = value / 127.0f;
            break;
        case SID_CC_VOICE2_RELEASE:
            param = kParameterVoice2Release;
            paramValue = value / 127.0f;
            break;
        case SID_CC_VOICE2_RING_MOD:
            param = kParameterVoice2RingMod;
            paramValue = (value >= 64) ? 1.0f : 0.0f;
            break;
        case SID_CC_VOICE2_SYNC:
            param = kParameterVoice2Sync;
            paramValue = (value >= 64) ? 1.0f : 0.0f;
            break;

        // Voice 3
        case SID_CC_VOICE3_WAVEFORM:
            param = kParameterVoice3Waveform;
            paramValue = (float)value;
            break;
        case SID_CC_VOICE3_PULSEWIDTH:
            param = kParameterVoice3PulseWidth;
            paramValue = value / 127.0f;
            break;
        case SID_CC_VOICE3_ATTACK:
            param = kParameterVoice3Attack;
            paramValue = value / 127.0f;
            break;
        case SID_CC_VOICE3_DECAY:
            param = kParameterVoice3Decay;
            paramValue = value / 127.0f;
            break;
        case SID_CC_VOICE3_SUSTAIN:
            param = kParameterVoice3Sustain;
            paramValue = value / 127.0f;
            break;
        case SID_CC_VOICE3_RELEASE:
            param = kParameterVoice3Release;
            paramValue = value / 127.0f;
            break;
        case SID_CC_VOICE3_RING_MOD:
            param = kParameterVoice3RingMod;
            paramValue = (value >= 64) ? 1.0f : 0.0f;
            break;
        case SID_CC_VOICE3_SYNC:
            param = kParameterVoice3Sync;
            paramValue = (value >= 64) ? 1.0f : 0.0f;
            break;

        // Filter
        case SID_CC_FILTER_MODE:
            param = kParameterFilterMode;
            paramValue = (float)value;  // 0-3
            break;
        case SID_CC_FILTER_CUTOFF:
            param = kParameterFilterCutoff;
            paramValue = value / 127.0f;
            break;
        case SID_CC_FILTER_RESONANCE:
            param = kParameterFilterResonance;
            paramValue = value / 127.0f;
            break;
        case SID_CC_FILTER_VOICE1:
            param = kParameterFilterVoice1;
            paramValue = (value >= 64) ? 1.0f : 0.0f;
            break;
        case SID_CC_FILTER_VOICE2:
            param = kParameterFilterVoice2;
            paramValue = (value >= 64) ? 1.0f : 0.0f;
            break;
        case SID_CC_FILTER_VOICE3:
            param = kParameterFilterVoice3;
            paramValue = (value >= 64) ? 1.0f : 0.0f;
            break;

        // Global
        case SID_CC_VOLUME:
            param = kParameterVolume;
            paramValue = value / 127.0f;
            break;

        default:
            return false;  // CC not mapped to parameter
        }

        // Update parameter (this will trigger setParameterValue)
        // Set flag to prevent feedback loop
        fUpdatingFromCC = true;
        setParameterValue(param, paramValue);
        fUpdatingFromCC = false;

        return true;
    }

    void applyParametersToSID()
    {
        // Voice 1
        synth_sid_set_waveform(fSID, 0, (uint8_t)fVoice1Waveform);
        synth_sid_set_pulse_width(fSID, 0, fVoice1PulseWidth);
        synth_sid_set_attack(fSID, 0, fVoice1Attack);
        synth_sid_set_decay(fSID, 0, fVoice1Decay);
        synth_sid_set_sustain(fSID, 0, fVoice1Sustain);
        synth_sid_set_release(fSID, 0, fVoice1Release);
        synth_sid_set_ring_mod(fSID, 0, fVoice1RingMod > 0.5f);
        synth_sid_set_sync(fSID, 0, fVoice1Sync > 0.5f);

        // Voice 2
        synth_sid_set_waveform(fSID, 1, (uint8_t)fVoice2Waveform);
        synth_sid_set_pulse_width(fSID, 1, fVoice2PulseWidth);
        synth_sid_set_attack(fSID, 1, fVoice2Attack);
        synth_sid_set_decay(fSID, 1, fVoice2Decay);
        synth_sid_set_sustain(fSID, 1, fVoice2Sustain);
        synth_sid_set_release(fSID, 1, fVoice2Release);
        synth_sid_set_ring_mod(fSID, 1, fVoice2RingMod > 0.5f);
        synth_sid_set_sync(fSID, 1, fVoice2Sync > 0.5f);

        // Voice 3
        synth_sid_set_waveform(fSID, 2, (uint8_t)fVoice3Waveform);
        synth_sid_set_pulse_width(fSID, 2, fVoice3PulseWidth);
        synth_sid_set_attack(fSID, 2, fVoice3Attack);
        synth_sid_set_decay(fSID, 2, fVoice3Decay);
        synth_sid_set_sustain(fSID, 2, fVoice3Sustain);
        synth_sid_set_release(fSID, 2, fVoice3Release);
        synth_sid_set_ring_mod(fSID, 2, fVoice3RingMod > 0.5f);
        synth_sid_set_sync(fSID, 2, fVoice3Sync > 0.5f);

        // Filter
        synth_sid_set_filter_mode(fSID, (SIDFilterMode)(int)fFilterMode);
        synth_sid_set_filter_cutoff(fSID, fFilterCutoff);
        synth_sid_set_filter_resonance(fSID, fFilterResonance);
        synth_sid_set_filter_voice(fSID, 0, fFilterVoice1 > 0.5f);
        synth_sid_set_filter_voice(fSID, 1, fFilterVoice2 > 0.5f);
        synth_sid_set_filter_voice(fSID, 2, fFilterVoice3 > 0.5f);

        // Global
        synth_sid_set_volume(fSID, fVolume);
    }

    SynthSID* fSID;

    // Flag to prevent CC feedback loop (parameter→CC→parameter)
    bool fUpdatingFromCC;

    // Parameter storage - Voice 1
    float fVoice1Waveform, fVoice1PulseWidth;
    float fVoice1Attack, fVoice1Decay, fVoice1Sustain, fVoice1Release;
    float fVoice1RingMod, fVoice1Sync;

    // Parameter storage - Voice 2
    float fVoice2Waveform, fVoice2PulseWidth;
    float fVoice2Attack, fVoice2Decay, fVoice2Sustain, fVoice2Release;
    float fVoice2RingMod, fVoice2Sync;

    // Parameter storage - Voice 3
    float fVoice3Waveform, fVoice3PulseWidth;
    float fVoice3Attack, fVoice3Decay, fVoice3Sustain, fVoice3Release;
    float fVoice3RingMod, fVoice3Sync;

    // Parameter storage - Filter
    float fFilterMode, fFilterCutoff, fFilterResonance;
    float fFilterVoice1, fFilterVoice2, fFilterVoice3;

    // Parameter storage - Global
    float fVolume;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RGSID_SynthPlugin)
};

Plugin* createPlugin()
{
    return new RGSID_SynthPlugin();
}

END_NAMESPACE_DISTRHO
