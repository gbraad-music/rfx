#include "DistrhoPlugin.hpp"
#include "fx_freqshift.h"
#include "../rfx_plugin_utils.h"
#include <cstring>
#include <cstdio>

START_NAMESPACE_DISTRHO

class RFX_FreqShiftPlugin : public Plugin
{
public:
    RFX_FreqShiftPlugin()
        : Plugin(kParameterCount, 0, 2)  // 2 state values for explicit VST3 state save/restore
        , fFreq(0.5f)       // 0 Hz
        , fMix(1.0f)        // 100% wet
    {
        fEffect = fx_freqshift_create();
        fx_freqshift_set_enabled(fEffect, true);
        // Initialize with default values
        fx_freqshift_set_freq(fEffect, fFreq);
        fx_freqshift_set_mix(fEffect, fMix);
    }

    ~RFX_FreqShiftPlugin() override
    {
        if (fEffect) fx_freqshift_destroy(fEffect);
    }

protected:
    const char* getLabel() const override { return "RFX_FreqShift"; }
    const char* getDescription() const override { return "Bode-style frequency shifter"; }
    const char* getMaker() const override { return "Regroove"; }
    const char* getHomePage() const override { return "https://github.com/gbraad/rfx"; }
    const char* getLicense() const override { return "ISC"; }
    uint32_t getVersion() const override { return d_version(1, 0, 0); }
    int64_t getUniqueId() const override { return d_cconst('R', 'F', 'F', 'S'); }

    void initParameter(uint32_t index, Parameter& param) override
    {
        param.hints = kParameterIsAutomatable;
        param.ranges.min = 0.0f;
        param.ranges.max = 1.0f;
        param.ranges.def = 0.5f;

        switch (index) {
        case kParameterFreq:
            param.name = "Frequency";
            param.symbol = "freq";
            param.ranges.def = 0.5f;  // 0 Hz
            break;
        case kParameterMix:
            param.name = "Mix";
            param.symbol = "mix";
            param.ranges.def = 1.0f;  // 100% wet
            break;
        }
    }

    float getParameterValue(uint32_t index) const override
    {
        switch (index) {
        case kParameterFreq: return fFreq;
        case kParameterMix: return fMix;
        default: return 0.0f;
        }
    }

    void setParameterValue(uint32_t index, float value) override
    {
        // Store parameter value
        switch (index) {
        case kParameterFreq: fFreq = value; break;
        case kParameterMix: fMix = value; break;
        }

        // Apply to DSP engine if it exists
        if (fEffect) {
            switch (index) {
            case kParameterFreq: fx_freqshift_set_freq(fEffect, value); break;
            case kParameterMix: fx_freqshift_set_mix(fEffect, value); break;
            }
        }
    }

    void initState(uint32_t index, State& state) override
    {
        switch (index) {
        case 0:
            state.key = "freq";
            state.defaultValue = "0.5";
            break;
        case 1:
            state.key = "mix";
            state.defaultValue = "1.0";
            break;
        }
        state.hints = kStateIsOnlyForDSP;
    }

    void setState(const char* key, const char* value) override
    {
        float fValue = std::atof(value);

        if (std::strcmp(key, "freq") == 0) {
            fFreq = fValue;
            if (fEffect) fx_freqshift_set_freq(fEffect, fFreq);
        }
        else if (std::strcmp(key, "mix") == 0) {
            fMix = fValue;
            if (fEffect) fx_freqshift_set_mix(fEffect, fMix);
        }
    }

    String getState(const char* key) const override
    {
        char buf[32];

        if (std::strcmp(key, "freq") == 0) {
            std::snprintf(buf, sizeof(buf), "%.6f", fFreq);
            return String(buf);
        }
        if (std::strcmp(key, "mix") == 0) {
            std::snprintf(buf, sizeof(buf), "%.6f", fMix);
            return String(buf);
        }

        return String("0.5");
    }

    void activate() override
    {
        if (fEffect) {
            fx_freqshift_reset(fEffect);
            // Restore parameters after reset
            fx_freqshift_set_freq(fEffect, fFreq);
            fx_freqshift_set_mix(fEffect, fMix);
        }
    }

    void run(const float** inputs, float** outputs, uint32_t frames) override
    {
        RFX::processStereo(inputs, outputs, frames, fEffect,
                          fx_freqshift_process_f32, (int)getSampleRate());
    }

private:
    FXFreqShift* fEffect;

    // Store parameters to persist across activate/deactivate
    float fFreq;
    float fMix;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RFX_FreqShiftPlugin)
};

Plugin* createPlugin()
{
    return new RFX_FreqShiftPlugin();
}

END_NAMESPACE_DISTRHO
