/*
 * RGResonate1 Synthesizer Plugin - DPF Wrapper
 * Polyphonic subtractive synthesizer
 */

#include "DistrhoPlugin.hpp"
#include "../../synth/synth_resonate1.h"
#include <cstring>
#include <cmath>

START_NAMESPACE_DISTRHO

class RGResonate1_SynthPlugin : public Plugin
{
public:
    RGResonate1_SynthPlugin()
        : Plugin(kParameterCount, 0, kParameterCount)  // parameters, programs, states
    {
        // Create synth instance
        fSynth = synth_resonate1_create(getSampleRate());

        if (!fSynth) {
            d_stderr("Failed to create RGResonate1 synth instance!");
            return;
        }

        // Initialize parameter values from synth defaults
        fParameters[kParameterWaveform] = (float)synth_resonate1_get_waveform(fSynth) / 3.0f;
        fParameters[kParameterFilterCutoff] = synth_resonate1_get_filter_cutoff(fSynth);
        fParameters[kParameterFilterResonance] = synth_resonate1_get_filter_resonance(fSynth);
        fParameters[kParameterAmpAttack] = synth_resonate1_get_amp_attack(fSynth);
        fParameters[kParameterAmpDecay] = synth_resonate1_get_amp_decay(fSynth);
        fParameters[kParameterAmpSustain] = synth_resonate1_get_amp_sustain(fSynth);
        fParameters[kParameterAmpRelease] = synth_resonate1_get_amp_release(fSynth);
        fParameters[kParameterFilterEnvAmount] = synth_resonate1_get_filter_env_amount(fSynth);
        fParameters[kParameterFilterAttack] = synth_resonate1_get_filter_attack(fSynth);
        fParameters[kParameterFilterDecay] = synth_resonate1_get_filter_decay(fSynth);
        fParameters[kParameterFilterSustain] = synth_resonate1_get_filter_sustain(fSynth);
        fParameters[kParameterFilterRelease] = synth_resonate1_get_filter_release(fSynth);
    }

    ~RGResonate1_SynthPlugin() override
    {
        if (fSynth) {
            synth_resonate1_destroy(fSynth);
        }
    }

protected:
    // =========================================================================
    // Information

    const char* getLabel() const override
    {
        return "RGResonate1";
    }

    const char* getDescription() const override
    {
        return RGRESONATE1_DESCRIPTION;
    }

    const char* getMaker() const override
    {
        return "Regroove";
    }

    const char* getHomePage() const override
    {
        return "https://music.gbraad.nl";
    }

    const char* getLicense() const override
    {
        return "ISC";
    }

    uint32_t getVersion() const override
    {
        return d_version(1, 0, 0);
    }

    int64_t getUniqueId() const override
    {
        return d_cconst('R', 'G', 'R', '1');
    }

    // =========================================================================
    // Init

