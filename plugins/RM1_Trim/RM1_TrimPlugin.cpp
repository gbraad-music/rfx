/*
 * RM1_Trim Plugin - DPF
 * Copyright (C) 2024
 *
 * Based on the MODEL 1 Mixer Trim/Drive
 */

#include "DistrhoPlugin.hpp"
#include "../../effects/fx_model1_trim.h"

START_NAMESPACE_DISTRHO

class RM1_TrimPlugin : public Plugin
{
public:
    RM1_TrimPlugin()
        : Plugin(kParameterCount, 0, 1) // 2 parameters (1 input, 1 output), 0 programs, 1 state (drive)
    {
        fx = fx_model1_trim_create();
        fx_model1_trim_reset(fx);
        fx_model1_trim_set_enabled(fx, 1);  // Enable by default
        fx_model1_trim_set_drive(fx, 0.7f); // Set default drive to unity gain
        fPeakLevel = 0.0f;
    }

    ~RM1_TrimPlugin()
    {
        if (fx) {
            fx_model1_trim_destroy(fx);
        }
    }

protected:
    const char* getLabel() const override
    {
        return "RM1_Trim";
    }

    const char* getDescription() const override
    {
        return "Analog-style trim/drive based on the MODEL 1 mixer.";
    }

    const char* getMaker() const override
    {
        return "Regroove";
    }

    const char* getHomePage() const override
    {
        return "https://github.com/gbraad/rfx";
    }

    const char* getLicense() const override
    {
        return "MIT";
    }

    uint32_t getVersion() const override
    {
        return d_version(1, 0, 0);
    }

    int64_t getUniqueId() const override
    {
        return d_cconst('R', 'M', '1', 'T');
    }

    void initParameter(uint32_t index, Parameter& parameter) override
    {
        switch (index)
        {
        case kParameterDrive:
            parameter.hints = kParameterIsAutomatable;
            parameter.name = "Drive";
            parameter.symbol = "drive";
            parameter.ranges.def = 0.7f;  // Unity gain at 70%
            parameter.ranges.min = 0.0f;
            parameter.ranges.max = 1.0f;
            break;
        case kParameterPeakLevel:
            parameter.hints = kParameterIsOutput;
            parameter.name = "Peak Level";
            parameter.symbol = "peak_level";
            parameter.ranges.def = 0.0f;
            parameter.ranges.min = 0.0f;
            parameter.ranges.max = 1.0f;
            break;
        }
    }

    float getParameterValue(uint32_t index) const override
    {
        switch (index)
        {
        case kParameterDrive:
            return fx_model1_trim_get_drive(fx);
        case kParameterPeakLevel:
            return fPeakLevel;
        }
        return 0.0f;
    }

    void setParameterValue(uint32_t index, float value) override
    {
        switch (index)
        {
        case kParameterDrive:
            fx_model1_trim_set_drive(fx, value);
            break;
        }
    }

    void initState(uint32_t index, String& stateKey, String& defaultStateValue) override
    {
        // State for drive level
        if (index == 0) {
            stateKey = "drive";
            defaultStateValue = "0.7";  // Unity gain at 70%
        }
    }

    String getState(const char* key) const override
    {
        if (strcmp(key, "drive") == 0) {
            return String(fx_model1_trim_get_drive(fx));
        }
        return String();
    }

    void setState(const char* key, const char* value) override
    {
        if (strcmp(key, "drive") == 0) {
            float drive_val;
            if (sscanf(value, "%f", &drive_val) == 1) {
                fx_model1_trim_set_drive(fx, drive_val);
            }
        }
    }

    void run(const float** inputs, float** outputs, uint32_t frames) override
    {
        // Copy input to output for in-place processing
        memcpy(outputs[0], inputs[0], frames * sizeof(float));
        memcpy(outputs[1], inputs[1], frames * sizeof(float));

        // Apply the drive effect
        fx_model1_trim_process_f32(fx, outputs[0], frames, getSampleRate());
        // Note: The original `fx_model1_trim_process_f32` only processes one channel.
        // To make it stereo, we process both.
        fx_model1_trim_process_f32(fx, outputs[1], frames, getSampleRate());

        // Calculate peak level for LED indicator
        float peak = 0.0f;
        for (uint32_t i = 0; i < frames; ++i)
        {
            float absL = fabsf(outputs[0][i]);
            float absR = fabsf(outputs[1][i]);
            float sample_peak = (absL > absR) ? absL : absR;
            if (sample_peak > peak)
                peak = sample_peak;
        }

        // Peak hold with decay (smooth LED response)
        const float decay_rate = 0.95f;
        if (peak > fPeakLevel)
            fPeakLevel = peak;
        else
            fPeakLevel *= decay_rate;

        // Notify UI of peak level change
        setParameterValue(kParameterPeakLevel, fPeakLevel);
    }

private:
    FXModel1Trim* fx;
    float fPeakLevel;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RM1_TrimPlugin)
};

Plugin* createPlugin()
{
    return new RM1_TrimPlugin();
}

END_NAMESPACE_DISTRHO
