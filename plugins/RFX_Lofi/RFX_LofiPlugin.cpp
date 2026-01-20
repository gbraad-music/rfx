#include "DistrhoPlugin.hpp"
#include "fx_lofi.h"
#include "../rfx_plugin_utils.h"
#include <cstring>
#include <cstdio>

START_NAMESPACE_DISTRHO

class RFX_LofiPlugin : public Plugin
{
private:
    // Map bit depth index to actual bit depth value
    static float indexToBitDepth(int index) {
        static const float bit_depths[] = {2.0f, 8.0f, 12.0f, 16.0f};
        if (index < 0) index = 0;
        if (index > 3) index = 3;
        return bit_depths[index];
    }

    // Map sample rate index to actual ratio (Amiga + AKAI vintage rates)
    static float indexToSampleRate(int index, uint32_t base_sample_rate) {
        // Vintage sample rates: AKAI S950 + Amiga Paula rates
        static const float sample_rate_hz[] = {
            7500.0f,   // 0: AKAI S950 lowest
            8363.0f,   // 1: Amiga Paula (most common!)
            10000.0f,  // 2: AKAI S950
            15000.0f,  // 3: AKAI S950
            16726.0f,  // 4: Amiga Paula 2x
            22050.0f,  // 5: AKAI S1000/S3000, standard
            32000.0f,  // 6: Higher quality
            48000.0f   // 7: Clean/no reduction
        };
        if (index < 0) index = 0;
        if (index > 7) index = 7;

        // Return ratio relative to current sample rate
        return sample_rate_hz[index] / (float)base_sample_rate;
    }

public:
    RFX_LofiPlugin()
        : Plugin(kParameterCount, 0, 7)  // 7 state values for explicit VST3 state save/restore
        , fBitDepthIndex(3.0f)  // Default to 16-bit (index 3)
        , fSampleRateIndex(7.0f)  // Default to 48000 Hz clean (index 7)
        , fFilterCutoff(20000.0f)
        , fSaturation(0.0f)
        , fNoiseLevel(0.0f)
        , fWowFlutterDepth(0.0f)
        , fWowFlutterRate(0.5f)
    {
        fEffect = fx_lofi_create(getSampleRate());
        // Enable the effect
        fx_lofi_set_enabled(fEffect, true);
        // Initialize with default values
        fx_lofi_set_bit_depth(fEffect, indexToBitDepth((int)fBitDepthIndex));
        fx_lofi_set_sample_rate_ratio(fEffect, indexToSampleRate((int)fSampleRateIndex, getSampleRate()));
        fx_lofi_set_filter_cutoff(fEffect, fFilterCutoff);
        fx_lofi_set_saturation(fEffect, fSaturation);
        fx_lofi_set_noise_level(fEffect, fNoiseLevel);
        fx_lofi_set_wow_flutter_depth(fEffect, fWowFlutterDepth);
        fx_lofi_set_wow_flutter_rate(fEffect, fWowFlutterRate);
    }

    ~RFX_LofiPlugin() override
    {
        if (fEffect) fx_lofi_destroy(fEffect);
    }

protected:
    const char* getLabel() const override { return "RFX_Lofi"; }
    const char* getDescription() const override { return "Lo-fi bit crusher and degradation"; }
    const char* getMaker() const override { return "Regroove"; }
    const char* getHomePage() const override { return "https://music.gbraad.nl"; }
    const char* getLicense() const override { return "ISC"; }
    uint32_t getVersion() const override { return d_version(1, 0, 0); }
    int64_t getUniqueId() const override { return d_cconst('R', 'F', 'L', 'F'); }

