#include "DistrhoPlugin.hpp"
#include "fx_model1_lpf.h"
#include "fx_model1_hpf.h"
#include "fx_model1_sculpt.h"
#include <cstring>
#include <cstdio>

START_NAMESPACE_DISTRHO

class RegrooveM1Plugin : public Plugin
{
public:
    RegrooveM1Plugin()
        : Plugin(kParameterCount, 0, 4)  // 4 state values for VST3 state save/restore
        , fLPFCutoff(1.0f)      // Default: FLAT (wide open)
        , fHPFCutoff(0.0f)      // Default: FLAT (wide open)
        , fSculptFreq(0.5f)     // Default: center frequency
        , fSculptGain(0.5f)     // Default: 0dB (neutral)
    {
        fLPF = fx_model1_lpf_create();
        fHPF = fx_model1_hpf_create();
        fSculpt = fx_model1_sculpt_create();

        // Initialize with default values
        fx_model1_lpf_set_enabled(fLPF, true);
        fx_model1_lpf_set_cutoff(fLPF, fLPFCutoff);

        fx_model1_hpf_set_enabled(fHPF, true);
        fx_model1_hpf_set_cutoff(fHPF, fHPFCutoff);

        fx_model1_sculpt_set_enabled(fSculpt, true);
        fx_model1_sculpt_set_frequency(fSculpt, fSculptFreq);
        fx_model1_sculpt_set_gain(fSculpt, fSculptGain);
    }

    ~RegrooveM1Plugin() override
    {
        if (fLPF) fx_model1_lpf_destroy(fLPF);
        if (fHPF) fx_model1_hpf_destroy(fHPF);
        if (fSculpt) fx_model1_sculpt_destroy(fSculpt);
    }

protected:
    const char* getLabel() const override { return "RegrooveM1"; }
    const char* getDescription() const override { return "MODEL 1 Mixer Channel Strip: LPF, HPF, Sculpt"; }
    const char* getMaker() const override { return "Regroove"; }
    const char* getHomePage() const override { return "https://github.com/gbraad/rfx"; }
    const char* getLicense() const override { return "ISC"; }
    uint32_t getVersion() const override { return d_version(1, 0, 0); }
    int64_t getUniqueId() const override { return d_cconst('R', 'g', 'M', '1'); }

    void initParameter(uint32_t index, Parameter& param) override
    {
        param.hints = kParameterIsAutomatable;
        param.ranges.min = 0.0f;
        param.ranges.max = 1.0f;
        param.ranges.def = 0.5f;

        switch (index) {
        case kParameterLPFCutoff:
            param.name = "Contour LPF";
            param.symbol = "contour_lpf";
            param.ranges.def = 1.0f;  // Default: FLAT (right)
            break;
        case kParameterSculptFrequency:
            param.name = "Sculpt Frequency";
            param.symbol = "sculpt_freq";
            param.ranges.def = 0.5f;  // Default: center (1kHz)
            break;
        case kParameterSculptGain:
            param.name = "Sculpt Cut/Boost";
            param.symbol = "sculpt_gain";
            param.ranges.def = 0.5f;  // Default: 0dB (center)
            break;
        case kParameterHPFCutoff:
            param.name = "Contour HPF";
            param.symbol = "contour_hpf";
            param.ranges.def = 0.0f;  // Default: FLAT (left)
            break;
        }
    }

    float getParameterValue(uint32_t index) const override
    {
        switch (index) {
        case kParameterLPFCutoff: return fLPFCutoff;
        case kParameterSculptFrequency: return fSculptFreq;
        case kParameterSculptGain: return fSculptGain;
        case kParameterHPFCutoff: return fHPFCutoff;
        default: return 0.0f;
        }
    }

    void setParameterValue(uint32_t index, float value) override
    {
        // Store parameter value
        switch (index) {
        case kParameterLPFCutoff: fLPFCutoff = value; break;
        case kParameterSculptFrequency: fSculptFreq = value; break;
        case kParameterSculptGain: fSculptGain = value; break;
        case kParameterHPFCutoff: fHPFCutoff = value; break;
        }

        // Apply to DSP engines
        switch (index) {
        case kParameterLPFCutoff:
            if (fLPF) fx_model1_lpf_set_cutoff(fLPF, value);
            break;
        case kParameterSculptFrequency:
            if (fSculpt) fx_model1_sculpt_set_frequency(fSculpt, value);
            break;
        case kParameterSculptGain:
            if (fSculpt) fx_model1_sculpt_set_gain(fSculpt, value);
            break;
        case kParameterHPFCutoff:
            if (fHPF) fx_model1_hpf_set_cutoff(fHPF, value);
            break;
        }
    }

