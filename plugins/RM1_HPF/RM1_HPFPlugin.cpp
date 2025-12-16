/*
 * RM1_HPF Plugin - DPF
 * Copyright (C) 2024
 *
 * Based on the MODEL 1 Mixer Contour HPF
 */

#include "DistrhoPlugin.hpp"
#include "../../effects/fx_model1_hpf.h"

START_NAMESPACE_DISTRHO

class RM1_HPFPlugin : public Plugin
{
public:
    RM1_HPFPlugin()
        : Plugin(kParameterCount, 0, 1)
    {
        fx = fx_model1_hpf_create();
        fx_model1_hpf_reset(fx);
        fx_model1_hpf_set_enabled(fx, 1); // Always on
    }

    ~RM1_HPFPlugin()
    {
        if (fx) {
            fx_model1_hpf_destroy(fx);
        }
    }

protected:
    const char* getLabel() const override { return "RM1_HPF"; }
    const char* getDescription() const override { return "High-pass filter based on the MODEL 1 mixer."; }
    const char* getMaker() const override { return "Regroove"; }
    const char* getHomePage() const override { return "https://github.com/gbraad/rfx"; }
    const char* getLicense() const override { return "MIT"; }
    uint32_t getVersion() const override { return d_version(1, 0, 0); }
    int64_t getUniqueId() const override { return d_cconst('R', 'M', '1', 'H'); }

    void initParameter(uint32_t index, Parameter& parameter) override
    {
        if (index == kParameterCutoff) {
            parameter.hints = kParameterIsAutomable;
            parameter.name = "Cutoff";
            parameter.symbol = "cutoff";
            parameter.ranges.def = 0.0f;
            parameter.ranges.min = 0.0f;
            parameter.ranges.max = 1.0f;
        }
    }

    float getParameterValue(uint32_t index) const override
    {
        if (index == kParameterCutoff) {
            return fx_model1_hpf_get_cutoff(fx);
        }
        return 0.0f;
    }

    void setParameterValue(uint32_t index, float value) override
    {
        if (index == kParameterCutoff) {
            fx_model1_hpf_set_cutoff(fx, value);
        }
    }

    void initState(uint32_t index, String& stateKey, String& defaultStateValue) override
    {
        if (index == 0) {
            stateKey = "cutoff";
            defaultStateValue = "0.0";
        }
    }

    String getState(const char* key) const override
    {
        if (strcmp(key, "cutoff") == 0) {
            return String(fx_model1_hpf_get_cutoff(fx));
        }
        return String();
    }

    void setState(const char* key, const char* value) override
    {
        if (strcmp(key, "cutoff") == 0) {
            float val;
            if (sscanf(value, "%f", &val) == 1) {
                fx_model1_hpf_set_cutoff(fx, val);
            }
        }
    }

    void run(const float** inputs, float** outputs, uint32_t frames) override
    {
        // Process in-place, so copy input to output first
        memcpy(outputs[0], inputs[0], frames * sizeof(float));
        memcpy(outputs[1], inputs[1], frames * sizeof(float));
        
        fx_model1_hpf_process_f32(fx, outputs[0], outputs[1], frames, getSampleRate());
    }

private:
    FXModel1HPF* fx;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RM1_HPFPlugin)
};

Plugin* createPlugin()
{
    return new RM1_HPFPlugin();
}

END_NAMESPACE_DISTRHO
