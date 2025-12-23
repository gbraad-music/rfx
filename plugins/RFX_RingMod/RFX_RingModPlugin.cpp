#include "DistrhoPlugin.hpp"
#include "fx_ring_mod.h"
#include "../rfx_plugin_utils.h"
#include <cstring>
#include <cstdio>

START_NAMESPACE_DISTRHO

class RFX_RingModPlugin : public Plugin
{
public:
    RFX_RingModPlugin()
        : Plugin(kParameterCount, 0, 2)  // 2 state values for explicit VST3 state save/restore
        , fFrequency(0.1f)
        , fMix(1.0f)
    {
        fEffect = fx_ring_mod_create();
        fx_ring_mod_set_enabled(fEffect, true);
        // Initialize with default values
        fx_ring_mod_set_frequency(fEffect, fFrequency);
        fx_ring_mod_set_mix(fEffect, fMix);
    }

    ~RFX_RingModPlugin() override
    {
        if (fEffect) fx_ring_mod_destroy(fEffect);
    }

protected:
    const char* getLabel() const override { return "RFX_RingMod"; }
    const char* getDescription() const override { return "Ring modulator with internal carrier oscillator"; }
    const char* getMaker() const override { return "Regroove"; }
    const char* getHomePage() const override { return "https://github.com/gbraad/rfx"; }
    const char* getLicense() const override { return "ISC"; }
    uint32_t getVersion() const override { return d_version(1, 0, 0); }
    int64_t getUniqueId() const override { return d_cconst('R', 'F', 'R', 'M'); }

    void initParameter(uint32_t index, Parameter& param) override
    {
        param.hints = kParameterIsAutomatable;
        param.ranges.min = fx_ring_mod_get_parameter_min(index);
        param.ranges.max = fx_ring_mod_get_parameter_max(index);
        param.ranges.def = fx_ring_mod_get_parameter_default(index);
        param.name = fx_ring_mod_get_parameter_name(index);
        param.symbol = param.name;  // Use name as symbol
    }

    float getParameterValue(uint32_t index) const override
    {
        switch (index) {
        case kParameterFrequency: return fFrequency;
        case kParameterMix: return fMix;
        default: return 0.0f;
        }
    }

    void setParameterValue(uint32_t index, float value) override
    {
        // Store parameter value
        switch (index) {
        case kParameterFrequency: fFrequency = value; break;
        case kParameterMix: fMix = value; break;
        }

        // Apply to DSP engine using generic interface
        if (fEffect) {
            fx_ring_mod_set_parameter(fEffect, index, value);
        }
    }

    void initState(uint32_t index, State& state) override
    {
        switch (index) {
        case 0:
            state.key = "frequency";
            state.defaultValue = "0.1";
            break;
        case 1:
            state.key = "mix";
            state.defaultValue = "1.0";
            break;
        }
        state.hints = kStateIsOnlyForDSP;
    }

    void setState(const char* key, const char* value) override
    {
        float fValue = std::atof(value);

        if (std::strcmp(key, "frequency") == 0) {
            fFrequency = fValue;
            if (fEffect) fx_ring_mod_set_frequency(fEffect, fFrequency);
        }
        else if (std::strcmp(key, "mix") == 0) {
            fMix = fValue;
            if (fEffect) fx_ring_mod_set_mix(fEffect, fMix);
        }
    }

    String getState(const char* key) const override
    {
        char buf[32];

        if (std::strcmp(key, "frequency") == 0) {
            std::snprintf(buf, sizeof(buf), "%.6f", fFrequency);
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
            fx_ring_mod_reset(fEffect);
            // Restore parameters after reset
            for (uint32_t i = 0; i < kParameterCount; ++i) {
                fx_ring_mod_set_parameter(fEffect, i, getParameterValue(i));
            }
        }
    }

    void run(const float** inputs, float** outputs, uint32_t frames) override
    {
        if (!fEffect) {
            // Passthrough if no effect
            std::memcpy(outputs[0], inputs[0], sizeof(float) * frames);
            std::memcpy(outputs[1], inputs[1], sizeof(float) * frames);
            return;
        }

        // Copy input to output first (since effect processes in-place)
        std::memcpy(outputs[0], inputs[0], sizeof(float) * frames);
        std::memcpy(outputs[1], inputs[1], sizeof(float) * frames);

        // Interleave for processing
        float* interleavedBuffer = (float*)std::malloc(frames * 2 * sizeof(float));
        if (!interleavedBuffer) return;

        for (uint32_t i = 0; i < frames; ++i) {
            interleavedBuffer[i * 2] = outputs[0][i];
            interleavedBuffer[i * 2 + 1] = outputs[1][i];
        }

        // Process
        fx_ring_mod_process_f32(fEffect, interleavedBuffer, frames, getSampleRate());

        // De-interleave
        for (uint32_t i = 0; i < frames; ++i) {
            outputs[0][i] = interleavedBuffer[i * 2];
            outputs[1][i] = interleavedBuffer[i * 2 + 1];
        }

        std::free(interleavedBuffer);
    }

private:
    FXRingMod* fEffect;
    float fFrequency;
    float fMix;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RFX_RingModPlugin)
};

Plugin* createPlugin()
{
    return new RFX_RingModPlugin();
}

END_NAMESPACE_DISTRHO
