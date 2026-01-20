#include "DistrhoPlugin.hpp"
#include "../../effects/fx_amiga_filter.h"
#include <cstring>
#include <cmath>

START_NAMESPACE_DISTRHO

class RFX_AmigaPlugin : public Plugin
{
public:
    RFX_AmigaPlugin()
        : Plugin(kParameterCount, 0, 0),  // 0 programs, 0 states
          fAmigaFilter(nullptr),
          fAmigaFilterType(0.0f),  // A500
          fAmigaFilterMix(1.0f)
    {
        // Create Amiga filter
        fAmigaFilter = fx_amiga_filter_create();

        if (fAmigaFilter) {
            fx_amiga_filter_set_enabled(fAmigaFilter, true);
        }
    }

    ~RFX_AmigaPlugin() override
    {
        if (fAmigaFilter) fx_amiga_filter_destroy(fAmigaFilter);
    }

protected:
    const char* getLabel() const override { return "RFX_AmigaFilter"; }
    const char* getDescription() const override { return "Amiga Paula hardware RC filter emulation"; }
    const char* getMaker() const override { return "Regroove"; }
    const char* getHomePage() const override { return "https://music.gbraad.nl"; }
    const char* getLicense() const override { return "ISC"; }
    uint32_t getVersion() const override { return d_version(1, 0, 0); }
    int64_t getUniqueId() const override { return d_cconst('R', 'G', 'A', 'M'); }

    void initParameter(uint32_t index, Parameter& param) override
    {
        param.hints = kParameterIsAutomatable;

        switch (index) {
            case kParameterAmigaFilterType:
                param.name = "Filter Type";
                param.symbol = "filter_type";
                param.hints |= kParameterIsInteger;
                param.ranges.min = 0.0f;
                param.ranges.max = 3.0f;
                param.ranges.def = 0.0f;  // A500
                param.enumValues.count = 4;
                param.enumValues.restrictedMode = true;
                {
                    static ParameterEnumerationValue enumValues[] = {
                        {0.0f, "A500 (4.9kHz)"},
                        {1.0f, "A500+LED (3.3kHz)"},
                        {2.0f, "A1200 (32kHz)"},
                        {3.0f, "A1200+LED (3.3kHz)"}
                    };
                    param.enumValues.values = enumValues;
                }
                break;
            case kParameterAmigaFilterMix:
                param.name = "Filter Mix";
                param.symbol = "filter_mix";
                param.ranges.min = 0.0f;
                param.ranges.max = 1.0f;
                param.ranges.def = 1.0f;
                param.unit = "%";
                break;
        }
    }

    float getParameterValue(uint32_t index) const override
    {
        switch (index) {
            case kParameterAmigaFilterType: return fAmigaFilterType;
            case kParameterAmigaFilterMix: return fAmigaFilterMix;
            default: return 0.0f;
        }
    }

    void setParameterValue(uint32_t index, float value) override
    {
        switch (index) {
            case kParameterAmigaFilterType:
                fAmigaFilterType = value;
                if (fAmigaFilter) fx_amiga_filter_set_type(fAmigaFilter, (AmigaFilterType)(int)value);
                break;
            case kParameterAmigaFilterMix:
                fAmigaFilterMix = value;
                if (fAmigaFilter) fx_amiga_filter_set_mix(fAmigaFilter, value);
                break;
        }
    }

    // State handling disabled for testing
    // void initState(uint32_t index, State& state) override { ... }
    // void setState(const char* key, const char* value) override { ... }

    void activate() override
    {
        if (fAmigaFilter) fx_amiga_filter_reset(fAmigaFilter);
    }

    void run(const float** inputs, float** outputs, uint32_t frames) override
    {
        const float* in0 = inputs[0];
        const float* in1 = inputs[1];
        float* out0 = outputs[0];
        float* out1 = outputs[1];

        // Copy input to output first
        std::memcpy(out0, in0, frames * sizeof(float));
        std::memcpy(out1, in1, frames * sizeof(float));

        // Process Amiga Filter
        if (fAmigaFilter) {
            const int sr = (int)getSampleRate();
            for (uint32_t i = 0; i < frames; ++i) {
                fx_amiga_filter_process_frame(fAmigaFilter, &out0[i], &out1[i], sr);
            }
        }
    }

private:
    FXAmigaFilter* fAmigaFilter;

    // Parameters
    float fAmigaFilterType;
    float fAmigaFilterMix;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RFX_AmigaPlugin)
};

Plugin* createPlugin()
{
    return new RFX_AmigaPlugin();
}

END_NAMESPACE_DISTRHO
