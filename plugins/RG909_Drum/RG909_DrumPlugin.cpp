/*
 * RG909 Drum Plugin - Thin Wrapper Around Circuit-Accurate Synth
 */

#include "DistrhoPlugin.hpp"
#include "synth/rg909_drum_synth.h"
#include <cstring>

START_NAMESPACE_DISTRHO

class RG909_DrumPlugin : public Plugin
{
public:
    RG909_DrumPlugin()
        : Plugin(kParameterCount, 0, 0)
    {
        fSynth = rg909_synth_create();
    }

    ~RG909_DrumPlugin() override
    {
        if (fSynth) rg909_synth_destroy(fSynth);
    }

protected:
    const char* getLabel() const override { return RG909_DISPLAY_NAME; }
    const char* getDescription() const override { return RG909_DESCRIPTION; }
    const char* getMaker() const override { return "Regroove"; }
    const char* getHomePage() const override { return "https://music.gbraad.nl/regrooved/"; }
    const char* getLicense() const override { return "GPL-3.0"; }
    uint32_t getVersion() const override { return d_version(1, 0, 0); }
    int64_t getUniqueId() const override { return d_cconst('R', 'G', '9', '9'); }

    void initParameter(uint32_t index, Parameter& param) override
    {
        param.hints = kParameterIsAutomatable;
        param.ranges.min = 0.0f;
        param.ranges.max = 1.0f;
        param.ranges.def = 0.5f;

        switch (index) {
        case kParameterBDLevel: param.name = "BD Level"; param.symbol = "bd_level"; param.ranges.def = 0.8f; break;
        case kParameterBDTune: param.name = "BD Tune"; param.symbol = "bd_tune"; break;
        case kParameterBDDecay: param.name = "BD Decay"; param.symbol = "bd_decay"; break;
        case kParameterBDAttack: param.name = "BD Attack"; param.symbol = "bd_attack"; param.ranges.def = 0.0f; break;

        case kParameterSDLevel: param.name = "SD Level"; param.symbol = "sd_level"; param.ranges.def = 0.7f; break;
        case kParameterSDTone: param.name = "SD Tone"; param.symbol = "sd_tone"; break;
        case kParameterSDSnappy: param.name = "SD Snappy"; param.symbol = "sd_snappy"; break;
        case kParameterSDTuning: param.name = "SD Tuning"; param.symbol = "sd_tuning"; break;

        case kParameterLTLevel: param.name = "LT Level"; param.symbol = "lt_level"; param.ranges.def = 0.7f; break;
        case kParameterLTTuning: param.name = "LT Tuning"; param.symbol = "lt_tuning"; break;
        case kParameterLTDecay: param.name = "LT Decay"; param.symbol = "lt_decay"; break;

        case kParameterMTLevel: param.name = "MT Level"; param.symbol = "mt_level"; param.ranges.def = 0.7f; break;
        case kParameterMTTuning: param.name = "MT Tuning"; param.symbol = "mt_tuning"; break;
        case kParameterMTDecay: param.name = "MT Decay"; param.symbol = "mt_decay"; break;

        case kParameterHTLevel: param.name = "HT Level"; param.symbol = "ht_level"; param.ranges.def = 0.7f; break;
        case kParameterHTTuning: param.name = "HT Tuning"; param.symbol = "ht_tuning"; break;
        case kParameterHTDecay: param.name = "HT Decay"; param.symbol = "ht_decay"; break;

        case kParameterRSLevel: param.name = "RS Level"; param.symbol = "rs_level"; param.ranges.def = 0.6f; break;
        case kParameterRSTuning: param.name = "RS Tuning"; param.symbol = "rs_tuning"; break;

        case kParameterHCLevel: param.name = "HC Level"; param.symbol = "hc_level"; param.ranges.def = 0.6f; break;
        case kParameterHCTone: param.name = "HC Tone"; param.symbol = "hc_tone"; break;

        case kParameterMasterVolume: param.name = "Master"; param.symbol = "master"; param.ranges.def = 0.6f; break;
        }
    }

    float getParameterValue(uint32_t index) const override
    {
        return fSynth ? rg909_synth_get_parameter(fSynth, index) : 0.0f;
    }

    void setParameterValue(uint32_t index, float value) override
    {
        if (fSynth) rg909_synth_set_parameter(fSynth, index, value);
    }

    void run(const float**, float** outputs, uint32_t frames, const MidiEvent* midiEvents, uint32_t midiEventCount) override
    {
        float* outL = outputs[0];
        float* outR = outputs[1];
        std::memset(outL, 0, sizeof(float) * frames);
        std::memset(outR, 0, sizeof(float) * frames);

        if (!fSynth) return;

        const float sampleRate = (float)getSampleRate();
        uint32_t framePos = 0;

        // Allocate interleaved stereo buffer
        float* interleavedBuffer = new float[frames * 2];

        for (uint32_t i = 0; i < midiEventCount; ++i) {
            const MidiEvent& event = midiEvents[i];

            // Process audio up to this event
            if (event.frame > framePos) {
                uint32_t framesToProcess = event.frame - framePos;
                rg909_synth_process_interleaved(fSynth, interleavedBuffer, framesToProcess, sampleRate);

                // Deinterleave
                for (uint32_t f = 0; f < framesToProcess; f++) {
                    outL[framePos + f] = interleavedBuffer[f * 2];
                    outR[framePos + f] = interleavedBuffer[f * 2 + 1];
                }
                framePos = event.frame;
            }

            // Handle MIDI event
            if (event.size == 3) {
                const uint8_t status = event.data[0] & 0xF0;
                const uint8_t note = event.data[1];
                const uint8_t velocity = event.data[2];

                if (status == 0x90 && velocity > 0) {
                    rg909_synth_trigger_drum(fSynth, note, velocity, sampleRate);
                }
            }
        }

        // Process remaining frames
        if (framePos < frames) {
            uint32_t framesToProcess = frames - framePos;
            rg909_synth_process_interleaved(fSynth, interleavedBuffer, framesToProcess, sampleRate);

            // Deinterleave
            for (uint32_t f = 0; f < framesToProcess; f++) {
                outL[framePos + f] = interleavedBuffer[f * 2];
                outR[framePos + f] = interleavedBuffer[f * 2 + 1];
            }
        }

        delete[] interleavedBuffer;
    }

private:
    RG909Synth* fSynth;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RG909_DrumPlugin)
};

Plugin* createPlugin()
{
    return new RG909_DrumPlugin();
}

END_NAMESPACE_DISTRHO