    void initParameter(uint32_t index, Parameter& parameter) override
    {
        parameter.hints = kParameterIsAutomatable;

        switch (index) {
            case kParameterWaveform:
                parameter.name = "Waveform";
                parameter.symbol = "waveform";
                parameter.ranges.def = 0.0f;
                parameter.ranges.min = 0.0f;
                parameter.ranges.max = 3.0f;
                parameter.hints |= kParameterIsInteger;
                parameter.enumValues.count = 4;
                parameter.enumValues.restrictedMode = true;
                {
                    ParameterEnumerationValue* const values = new ParameterEnumerationValue[4];
                    values[0].label = "Saw";
                    values[0].value = 0.0f;
                    values[1].label = "Square";
                    values[1].value = 1.0f;
                    values[2].label = "Triangle";
                    values[2].value = 2.0f;
                    values[3].label = "Sine";
                    values[3].value = 3.0f;
                    parameter.enumValues.values = values;
                }
                break;

            case kParameterFilterCutoff:
                parameter.name = "Filter Cutoff";
                parameter.symbol = "filter_cutoff";
                parameter.unit = "Hz";
                parameter.ranges.def = 0.8f;
                parameter.ranges.min = 0.0f;
                parameter.ranges.max = 1.0f;
                break;

            case kParameterFilterResonance:
                parameter.name = "Filter Resonance";
                parameter.symbol = "filter_resonance";
                parameter.unit = "%";
                parameter.ranges.def = 0.3f;
                parameter.ranges.min = 0.0f;
                parameter.ranges.max = 1.0f;
                break;

            case kParameterAmpAttack:
                parameter.name = "Amp Attack";
                parameter.symbol = "amp_attack";
                parameter.unit = "s";
                parameter.ranges.def = 0.01f;
                parameter.ranges.min = 0.0f;
                parameter.ranges.max = 1.0f;
                break;

            case kParameterAmpDecay:
                parameter.name = "Amp Decay";
                parameter.symbol = "amp_decay";
                parameter.unit = "s";
                parameter.ranges.def = 0.3f;
                parameter.ranges.min = 0.0f;
                parameter.ranges.max = 1.0f;
                break;

            case kParameterAmpSustain:
                parameter.name = "Amp Sustain";
                parameter.symbol = "amp_sustain";
                parameter.unit = "%";
                parameter.ranges.def = 0.7f;
                parameter.ranges.min = 0.0f;
                parameter.ranges.max = 1.0f;
                break;

            case kParameterAmpRelease:
                parameter.name = "Amp Release";
                parameter.symbol = "amp_release";
                parameter.unit = "s";
                parameter.ranges.def = 0.2f;
                parameter.ranges.min = 0.0f;
                parameter.ranges.max = 1.0f;
                break;

            case kParameterFilterEnvAmount:
                parameter.name = "Filter Env Amount";
                parameter.symbol = "filter_env_amount";
                parameter.unit = "%";
                parameter.ranges.def = 0.5f;
                parameter.ranges.min = 0.0f;
                parameter.ranges.max = 1.0f;
                break;

            case kParameterFilterAttack:
                parameter.name = "Filter Attack";
                parameter.symbol = "filter_attack";
                parameter.unit = "s";
                parameter.ranges.def = 0.05f;
                parameter.ranges.min = 0.0f;
                parameter.ranges.max = 1.0f;
                break;

            case kParameterFilterDecay:
                parameter.name = "Filter Decay";
                parameter.symbol = "filter_decay";
                parameter.unit = "s";
                parameter.ranges.def = 0.3f;
                parameter.ranges.min = 0.0f;
                parameter.ranges.max = 1.0f;
                break;

            case kParameterFilterSustain:
                parameter.name = "Filter Sustain";
                parameter.symbol = "filter_sustain";
                parameter.unit = "%";
                parameter.ranges.def = 0.5f;
                parameter.ranges.min = 0.0f;
                parameter.ranges.max = 1.0f;
                break;

            case kParameterFilterRelease:
                parameter.name = "Filter Release";
                parameter.symbol = "filter_release";
                parameter.unit = "s";
                parameter.ranges.def = 0.2f;
                parameter.ranges.min = 0.0f;
                parameter.ranges.max = 1.0f;
                break;
        }
    }

