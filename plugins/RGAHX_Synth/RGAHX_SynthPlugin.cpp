/*
 * RGAHX Synth Plugin Implementation
 */

#include "DistrhoPlugin.hpp"

extern "C" {
#include "../../synth/ahx_instrument.h"
#include "../../synth/synth_midi.h"
}

#include <cstring>
#include <cmath>

START_NAMESPACE_DISTRHO

#define MAX_VOICES 8

struct AhxVoice {
    AhxInstrument inst;
};

class RGAHX_SynthPlugin : public Plugin
{
public:
    RGAHX_SynthPlugin()
        : Plugin(kParameterCount, 0, 0)
        , fMidi(nullptr)
        , fMasterVolume(0.7f)
    {
        // Create MIDI handler with polyphonic voice allocation
        fMidi = synth_midi_create(MAX_VOICES, VOICE_ALLOC_POLYPHONIC);

        // Initialize voices with default parameters
        for (int i = 0; i < MAX_VOICES; i++) {
            ahx_instrument_init(&fVoices[i].inst);
        }

        // Initialize parameters to defaults
        loadDefaults();

        // Apply initial parameters to all voices
        updateAllVoices();
    }

    ~RGAHX_SynthPlugin() override
    {
        if (fMidi) {
            synth_midi_destroy(fMidi);
        }
    }

protected:
    const char* getLabel() const override { return RGAHX_DISPLAY_NAME; }
    const char* getDescription() const override { return RGAHX_DESCRIPTION; }
    const char* getMaker() const override { return "Regroove"; }
    const char* getHomePage() const override { return "https://music.gbraad.nl/regrooved/"; }
    const char* getLicense() const override { return "GPL-3.0"; }
    uint32_t getVersion() const override { return d_version(1, 0, 0); }
    int64_t getUniqueId() const override { return d_cconst('R', 'G', 'A', 'H'); }

