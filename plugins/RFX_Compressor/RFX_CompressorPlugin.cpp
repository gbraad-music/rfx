#include "DistrhoPlugin.hpp"
#include "fx_compressor.h"
#include "../rfx_plugin_utils.h"

START_NAMESPACE_DISTRHO

class RFX_CompressorPlugin : public Plugin
{
public:
    RFX_CompressorPlugin()
        : Plugin(kParameterCount, 0, 0)
    {
        fEffect = fx_compressor_create();
        fx_compressor_set_enabled(fEffect, true);
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
        param.ranges.min = 0.0f;
        param.ranges.max = 1.0f;
        param.ranges.def = 0.5f;

        switch (index) {
        case kParameterThreshold:
            param.name = "Threshold";
            param.symbol = "threshold";
            param.ranges.def = 0.4f;
            break;
        case kParameterRatio:
            param.name = "Ratio";
            param.symbol = "ratio";
            param.ranges.def = 0.4f;
            break;
        case kParameterAttack:
            param.name = "Attack";
            param.symbol = "attack";
            param.ranges.def = 0.05f;
            break;
        case kParameterRelease:
            param.name = "Release";
            param.symbol = "release";
            break;
        case kParameterMakeup:
            param.name = "Makeup";
            param.symbol = "makeup";
            param.ranges.def = 0.65f;
            break;
        }
    }

    float getParameterValue(uint32_t index) const override
    {
        if (!fEffect) return 0.0f;

        switch (index) {
        case kParameterThreshold: return fx_compressor_get_threshold(fEffect);
        case kParameterRatio: return fx_compressor_get_ratio(fEffect);
        case kParameterAttack: return fx_compressor_get_attack(fEffect);
        case kParameterRelease: return fx_compressor_get_release(fEffect);
        case kParameterMakeup: return fx_compressor_get_makeup(fEffect);
        default: return 0.0f;
        }
    }

    void setParameterValue(uint32_t index, float value) override
    {
        if (!fEffect) return;

        switch (index) {
        case kParameterThreshold: fx_compressor_set_threshold(fEffect, value); break;
        case kParameterRatio: fx_compressor_set_ratio(fEffect, value); break;
        case kParameterAttack: fx_compressor_set_attack(fEffect, value); break;
        case kParameterRelease: fx_compressor_set_release(fEffect, value); break;
        case kParameterMakeup: fx_compressor_set_makeup(fEffect, value); break;
        }
    }

    void activate() override
    {
        if (fEffect) fx_compressor_reset(fEffect);
    }

    void run(const float** inputs, float** outputs, uint32_t frames) override
    {
        RFX::processStereo(inputs, outputs, frames, fEffect,
                          fx_compressor_process_f32, (int)getSampleRate());
    }

private:
    FXCompressor* fEffect;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RFX_CompressorPlugin)
};

Plugin* createPlugin()
{
    return new RFX_CompressorPlugin();
}

END_NAMESPACE_DISTRHO
