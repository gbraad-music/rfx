#include "DistrhoPlugin.hpp"
#include "fx_filter.h"
#include "../rfx_plugin_utils.h"
#include <cstring>
#include <cstdio>

START_NAMESPACE_DISTRHO

class RFX_FilterPlugin : public Plugin
{
public:
    RFX_FilterPlugin()
        : Plugin(kParameterCount, 0, 2)  // 2 state values for explicit VST3 state save/restore
        , fCutoff(0.8f)
        , fResonance(0.3f)
    {
        fEffect = fx_filter_create();
        fx_filter_set_enabled(fEffect, true);
        // Initialize with default values
        fx_filter_set_cutoff(fEffect, fCutoff);
        fx_filter_set_resonance(fEffect, fResonance);
    }

    ~RFX_FilterPlugin() override
    {
        if (fEffect) fx_filter_destroy(fEffect);
    }

protected:
    const char* getLabel() const override { return "RFX_Filter"; }
    const char* getDescription() const override { return "Resonant lowpass filter"; }
    const char* getMaker() const override { return "Regroove"; }
    const char* getHomePage() const override { return "https://github.com/gbraad/rfx"; }
    const char* getLicense() const override { return "ISC"; }
    uint32_t getVersion() const override { return d_version(1, 0, 0); }
    int64_t getUniqueId() const override { return d_cconst('R', 'F', 'F', 'L'); }

    void initParameter(uint32_t index, Parameter& param) override
    {
        param.hints = kParameterIsAutomatable;
        param.ranges.min = 0.0f;
        param.ranges.max = 1.0f;
        param.ranges.def = 0.5f;

        switch (index) {
        case kParameterCutoff:
            param.name = "Cutoff";
            param.symbol = "cutoff";
            param.ranges.def = 0.8f;
            break;
        case kParameterResonance:
            param.name = "Resonance";
            param.symbol = "resonance";
            param.ranges.def = 0.3f;
            break;
        }
    }

    float getParameterValue(uint32_t index) const override
    {
        switch (index) {
        case kParameterCutoff: return fCutoff;
        case kParameterResonance: return fResonance;
        default: return 0.0f;
        }
    }

    void setParameterValue(uint32_t index, float value) override
    {
        // Store parameter value
        switch (index) {
        case kParameterCutoff: fCutoff = value; break;
        case kParameterResonance: fResonance = value; break;
        }

        // Apply to DSP engine if it exists
        if (fEffect) {
            switch (index) {
            case kParameterCutoff: fx_filter_set_cutoff(fEffect, value); break;
            case kParameterResonance: fx_filter_set_resonance(fEffect, value); break;
            }
        }
    }

    void initState(uint32_t index, State& state) override
    {
        switch (index) {
        case 0:
            state.key = "cutoff";
            state.defaultValue = "0.8";
            break;
        case 1:
            state.key = "resonance";
            state.defaultValue = "0.3";
            break;
        }
        state.hints = kStateIsOnlyForDSP;
    }

    void setState(const char* key, const char* value) override
    {
        float fValue = std::atof(value);

        if (std::strcmp(key, "cutoff") == 0) {
            fCutoff = fValue;
            if (fEffect) fx_filter_set_cutoff(fEffect, fCutoff);
        }
        else if (std::strcmp(key, "resonance") == 0) {
            fResonance = fValue;
            if (fEffect) fx_filter_set_resonance(fEffect, fResonance);
        }
    }

    String getState(const char* key) const override
    {
        char buf[32];

        if (std::strcmp(key, "cutoff") == 0) {
            std::snprintf(buf, sizeof(buf), "%.6f", fCutoff);
            return String(buf);
        }
        if (std::strcmp(key, "resonance") == 0) {
            std::snprintf(buf, sizeof(buf), "%.6f", fResonance);
            return String(buf);
        }

        return String("0.5");
    }

    void activate() override
    {
        if (fEffect) {
            fx_filter_reset(fEffect);
            // Restore parameters after reset
            fx_filter_set_cutoff(fEffect, fCutoff);
            fx_filter_set_resonance(fEffect, fResonance);
        }
    }

    void run(const float** inputs, float** outputs, uint32_t frames) override
    {
        RFX::processStereo(inputs, outputs, frames, fEffect,
                          fx_filter_process_f32, (int)getSampleRate());
    }

private:
    FXFilter* fEffect;

    // Store parameters to persist across activate/deactivate
    float fCutoff;
    float fResonance;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RFX_FilterPlugin)
};

Plugin* createPlugin()
{
    return new RFX_FilterPlugin();
}

END_NAMESPACE_DISTRHO