    void initParameter(uint32_t index, Parameter& param) override
    {
        param.hints = kParameterIsAutomatable;

        switch (index) {
            // Oscillator Group
            case kParameterWaveform:
                param.name = "Waveform";
                param.symbol = "waveform";
                param.ranges.min = 0.0f;
                param.ranges.max = 3.0f;
                param.ranges.def = 1.0f;  // Sawtooth
                param.hints |= kParameterIsInteger;
                param.groupId = GROUP_OSCILLATOR;
                break;

            case kParameterWaveLength:
                param.name = "Wave Length";
                param.symbol = "wave_length";
                param.ranges.min = 0.0f;
                param.ranges.max = 7.0f;
                param.ranges.def = 3.0f;
                param.hints |= kParameterIsInteger;
                param.groupId = GROUP_OSCILLATOR;
                break;

            case kParameterOscVolume:
                param.name = "Osc Volume";
                param.symbol = "osc_volume";
                param.ranges.min = 0.0f;
                param.ranges.max = 64.0f;
                param.ranges.def = 64.0f;
                param.hints |= kParameterIsInteger;
                param.groupId = GROUP_OSCILLATOR;
                break;

            // Envelope Group
            case kParameterAttackFrames:
                param.name = "Attack Frames";
                param.symbol = "attack_frames";
                param.ranges.min = 0.0f;
                param.ranges.max = 255.0f;
                param.ranges.def = 1.0f;
                param.hints |= kParameterIsInteger;
                param.groupId = GROUP_ENVELOPE;
                break;

            case kParameterAttackVolume:
                param.name = "Attack Volume";
                param.symbol = "attack_volume";
                param.ranges.min = 0.0f;
                param.ranges.max = 64.0f;
                param.ranges.def = 64.0f;
                param.hints |= kParameterIsInteger;
                param.groupId = GROUP_ENVELOPE;
                break;

            case kParameterDecayFrames:
                param.name = "Decay Frames";
                param.symbol = "decay_frames";
                param.ranges.min = 0.0f;
                param.ranges.max = 255.0f;
                param.ranges.def = 10.0f;
                param.hints |= kParameterIsInteger;
                param.groupId = GROUP_ENVELOPE;
                break;

            case kParameterDecayVolume:
                param.name = "Decay Volume";
                param.symbol = "decay_volume";
                param.ranges.min = 0.0f;
                param.ranges.max = 64.0f;
                param.ranges.def = 48.0f;
                param.hints |= kParameterIsInteger;
                param.groupId = GROUP_ENVELOPE;
                break;

            case kParameterSustainFrames:
                param.name = "Sustain Frames";
                param.symbol = "sustain_frames";
                param.ranges.min = 0.0f;
                param.ranges.max = 255.0f;
                param.ranges.def = 0.0f;  // Infinite
                param.hints |= kParameterIsInteger;
                param.groupId = GROUP_ENVELOPE;
                break;

            case kParameterReleaseFrames:
                param.name = "Release Frames";
                param.symbol = "release_frames";
                param.ranges.min = 0.0f;
                param.ranges.max = 255.0f;
                param.ranges.def = 20.0f;
                param.hints |= kParameterIsInteger;
                param.groupId = GROUP_ENVELOPE;
                break;

            case kParameterReleaseVolume:
                param.name = "Release Volume";
                param.symbol = "release_volume";
                param.ranges.min = 0.0f;
                param.ranges.max = 64.0f;
                param.ranges.def = 0.0f;
                param.hints |= kParameterIsInteger;
                param.groupId = GROUP_ENVELOPE;
                break;

            // Filter Group
            case kParameterFilterLower:
                param.name = "Filter Lower";
                param.symbol = "filter_lower";
                param.ranges.min = 0.0f;
                param.ranges.max = 63.0f;
                param.ranges.def = 0.0f;
                param.hints |= kParameterIsInteger;
                param.groupId = GROUP_FILTER;
                break;

            case kParameterFilterUpper:
                param.name = "Filter Upper";
                param.symbol = "filter_upper";
                param.ranges.min = 0.0f;
                param.ranges.max = 63.0f;
                param.ranges.def = 63.0f;
                param.hints |= kParameterIsInteger;
                param.groupId = GROUP_FILTER;
                break;

            case kParameterFilterSpeed:
                param.name = "Filter Speed";
                param.symbol = "filter_speed";
                param.ranges.min = 0.0f;
                param.ranges.max = 63.0f;
                param.ranges.def = 4.0f;
                param.hints |= kParameterIsInteger;
                param.groupId = GROUP_FILTER;
                break;

            case kParameterFilterEnable:
                param.name = "Filter Enable";
                param.symbol = "filter_enable";
                param.ranges.min = 0.0f;
                param.ranges.max = 1.0f;
                param.ranges.def = 0.0f;
                param.hints |= kParameterIsBoolean;
                param.groupId = GROUP_FILTER;
                break;

            // PWM / Square Group
            case kParameterSquareLower:
                param.name = "PWM Lower";
                param.symbol = "pwm_lower";
                param.ranges.min = 0.0f;
                param.ranges.max = 255.0f;
                param.ranges.def = 64.0f;
                param.hints |= kParameterIsInteger;
                param.groupId = GROUP_PWM;
                break;

            case kParameterSquareUpper:
                param.name = "PWM Upper";
                param.symbol = "pwm_upper";
                param.ranges.min = 0.0f;
                param.ranges.max = 255.0f;
                param.ranges.def = 192.0f;
                param.hints |= kParameterIsInteger;
                param.groupId = GROUP_PWM;
                break;

            case kParameterSquareSpeed:
                param.name = "PWM Speed";
                param.symbol = "pwm_speed";
                param.ranges.min = 0.0f;
                param.ranges.max = 255.0f;
                param.ranges.def = 4.0f;
                param.hints |= kParameterIsInteger;
                param.groupId = GROUP_PWM;
                break;

            case kParameterSquareEnable:
                param.name = "PWM Enable";
                param.symbol = "pwm_enable";
                param.ranges.min = 0.0f;
                param.ranges.max = 1.0f;
                param.ranges.def = 0.0f;
                param.hints |= kParameterIsBoolean;
                param.groupId = GROUP_PWM;
                break;

            // Vibrato Group
            case kParameterVibratoDelay:
                param.name = "Vibrato Delay";
                param.symbol = "vibrato_delay";
                param.ranges.min = 0.0f;
                param.ranges.max = 255.0f;
                param.ranges.def = 0.0f;
                param.hints |= kParameterIsInteger;
                param.groupId = GROUP_VIBRATO;
                break;

            case kParameterVibratoDepth:
                param.name = "Vibrato Depth";
                param.symbol = "vibrato_depth";
                param.ranges.min = 0.0f;
                param.ranges.max = 15.0f;
                param.ranges.def = 0.0f;
                param.hints |= kParameterIsInteger;
                param.groupId = GROUP_VIBRATO;
                break;

            case kParameterVibratoSpeed:
                param.name = "Vibrato Speed";
                param.symbol = "vibrato_speed";
                param.ranges.min = 0.0f;
                param.ranges.max = 255.0f;
                param.ranges.def = 0.0f;
                param.hints |= kParameterIsInteger;
                param.groupId = GROUP_VIBRATO;
                break;

            // Release Group
            case kParameterHardCutRelease:
                param.name = "Hard Cut Release";
                param.symbol = "hard_cut_release";
                param.ranges.min = 0.0f;
                param.ranges.max = 1.0f;
                param.ranges.def = 0.0f;
                param.hints |= kParameterIsBoolean;
                param.groupId = GROUP_RELEASE;
                break;

            case kParameterHardCutFrames:
                param.name = "Hard Cut Frames";
                param.symbol = "hard_cut_frames";
                param.ranges.min = 0.0f;
                param.ranges.max = 7.0f;
                param.ranges.def = 2.0f;
                param.hints |= kParameterIsInteger;
                param.groupId = GROUP_RELEASE;
                break;

            // Master Group
            case kParameterMasterVolume:
                param.name = "Master Volume";
                param.symbol = "master_volume";
                param.ranges.min = 0.0f;
                param.ranges.max = 1.0f;
                param.ranges.def = 0.7f;
                param.groupId = GROUP_MASTER;
                break;
        }
    }

