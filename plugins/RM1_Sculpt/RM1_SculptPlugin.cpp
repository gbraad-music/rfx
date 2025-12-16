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
        switch (index) {
            case kParameterFrequency:
                parameter.name = "Frequency";
                parameter.symbol = "freq";
                parameter.ranges.def = 0.5f;
                parameter.ranges.min = 0.0f;
                parameter.ranges.max = 1.0f;
                break;
            case kParameterGain:
                parameter.name = "Gain";
                parameter.symbol = "gain";
                parameter.ranges.def = 0.5f;
                parameter.ranges.min = 0.0f;
                parameter.ranges.max = 1.0f;
                break;
        }
    }

    float getParameterValue(uint32_t index) const override
    {
        switch (index) {
            case kParameterFrequency: return fx_model1_sculpt_get_frequency(fx);
            case kParameterGain: return fx_model1_sculpt_get_gain(fx);
        }
        return 0.0f;
    }

    void setParameterValue(uint32_t index, float value) override
    {
        switch (index) {
            case kParameterFrequency: fx_model1_sculpt_set_frequency(fx, value); break;
            case kParameterGain: fx_model1_sculpt_set_gain(fx, value); break;
        }
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
