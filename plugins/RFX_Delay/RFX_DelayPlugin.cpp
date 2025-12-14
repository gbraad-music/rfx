#include "DistrhoPlugin.hpp"
#include "fx_delay.h"
#include "../rfx_plugin_utils.h"
#include <cstring>
#include <cstdio>

START_NAMESPACE_DISTRHO

class RFX_DelayPlugin : public Plugin
{
public:
    RFX_DelayPlugin()
        : Plugin(kParameterCount, 0, 3)  // 3 state values for explicit VST3 state save/restore
        , fTime(0.5f)
        , fFeedback(0.4f)
        , fMix(0.3f)
    {
        fEffect = fx_delay_create();
        fx_delay_set_enabled(fEffect, true);
        // Initialize with default values
        fx_delay_set_time(fEffect, fTime);
        fx_delay_set_feedback(fEffect, fFeedback);
        fx_delay_set_mix(fEffect, fMix);
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
        switch (index) {
        case kParameterTime: return fTime;
        case kParameterFeedback: return fFeedback;
        case kParameterMix: return fMix;
        default: return 0.0f;
        }
    }

    void setParameterValue(uint32_t index, float value) override
    {
        // Store parameter value
        switch (index) {
        case kParameterTime: fTime = value; break;
        case kParameterFeedback: fFeedback = value; break;
        case kParameterMix: fMix = value; break;
        }

        // Apply to DSP engine if it exists
        if (fEffect) {
            switch (index) {
            case kParameterTime: fx_delay_set_time(fEffect, value); break;
            case kParameterFeedback: fx_delay_set_feedback(fEffect, value); break;
            case kParameterMix: fx_delay_set_mix(fEffect, value); break;
            }
        }
    }

    void initState(uint32_t index, State& state) override
    {
        switch (index) {
        case 0:
            state.key = "time";
            state.defaultValue = "0.5";
            break;
        case 1:
            state.key = "feedback";
            state.defaultValue = "0.4";
            break;
        case 2:
            state.key = "mix";
            state.defaultValue = "0.3";
            break;
        }
        state.hints = kStateIsOnlyForDSP;
    }

    void setState(const char* key, const char* value) override
    {
        float fValue = std::atof(value);

        if (std::strcmp(key, "time") == 0) {
            fTime = fValue;
            if (fEffect) fx_delay_set_time(fEffect, fTime);
        }
        else if (std::strcmp(key, "feedback") == 0) {
            fFeedback = fValue;
            if (fEffect) fx_delay_set_feedback(fEffect, fFeedback);
        }
        else if (std::strcmp(key, "mix") == 0) {
            fMix = fValue;
            if (fEffect) fx_delay_set_mix(fEffect, fMix);
        }
    }

    String getState(const char* key) const override
    {
        char buf[32];

        if (std::strcmp(key, "time") == 0) {
            std::snprintf(buf, sizeof(buf), "%.6f", fTime);
            return String(buf);
        }
        if (std::strcmp(key, "feedback") == 0) {
            std::snprintf(buf, sizeof(buf), "%.6f", fFeedback);
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
            fx_delay_reset(fEffect);
            // Restore parameters after reset
            fx_delay_set_time(fEffect, fTime);
            fx_delay_set_feedback(fEffect, fFeedback);
            fx_delay_set_mix(fEffect, fMix);
        }
    }

    void run(const float** inputs, float** outputs, uint32_t frames) override
    {
        RFX::processStereo(inputs, outputs, frames, fEffect,
                          fx_delay_process_f32, (int)getSampleRate());
    }

private:
    FXDelay* fEffect;

    // Store parameters to persist across activate/deactivate
    float fTime;
    float fFeedback;
    float fMix;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RFX_DelayPlugin)
};

Plugin* createPlugin()
{
    return new RFX_DelayPlugin();
}

END_NAMESPACE_DISTRHO