    void initParameter(uint32_t index, Parameter& param) override
    {
        param.hints = kParameterIsAutomatable;

        switch (index) {
            case kParameterBitDepth:
                param.name = "Bit Depth";
                param.symbol = "bit_depth";
                param.ranges.min = 0.0f;
                param.ranges.max = 3.0f;
                param.ranges.def = 3.0f;  // 16-bit
                param.hints |= kParameterIsInteger;
                param.enumValues.count = 4;
                param.enumValues.restrictedMode = true;
                {
                    static ParameterEnumerationValue enumValues[] = {
                        {0.0f, "2-bit"},
                        {1.0f, "8-bit"},
                        {2.0f, "12-bit"},
                        {3.0f, "16-bit"}
                    };
                    param.enumValues.values = enumValues;
                }
                break;
            case kParameterSampleRateRatio:
                param.name = "Sample Rate";
                param.symbol = "sample_rate";
                param.ranges.min = 0.0f;
                param.ranges.max = 7.0f;
                param.ranges.def = 7.0f;  // 48000 Hz (clean)
                param.hints |= kParameterIsInteger;
                param.enumValues.count = 8;
                param.enumValues.restrictedMode = true;
                {
                    static ParameterEnumerationValue enumValues[] = {
                        {0.0f, "7.5kHz (AKAI S950)"},
                        {1.0f, "8363Hz (Amiga)"},
                        {2.0f, "10kHz (AKAI S950)"},
                        {3.0f, "15kHz (AKAI S950)"},
                        {4.0f, "16726Hz (Amiga 2x)"},
                        {5.0f, "22.05kHz (AKAI)"},
                        {6.0f, "32kHz"},
                        {7.0f, "48kHz (Clean)"}
                    };
                    param.enumValues.values = enumValues;
                }
                break;
            case kParameterFilterCutoff:
                param.name = "Filter";
                param.symbol = "filter";
                param.ranges.min = 200.0f;
                param.ranges.max = 20000.0f;
                param.ranges.def = 20000.0f;
                param.unit = "Hz";
                break;
            case kParameterSaturation:
                param.name = "Saturation";
                param.symbol = "saturation";
                param.ranges.min = 0.0f;
                param.ranges.max = 2.0f;
                param.ranges.def = 0.0f;
                break;
            case kParameterNoiseLevel:
                param.name = "Noise";
                param.symbol = "noise";
                param.ranges.min = 0.0f;
                param.ranges.max = 1.0f;
                param.ranges.def = 0.0f;
                break;
            case kParameterWowFlutterDepth:
                param.name = "Wow/Flutter Depth";
                param.symbol = "wow_flutter_depth";
                param.ranges.min = 0.0f;
                param.ranges.max = 1.0f;
                param.ranges.def = 0.0f;
                break;
            case kParameterWowFlutterRate:
                param.name = "Wow/Flutter Rate";
                param.symbol = "wow_flutter_rate";
                param.ranges.min = 0.1f;
                param.ranges.max = 10.0f;
                param.ranges.def = 0.5f;
                param.unit = "Hz";
                break;
        }
    }

    float getParameterValue(uint32_t index) const override
    {
        switch (index) {
        case kParameterBitDepth: return fBitDepthIndex;
        case kParameterSampleRateRatio: return fSampleRateIndex;
        case kParameterFilterCutoff: return fFilterCutoff;
        case kParameterSaturation: return fSaturation;
        case kParameterNoiseLevel: return fNoiseLevel;
        case kParameterWowFlutterDepth: return fWowFlutterDepth;
        case kParameterWowFlutterRate: return fWowFlutterRate;
        default: return 0.0f;
        }
    }

    void setParameterValue(uint32_t index, float value) override
    {
        // Store parameter value
        switch (index) {
        case kParameterBitDepth: fBitDepthIndex = value; break;
        case kParameterSampleRateRatio: fSampleRateIndex = value; break;
        case kParameterFilterCutoff: fFilterCutoff = value; break;
        case kParameterSaturation: fSaturation = value; break;
        case kParameterNoiseLevel: fNoiseLevel = value; break;
        case kParameterWowFlutterDepth: fWowFlutterDepth = value; break;
        case kParameterWowFlutterRate: fWowFlutterRate = value; break;
        }

        // Apply to DSP engine
        if (fEffect) {
            switch (index) {
            case kParameterBitDepth: fx_lofi_set_bit_depth(fEffect, indexToBitDepth((int)value)); break;
            case kParameterSampleRateRatio: fx_lofi_set_sample_rate_ratio(fEffect, indexToSampleRate((int)value, getSampleRate())); break;
            case kParameterFilterCutoff: fx_lofi_set_filter_cutoff(fEffect, value); break;
            case kParameterSaturation: fx_lofi_set_saturation(fEffect, value); break;
            case kParameterNoiseLevel: fx_lofi_set_noise_level(fEffect, value); break;
            case kParameterWowFlutterDepth: fx_lofi_set_wow_flutter_depth(fEffect, value); break;
            case kParameterWowFlutterRate: fx_lofi_set_wow_flutter_rate(fEffect, value); break;
            }
        }
    }

