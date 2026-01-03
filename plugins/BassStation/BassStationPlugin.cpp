#include "DistrhoPlugin.hpp"
#include "../../synth/bass_station.h"
#include <cstring>
#include <cmath>

START_NAMESPACE_DISTRHO

class BassStationPlugin : public Plugin
{
public:
    BassStationPlugin()
        : Plugin(kParameterCount, 0, 0)
    {
        // Create the Bass Station synth engine
        fSynth = bass_station_create();

        if (!fSynth) {
            // Handle creation failure - synth will produce silence
            return;
        }

        // Initialize all parameters to their defaults
        for (uint32_t i = 0; i < kParameterCount; i++) {
            Parameter param;
            initParameter(i, param);
            fParams[i] = param.ranges.def;
        }
    }

    ~BassStationPlugin() override
    {
        if (fSynth) {
            bass_station_destroy(fSynth);
        }
    }

protected:
    const char* getLabel() const override { return "BassStation"; }
    const char* getDescription() const override { return BASS_STATION_DESCRIPTION; }
    const char* getMaker() const override { return DISTRHO_PLUGIN_BRAND; }
    const char* getHomePage() const override { return "https://github.com/gbraad/rfx"; }
    const char* getLicense() const override { return "ISC"; }
    uint32_t getVersion() const override { return d_version(1, 0, 0); }
    int64_t getUniqueId() const override { return d_cconst('B', 'S', 'T', 'N'); }

