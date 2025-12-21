/*
 * RGHX - AHX-Style Synthesizer Plugin
 * Amiga-style wavetable synth with ADSR, filter, and modulation
 */

#include "DistrhoPlugin.hpp"
#include "../../synth/synth_ahx.h"
#include <cstring>

START_NAMESPACE_DISTRHO


enum Parameters {
    kParameterWaveform = 0,
    kParameterWaveLength,
    kParameterAttack,
    kParameterDecay,
    kParameterSustain,
    kParameterRelease,
    kParameterFilterType,
    kParameterFilterCutoff,
    kParameterFilterResonance,
    kParameterVibratoDepth,
    kParameterVibratoSpeed,
    kParameterVolume,
    kParameterCount
};

class RGHX_SynthPlugin : public Plugin
{
public:
    RGHX_SynthPlugin()
        : Plugin(kParameterCount, 0, 0)
        , fWaveform(1.0f)  // Sawtooth
        , fWaveLength(0.125f)  // 32 samples
        , fAttack(0.01f)
        , fDecay(0.1f)
        , fSustain(0.7f)
        , fRelease(0.1f)
        , fFilterType(1.0f)  // Lowpass
        , fFilterCutoff(1.0f)
        , fFilterResonance(0.0f)
        , fVibratoDepth(0.0f)
        , fVibratoSpeed(0.0f)
        , fVolume(0.7f)
    {
        fVoice = synth_ahx_voice_create();
        if (fVoice) {
            updateVoiceParameters();
        }
    }

    ~RGHX_SynthPlugin() override
    {
        if (fVoice) synth_ahx_voice_destroy(fVoice);
    }

protected:
    const char* getLabel() const override
    {
        return RGHX_DISPLAY_NAME;
    }

    const char* getDescription() const override
    {
        return RGHX_DESCRIPTION;
    }

    const char* getMaker() const override { return "Regroove"; }
    const char* getHomePage() const override { return "https://github.com/gbraad/rfx"; }
    const char* getLicense() const override { return "ISC"; }
    uint32_t getVersion() const override { return d_version(1, 0, 0); }
    int64_t getUniqueId() const override { return d_cconst('R', 'G', 'H', 'X'); }

    void initParameter(uint32_t index, Parameter& param) override
    {
        param.hints = kParameterIsAutomatable;
        param.ranges.min = 0.0f;
        param.ranges.max = 1.0f;
        param.ranges.def = 0.5f;

        switch (index) {
        case kParameterWaveform:
            param.name = "Waveform";
            param.symbol = "waveform";
            param.hints |= kParameterIsInteger;
            param.ranges.min = 0.0f;
            param.ranges.max = 3.0f;  // Triangle, Saw, Square, Noise
            param.ranges.def = 1.0f;  // Sawtooth
            param.enumValues.count = 4;
            param.enumValues.restrictedMode = true;
            {
                ParameterEnumerationValue* const values = new ParameterEnumerationValue[4];
                param.enumValues.values = values;
                values[0].label = "Triangle";
                values[0].value = 0.0f;
                values[1].label = "Sawtooth";
                values[1].value = 1.0f;
                values[2].label = "Square";
                values[2].value = 2.0f;
                values[3].label = "Noise";
                values[3].value = 3.0f;
            }
            break;

        case kParameterWaveLength:
            param.name = "Wave Length";
            param.symbol = "wavelength";
            param.ranges.min = 0.0f;
            param.ranges.max = 1.0f;
            param.ranges.def = 0.125f;  // 32 samples
            break;

        case kParameterAttack:
            param.name = "Attack";
            param.symbol = "attack";
            param.ranges.def = 0.01f;
            break;

        case kParameterDecay:
            param.name = "Decay";
            param.symbol = "decay";
            param.ranges.def = 0.1f;
            break;

        case kParameterSustain:
            param.name = "Sustain";
            param.symbol = "sustain";
            param.ranges.def = 0.7f;
            break;

        case kParameterRelease:
            param.name = "Release";
            param.symbol = "release";
            param.ranges.def = 0.1f;
            break;

        case kParameterFilterType:
            param.name = "Filter Type";
            param.symbol = "filtertype";
            param.hints |= kParameterIsInteger;
            param.ranges.min = 0.0f;
            param.ranges.max = 2.0f;
            param.ranges.def = 1.0f;  // Lowpass
            param.enumValues.count = 3;
            param.enumValues.restrictedMode = true;
            {
                ParameterEnumerationValue* const values = new ParameterEnumerationValue[3];
                param.enumValues.values = values;
                values[0].label = "None";
                values[0].value = 0.0f;
                values[1].label = "Lowpass";
                values[1].value = 1.0f;
                values[2].label = "Highpass";
                values[2].value = 2.0f;
            }
            break;

        case kParameterFilterCutoff:
            param.name = "Filter Cutoff";
            param.symbol = "filtercutoff";
            param.ranges.def = 1.0f;
            break;

        case kParameterFilterResonance:
            param.name = "Filter Resonance";
            param.symbol = "filterresonance";
            param.ranges.def = 0.0f;
            break;

        case kParameterVibratoDepth:
            param.name = "Vibrato Depth";
            param.symbol = "vibratodepth";
            param.ranges.def = 0.0f;
            break;

        case kParameterVibratoSpeed:
            param.name = "Vibrato Speed";
            param.symbol = "vibratospeed";
            param.ranges.def = 0.0f;
            break;

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
        case kParameterWaveform: return fWaveform;
        case kParameterWaveLength: return fWaveLength;
        case kParameterAttack: return fAttack;
        case kParameterDecay: return fDecay;
        case kParameterSustain: return fSustain;
        case kParameterRelease: return fRelease;
        case kParameterFilterType: return fFilterType;
        case kParameterFilterCutoff: return fFilterCutoff;
        case kParameterFilterResonance: return fFilterResonance;
        case kParameterVibratoDepth: return fVibratoDepth;
        case kParameterVibratoSpeed: return fVibratoSpeed;
        case kParameterVolume: return fVolume;
        default: return 0.0f;
        }
    }

