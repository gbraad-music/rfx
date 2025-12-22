/*
 * RM1_LPF Plugin - DPF
 * Copyright (C) 2024
 *
 * Based on the MODEL 1 Mixer Contour LPF
 */

#include "DistrhoPlugin.hpp"
#include "../../effects/fx_model1_lpf.h"

START_NAMESPACE_DISTRHO

class RM1_LPFPlugin : public Plugin
{
public:
    RM1_LPFPlugin()
        : Plugin(kParameterCount, 0, 1)
    {
        fx = fx_model1_lpf_create();
        fx_model1_lpf_reset(fx);
        fx_model1_lpf_set_enabled(fx, 1); // Always on
    }

    ~RM1_LPFPlugin()
    {
        if (fx) {
            fx_model1_lpf_destroy(fx);
        }
    }

protected:
    const char* getLabel() const override { return "RM1_LPF"; }
    const char* getDescription() const override { return "Low-pass filter based on the MODEL 1 mixer."; }
    const char* getMaker() const override { return "Regroove"; }
    const char* getHomePage() const override { return "https://github.com/gbraad/rfx"; }
    const char* getLicense() const override { return "MIT"; }
    uint32_t getVersion() const override { return d_version(1, 0, 0); }
    int64_t getUniqueId() const override { return d_cconst('R', 'M', '1', 'L'); }

    void initParameter(uint32_t index, Parameter& parameter) override
    {
        parameter.hints = kParameterIsAutomable;
        parameter.ranges.min = fx_model1_lpf_get_parameter_min(index);
        parameter.ranges.max = fx_model1_lpf_get_parameter_max(index);
        parameter.ranges.def = fx_model1_lpf_get_parameter_default(index);
        parameter.name = fx_model1_lpf_get_parameter_name(index);
        parameter.symbol = parameter.name;
    }

    float getParameterValue(uint32_t index) const override
    {
        if (index == kParameterCutoff) {
            return fx_model1_lpf_get_cutoff(fx);
        }
        return 0.0f;
    }

    void setParameterValue(uint32_t index, float value) override
    {
        fx_model1_lpf_set_parameter_value(fx, index, value);
    }

    void initState(uint32_t index, String& stateKey, String& defaultStateValue) override
    {
        if (index == 0) {
            stateKey = "cutoff";
            defaultStateValue = "1.0";
        }
    }

    String getState(const char* key) const override
    {
        if (strcmp(key, "cutoff") == 0) {
            return String(fx_model1_lpf_get_cutoff(fx));
        }
        return String();
    }

    void setState(const char* key, const char* value) override
    {
        if (strcmp(key, "cutoff") == 0) {
            float val;
            if (sscanf(value, "%f", &val) == 1) {
                fx_model1_lpf_set_cutoff(fx, val);
            }
        }
    }

    void run(const float** inputs, float** outputs, uint32_t frames) override
    {
        memcpy(outputs[0], inputs[0], frames * sizeof(float));
        memcpy(outputs[1], inputs[1], frames * sizeof(float));
        
        fx_model1_lpf_process_f32(fx, outputs[0], outputs[1], frames, getSampleRate());
    }

private:
    FXModel1LPF* fx;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RM1_LPFPlugin)
};

Plugin* createPlugin()
{
    return new RM1_LPFPlugin();
}

END_NAMESPACE_DISTRHO
