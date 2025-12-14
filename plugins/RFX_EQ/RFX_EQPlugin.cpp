#include "DistrhoPlugin.hpp"
#include "fx_eq.h"
#include "../rfx_plugin_utils.h"

START_NAMESPACE_DISTRHO

class RFX_EQPlugin : public Plugin
{
public:
    RFX_EQPlugin()
        : Plugin(kParameterCount, 0, 0)
    {
        fEffect = fx_eq_create();
        fx_eq_set_enabled(fEffect, true);
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
        param.ranges.min = 0.0f;
        param.ranges.max = 1.0f;
        param.ranges.def = 0.5f;

        switch (index) {
        case kParameterLow:
            param.name = "Low";
            param.symbol = "low";
            break;
        case kParameterMid:
            param.name = "Mid";
            param.symbol = "mid";
            break;
        case kParameterHigh:
            param.name = "High";
            param.symbol = "high";
            break;
        }
    }

    float getParameterValue(uint32_t index) const override
    {
        if (!fEffect) return 0.0f;

        switch (index) {
        case kParameterLow: return fx_eq_get_low(fEffect);
        case kParameterMid: return fx_eq_get_mid(fEffect);
        case kParameterHigh: return fx_eq_get_high(fEffect);
        default: return 0.0f;
        }
    }

    void setParameterValue(uint32_t index, float value) override
    {
        if (!fEffect) return;

        switch (index) {
        case kParameterLow: fx_eq_set_low(fEffect, value); break;
        case kParameterMid: fx_eq_set_mid(fEffect, value); break;
        case kParameterHigh: fx_eq_set_high(fEffect, value); break;
        }
    }

    void activate() override
    {
        if (fEffect) fx_eq_reset(fEffect);
    }

    void run(const float** inputs, float** outputs, uint32_t frames) override
    {
        RFX::processStereo(inputs, outputs, frames, fEffect,
                          fx_eq_process_f32, (int)getSampleRate());
    }

private:
    FXEqualizer* fEffect;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RFX_EQPlugin)
};

Plugin* createPlugin()
{
    return new RFX_EQPlugin();
}

END_NAMESPACE_DISTRHO