    void setParameterValue(uint32_t index, float value) override
    {
        switch (index) {
        case kParameterWaveform:
            fWaveform = value;
            if (fVoice) synth_ahx_voice_set_waveform(fVoice, (SynthAHXWaveform)(int)value);
            break;
        case kParameterWaveLength:
            fWaveLength = value;
            if (fVoice) {
                int len = 4 + (int)(value * 252.0f);  // 4-256
                synth_ahx_voice_set_wave_length(fVoice, len);
            }
            break;
        case kParameterAttack:
            fAttack = value;
            if (fVoice) synth_ahx_voice_set_attack(fVoice, value);
            break;
        case kParameterDecay:
            fDecay = value;
            if (fVoice) synth_ahx_voice_set_decay(fVoice, value);
            break;
        case kParameterSustain:
            fSustain = value;
            if (fVoice) synth_ahx_voice_set_sustain(fVoice, value);
            break;
        case kParameterRelease:
            fRelease = value;
            if (fVoice) synth_ahx_voice_set_release(fVoice, value);
            break;
        case kParameterFilterType:
            fFilterType = value;
            if (fVoice) synth_ahx_voice_set_filter_type(fVoice, (SynthAHXFilterType)(int)value);
            break;
        case kParameterFilterCutoff:
            fFilterCutoff = value;
            if (fVoice) synth_ahx_voice_set_filter_cutoff(fVoice, value);
            break;
        case kParameterFilterResonance:
            fFilterResonance = value;
            if (fVoice) synth_ahx_voice_set_filter_resonance(fVoice, value);
            break;
        case kParameterVibratoDepth:
            fVibratoDepth = value;
            if (fVoice) synth_ahx_voice_set_vibrato_depth(fVoice, value);
            break;
        case kParameterVibratoSpeed:
            fVibratoSpeed = value;
            if (fVoice) synth_ahx_voice_set_vibrato_speed(fVoice, value);
            break;
        case kParameterVolume:
            fVolume = value;
            break;
        }
    }

    void activate() override
    {
        if (fVoice) synth_ahx_voice_reset(fVoice);
    }

    void run(const float**, float** outputs, uint32_t frames,
             const MidiEvent* midiEvents, uint32_t midiEventCount) override
    {
        float* outL = outputs[0];
        float* outR = outputs[1];

        std::memset(outL, 0, sizeof(float) * frames);
        std::memset(outR, 0, sizeof(float) * frames);

        if (!fVoice) return;

        int sampleRate = (int)getSampleRate();
        if (sampleRate <= 0) sampleRate = 44100;

        uint32_t framePos = 0;
        uint32_t midiEventIndex = 0;

        while (framePos < frames) {
            // Process MIDI events
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
                        synth_ahx_voice_note_on(fVoice, note, velocity);
                    } else {
                        synth_ahx_voice_note_off(fVoice);
                    }
                }
                else if (status == 0x80 && event.size >= 2) {
                    // Note Off
                    synth_ahx_voice_note_off(fVoice);
                }
            }

            // Generate audio
            float sample = synth_ahx_voice_process(fVoice, sampleRate);
            sample *= fVolume;

            // Soft clipping
            if (sample > 1.0f) sample = 1.0f;
            if (sample < -1.0f) sample = -1.0f;

            outL[framePos] = sample;
            outR[framePos] = sample;

            framePos++;
        }
    }

private:
    void updateVoiceParameters()
    {
        if (!fVoice) return;

        synth_ahx_voice_set_waveform(fVoice, (SynthAHXWaveform)(int)fWaveform);
        int len = 4 + (int)(fWaveLength * 252.0f);
        synth_ahx_voice_set_wave_length(fVoice, len);
        synth_ahx_voice_set_attack(fVoice, fAttack);
        synth_ahx_voice_set_decay(fVoice, fDecay);
        synth_ahx_voice_set_sustain(fVoice, fSustain);
        synth_ahx_voice_set_release(fVoice, fRelease);
        synth_ahx_voice_set_filter_type(fVoice, (SynthAHXFilterType)(int)fFilterType);
        synth_ahx_voice_set_filter_cutoff(fVoice, fFilterCutoff);
        synth_ahx_voice_set_filter_resonance(fVoice, fFilterResonance);
        synth_ahx_voice_set_vibrato_depth(fVoice, fVibratoDepth);
        synth_ahx_voice_set_vibrato_speed(fVoice, fVibratoSpeed);
    }

    SynthAHXVoice* fVoice;

    float fWaveform;
    float fWaveLength;
    float fAttack;
    float fDecay;
    float fSustain;
    float fRelease;
    float fFilterType;
    float fFilterCutoff;
    float fFilterResonance;
    float fVibratoDepth;
    float fVibratoSpeed;
    float fVolume;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RGHX_SynthPlugin)
};

Plugin* createPlugin()
{
    return new RGHX_SynthPlugin();
}

END_NAMESPACE_DISTRHO