    float getParameterValue(uint32_t index) const override
    {
        switch (index) {
            case kParameterWaveform: return (float)fParams.waveform;
            case kParameterWaveLength: return (float)fParams.wave_length;
            case kParameterOscVolume: return (float)fParams.volume;
            case kParameterAttackFrames: return (float)fParams.envelope.attack_frames;
            case kParameterAttackVolume: return (float)fParams.envelope.attack_volume;
            case kParameterDecayFrames: return (float)fParams.envelope.decay_frames;
            case kParameterDecayVolume: return (float)fParams.envelope.decay_volume;
            case kParameterSustainFrames: return (float)fParams.envelope.sustain_frames;
            case kParameterReleaseFrames: return (float)fParams.envelope.release_frames;
            case kParameterReleaseVolume: return (float)fParams.envelope.release_volume;
            case kParameterFilterLower: return (float)fParams.filter_lower;
            case kParameterFilterUpper: return (float)fParams.filter_upper;
            case kParameterFilterSpeed: return (float)fParams.filter_speed;
            case kParameterFilterEnable: return fParams.filter_enabled ? 1.0f : 0.0f;
            case kParameterSquareLower: return (float)fParams.square_lower;
            case kParameterSquareUpper: return (float)fParams.square_upper;
            case kParameterSquareSpeed: return (float)fParams.square_speed;
            case kParameterSquareEnable: return fParams.square_enabled ? 1.0f : 0.0f;
            case kParameterVibratoDelay: return (float)fParams.vibrato_delay;
            case kParameterVibratoDepth: return (float)fParams.vibrato_depth;
            case kParameterVibratoSpeed: return (float)fParams.vibrato_speed;
            case kParameterHardCutRelease: return fParams.hard_cut_release ? 1.0f : 0.0f;
            case kParameterHardCutFrames: return (float)fParams.hard_cut_frames;
            case kParameterMasterVolume: return fMasterVolume;
            default: return 0.0f;
        }
    }