    void initParameter(uint32_t index, Parameter& param) override
    {
        param.hints = kParameterIsAutomatable;
        param.ranges.min = 0.0f;
        param.ranges.max = 1.0f;
        param.ranges.def = 0.5f;

        switch (index) {
        // Oscillator 1
        case kParameterOsc1Waveform:
            param.name = "OSC1 Waveform";
            param.symbol = "osc1_waveform";
            param.hints |= kParameterIsInteger;
            param.ranges.min = 0.0f;
            param.ranges.max = 3.0f;
            param.ranges.def = 1.0f;  // Sawtooth
            break;
        case kParameterOsc1Octave:
            param.name = "OSC1 Octave";
            param.symbol = "osc1_octave";
            param.hints |= kParameterIsInteger;
            param.ranges.min = -2.0f;
            param.ranges.max = 2.0f;
            param.ranges.def = 0.0f;
            break;
        case kParameterOsc1Fine:
            param.name = "OSC1 Fine";
            param.symbol = "osc1_fine";
            param.ranges.min = -12.0f;
            param.ranges.max = 12.0f;
            param.ranges.def = 0.0f;
            param.unit = "semitones";
            break;
        case kParameterOsc1PW:
            param.name = "OSC1 Pulse Width";
            param.symbol = "osc1_pw";
            param.ranges.def = 0.5f;
            break;

        // Oscillator 2
        case kParameterOsc2Waveform:
            param.name = "OSC2 Waveform";
            param.symbol = "osc2_waveform";
            param.hints |= kParameterIsInteger;
            param.ranges.min = 0.0f;
            param.ranges.max = 3.0f;
            param.ranges.def = 1.0f;
            break;
        case kParameterOsc2Octave:
            param.name = "OSC2 Octave";
            param.symbol = "osc2_octave";
            param.hints |= kParameterIsInteger;
            param.ranges.min = -2.0f;
            param.ranges.max = 2.0f;
            param.ranges.def = 0.0f;
            break;
        case kParameterOsc2Fine:
            param.name = "OSC2 Fine";
            param.symbol = "osc2_fine";
            param.ranges.min = -12.0f;
            param.ranges.max = 12.0f;
            param.ranges.def = 0.0f;
            param.unit = "semitones";
            break;
        case kParameterOsc2PW:
            param.name = "OSC2 Pulse Width";
            param.symbol = "osc2_pw";
            param.ranges.def = 0.5f;
            break;

        // Mix & Sync
        case kParameterOscMix:
            param.name = "OSC Mix";
            param.symbol = "osc_mix";
            param.ranges.def = 0.5f;
            break;
        case kParameterOscSync:
            param.name = "OSC Sync";
            param.symbol = "osc_sync";
            param.hints |= kParameterIsBoolean;
            param.ranges.min = 0.0f;
            param.ranges.max = 1.0f;
            param.ranges.def = 0.0f;
            break;

        // Sub-Oscillator
        case kParameterSubMode:
            param.name = "Sub Mode";
            param.symbol = "sub_mode";
            param.hints |= kParameterIsInteger;
            param.ranges.min = 0.0f;
            param.ranges.max = 2.0f;
            param.ranges.def = 1.0f;  // -1 octave
            break;
        case kParameterSubWave:
            param.name = "Sub Wave";
            param.symbol = "sub_wave";
            param.hints |= kParameterIsInteger;
            param.ranges.min = 0.0f;
            param.ranges.max = 2.0f;
            param.ranges.def = 0.0f;  // Square
            break;
        case kParameterSubLevel:
            param.name = "Sub Level";
            param.symbol = "sub_level";
            param.ranges.def = 0.3f;
            break;

        // Filter
        case kParameterFilterMode:
            param.name = "Filter Mode";
            param.symbol = "filter_mode";
            param.hints |= kParameterIsInteger;
            param.ranges.min = 0.0f;
            param.ranges.max = 1.0f;
            param.ranges.def = 0.0f;  // Classic
            break;
        case kParameterFilterType:
            param.name = "Filter Type";
            param.symbol = "filter_type";
            param.hints |= kParameterIsInteger;
            param.ranges.min = 0.0f;
            param.ranges.max = 5.0f;
            param.ranges.def = 1.0f;  // LPF 24dB
            break;
        case kParameterFilterCutoff:
            param.name = "Filter Cutoff";
            param.symbol = "filter_cutoff";
            param.ranges.def = 0.5f;
            break;
        case kParameterFilterResonance:
            param.name = "Filter Resonance";
            param.symbol = "filter_resonance";
            param.ranges.def = 0.3f;
            break;
        case kParameterFilterDrive:
            param.name = "Filter Drive";
            param.symbol = "filter_drive";
            param.ranges.def = 0.0f;
            break;

        // Amp Envelope
        case kParameterAmpAttack:
            param.name = "Amp Attack";
            param.symbol = "amp_attack";
            param.ranges.min = 0.0f;
            param.ranges.max = 5.0f;
            param.ranges.def = 0.01f;
            param.unit = "s";
            break;
        case kParameterAmpDecay:
            param.name = "Amp Decay";
            param.symbol = "amp_decay";
            param.ranges.min = 0.0f;
            param.ranges.max = 5.0f;
            param.ranges.def = 0.3f;
            param.unit = "s";
            break;
        case kParameterAmpSustain:
            param.name = "Amp Sustain";
            param.symbol = "amp_sustain";
            param.ranges.def = 0.7f;
            break;
        case kParameterAmpRelease:
            param.name = "Amp Release";
            param.symbol = "amp_release";
            param.ranges.min = 0.0f;
            param.ranges.max = 5.0f;
            param.ranges.def = 0.5f;
            param.unit = "s";
            break;

        // Mod Envelope
        case kParameterModAttack:
            param.name = "Mod Attack";
            param.symbol = "mod_attack";
            param.ranges.min = 0.0f;
            param.ranges.max = 5.0f;
            param.ranges.def = 0.01f;
            param.unit = "s";
            break;
        case kParameterModDecay:
            param.name = "Mod Decay";
            param.symbol = "mod_decay";
            param.ranges.min = 0.0f;
            param.ranges.max = 5.0f;
            param.ranges.def = 0.5f;
            param.unit = "s";
            break;
        case kParameterModSustain:
            param.name = "Mod Sustain";
            param.symbol = "mod_sustain";
            param.ranges.def = 0.3f;
            break;
        case kParameterModRelease:
            param.name = "Mod Release";
            param.symbol = "mod_release";
            param.ranges.min = 0.0f;
            param.ranges.max = 5.0f;
            param.ranges.def = 0.3f;
            param.unit = "s";
            break;

        // Modulation Amounts
        case kParameterModEnvToFilter:
            param.name = "Mod Env -> Filter";
            param.symbol = "mod_env_to_filter";
            param.ranges.min = -1.0f;
            param.ranges.max = 1.0f;
            param.ranges.def = 0.5f;
            break;
        case kParameterModEnvToPitch:
            param.name = "Mod Env -> Pitch";
            param.symbol = "mod_env_to_pitch";
            param.ranges.min = -1.0f;
            param.ranges.max = 1.0f;
            param.ranges.def = 0.0f;
            break;
        case kParameterModEnvToPW:
            param.name = "Mod Env -> PW";
            param.symbol = "mod_env_to_pw";
            param.ranges.min = -1.0f;
            param.ranges.max = 1.0f;
            param.ranges.def = 0.0f;
            break;

        // LFO 1
        case kParameterLFO1Rate:
            param.name = "LFO1 Rate";
            param.symbol = "lfo1_rate";
            param.ranges.min = 0.1f;
            param.ranges.max = 20.0f;
            param.ranges.def = 5.0f;
            param.unit = "Hz";
            break;
        case kParameterLFO1Waveform:
            param.name = "LFO1 Waveform";
            param.symbol = "lfo1_waveform";
            param.hints |= kParameterIsInteger;
            param.ranges.min = 0.0f;
            param.ranges.max = 5.0f;
            param.ranges.def = 0.0f;  // Sine
            break;
        case kParameterLFO1ToPitch:
            param.name = "LFO1 -> Pitch";
            param.symbol = "lfo1_to_pitch";
            param.ranges.min = -1.0f;
            param.ranges.max = 1.0f;
            param.ranges.def = 0.0f;
            break;

        // LFO 2
        case kParameterLFO2Rate:
            param.name = "LFO2 Rate";
            param.symbol = "lfo2_rate";
            param.ranges.min = 0.1f;
            param.ranges.max = 20.0f;
            param.ranges.def = 3.0f;
            param.unit = "Hz";
            break;
        case kParameterLFO2Waveform:
            param.name = "LFO2 Waveform";
            param.symbol = "lfo2_waveform";
            param.hints |= kParameterIsInteger;
            param.ranges.min = 0.0f;
            param.ranges.max = 5.0f;
            param.ranges.def = 1.0f;  // Triangle
            break;
        case kParameterLFO2ToPW:
            param.name = "LFO2 -> PW";
            param.symbol = "lfo2_to_pw";
            param.ranges.min = -1.0f;
            param.ranges.max = 1.0f;
            param.ranges.def = 0.0f;
            break;
        case kParameterLFO2ToFilter:
            param.name = "LFO2 -> Filter";
            param.symbol = "lfo2_to_filter";
            param.ranges.min = -1.0f;
            param.ranges.max = 1.0f;
            param.ranges.def = 0.0f;
            break;

        // Performance
        case kParameterPortamento:
            param.name = "Portamento";
            param.symbol = "portamento";
            param.ranges.min = 0.0f;
            param.ranges.max = 1.0f;
            param.ranges.def = 0.0f;
            param.unit = "s";
            break;
        case kParameterVolume:
            param.name = "Volume";
            param.symbol = "volume";
            param.ranges.def = 0.7f;
            break;
        case kParameterDistortion:
            param.name = "Distortion";
            param.symbol = "distortion";
            param.ranges.def = 0.0f;
            break;
        }
    }

