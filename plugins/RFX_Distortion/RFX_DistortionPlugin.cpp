#include "DistrhoPlugin.hpp"
#include "fx_distortion.h"
#include "../rfx_plugin_utils.h"
#include <cstring>
#include <cstdio>

START_NAMESPACE_DISTRHO

class RFX_DistortionPlugin : public Plugin
{
public:
    RFX_DistortionPlugin()
        : Plugin(kParameterCount, 0, 2)  // 2 state values for explicit VST3 state save/restore
        , fDrive(0.5f)
        , fMix(0.5f)
    {
        fEffect = fx_distortion_create();
        fx_distortion_set_enabled(fEffect, true);
        // Initialize with default values
        fx_distortion_set_drive(fEffect, fDrive);
        fx_distortion_set_mix(fEffect, fMix);
    }

    ~RFX_DistortionPlugin() override
    {
        if (fEffect) fx_distortion_destroy(fEffect);
    }

protected:
    const char* getLabel() const override { return "RFX_Distortion"; }
    const char* getDescription() const override { return "Distortion effect with drive and mix controls"; }
    const char* getMaker() const override { return "Regroove"; }
    const char* getHomePage() const override { return "https://github.com/gbraad/rfx"; }
    const char* getLicense() const override { return "ISC"; }
    uint32_t getVersion() const override { return d_version(1, 0, 0); }
    int64_t getUniqueId() const override { return d_cconst('R', 'F', 'D', 'S'); }

    void initParameter(uint32_t index, Parameter& param) override
    {
        param.hints = kParameterIsAutomatable;
        param.ranges.min = fx_distortion_get_parameter_min(index);
        param.ranges.max = fx_distortion_get_parameter_max(index);
        param.ranges.def = fx_distortion_get_parameter_default(index);
        param.name = fx_distortion_get_parameter_name(index);
        param.symbol = param.name;  // Use name as symbol
    }

    float getParameterValue(uint32_t index) const override
    {
        switch (index) {
        case kParameterDrive: return fDrive;
        case kParameterMix: return fMix;
        default: return 0.0f;
        }
    }

    void setParameterValue(uint32_t index, float value) override
    {
        // Store parameter value
        switch (index) {
        case kParameterDrive: fDrive = value; break;
        case kParameterMix: fMix = value; break;
        }

        // Apply to DSP engine using generic interface - NO SWITCH NEEDED!
        if (fEffect) {
            fx_distortion_set_parameter_value(fEffect, index, value);
        }
    }

    void initState(uint32_t index, State& state) override
    {
        switch (index) {
        case 0:
            state.key = "drive";
            state.defaultValue = "0.5";
            break;
        case 1:
            state.key = "mix";
            state.defaultValue = "0.5";
            break;
        }
        state.hints = kStateIsOnlyForDSP;
    }

    void setState(const char* key, const char* value) override
    {
        float fValue = std::atof(value);

        if (std::strcmp(key, "drive") == 0) {
            fDrive = fValue;
            if (fEffect) fx_distortion_set_drive(fEffect, fDrive);
        }
        else if (std::strcmp(key, "mix") == 0) {
            fMix = fValue;
            if (fEffect) fx_distortion_set_mix(fEffect, fMix);
        }
    }

    String getState(const char* key) const override
    {
        char buf[32];

        if (std::strcmp(key, "drive") == 0) {
            std::snprintf(buf, sizeof(buf), "%.6f", fDrive);
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
            fx_distortion_reset(fEffect);
            // Restore ALL parameters after reset using generic interface - NO HARDCODING!
            for (uint32_t i = 0; i < kParameterCount; ++i) {
                fx_distortion_set_parameter_value(fEffect, i, getParameterValue(i));
            }
        }
    }

    void run(const float** inputs, float** outputs, uint32_t frames) override
    {
        RFX::processStereo(inputs, outputs, frames, fEffect,
                          fx_distortion_process_f32, (int)getSampleRate());
    }

private:
    FXDistortion* fEffect;

    // Store parameters to persist across activate/deactivate
    float fDrive;
    float fMix;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RFX_DistortionPlugin)
};

Plugin* createPlugin()
{
    return new RFX_DistortionPlugin();
}

END_NAMESPACE_DISTRHO
