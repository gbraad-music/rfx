#include "DistrhoPlugin.hpp"
#include "fx_pitchshift.h"
#include "../rfx_plugin_utils.h"
#include <cstring>
#include <cstdio>

START_NAMESPACE_DISTRHO

class RFX_PitchShiftPlugin : public Plugin
{
public:
    RFX_PitchShiftPlugin()
        : Plugin(kParameterCount, 0, 3)  // 3 state values for explicit VST3 state save/restore
        , fPitch(0.5f)      // 0 semitones
        , fMix(1.0f)        // 100% wet
        , fFormant(0.5f)    // Neutral
    {
        fEffect = fx_pitchshift_create();
        fx_pitchshift_set_enabled(fEffect, true);
        // Initialize with default values
        fx_pitchshift_set_pitch(fEffect, fPitch);
        fx_pitchshift_set_mix(fEffect, fMix);
        fx_pitchshift_set_formant(fEffect, fFormant);
    }

    ~RFX_PitchShiftPlugin() override
    {
        if (fEffect) fx_pitchshift_destroy(fEffect);
    }

protected:
    const char* getLabel() const override { return "RFX_PitchShift"; }
    const char* getDescription() const override { return "Real-time pitch shifter"; }
    const char* getMaker() const override { return "Regroove"; }
    const char* getHomePage() const override { return "https://github.com/gbraad/rfx"; }
    const char* getLicense() const override { return "ISC"; }
    uint32_t getVersion() const override { return d_version(1, 0, 0); }
    int64_t getUniqueId() const override { return d_cconst('R', 'F', 'P', 'S'); }

    void initParameter(uint32_t index, Parameter& param) override
    {
        param.hints = kParameterIsAutomatable;
        param.ranges.min = 0.0f;
        param.ranges.max = 1.0f;
        param.ranges.def = 0.5f;

        switch (index) {
        case kParameterPitch:
            param.name = "Pitch";
            param.symbol = "pitch";
            param.ranges.def = 0.5f;  // 0 semitones
            break;
        case kParameterMix:
            param.name = "Mix";
            param.symbol = "mix";
            param.ranges.def = 1.0f;  // 100% wet
            break;
        case kParameterFormant:
            param.name = "Formant";
            param.symbol = "formant";
            param.ranges.def = 0.5f;  // Neutral
            break;
        }
    }

    float getParameterValue(uint32_t index) const override
    {
        switch (index) {
        case kParameterPitch: return fPitch;
        case kParameterMix: return fMix;
        case kParameterFormant: return fFormant;
        default: return 0.0f;
        }
    }

    void setParameterValue(uint32_t index, float value) override
    {
        // Store parameter value
        switch (index) {
        case kParameterPitch: fPitch = value; break;
        case kParameterMix: fMix = value; break;
        case kParameterFormant: fFormant = value; break;
        }

        // Apply to DSP engine if it exists
        if (fEffect) {
            switch (index) {
            case kParameterPitch: fx_pitchshift_set_pitch(fEffect, value); break;
            case kParameterMix: fx_pitchshift_set_mix(fEffect, value); break;
            case kParameterFormant: fx_pitchshift_set_formant(fEffect, value); break;
            }
        }
    }

    void initState(uint32_t index, State& state) override
    {
        switch (index) {
        case 0:
            state.key = "pitch";
            state.defaultValue = "0.5";
            break;
        case 1:
            state.key = "mix";
            state.defaultValue = "1.0";
            break;
        case 2:
            state.key = "formant";
            state.defaultValue = "0.5";
            break;
        }
        state.hints = kStateIsOnlyForDSP;
    }

    void setState(const char* key, const char* value) override
    {
        float fValue = std::atof(value);

        if (std::strcmp(key, "pitch") == 0) {
            fPitch = fValue;
            if (fEffect) fx_pitchshift_set_pitch(fEffect, fPitch);
        }
        else if (std::strcmp(key, "mix") == 0) {
            fMix = fValue;
            if (fEffect) fx_pitchshift_set_mix(fEffect, fMix);
        }
        else if (std::strcmp(key, "formant") == 0) {
            fFormant = fValue;
            if (fEffect) fx_pitchshift_set_formant(fEffect, fFormant);
        }
    }

    String getState(const char* key) const override
    {
        char buf[32];

        if (std::strcmp(key, "pitch") == 0) {
            std::snprintf(buf, sizeof(buf), "%.6f", fPitch);
            return String(buf);
        }
        if (std::strcmp(key, "mix") == 0) {
            std::snprintf(buf, sizeof(buf), "%.6f", fMix);
            return String(buf);
        }
        if (std::strcmp(key, "formant") == 0) {
            std::snprintf(buf, sizeof(buf), "%.6f", fFormant);
            return String(buf);
        }

        return String("0.5");
    }

    void activate() override
    {
        if (fEffect) {
            fx_pitchshift_reset(fEffect);
            // Restore parameters after reset
            fx_pitchshift_set_pitch(fEffect, fPitch);
            fx_pitchshift_set_mix(fEffect, fMix);
            fx_pitchshift_set_formant(fEffect, fFormant);
        }
    }

    void run(const float** inputs, float** outputs, uint32_t frames) override
    {
        RFX::processStereo(inputs, outputs, frames, fEffect,
                          fx_pitchshift_process_f32, (int)getSampleRate());
    }

private:
    FXPitchShift* fEffect;

    // Store parameters to persist across activate/deactivate
    float fPitch;
    float fMix;
    float fFormant;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RFX_PitchShiftPlugin)
};

Plugin* createPlugin()
{
    return new RFX_PitchShiftPlugin();
}

END_NAMESPACE_DISTRHO