    void initState(uint32_t index, State& state) override
    {
        switch (index) {
        case 0: state.key = "lpf_cutoff"; state.defaultValue = "1.0"; break;
        case 1: state.key = "hpf_cutoff"; state.defaultValue = "0.0"; break;
        case 2: state.key = "sculpt_freq"; state.defaultValue = "0.5"; break;
        case 3: state.key = "sculpt_gain"; state.defaultValue = "0.5"; break;
        }
        state.hints = kStateIsOnlyForDSP;
    }

    void setState(const char* key, const char* value) override
    {
        float fValue = std::atof(value);

        if (std::strcmp(key, "lpf_cutoff") == 0) {
            fLPFCutoff = fValue;
            if (fLPF) fx_model1_lpf_set_cutoff(fLPF, fLPFCutoff);
        }
        else if (std::strcmp(key, "hpf_cutoff") == 0) {
            fHPFCutoff = fValue;
            if (fHPF) fx_model1_hpf_set_cutoff(fHPF, fHPFCutoff);
        }
        else if (std::strcmp(key, "sculpt_freq") == 0) {
            fSculptFreq = fValue;
            if (fSculpt) fx_model1_sculpt_set_frequency(fSculpt, fSculptFreq);
        }
        else if (std::strcmp(key, "sculpt_gain") == 0) {
            fSculptGain = fValue;
            if (fSculpt) fx_model1_sculpt_set_gain(fSculpt, fSculptGain);
        }
    }

    String getState(const char* key) const override
    {
        char buf[32];

        if (std::strcmp(key, "lpf_cutoff") == 0) {
            std::snprintf(buf, sizeof(buf), "%.6f", fLPFCutoff);
            return String(buf);
        }
        if (std::strcmp(key, "hpf_cutoff") == 0) {
            std::snprintf(buf, sizeof(buf), "%.6f", fHPFCutoff);
            return String(buf);
        }
        if (std::strcmp(key, "sculpt_freq") == 0) {
            std::snprintf(buf, sizeof(buf), "%.6f", fSculptFreq);
            return String(buf);
        }
        if (std::strcmp(key, "sculpt_gain") == 0) {
            std::snprintf(buf, sizeof(buf), "%.6f", fSculptGain);
            return String(buf);
        }

        return String("0.5");
    }

    void activate() override
    {
        if (fLPF) {
            fx_model1_lpf_reset(fLPF);
            fx_model1_lpf_set_cutoff(fLPF, fLPFCutoff);
        }
        if (fHPF) {
            fx_model1_hpf_reset(fHPF);
            fx_model1_hpf_set_cutoff(fHPF, fHPFCutoff);
        }
        if (fSculpt) {
            fx_model1_sculpt_reset(fSculpt);
            fx_model1_sculpt_set_frequency(fSculpt, fSculptFreq);
            fx_model1_sculpt_set_gain(fSculpt, fSculptGain);
        }
    }

    void run(const float** inputs, float** outputs, uint32_t frames) override
    {
        if (!fLPF || !fHPF || !fSculpt) {
            std::memcpy(outputs[0], inputs[0], sizeof(float) * frames);
            std::memcpy(outputs[1], inputs[1], sizeof(float) * frames);
            return;
        }

        // Create interleaved stereo buffer for processing
        float* buffer = new float[frames * 2];

        // Interleave input channels
        for (uint32_t i = 0; i < frames; i++) {
            buffer[i * 2]     = inputs[0][i];
            buffer[i * 2 + 1] = inputs[1][i];
        }

        // Process through effect chain: Sculpt -> LPF -> HPF (MODEL 1 mixer order)
        int sampleRate = (int)getSampleRate();

        // Process frame by frame for proper interleaving
        // Signal chain: Sculpt (EQ) first, then filters (like MODEL 1 mixer)
        for (uint32_t i = 0; i < frames; i++) {
            float* left = &buffer[i * 2];
            float* right = &buffer[i * 2 + 1];

            fx_model1_sculpt_process_frame(fSculpt, left, right, sampleRate);
            fx_model1_lpf_process_frame(fLPF, left, right, sampleRate);
            fx_model1_hpf_process_frame(fHPF, left, right, sampleRate);
        }

        // Deinterleave output channels
        for (uint32_t i = 0; i < frames; i++) {
            outputs[0][i] = buffer[i * 2];
            outputs[1][i] = buffer[i * 2 + 1];
        }

        delete[] buffer;
    }

private:
    FXModel1LPF* fLPF;
    FXModel1HPF* fHPF;
    FXModel1Sculpt* fSculpt;

    // Store parameters to persist across activate/deactivate
    float fLPFCutoff;
    float fHPFCutoff;
    float fSculptFreq;
    float fSculptGain;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RegrooveM1Plugin)
};

Plugin* createPlugin()
{
    return new RegrooveM1Plugin();
}

END_NAMESPACE_DISTRHO
