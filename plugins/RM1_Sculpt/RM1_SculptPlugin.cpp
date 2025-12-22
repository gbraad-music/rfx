/*
 * RM1_Sculpt Plugin - DPF
 * Copyright (C) 2024
 *
 * Based on the MODEL 1 Mixer Sculpt EQ
 */

#include "DistrhoPlugin.hpp"
#include "../../effects/fx_model1_sculpt.h"

START_NAMESPACE_DISTRHO

class RM1_SculptPlugin : public Plugin
{
public:
    RM1_SculptPlugin()
        : Plugin(kParameterCount, 0, 1)
    {
        fx = fx_model1_sculpt_create();
        fx_model1_sculpt_reset(fx);
        fx_model1_sculpt_set_enabled(fx, 1); // Always on
    }

    ~RM1_SculptPlugin()
    {
        if (fx) {
            fx_model1_sculpt_destroy(fx);
        }
    }

protected:
    const char* getLabel() const override { return "RM1_Sculpt"; }
    const char* getDescription() const override { return "Semi-parametric EQ based on the MODEL 1 mixer."; }
    const char* getMaker() const override { return "Regroove"; }
    const char* getHomePage() const override { return "https://github.com/gbraad/rfx"; }
    const char* getLicense() const override { return "MIT"; }
    uint32_t getVersion() const override { return d_version(1, 0, 0); }
    int64_t getUniqueId() const override { return d_cconst('R', 'M', '1', 'S'); }

    void initParameter(uint32_t index, Parameter& parameter) override
    {
        parameter.hints = kParameterIsAutomable;
        parameter.ranges.min = fx_model1_sculpt_get_parameter_min(index);
        parameter.ranges.max = fx_model1_sculpt_get_parameter_max(index);
        parameter.ranges.def = fx_model1_sculpt_get_parameter_default(index);
        parameter.name = fx_model1_sculpt_get_parameter_name(index);
        parameter.symbol = parameter.name;
    }

    float getParameterValue(uint32_t index) const override
    {
        return fx_model1_sculpt_get_parameter_value((FXModel1Sculpt*)fx, index);
    }

    void setParameterValue(uint32_t index, float value) override
    {
        fx_model1_sculpt_set_parameter_value(fx, index, value);
    }

    void initState(uint32_t index, String& stateKey, String& defaultStateValue) override
    {
        switch (index) {
            case 0: stateKey = "freq"; defaultStateValue = "0.5"; break;
            case 1: stateKey = "gain"; defaultStateValue = "0.5"; break;
        }
    }

    String getState(const char* key) const override
    {
        if (strcmp(key, "freq") == 0) return String(fx_model1_sculpt_get_frequency(fx));
        if (strcmp(key, "gain") == 0) return String(fx_model1_sculpt_get_gain(fx));
        return String();
    }

    void setState(const char* key, const char* value) override
    {
        float val;
        if (sscanf(value, "%f", &val) != 1) return;

        if (strcmp(key, "freq") == 0) fx_model1_sculpt_set_frequency(fx, val);
        else if (strcmp(key, "gain") == 0) fx_model1_sculpt_set_gain(fx, val);
    }

    void run(const float** inputs, float** outputs, uint32_t frames) override
    {
        memcpy(outputs[0], inputs[0], frames * sizeof(float));
        memcpy(outputs[1], inputs[1], frames * sizeof(float));
        
        fx_model1_sculpt_process_f32(fx, outputs[0], outputs[1], frames, getSampleRate());
    }

private:
    FXModel1Sculpt* fx;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RM1_SculptPlugin)
};

Plugin* createPlugin()
{
    return new RM1_SculptPlugin();
}

END_NAMESPACE_DISTRHO