    void initState(uint32_t index, State& state) override
    {
        // Use state for parameter values (for preset save/load)
        state.hints = kStateIsOnlyForDSP;

        switch (index) {
            case kParameterWaveform:
                state.key = "waveform";
                state.defaultValue = "0.0";
                break;
            case kParameterFilterCutoff:
                state.key = "filter_cutoff";
                state.defaultValue = "0.8";
                break;
            case kParameterFilterResonance:
                state.key = "filter_resonance";
                state.defaultValue = "0.3";
                break;
            case kParameterAmpAttack:
                state.key = "amp_attack";
                state.defaultValue = "0.01";
                break;
            case kParameterAmpDecay:
                state.key = "amp_decay";
                state.defaultValue = "0.3";
                break;
            case kParameterAmpSustain:
                state.key = "amp_sustain";
                state.defaultValue = "0.7";
                break;
            case kParameterAmpRelease:
                state.key = "amp_release";
                state.defaultValue = "0.2";
                break;
            case kParameterFilterEnvAmount:
                state.key = "filter_env_amount";
                state.defaultValue = "0.5";
                break;
            case kParameterFilterAttack:
                state.key = "filter_attack";
                state.defaultValue = "0.05";
                break;
            case kParameterFilterDecay:
                state.key = "filter_decay";
                state.defaultValue = "0.3";
                break;
            case kParameterFilterSustain:
                state.key = "filter_sustain";
                state.defaultValue = "0.5";
                break;
            case kParameterFilterRelease:
                state.key = "filter_release";
                state.defaultValue = "0.2";
                break;
        }
    }

    // =========================================================================
    // Internal data

    float getParameterValue(uint32_t index) const override
    {
        if (index < kParameterCount)
            return fParameters[index];
        return 0.0f;
    }

    void setParameterValue(uint32_t index, float value) override
    {
        if (index >= kParameterCount || !fSynth)
            return;

        fParameters[index] = value;

        // Forward to synth engine
        switch (index) {
            case kParameterWaveform: {
                int waveform = (int)(value + 0.5f);  // Round to nearest
                synth_resonate1_set_waveform(fSynth, (Resonate1Waveform)waveform);
                break;
            }
            case kParameterFilterCutoff:
                synth_resonate1_set_filter_cutoff(fSynth, value);
                break;
            case kParameterFilterResonance:
                synth_resonate1_set_filter_resonance(fSynth, value);
                break;
            case kParameterAmpAttack:
                synth_resonate1_set_amp_attack(fSynth, value);
                break;
            case kParameterAmpDecay:
                synth_resonate1_set_amp_decay(fSynth, value);
                break;
            case kParameterAmpSustain:
                synth_resonate1_set_amp_sustain(fSynth, value);
                break;
            case kParameterAmpRelease:
                synth_resonate1_set_amp_release(fSynth, value);
                break;
            case kParameterFilterEnvAmount:
                synth_resonate1_set_filter_env_amount(fSynth, value);
                break;
            case kParameterFilterAttack:
                synth_resonate1_set_filter_attack(fSynth, value);
                break;
            case kParameterFilterDecay:
                synth_resonate1_set_filter_decay(fSynth, value);
                break;
            case kParameterFilterSustain:
                synth_resonate1_set_filter_sustain(fSynth, value);
                break;
            case kParameterFilterRelease:
                synth_resonate1_set_filter_release(fSynth, value);
                break;
        }
    }

    String getState(const char* key) const override
    {
        // Map state key to parameter index
        if (std::strcmp(key, "waveform") == 0)
            return String(fParameters[kParameterWaveform]);
        if (std::strcmp(key, "filter_cutoff") == 0)
            return String(fParameters[kParameterFilterCutoff]);
        if (std::strcmp(key, "filter_resonance") == 0)
            return String(fParameters[kParameterFilterResonance]);
        if (std::strcmp(key, "amp_attack") == 0)
            return String(fParameters[kParameterAmpAttack]);
        if (std::strcmp(key, "amp_decay") == 0)
            return String(fParameters[kParameterAmpDecay]);
        if (std::strcmp(key, "amp_sustain") == 0)
            return String(fParameters[kParameterAmpSustain]);
        if (std::strcmp(key, "amp_release") == 0)
            return String(fParameters[kParameterAmpRelease]);
        if (std::strcmp(key, "filter_env_amount") == 0)
            return String(fParameters[kParameterFilterEnvAmount]);
        if (std::strcmp(key, "filter_attack") == 0)
            return String(fParameters[kParameterFilterAttack]);
        if (std::strcmp(key, "filter_decay") == 0)
            return String(fParameters[kParameterFilterDecay]);
        if (std::strcmp(key, "filter_sustain") == 0)
            return String(fParameters[kParameterFilterSustain]);
        if (std::strcmp(key, "filter_release") == 0)
            return String(fParameters[kParameterFilterRelease]);

        return String();
    }