    void setParameterValue(uint32_t index, float value) override
    {
        switch (index) {
            case kParameterWaveform:
                fParams.waveform = (AhxWaveform)(int)value;
                updateAllVoices();
                break;
            case kParameterWaveLength:
                fParams.wave_length = (uint8_t)value;
                updateAllVoices();
                break;
            case kParameterOscVolume:
                fParams.volume = (uint8_t)value;
                updateAllVoices();
                break;
            case kParameterAttackFrames:
                fParams.envelope.attack_frames = (uint8_t)value;
                updateAllVoices();
                break;
            case kParameterAttackVolume:
                fParams.envelope.attack_volume = (uint8_t)value;
                updateAllVoices();
                break;
            case kParameterDecayFrames:
                fParams.envelope.decay_frames = (uint8_t)value;
                updateAllVoices();
                break;
            case kParameterDecayVolume:
                fParams.envelope.decay_volume = (uint8_t)value;
                updateAllVoices();
                break;
            case kParameterSustainFrames:
                fParams.envelope.sustain_frames = (uint8_t)value;
                updateAllVoices();
                break;
            case kParameterReleaseFrames:
                fParams.envelope.release_frames = (uint8_t)value;
                updateAllVoices();
                break;
            case kParameterReleaseVolume:
                fParams.envelope.release_volume = (uint8_t)value;
                updateAllVoices();
                break;
            case kParameterFilterLower:
                fParams.filter_lower = (uint8_t)value;
                updateAllVoices();
                break;
            case kParameterFilterUpper:
                fParams.filter_upper = (uint8_t)value;
                updateAllVoices();
                break;
            case kParameterFilterSpeed:
                fParams.filter_speed = (uint8_t)value;
                updateAllVoices();
                break;
            case kParameterFilterEnable:
                fParams.filter_enabled = value >= 0.5f;
                updateAllVoices();
                break;
            case kParameterSquareLower:
                fParams.square_lower = (uint8_t)value;
                updateAllVoices();
                break;
            case kParameterSquareUpper:
                fParams.square_upper = (uint8_t)value;
                updateAllVoices();
                break;
            case kParameterSquareSpeed:
                fParams.square_speed = (uint8_t)value;
                updateAllVoices();
                break;
            case kParameterSquareEnable:
                fParams.square_enabled = value >= 0.5f;
                updateAllVoices();
                break;
            case kParameterVibratoDelay:
                fParams.vibrato_delay = (uint8_t)value;
                updateAllVoices();
                break;
            case kParameterVibratoDepth:
                fParams.vibrato_depth = (uint8_t)value;
                updateAllVoices();
                break;
            case kParameterVibratoSpeed:
                fParams.vibrato_speed = (uint8_t)value;
                updateAllVoices();
                break;
            case kParameterHardCutRelease:
                fParams.hard_cut_release = value >= 0.5f;
                updateAllVoices();
                break;
            case kParameterHardCutFrames:
                fParams.hard_cut_frames = (uint8_t)value;
                updateAllVoices();
                break;
            case kParameterMasterVolume:
                fMasterVolume = value;
                break;
        }
    }

