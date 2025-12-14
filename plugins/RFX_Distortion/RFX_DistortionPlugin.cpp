#include "DistrhoPlugin.hpp"
#include "fx_distortion.h"
#include "../rfx_plugin_utils.h"

START_NAMESPACE_DISTRHO

class RFX_DistortionPlugin : public Plugin
{
public:
    RFX_DistortionPlugin()
        : Plugin(kParameterCount, 0, 0)
    {
        fEffect = fx_distortion_create();
        fx_distortion_set_enabled(fEffect, true);
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
        param.ranges.min = 0.0f;
        param.ranges.max = 1.0f;
        param.ranges.def = 0.5f;

        switch (index) {
        case kParameterDrive:
            param.name = "Drive";
            param.symbol = "drive";
            break;
        case kParameterMix:
            param.name = "Mix";
            param.symbol = "mix";
            break;
        }
    }

    float getParameterValue(uint32_t index) const override
    {
        if (!fEffect) return 0.0f;

        switch (index) {
        case kParameterDrive: return fx_distortion_get_drive(fEffect);
        case kParameterMix: return fx_distortion_get_mix(fEffect);
        default: return 0.0f;
        }
    }

    void setParameterValue(uint32_t index, float value) override
    {
        if (!fEffect) return;

        switch (index) {
        case kParameterDrive: fx_distortion_set_drive(fEffect, value); break;
        case kParameterMix: fx_distortion_set_mix(fEffect, value); break;
        }
    }

    void activate() override
    {
        if (fEffect) fx_distortion_reset(fEffect);
    }

    void run(const float** inputs, float** outputs, uint32_t frames) override
    {
        RFX::processStereo(inputs, outputs, frames, fEffect, 
                          fx_distortion_process_f32, (int)getSampleRate());
    }

private:
    FXDistortion* fEffect;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RFX_DistortionPlugin)
};

Plugin* createPlugin()
{
    return new RFX_DistortionPlugin();
}

END_NAMESPACE_DISTRHO