    void setState(const char* key, const char* value) override
    {
        const float val = std::atof(value);

        // Map state key to parameter index
        if (std::strcmp(key, "waveform") == 0)
            setParameterValue(kParameterWaveform, val);
        else if (std::strcmp(key, "filter_cutoff") == 0)
            setParameterValue(kParameterFilterCutoff, val);
        else if (std::strcmp(key, "filter_resonance") == 0)
            setParameterValue(kParameterFilterResonance, val);
        else if (std::strcmp(key, "amp_attack") == 0)
            setParameterValue(kParameterAmpAttack, val);
        else if (std::strcmp(key, "amp_decay") == 0)
            setParameterValue(kParameterAmpDecay, val);
        else if (std::strcmp(key, "amp_sustain") == 0)
            setParameterValue(kParameterAmpSustain, val);
        else if (std::strcmp(key, "amp_release") == 0)
            setParameterValue(kParameterAmpRelease, val);
        else if (std::strcmp(key, "filter_env_amount") == 0)
            setParameterValue(kParameterFilterEnvAmount, val);
        else if (std::strcmp(key, "filter_attack") == 0)
            setParameterValue(kParameterFilterAttack, val);
        else if (std::strcmp(key, "filter_decay") == 0)
            setParameterValue(kParameterFilterDecay, val);
        else if (std::strcmp(key, "filter_sustain") == 0)
            setParameterValue(kParameterFilterSustain, val);
        else if (std::strcmp(key, "filter_release") == 0)
            setParameterValue(kParameterFilterRelease, val);
    }

    // =========================================================================
    // Process

    void activate() override
    {
        if (fSynth) {
            synth_resonate1_reset(fSynth);
            // Restore all parameters after reset
            for (uint32_t i = 0; i < kParameterCount; ++i) {
                setParameterValue(i, fParameters[i]);
            }
        }
    }

    void deactivate() override
    {
        if (fSynth) {
            synth_resonate1_all_notes_off(fSynth);
        }
    }

    void run(const float**, float** outputs, uint32_t frames,
             const MidiEvent* midiEvents, uint32_t midiEventCount) override
    {
        if (!fSynth)
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

            // Process all channels (omni mode)
            switch (status) {
                case 0x90: // Note On
                    if (data[2] > 0) {
                        synth_resonate1_note_on(fSynth, data[1], data[2]);
                    } else {
                        synth_resonate1_note_off(fSynth, data[1]);
                    }
                    break;

                case 0x80: // Note Off
                    synth_resonate1_note_off(fSynth, data[1]);
                    break;

                case 0xB0: // Control Change
                    // Future: map MIDI CCs to parameters
                    break;
            }
        }

        // Process audio (interleaved buffer required by synth)
        float interleavedBuffer[frames * 2];
        synth_resonate1_process_f32(fSynth, interleavedBuffer, frames, getSampleRate());

        // De-interleave to separate L/R outputs
        for (uint32_t i = 0; i < frames; i++) {
            outL[i] = interleavedBuffer[i * 2 + 0];
            outR[i] = interleavedBuffer[i * 2 + 1];
        }
    }

    // =========================================================================
    // Callbacks (optional)

    void sampleRateChanged(double newSampleRate) override
    {
        // Recreate synth with new sample rate
        if (fSynth) {
            synth_resonate1_destroy(fSynth);
        }
        fSynth = synth_resonate1_create(newSampleRate);
    }

private:
    SynthResonate1* fSynth;
    float fParameters[kParameterCount];

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RGResonate1_SynthPlugin)
};

Plugin* createPlugin()
{
    return new RGResonate1_SynthPlugin();
}

END_NAMESPACE_DISTRHO
