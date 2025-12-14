#include "DistrhoPlugin.hpp"
#include "fx_delay.h"
#include "../rfx_plugin_utils.h"

START_NAMESPACE_DISTRHO

class RFX_DelayPlugin : public Plugin
{
public:
    RFX_DelayPlugin()
        : Plugin(kParameterCount, 0, 0)
    {
        fEffect = fx_delay_create();
        fx_delay_set_enabled(fEffect, true);
    }

    ~RFX_DelayPlugin() override
    {
        if (fEffect) fx_delay_destroy(fEffect);
    }

protected:
    const char* getLabel() const override { return "RFX_Delay"; }
    const char* getDescription() const override { return "Stereo delay with feedback"; }
    const char* getMaker() const override { return "Regroove"; }
    const char* getHomePage() const override { return "https://github.com/gbraad/rfx"; }
    const char* getLicense() const override { return "ISC"; }
    uint32_t getVersion() const override { return d_version(1, 0, 0); }
    int64_t getUniqueId() const override { return d_cconst('R', 'F', 'D', 'L'); }

    void initParameter(uint32_t index, Parameter& param) override
    {
        param.hints = kParameterIsAutomatable;
        param.ranges.min = 0.0f;
        param.ranges.max = 1.0f;
        param.ranges.def = 0.5f;

        switch (index) {
        case kParameterTime:
            param.name = "Time";
            param.symbol = "time";
            break;
        case kParameterFeedback:
            param.name = "Feedback";
            param.symbol = "feedback";
            param.ranges.def = 0.4f;
            break;
        case kParameterMix:
            param.name = "Mix";
            param.symbol = "mix";
            param.ranges.def = 0.3f;
            break;
        }
    }

    float getParameterValue(uint32_t index) const override
    {
        if (!fEffect) return 0.0f;

        switch (index) {
        case kParameterTime: return fx_delay_get_time(fEffect);
        case kParameterFeedback: return fx_delay_get_feedback(fEffect);
        case kParameterMix: return fx_delay_get_mix(fEffect);
        default: return 0.0f;
        }
    }

    void setParameterValue(uint32_t index, float value) override
    {
        if (!fEffect) return;

        switch (index) {
        case kParameterTime: fx_delay_set_time(fEffect, value); break;
        case kParameterFeedback: fx_delay_set_feedback(fEffect, value); break;
        case kParameterMix: fx_delay_set_mix(fEffect, value); break;
        }
    }

    void activate() override
    {
        if (fEffect) fx_delay_reset(fEffect);
    }

    void run(const float** inputs, float** outputs, uint32_t frames) override
    {
        RFX::processStereo(inputs, outputs, frames, fEffect,
                          fx_delay_process_f32, (int)getSampleRate());
    }

private:
    FXDelay* fEffect;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RFX_DelayPlugin)
};

Plugin* createPlugin()
{
    return new RFX_DelayPlugin();
}

END_NAMESPACE_DISTRHO