    float getParameterValue(uint32_t index) const override
    {
        return fParams[index];
    }

    void setParameterValue(uint32_t index, float value) override
    {
        if (!fSynth) return;

        fParams[index] = value;

        switch (index) {
        // Oscillator 1
        case kParameterOsc1Waveform:
            bass_station_set_osc1_waveform(fSynth, value);
            break;
        case kParameterOsc1Octave:
            bass_station_set_osc1_octave(fSynth, (int)value);
            break;
        case kParameterOsc1Fine:
            bass_station_set_osc1_fine(fSynth, value);
            break;
        case kParameterOsc1PW:
            bass_station_set_osc1_pw(fSynth, value);
            break;

        // Oscillator 2
        case kParameterOsc2Waveform:
            bass_station_set_osc2_waveform(fSynth, value);
            break;
        case kParameterOsc2Octave:
            bass_station_set_osc2_octave(fSynth, (int)value);
            break;
        case kParameterOsc2Fine:
            bass_station_set_osc2_fine(fSynth, value);
            break;
        case kParameterOsc2PW:
            bass_station_set_osc2_pw(fSynth, value);
            break;

        // Mix & Sync
        case kParameterOscMix:
            bass_station_set_osc_mix(fSynth, value);
            break;
        case kParameterOscSync:
            bass_station_set_osc_sync(fSynth, value > 0.5f ? 1 : 0);
            break;

        // Sub-Oscillator
        case kParameterSubMode:
            bass_station_set_sub_mode(fSynth, (BassStationSubMode)(int)value);
            break;
        case kParameterSubWave:
            bass_station_set_sub_wave(fSynth, (BassStationSubWave)(int)value);
            break;
        case kParameterSubLevel:
            bass_station_set_sub_level(fSynth, value);
            break;

        // Filter
        case kParameterFilterMode:
            bass_station_set_filter_mode(fSynth, (BassStationFilterMode)(int)value);
            break;
        case kParameterFilterType:
            bass_station_set_filter_type(fSynth, (BassStationFilterType)(int)value);
            break;
        case kParameterFilterCutoff:
            bass_station_set_filter_cutoff(fSynth, value);
            break;
        case kParameterFilterResonance:
            bass_station_set_filter_resonance(fSynth, value);
            break;
        case kParameterFilterDrive:
            bass_station_set_filter_drive(fSynth, value);
            break;

        // Amp Envelope
        case kParameterAmpAttack:
            bass_station_set_amp_attack(fSynth, value);
            break;
        case kParameterAmpDecay:
            bass_station_set_amp_decay(fSynth, value);
            break;
        case kParameterAmpSustain:
            bass_station_set_amp_sustain(fSynth, value);
            break;
        case kParameterAmpRelease:
            bass_station_set_amp_release(fSynth, value);
            break;

        // Mod Envelope
        case kParameterModAttack:
            bass_station_set_mod_attack(fSynth, value);
            break;
        case kParameterModDecay:
            bass_station_set_mod_decay(fSynth, value);
            break;
        case kParameterModSustain:
            bass_station_set_mod_sustain(fSynth, value);
            break;
        case kParameterModRelease:
            bass_station_set_mod_release(fSynth, value);
            break;

        // Modulation Amounts
        case kParameterModEnvToFilter:
            bass_station_set_mod_env_to_filter(fSynth, value);
            break;
        case kParameterModEnvToPitch:
            bass_station_set_mod_env_to_pitch(fSynth, value);
            break;
        case kParameterModEnvToPW:
            bass_station_set_mod_env_to_pw(fSynth, value);
            break;

        // LFO 1
        case kParameterLFO1Rate:
            bass_station_set_lfo1_rate(fSynth, value);
            break;
        case kParameterLFO1Waveform:
            bass_station_set_lfo1_waveform(fSynth, value);
            break;
        case kParameterLFO1ToPitch:
            bass_station_set_lfo1_to_pitch(fSynth, value);
            break;

        // LFO 2
        case kParameterLFO2Rate:
            bass_station_set_lfo2_rate(fSynth, value);
            break;
        case kParameterLFO2Waveform:
            bass_station_set_lfo2_waveform(fSynth, value);
            break;
        case kParameterLFO2ToPW:
            bass_station_set_lfo2_to_pw(fSynth, value);
            break;
        case kParameterLFO2ToFilter:
            bass_station_set_lfo2_to_filter(fSynth, value);
            break;

        // Performance
        case kParameterPortamento:
            bass_station_set_portamento(fSynth, value);
            break;
        case kParameterVolume:
            bass_station_set_volume(fSynth, value);
            break;
        case kParameterDistortion:
            bass_station_set_distortion(fSynth, value);
            break;
        }
    }

