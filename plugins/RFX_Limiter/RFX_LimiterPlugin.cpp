#include "DistrhoPlugin.hpp"
#include "fx_limiter.h"
#include "../rfx_plugin_utils.h"
#include <cstring>
#include <cstdio>

START_NAMESPACE_DISTRHO

class RFX_LimiterPlugin : public Plugin
{
public:
    RFX_LimiterPlugin()
        : Plugin(kParameterCount, 0, 4)  // 4 state values for explicit VST3 state save/restore
        , fThreshold(0.75f)    // -6dB
        , fRelease(0.2f)       // ~200ms
        , fCeiling(1.0f)       // 0dB
        , fLookahead(0.3f)     // ~3ms
    {
        fEffect = fx_limiter_create();
        fx_limiter_set_enabled(fEffect, true);
        // Initialize with default values
        fx_limiter_set_threshold(fEffect, fThreshold);
        fx_limiter_set_release(fEffect, fRelease);
        fx_limiter_set_ceiling(fEffect, fCeiling);
        fx_limiter_set_lookahead(fEffect, fLookahead);
    }

    ~RFX_LimiterPlugin() override
    {
        if (fEffect) fx_limiter_destroy(fEffect);
    }

protected:
    const char* getLabel() const override { return "RFX_Limiter"; }
    const char* getDescription() const override { return "Brick-wall limiter with lookahead"; }
    const char* getMaker() const override { return "Regroove"; }
    const char* getHomePage() const override { return "https://github.com/gbraad/rfx"; }
    const char* getLicense() const override { return "ISC"; }
    uint32_t getVersion() const override { return d_version(1, 0, 0); }
    int64_t getUniqueId() const override { return d_cconst('R', 'F', 'L', 'M'); }

    void initParameter(uint32_t index, Parameter& param) override
    {
        param.hints = kParameterIsAutomatable;
        param.ranges.min = fx_limiter_get_parameter_min(index);
        param.ranges.max = fx_limiter_get_parameter_max(index);
        param.ranges.def = fx_limiter_get_parameter_default(index);
        param.name = fx_limiter_get_parameter_name(index);
        param.symbol = param.name;
    }

    float getParameterValue(uint32_t index) const override
    {
        switch (index) {
        case kParameterThreshold: return fThreshold;
        case kParameterRelease: return fRelease;
        case kParameterCeiling: return fCeiling;
        case kParameterLookahead: return fLookahead;
        default: return 0.0f;
        }
    }

    void setParameterValue(uint32_t index, float value) override
    {
        // Store parameter value
        switch (index) {
        case kParameterThreshold: fThreshold = value; break;
        case kParameterRelease: fRelease = value; break;
        case kParameterCeiling: fCeiling = value; break;
        case kParameterLookahead: fLookahead = value; break;
        }

        // Apply to DSP engine using generic interface
        if (fEffect) {
            fx_limiter_set_parameter_value(fEffect, index, value);
        }
    }

    void initState(uint32_t index, State& state) override
    {
        switch (index) {
        case 0:
            state.key = "threshold";
            state.defaultValue = "0.75";
            break;
        case 1:
            state.key = "release";
            state.defaultValue = "0.2";
            break;
        case 2:
            state.key = "ceiling";
            state.defaultValue = "1.0";
            break;
        case 3:
            state.key = "lookahead";
            state.defaultValue = "0.3";
            break;
        }
        state.hints = kStateIsOnlyForDSP;
    }

    void setState(const char* key, const char* value) override
    {
        float fValue = std::atof(value);

        if (std::strcmp(key, "threshold") == 0) {
            fThreshold = fValue;
            if (fEffect) fx_limiter_set_threshold(fEffect, fThreshold);
        }
        else if (std::strcmp(key, "release") == 0) {
            fRelease = fValue;
            if (fEffect) fx_limiter_set_release(fEffect, fRelease);
        }
        else if (std::strcmp(key, "ceiling") == 0) {
            fCeiling = fValue;
            if (fEffect) fx_limiter_set_ceiling(fEffect, fCeiling);
        }
        else if (std::strcmp(key, "lookahead") == 0) {
            fLookahead = fValue;
            if (fEffect) fx_limiter_set_lookahead(fEffect, fLookahead);
        }
    }

    String getState(const char* key) const override
    {
        char buf[32];

        if (std::strcmp(key, "threshold") == 0) {
            std::snprintf(buf, sizeof(buf), "%.6f", fThreshold);
            return String(buf);
        }
        if (std::strcmp(key, "release") == 0) {
            std::snprintf(buf, sizeof(buf), "%.6f", fRelease);
            return String(buf);
        }
        if (std::strcmp(key, "ceiling") == 0) {
            std::snprintf(buf, sizeof(buf), "%.6f", fCeiling);
            return String(buf);
        }
        if (std::strcmp(key, "lookahead") == 0) {
            std::snprintf(buf, sizeof(buf), "%.6f", fLookahead);
            return String(buf);
        }

        return String("0.5");
    }

    void activate() override
    {
        if (fEffect) {
            fx_limiter_reset(fEffect);
            for (uint32_t i = 0; i < kParameterCount; ++i) {
                fx_limiter_set_parameter_value(fEffect, i, getParameterValue(i));
            }
        }
    }

    void run(const float** inputs, float** outputs, uint32_t frames) override
    {
        RFX::processStereo(inputs, outputs, frames, fEffect,
                          fx_limiter_process_f32, (int)getSampleRate());
    }

private:
    FXLimiter* fEffect;

    // Store parameters to persist across activate/deactivate
    float fThreshold;
    float fRelease;
    float fCeiling;
    float fLookahead;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RFX_LimiterPlugin)
};

Plugin* createPlugin()
{
    return new RFX_LimiterPlugin();
}

END_NAMESPACE_DISTRHO