    void initState(uint32_t index, State& state) override
    {
        switch (index) {
        case 0:
            state.key = "bit_depth";
            state.defaultValue = "3.0";  // Index 3 = 16-bit
            break;
        case 1:
            state.key = "sample_rate_ratio";
            state.defaultValue = "7.0";  // Index 7 = 48000 Hz clean
            break;
        case 2:
            state.key = "filter_cutoff";
            state.defaultValue = "20000.0";
            break;
        case 3:
            state.key = "saturation";
            state.defaultValue = "0.0";
            break;
        case 4:
            state.key = "noise_level";
            state.defaultValue = "0.0";
            break;
        case 5:
            state.key = "wow_flutter_depth";
            state.defaultValue = "0.0";
            break;
        case 6:
            state.key = "wow_flutter_rate";
            state.defaultValue = "0.5";
            break;
        }
        state.hints = kStateIsOnlyForDSP;
    }

    void setState(const char* key, const char* value) override
    {
        float fValue = std::atof(value);

        if (std::strcmp(key, "bit_depth") == 0) {
            fBitDepthIndex = fValue;
            if (fEffect) fx_lofi_set_bit_depth(fEffect, indexToBitDepth((int)fBitDepthIndex));
        } else if (std::strcmp(key, "sample_rate_ratio") == 0) {
            fSampleRateIndex = fValue;
            if (fEffect) fx_lofi_set_sample_rate_ratio(fEffect, indexToSampleRate((int)fSampleRateIndex, getSampleRate()));
        } else if (std::strcmp(key, "filter_cutoff") == 0) {
            fFilterCutoff = fValue;
            if (fEffect) fx_lofi_set_filter_cutoff(fEffect, fFilterCutoff);
        } else if (std::strcmp(key, "saturation") == 0) {
            fSaturation = fValue;
            if (fEffect) fx_lofi_set_saturation(fEffect, fSaturation);
        } else if (std::strcmp(key, "noise_level") == 0) {
            fNoiseLevel = fValue;
            if (fEffect) fx_lofi_set_noise_level(fEffect, fNoiseLevel);
        } else if (std::strcmp(key, "wow_flutter_depth") == 0) {
            fWowFlutterDepth = fValue;
            if (fEffect) fx_lofi_set_wow_flutter_depth(fEffect, fWowFlutterDepth);
        } else if (std::strcmp(key, "wow_flutter_rate") == 0) {
            fWowFlutterRate = fValue;
            if (fEffect) fx_lofi_set_wow_flutter_rate(fEffect, fWowFlutterRate);
        }
    }

    void activate() override
    {
        if (fEffect) fx_lofi_reset(fEffect);
    }

    void run(const float** inputs, float** outputs, uint32_t frames) override
    {
        if (!fEffect) {
            // Bypass
            std::memcpy(outputs[0], inputs[0], frames * sizeof(float));
            std::memcpy(outputs[1], inputs[1], frames * sizeof(float));
            return;
        }

        // Interleave input
        float interleavedBuffer[frames * 2];
        for (uint32_t i = 0; i < frames; ++i) {
            interleavedBuffer[i * 2] = inputs[0][i];
            interleavedBuffer[i * 2 + 1] = inputs[1][i];
        }

        // Process
        fx_lofi_process_f32(fEffect, interleavedBuffer, frames, (uint32_t)getSampleRate());

        // De-interleave output
        for (uint32_t i = 0; i < frames; ++i) {
            outputs[0][i] = interleavedBuffer[i * 2];
            outputs[1][i] = interleavedBuffer[i * 2 + 1];
        }
    }

private:
    FX_Lofi* fEffect;

    // Parameters
    float fBitDepthIndex;     // 0-3 index, maps to {2, 8, 12, 16} bit
    float fSampleRateIndex;   // 0-7 index, maps to {7.5k, 8.3k(Amiga), 10k, 15k, 16.7k(Amiga), 22k, 32k, 48k} Hz
    float fFilterCutoff;
    float fSaturation;
    float fNoiseLevel;
    float fWowFlutterDepth;
    float fWowFlutterRate;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RFX_LofiPlugin)
};

Plugin* createPlugin()
{
    return new RFX_LofiPlugin();
}

END_NAMESPACE_DISTRHO
