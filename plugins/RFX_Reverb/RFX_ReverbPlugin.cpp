#include "DistrhoPlugin.hpp"
#include "fx_reverb.h"
#include "../rfx_plugin_utils.h"
#include <cstring>
#include <cstdio>

START_NAMESPACE_DISTRHO

class RFX_ReverbPlugin : public Plugin
{
public:
    RFX_ReverbPlugin()
        : Plugin(kParameterCount, 0, 3)
        , fSize(0.5f)
        , fDamping(0.5f)
        , fMix(0.3f)
    {
        fEffect = fx_reverb_create();
        fx_reverb_set_enabled(fEffect, 1);
        fx_reverb_set_size(fEffect, fSize);
        fx_reverb_set_damping(fEffect, fDamping);
        fx_reverb_set_mix(fEffect, fMix);
    }

    ~RFX_ReverbPlugin() override
    {
        if (fEffect) fx_reverb_destroy(fEffect);
    }

protected:
    const char* getLabel() const override { return "RFX_Reverb"; }
    const char* getDescription() const override { return "Algorithmic reverb effect"; }
    const char* getMaker() const override { return "Regroove"; }
    const char* getHomePage() const override { return "https://github.com/gbraad/rfx"; }
    const char* getLicense() const override { return "ISC"; }
    uint32_t getVersion() const override { return d_version(1, 0, 0); }
    int64_t getUniqueId() const override { return d_cconst('R', 'F', 'R', 'V'); }

    void initParameter(uint32_t index, Parameter& param) override
    {
        param.hints = kParameterIsAutomatable;
        param.ranges.min = 0.0f;
        param.ranges.max = 1.0f;
        param.ranges.def = 0.5f;
        switch (index) {
        case kParameterSize:
            param.name = "Size";
            param.symbol = "size";
            param.ranges.def = 0.5f;
            break;
        case kParameterDamping:
            param.name = "Damping";
            param.symbol = "damping";
            param.ranges.def = 0.5f;
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
        case kParameterSize: return fSize;
        case kParameterDamping: return fDamping;
        case kParameterMix: return fMix;
        default: return 0.0f;
        }
    }

    void setParameterValue(uint32_t index, float value) override
    {
        switch (index) {
        case kParameterSize: fSize = value; break;
        case kParameterDamping: fDamping = value; break;
        case kParameterMix: fMix = value; break;
        }
        if (fEffect) {
            switch (index) {
            case kParameterSize: fx_reverb_set_size(fEffect, value); break;
            case kParameterDamping: fx_reverb_set_damping(fEffect, value); break;
            case kParameterMix: fx_reverb_set_mix(fEffect, value); break;
            }
        }
    }

    void initState(uint32_t index, State& state) override
    {
        switch (index) {
        case 0:
            state.key = "size";
            state.defaultValue = "0.5";
            break;
        case 1:
            state.key = "damping";
            state.defaultValue = "0.5";
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
        const float fValue = std::atof(value);
        if (std::strcmp(key, "size") == 0) {
            fSize = fValue;
            if (fEffect) fx_reverb_set_size(fEffect, fSize);
        } else if (std::strcmp(key, "damping") == 0) {
            fDamping = fValue;
            if (fEffect) fx_reverb_set_damping(fEffect, fDamping);
        } else if (std::strcmp(key, "mix") == 0) {
            fMix = fValue;
            if (fEffect) fx_reverb_set_mix(fEffect, fMix);
        }
    }

    String getState(const char* key) const override
    {
        char buf[32];
        if (std::strcmp(key, "size") == 0) {
            std::snprintf(buf, sizeof(buf), "%.6f", fSize);
            return String(buf);
        }
        if (std::strcmp(key, "damping") == 0) {
            std::snprintf(buf, sizeof(buf), "%.6f", fDamping);
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
            fx_reverb_reset(fEffect);
            fx_reverb_set_size(fEffect, fSize);
            fx_reverb_set_damping(fEffect, fDamping);
            fx_reverb_set_mix(fEffect, fMix);
        }
    }

    void run(const float** inputs, float** outputs, uint32_t frames) override
    {
        RFX::processStereo(inputs, outputs, frames, fEffect,
                           fx_reverb_process_f32, (int)getSampleRate());
    }

private:
    FXReverb* fEffect;
    float fSize;
    float fDamping;
    float fMix;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RFX_ReverbPlugin)
};

Plugin* createPlugin()
{ return new RFX_ReverbPlugin(); }

END_NAMESPACE_DISTRHO