    void activate() override
    {
        if (fSynth) {
            bass_station_reset(fSynth);
        }
    }

    void run(const float**, float** outputs, uint32_t frames,
             const MidiEvent* midiEvents, uint32_t midiEventCount) override
    {
        float* outL = outputs[0];
        float* outR = outputs[1];

        // Clear output buffers
        std::memset(outL, 0, sizeof(float) * frames);
        std::memset(outR, 0, sizeof(float) * frames);

        if (!fSynth) return;

        int sampleRate = (int)getSampleRate();
        if (sampleRate <= 0) sampleRate = 48000;

        uint32_t framePos = 0;
        uint32_t midiEventIndex = 0;

        while (framePos < frames) {
            // Process MIDI events at their frame position
            while (midiEventIndex < midiEventCount &&
                   midiEvents[midiEventIndex].frame == framePos) {
                const MidiEvent& event = midiEvents[midiEventIndex++];

                if (event.size < 1) continue;

                const uint8_t status = event.data[0] & 0xF0;

                if (status == 0x90 && event.size >= 3) {
                    // Note On
                    const uint8_t note = event.data[1];
                    const uint8_t velocity = event.data[2];

                    if (velocity > 0) {
                        bass_station_note_on(fSynth, note, velocity);
                    } else {
                        bass_station_note_off(fSynth, note);
                    }
                }
                else if (status == 0x80 && event.size >= 3) {
                    // Note Off
                    const uint8_t note = event.data[1];
                    bass_station_note_off(fSynth, note);
                }
            }

            // Render audio for this sample
            float sample = bass_station_process(fSynth, sampleRate);

            outL[framePos] = sample;
            outR[framePos] = sample;

            framePos++;
        }
    }

private:
    BassStation* fSynth;
    float fParams[kParameterCount];

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BassStationPlugin)
};

Plugin* createPlugin()
{
    return new BassStationPlugin();
}

END_NAMESPACE_DISTRHO
