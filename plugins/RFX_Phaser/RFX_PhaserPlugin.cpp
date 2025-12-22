#include "DistrhoPlugin.hpp"
#include "fx_phaser.h"
#include "../rfx_plugin_utils.h"
#include <cstring>
#include <cstdio>

START_NAMESPACE_DISTRHO

class RFX_PhaserPlugin : public Plugin
{
public:
    RFX_PhaserPlugin()
        : Plugin(kParameterCount, 0, 3)
        , fRate(0.5f)
        , fDepth(0.5f)
        , fFeedback(0.5f)
    {
        fEffect = fx_phaser_create();
        fx_phaser_set_enabled(fEffect, 1);
        fx_phaser_set_rate(fEffect, fRate);
        fx_phaser_set_depth(fEffect, fDepth);
        fx_phaser_set_feedback(fEffect, fFeedback);
    }

    ~RFX_PhaserPlugin() override
    {
        if (fEffect) fx_phaser_destroy(fEffect);
    }

protected:
    const char* getLabel() const override { return "RFX_Phaser"; }
    const char* getDescription() const override { return "Phaser modulation effect"; }
    const char* getMaker() const override { return "Regroove"; }
    const char* getHomePage() const override { return "https://github.com/gbraad/rfx"; }
    const char* getLicense() const override { return "ISC"; }
    uint32_t getVersion() const override { return d_version(1, 0, 0); }
    int64_t getUniqueId() const override { return d_cconst('R', 'F', 'P', 'H'); }

    void initParameter(uint32_t index, Parameter& param) override
    {
        param.hints = kParameterIsAutomatable;
        param.ranges.min = fx_phaser_get_parameter_min(index);
        param.ranges.max = fx_phaser_get_parameter_max(index);
        param.ranges.def = fx_phaser_get_parameter_default(index);
        param.name = fx_phaser_get_parameter_name(index);
        param.symbol = param.name;
    }

    float getParameterValue(uint32_t index) const override
    {
        switch (index) {
        case kParameterRate: return fRate;
        case kParameterDepth: return fDepth;
        case kParameterFeedback: return fFeedback;
        default: return 0.0f;
        }
    }

    void setParameterValue(uint32_t index, float value) override
    {
        switch (index) {
        case kParameterRate: fRate = value; break;
        case kParameterDepth: fDepth = value; break;
        case kParameterFeedback: fFeedback = value; break;
        }
        if (fEffect) {
            switch (index) {
            case kParameterRate: fx_phaser_set_rate(fEffect, value); break;
            case kParameterDepth: fx_phaser_set_depth(fEffect, value); break;
            case kParameterFeedback: fx_phaser_set_feedback(fEffect, value); break;
            }
        }
    }

    void initState(uint32_t index, State& state) override
    {
        switch (index) {
        case 0:
            state.key = "rate";
            state.defaultValue = "0.5";
            break;
        case 1:
            state.key = "depth";
            state.defaultValue = "0.5";
            break;
        case 2:
            state.key = "feedback";
            state.defaultValue = "0.5";
            break;
        }
        state.hints = kStateIsOnlyForDSP;
    }

    void setState(const char* key, const char* value) override
    {
        const float fValue = std::atof(value);
        if (std::strcmp(key, "rate") == 0) {
            fRate = fValue;
            if (fEffect) fx_phaser_set_rate(fEffect, fRate);
        } else if (std::strcmp(key, "depth") == 0) {
            fDepth = fValue;
            if (fEffect) fx_phaser_set_depth(fEffect, fDepth);
        } else if (std::strcmp(key, "feedback") == 0) {
            fFeedback = fValue;
            if (fEffect) fx_phaser_set_feedback(fEffect, fFeedback);
        }
    }

    String getState(const char* key) const override
    {
        char buf[32];
        if (std::strcmp(key, "rate") == 0) {
            std::snprintf(buf, sizeof(buf), "%.6f", fRate);
            return String(buf);
        }
        if (std::strcmp(key, "depth") == 0) {
            std::snprintf(buf, sizeof(buf), "%.6f", fDepth);
            return String(buf);
        }
        if (std::strcmp(key, "feedback") == 0) {
            std::snprintf(buf, sizeof(buf), "%.6f", fFeedback);
            return String(buf);
        }
        return String("0.5");
    }

    void activate() override
    {
        if (fEffect) {
            fx_phaser_reset(fEffect);
            for (uint32_t i = 0; i < kParameterCount; ++i) {
                fx_phaser_set_parameter_value(fEffect, i, getParameterValue(i));
            }
        }
    }

    void run(const float** inputs, float** outputs, uint32_t frames) override
    {
        RFX::processStereo(inputs, outputs, frames, fEffect,
                           fx_phaser_process_f32, (int)getSampleRate());
    }

private:
    FXPhaser* fEffect;
    float fRate;
    float fDepth;
    float fFeedback;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RFX_PhaserPlugin)
};

Plugin* createPlugin()
{ return new RFX_PhaserPlugin(); }

END_NAMESPACE_DISTRHO
