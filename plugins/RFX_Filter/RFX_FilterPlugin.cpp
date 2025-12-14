#include "DistrhoPlugin.hpp"
#include "fx_filter.h"
#include "../rfx_plugin_utils.h"

START_NAMESPACE_DISTRHO

class RFX_FilterPlugin : public Plugin
{
public:
    RFX_FilterPlugin()
        : Plugin(kParameterCount, 0, 0)
    {
        fEffect = fx_filter_create();
        fx_filter_set_enabled(fEffect, true);
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
        if (!fEffect) return 0.0f;

        switch (index) {
        case kParameterCutoff: return fx_filter_get_cutoff(fEffect);
        case kParameterResonance: return fx_filter_get_resonance(fEffect);
        default: return 0.0f;
        }
    }

    void setParameterValue(uint32_t index, float value) override
    {
        if (!fEffect) return;

        switch (index) {
        case kParameterCutoff: fx_filter_set_cutoff(fEffect, value); break;
        case kParameterResonance: fx_filter_set_resonance(fEffect, value); break;
        }
    }

    void activate() override
    {
        if (fEffect) fx_filter_reset(fEffect);
    }

    void run(const float** inputs, float** outputs, uint32_t frames) override
    {
        RFX::processStereo(inputs, outputs, frames, fEffect,
                          fx_filter_process_f32, (int)getSampleRate());
    }

private:
    FXFilter* fEffect;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RFX_FilterPlugin)
};

Plugin* createPlugin()
{
    return new RFX_FilterPlugin();
}

END_NAMESPACE_DISTRHO
