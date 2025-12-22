#include "DistrhoPlugin.hpp"
#include "fx_stereo_widen.h"
#include "../rfx_plugin_utils.h"
#include <cstring>
#include <cstdio>

START_NAMESPACE_DISTRHO

class RFX_StereoWidenPlugin : public Plugin
{
public:
    RFX_StereoWidenPlugin()
        : Plugin(kParameterCount, 0, 2)
        , fWidth(1.0f)
        , fMix(1.0f)
    {
        fEffect = fx_stereo_widen_create();
        fx_stereo_widen_set_enabled(fEffect, true);
        fx_stereo_widen_set_width(fEffect, fWidth);
        fx_stereo_widen_set_mix(fEffect, fMix);
    }

    ~RFX_StereoWidenPlugin() override
    {
        if (fEffect) fx_stereo_widen_destroy(fEffect);
    }

protected:
    const char* getLabel() const override { return "RFX_StereoWiden"; }
    const char* getDescription() const override { return "Mid/Side stereo widener"; }
    const char* getMaker() const override { return "Regroove"; }
    const char* getHomePage() const override { return "https://github.com/gbraad/rfx"; }
    const char* getLicense() const override { return "ISC"; }
    uint32_t getVersion() const override { return d_version(1, 0, 0); }
    int64_t getUniqueId() const override { return d_cconst('R', 'F', 'S', 'W'); }

    void initParameter(uint32_t index, Parameter& param) override
    {
        param.hints = kParameterIsAutomatable;
        param.ranges.min = fx_stereo_widen_get_parameter_min(index);
        param.ranges.max = fx_stereo_widen_get_parameter_max(index);
        param.ranges.def = fx_stereo_widen_get_parameter_default(index);
        param.name = fx_stereo_widen_get_parameter_name(index);
        param.symbol = param.name;
    }

    float getParameterValue(uint32_t index) const override
    {
        switch (index) {
        case kParameterWidth: return fWidth;
        case kParameterMix: return fMix;
        default: return 0.0f;
        }
    }

    void setParameterValue(uint32_t index, float value) override
    {
        switch (index) {
        case kParameterWidth: fWidth = value; break;
        case kParameterMix: fMix = value; break;
        }
        if (fEffect) {
            switch (index) {
            case kParameterWidth: fx_stereo_widen_set_width(fEffect, value); break;
            case kParameterMix: fx_stereo_widen_set_mix(fEffect, value); break;
            }
        }
    }

    void initState(uint32_t index, State& state) override
    {
        switch (index) {
        case 0:
            state.key = "width";
            state.defaultValue = "0.9";
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
        const float fValue = std::atof(value);
        if (std::strcmp(key, "width") == 0) {
            fWidth = fValue;
            if (fEffect) fx_stereo_widen_set_width(fEffect, fWidth);
        } else if (std::strcmp(key, "mix") == 0) {
            fMix = fValue;
            if (fEffect) fx_stereo_widen_set_mix(fEffect, fMix);
        }
    }

    String getState(const char* key) const override
    {
        char buf[32];
        if (std::strcmp(key, "width") == 0) {
            std::snprintf(buf, sizeof(buf), "%.6f", fWidth);
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
            fx_stereo_widen_reset(fEffect);
            for (uint32_t i = 0; i < kParameterCount; ++i) {
                fx_stereo_widen_set_parameter_value(fEffect, i, getParameterValue(i));
            }
        }
    }

    void run(const float** inputs, float** outputs, uint32_t frames) override
    {
        RFX::processStereo(inputs, outputs, frames, fEffect,
                           fx_stereo_widen_process_interleaved, (int)getSampleRate());
    }

private:
    FXStereoWiden* fEffect;
    float fWidth;
    float fMix;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RFX_StereoWidenPlugin)
};

Plugin* createPlugin()
{ return new RFX_StereoWidenPlugin(); }

END_NAMESPACE_DISTRHO
