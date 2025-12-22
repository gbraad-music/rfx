#include "DistrhoPlugin.hpp"
#include "fx_compressor.h"
#include "../rfx_plugin_utils.h"
#include <cstring>
#include <cstdio>

START_NAMESPACE_DISTRHO

class RFX_CompressorPlugin : public Plugin
{
public:
    RFX_CompressorPlugin()
        : Plugin(kParameterCount, 0, 5)  // 5 state values for explicit VST3 state save/restore
        , fThreshold(0.4f)
        , fRatio(0.5f)
        , fAttack(0.5f)
        , fRelease(0.5f)
        , fMakeup(0.5f)
    {
        fEffect = fx_compressor_create();
        fx_compressor_set_enabled(fEffect, true);
        // Initialize with default values
        fx_compressor_set_threshold(fEffect, fThreshold);
        fx_compressor_set_ratio(fEffect, fRatio);
        fx_compressor_set_attack(fEffect, fAttack);
        fx_compressor_set_release(fEffect, fRelease);
        fx_compressor_set_makeup(fEffect, fMakeup);
    }

    ~RFX_CompressorPlugin() override
    {
        if (fEffect) fx_compressor_destroy(fEffect);
    }

protected:
    const char* getLabel() const override { return "RFX_Compressor"; }
    const char* getDescription() const override { return "Dynamic range compressor"; }
    const char* getMaker() const override { return "Regroove"; }
    const char* getHomePage() const override { return "https://github.com/gbraad/rfx"; }
    const char* getLicense() const override { return "ISC"; }
    uint32_t getVersion() const override { return d_version(1, 0, 0); }
    int64_t getUniqueId() const override { return d_cconst('R', 'F', 'C', 'P'); }

    void initParameter(uint32_t index, Parameter& param) override
    {
        param.hints = kParameterIsAutomatable;
        param.ranges.min = fx_compressor_get_parameter_min(index);
        param.ranges.max = fx_compressor_get_parameter_max(index);
        param.ranges.def = fx_compressor_get_parameter_default(index);
        param.name = fx_compressor_get_parameter_name(index);
        param.symbol = param.name;
    }

    float getParameterValue(uint32_t index) const override
    {
        switch (index) {
        case kParameterThreshold: return fThreshold;
        case kParameterRatio: return fRatio;
        case kParameterAttack: return fAttack;
        case kParameterRelease: return fRelease;
        case kParameterMakeup: return fMakeup;
        default: return 0.0f;
        }
    }

    void setParameterValue(uint32_t index, float value) override
    {
        // Store parameter value
        switch (index) {
        case kParameterThreshold: fThreshold = value; break;
        case kParameterRatio: fRatio = value; break;
        case kParameterAttack: fAttack = value; break;
        case kParameterRelease: fRelease = value; break;
        case kParameterMakeup: fMakeup = value; break;
        }

        // Apply to DSP engine using generic interface
        if (fEffect) {
            fx_compressor_set_parameter_value(fEffect, index, value);
        }
    }

    void initState(uint32_t index, State& state) override
    {
        switch (index) {
        case 0:
            state.key = "threshold";
            state.defaultValue = "0.4";
            break;
        case 1:
            state.key = "ratio";
            state.defaultValue = "0.5";
            break;
        case 2:
            state.key = "attack";
            state.defaultValue = "0.5";
            break;
        case 3:
            state.key = "release";
            state.defaultValue = "0.5";
            break;
        case 4:
            state.key = "makeup";
            state.defaultValue = "0.5";
            break;
        }
        state.hints = kStateIsOnlyForDSP;
    }

    void setState(const char* key, const char* value) override
    {
        float fValue = std::atof(value);

        if (std::strcmp(key, "threshold") == 0) {
            fThreshold = fValue;
            if (fEffect) fx_compressor_set_threshold(fEffect, fThreshold);
        }
        else if (std::strcmp(key, "ratio") == 0) {
            fRatio = fValue;
            if (fEffect) fx_compressor_set_ratio(fEffect, fRatio);
        }
        else if (std::strcmp(key, "attack") == 0) {
            fAttack = fValue;
            if (fEffect) fx_compressor_set_attack(fEffect, fAttack);
        }
        else if (std::strcmp(key, "release") == 0) {
            fRelease = fValue;
            if (fEffect) fx_compressor_set_release(fEffect, fRelease);
        }
        else if (std::strcmp(key, "makeup") == 0) {
            fMakeup = fValue;
            if (fEffect) fx_compressor_set_makeup(fEffect, fMakeup);
        }
    }

    String getState(const char* key) const override
    {
        char buf[32];

        if (std::strcmp(key, "threshold") == 0) {
            std::snprintf(buf, sizeof(buf), "%.6f", fThreshold);
            return String(buf);
        }
        if (std::strcmp(key, "ratio") == 0) {
            std::snprintf(buf, sizeof(buf), "%.6f", fRatio);
            return String(buf);
        }
        if (std::strcmp(key, "attack") == 0) {
            std::snprintf(buf, sizeof(buf), "%.6f", fAttack);
            return String(buf);
        }
        if (std::strcmp(key, "release") == 0) {
            std::snprintf(buf, sizeof(buf), "%.6f", fRelease);
            return String(buf);
        }
        if (std::strcmp(key, "makeup") == 0) {
            std::snprintf(buf, sizeof(buf), "%.6f", fMakeup);
            return String(buf);
        }

        return String("0.5");
    }

    void activate() override
    {
        if (fEffect) {
            fx_compressor_reset(fEffect);
            for (uint32_t i = 0; i < kParameterCount; ++i) {
                fx_compressor_set_parameter_value(fEffect, i, getParameterValue(i));
            }
        }
    }

    void run(const float** inputs, float** outputs, uint32_t frames) override
    {
        RFX::processStereo(inputs, outputs, frames, fEffect,
                          fx_compressor_process_f32, (int)getSampleRate());
    }

private:
    FXCompressor* fEffect;

    // Store parameters to persist across activate/deactivate
    float fThreshold;
    float fRatio;
    float fAttack;
    float fRelease;
    float fMakeup;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RFX_CompressorPlugin)
};

Plugin* createPlugin()
{
    return new RFX_CompressorPlugin();
}

END_NAMESPACE_DISTRHO