    void run(const float**, float** outputs, uint32_t frames,
             const MidiEvent* midiEvents, uint32_t midiEventCount) override
    {
        float* outL = outputs[0];
        float* outR = outputs[1];
        const uint32_t sampleRate = (uint32_t)getSampleRate();

        if (!fMidi) return;

        // Clear output
        memset(outL, 0, frames * sizeof(float));
        memset(outR, 0, frames * sizeof(float));

        // Process MIDI events
        for (uint32_t i = 0; i < midiEventCount; ++i) {
            const MidiEvent& event = midiEvents[i];

            // Parse MIDI message
            SynthMidiMessage msg;
            if (!synth_midi_parse(event.data, event.size, &msg)) {
                continue;
            }

            switch (msg.type) {
                case MIDI_NOTE_ON:
                    if (msg.velocity > 0) {
                        handleNoteOn(msg.channel, msg.note, msg.velocity, sampleRate);
                    } else {
                        handleNoteOff(msg.channel, msg.note);
                    }
                    break;

                case MIDI_NOTE_OFF:
                    handleNoteOff(msg.channel, msg.note);
                    break;

                case MIDI_CC:
                    if (msg.cc_number == MIDI_CC_ALL_NOTES_OFF ||
                        msg.cc_number == MIDI_CC_ALL_SOUND_OFF) {
                        synth_midi_all_notes_off(fMidi);
                        for (int j = 0; j < MAX_VOICES; j++) {
                            ahx_instrument_note_off(&fVoices[j].inst);
                        }
                    }
                    break;

                default:
                    break;
            }
        }

        // Render all active voices
        float voice_buffer[frames];
        for (int i = 0; i < MAX_VOICES; i++) {
            if (!fMidi->voices[i].active) continue;

            // Process voice
            ahx_instrument_process(&fVoices[i].inst, voice_buffer, frames, sampleRate);

            // Mix to output (mono to stereo)
            for (uint32_t s = 0; s < frames; s++) {
                float sample = voice_buffer[s] * fMasterVolume;
                outL[s] += sample;
                outR[s] += sample;
            }

            // Release voice if instrument stopped
            if (!ahx_instrument_is_active(&fVoices[i].inst)) {
                synth_midi_release_voice(fMidi, i);
            }
        }

        // Apply per-voice scaling (8 voices max)
        float scale = 0.2f;
        for (uint32_t s = 0; s < frames; s++) {
            outL[s] *= scale;
            outR[s] *= scale;
        }
    }

private:
    void loadDefaults()
    {
        fParams = ahx_instrument_default_params();
    }

    void updateAllVoices()
    {
        // Update parameters on all voices
        for (int i = 0; i < MAX_VOICES; i++) {
            ahx_instrument_set_params(&fVoices[i].inst, &fParams);
        }
    }

    void handleNoteOn(uint8_t channel, uint8_t note, uint8_t velocity, uint32_t sample_rate)
    {
        if (!fMidi) return;

        // Allocate voice
        int voice_idx = synth_midi_allocate_voice(fMidi, channel, note, velocity);
        if (voice_idx < 0 || voice_idx >= MAX_VOICES) return;

        // Trigger AHX instrument
        ahx_instrument_note_on(&fVoices[voice_idx].inst, note, velocity, sample_rate);
    }

    void handleNoteOff(uint8_t channel, uint8_t note)
    {
        if (!fMidi) return;

        // Find voices playing this note
        int released_voices[MAX_VOICES];
        int count = synth_midi_find_voices_for_note(fMidi, channel, note, released_voices);

        // Release found voices
        for (int i = 0; i < count; i++) {
            int voice_idx = released_voices[i];
            if (voice_idx >= 0 && voice_idx < MAX_VOICES) {
                ahx_instrument_note_off(&fVoices[voice_idx].inst);
                // Note: don't call synth_midi_release_voice yet - wait for envelope to finish
            }
        }
    }

    AhxVoice fVoices[MAX_VOICES];
    SynthMidiHandler* fMidi;
    AhxInstrumentParams fParams;
    float fMasterVolume;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RGAHX_SynthPlugin)
};

Plugin* createPlugin() { return new RGAHX_SynthPlugin(); }

END_NAMESPACE_DISTRHO
