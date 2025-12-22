#include "DistrhoPlugin.hpp"
#include "fx_eq.h"
#include "../rfx_plugin_utils.h"
#include <cstring>
#include <cstdio>

START_NAMESPACE_DISTRHO

class RFX_EQPlugin : public Plugin
{
public:
    RFX_EQPlugin()
        : Plugin(kParameterCount, 0, 3)  // 3 state values for explicit VST3 state save/restore
        , fLow(0.5f)
        , fMid(0.5f)
        , fHigh(0.5f)
    {
        fEffect = fx_eq_create();
        fx_eq_set_enabled(fEffect, true);
        fx_eq_set_low(fEffect, fLow);
        fx_eq_set_mid(fEffect, fMid);
        fx_eq_set_high(fEffect, fHigh);
    }

    ~RFX_EQPlugin() override
    {
        if (fEffect) fx_eq_destroy(fEffect);
    }

protected:
    const char* getLabel() const override { return "RFX_EQ"; }
    const char* getDescription() const override { return "3-band equalizer"; }
    const char* getMaker() const override { return "Regroove"; }
    const char* getHomePage() const override { return "https://github.com/gbraad/rfx"; }
    const char* getLicense() const override { return "ISC"; }
    uint32_t getVersion() const override { return d_version(1, 0, 0); }
    int64_t getUniqueId() const override { return d_cconst('R', 'F', 'E', 'Q'); }

    void initParameter(uint32_t index, Parameter& param) override
    {
        param.hints = kParameterIsAutomatable;
        param.ranges.min = fx_eq_get_parameter_min(index);
        param.ranges.max = fx_eq_get_parameter_max(index);
        param.ranges.def = fx_eq_get_parameter_default(index);
        param.name = fx_eq_get_parameter_name(index);
        param.symbol = param.name;
    }

    float getParameterValue(uint32_t index) const override
    {
        switch (index) {
        case kParameterLow: return fLow;
        case kParameterMid: return fMid;
        case kParameterHigh: return fHigh;
        default: return 0.0f;
        }
    }

    void setParameterValue(uint32_t index, float value) override
    {
        switch (index) {
        case kParameterLow: fLow = value; break;
        case kParameterMid: fMid = value; break;
        case kParameterHigh: fHigh = value; break;
        }

        if (fEffect) {
            fx_eq_set_parameter_value(fEffect, index, value);
        }
    }

    void initState(uint32_t index, State& state) override
    {
        switch (index) {
        case 0:
            state.key = "low";
            state.defaultValue = "0.5";
            break;
        case 1:
            state.key = "mid";
            state.defaultValue = "0.5";
            break;
        case 2:
            state.key = "high";
            state.defaultValue = "0.5";
            break;
        }
        state.hints = kStateIsOnlyForDSP;
    }

    void setState(const char* key, const char* value) override
    {
        float fValue = std::atof(value);

        if (std::strcmp(key, "low") == 0) {
            fLow = fValue;
            if (fEffect) fx_eq_set_low(fEffect, fLow);
        }
        else if (std::strcmp(key, "mid") == 0) {
            fMid = fValue;
            if (fEffect) fx_eq_set_mid(fEffect, fMid);
        }
        else if (std::strcmp(key, "high") == 0) {
            fHigh = fValue;
            if (fEffect) fx_eq_set_high(fEffect, fHigh);
        }
    }

    String getState(const char* key) const override
    {
        char buf[32];

        if (std::strcmp(key, "low") == 0) {
            std::snprintf(buf, sizeof(buf), "%.6f", fLow);
            return String(buf);
        }
        if (std::strcmp(key, "mid") == 0) {
            std::snprintf(buf, sizeof(buf), "%.6f", fMid);
            return String(buf);
        }
        if (std::strcmp(key, "high") == 0) {
            std::snprintf(buf, sizeof(buf), "%.6f", fHigh);
            return String(buf);
        }

        return String("0.5");
    }

    void activate() override
    {
        if (fEffect) {
            fx_eq_reset(fEffect);
            for (uint32_t i = 0; i < kParameterCount; ++i) {
                fx_eq_set_parameter_value(fEffect, i, getParameterValue(i));
            }
        }
    }

    void run(const float** inputs, float** outputs, uint32_t frames) override
    {
        RFX::processStereo(inputs, outputs, frames, fEffect,
                          fx_eq_process_f32, (int)getSampleRate());
    }

private:
    FXEqualizer* fEffect;

    // Store parameters to persist across activate/deactivate
    float fLow;
    float fMid;
    float fHigh;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RFX_EQPlugin)
};

Plugin* createPlugin()
{
    return new RFX_EQPlugin();
}

END_NAMESPACE_DISTRHO
