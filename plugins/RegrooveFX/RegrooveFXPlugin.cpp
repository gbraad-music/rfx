#include "DistrhoPlugin.hpp"
#include "fx_distortion.h"
#include "fx_filter.h"
#include "fx_eq.h"
#include "fx_compressor.h"
#include "fx_delay.h"
#include <cstring>
#include <cstdio>

START_NAMESPACE_DISTRHO

class RegrooveFXPlugin : public Plugin
{
public:
    RegrooveFXPlugin()
        : Plugin(kParameterCount, 0, 20)  // 20 state values for explicit VST3 state save/restore
        , fDistortionEnabled(false)
        , fDistortionDrive(0.5f)
        , fDistortionMix(0.5f)
        , fFilterEnabled(false)
        , fFilterCutoff(0.8f)
        , fFilterResonance(0.3f)
        , fEQEnabled(false)
        , fEQLow(0.5f)
        , fEQMid(0.5f)
        , fEQHigh(0.5f)
        , fCompressorEnabled(false)
        , fCompressorThreshold(0.4f)
        , fCompressorRatio(0.4f)
        , fCompressorAttack(0.05f)
        , fCompressorRelease(0.5f)
        , fCompressorMakeup(0.65f)
        , fDelayEnabled(false)
        , fDelayTime(0.5f)
        , fDelayFeedback(0.4f)
        , fDelayMix(0.3f)
    {
        // Create individual effect modules directly (like RegrooveM1)
        fDistortion = fx_distortion_create();
        fFilter = fx_filter_create();
        fEQ = fx_eq_create();
        fCompressor = fx_compressor_create();
        fDelay = fx_delay_create();

        // Initialize with default values
        if (fDistortion) {
            fx_distortion_set_enabled(fDistortion, fDistortionEnabled);
            fx_distortion_set_drive(fDistortion, fDistortionDrive);
            fx_distortion_set_mix(fDistortion, fDistortionMix);
        }
        if (fFilter) {
            fx_filter_set_enabled(fFilter, fFilterEnabled);
            fx_filter_set_cutoff(fFilter, fFilterCutoff);
            fx_filter_set_resonance(fFilter, fFilterResonance);
        }
        if (fEQ) {
            fx_eq_set_enabled(fEQ, fEQEnabled);
            fx_eq_set_low(fEQ, fEQLow);
            fx_eq_set_mid(fEQ, fEQMid);
            fx_eq_set_high(fEQ, fEQHigh);
        }
        if (fCompressor) {
            fx_compressor_set_enabled(fCompressor, fCompressorEnabled);
            fx_compressor_set_threshold(fCompressor, fCompressorThreshold);
            fx_compressor_set_ratio(fCompressor, fCompressorRatio);
            fx_compressor_set_attack(fCompressor, fCompressorAttack);
            fx_compressor_set_release(fCompressor, fCompressorRelease);
            fx_compressor_set_makeup(fCompressor, fCompressorMakeup);
        }
        if (fDelay) {
            fx_delay_set_enabled(fDelay, fDelayEnabled);
            fx_delay_set_time(fDelay, fDelayTime);
            fx_delay_set_feedback(fDelay, fDelayFeedback);
            fx_delay_set_mix(fDelay, fDelayMix);
        }
    }

    ~RegrooveFXPlugin() override
    {
        if (fDistortion) fx_distortion_destroy(fDistortion);
        if (fFilter) fx_filter_destroy(fFilter);
        if (fEQ) fx_eq_destroy(fEQ);
        if (fCompressor) fx_compressor_destroy(fCompressor);
        if (fDelay) fx_delay_destroy(fDelay);
    }

protected:
    const char* getLabel() const override { return "RegrooveFX"; }
    const char* getDescription() const override { return "DJ-style effects: Distortion, Filter, EQ, Compressor, Delay"; }
    const char* getMaker() const override { return "Regroove"; }
    const char* getHomePage() const override { return "https://github.com/gbraad/rfx"; }
    const char* getLicense() const override { return "ISC"; }
    uint32_t getVersion() const override { return d_version(1, 0, 0); }
    int64_t getUniqueId() const override { return d_cconst('R', 'g', 'F', 'X'); }

    void initParameter(uint32_t index, Parameter& param) override
    {
        param.hints = kParameterIsAutomatable;
        param.ranges.min = 0.0f;
        param.ranges.max = 1.0f;
        param.ranges.def = 0.5f;

        switch (index) {
        // Distortion
        case kParameterDistortionEnabled:
            param.name = "Distortion Enable";
            param.symbol = "dist_en";
            param.hints |= kParameterIsBoolean | kParameterIsInteger;
            param.ranges.def = 0.0f;
            break;
        case kParameterDistortionDrive:
            param.name = "Distortion Drive";
            param.symbol = "dist_drive";
            break;
        case kParameterDistortionMix:
            param.name = "Distortion Mix";
            param.symbol = "dist_mix";
            break;

        // Filter
        case kParameterFilterEnabled:
            param.name = "Filter Enable";
            param.symbol = "filt_en";
            param.hints |= kParameterIsBoolean | kParameterIsInteger;
            param.ranges.def = 0.0f;
            break;
        case kParameterFilterCutoff:
            param.name = "Filter Cutoff";
            param.symbol = "filt_cutoff";
            param.ranges.def = 0.8f;
            break;
        case kParameterFilterResonance:
            param.name = "Filter Resonance";
            param.symbol = "filt_res";
            param.ranges.def = 0.3f;
            break;

        // EQ
        case kParameterEQEnabled:
            param.name = "EQ Enable";
            param.symbol = "eq_en";
            param.hints |= kParameterIsBoolean | kParameterIsInteger;
            param.ranges.def = 0.0f;
            break;
        case kParameterEQLow:
            param.name = "EQ Low";
            param.symbol = "eq_low";
            break;
        case kParameterEQMid:
            param.name = "EQ Mid";
            param.symbol = "eq_mid";
            break;
        case kParameterEQHigh:
            param.name = "EQ High";
            param.symbol = "eq_high";
            break;

        // Compressor
        case kParameterCompressorEnabled:
            param.name = "Compressor Enable";
            param.symbol = "comp_en";
            param.hints |= kParameterIsBoolean | kParameterIsInteger;
            param.ranges.def = 0.0f;
            break;
        case kParameterCompressorThreshold:
            param.name = "Compressor Threshold";
            param.symbol = "comp_thresh";
            param.ranges.def = 0.4f;
            break;
        case kParameterCompressorRatio:
            param.name = "Compressor Ratio";
            param.symbol = "comp_ratio";
            param.ranges.def = 0.4f;
            break;
        case kParameterCompressorAttack:
            param.name = "Compressor Attack";
            param.symbol = "comp_attack";
            param.ranges.def = 0.05f;
            break;
        case kParameterCompressorRelease:
            param.name = "Compressor Release";
            param.symbol = "comp_release";
            break;
        case kParameterCompressorMakeup:
            param.name = "Compressor Makeup";
            param.symbol = "comp_makeup";
            param.ranges.def = 0.65f;
            break;

        // Delay
        case kParameterDelayEnabled:
            param.name = "Delay Enable";
            param.symbol = "delay_en";
            param.hints |= kParameterIsBoolean | kParameterIsInteger;
            param.ranges.def = 0.0f;
            break;
        case kParameterDelayTime:
            param.name = "Delay Time";
            param.symbol = "delay_time";
            break;
        case kParameterDelayFeedback:
            param.name = "Delay Feedback";
            param.symbol = "delay_fb";
            param.ranges.def = 0.4f;
            break;
        case kParameterDelayMix:
            param.name = "Delay Mix";
            param.symbol = "delay_mix";
            param.ranges.def = 0.3f;
            break;
        }
    }

    float getParameterValue(uint32_t index) const override
    {
        switch (index) {
        case kParameterDistortionEnabled: return fDistortionEnabled ? 1.0f : 0.0f;
        case kParameterDistortionDrive: return fDistortionDrive;
        case kParameterDistortionMix: return fDistortionMix;

        case kParameterFilterEnabled: return fFilterEnabled ? 1.0f : 0.0f;
        case kParameterFilterCutoff: return fFilterCutoff;
        case kParameterFilterResonance: return fFilterResonance;

        case kParameterEQEnabled: return fEQEnabled ? 1.0f : 0.0f;
        case kParameterEQLow: return fEQLow;
        case kParameterEQMid: return fEQMid;
        case kParameterEQHigh: return fEQHigh;

        case kParameterCompressorEnabled: return fCompressorEnabled ? 1.0f : 0.0f;
        case kParameterCompressorThreshold: return fCompressorThreshold;
        case kParameterCompressorRatio: return fCompressorRatio;
        case kParameterCompressorAttack: return fCompressorAttack;
        case kParameterCompressorRelease: return fCompressorRelease;
        case kParameterCompressorMakeup: return fCompressorMakeup;

        case kParameterDelayEnabled: return fDelayEnabled ? 1.0f : 0.0f;
        case kParameterDelayTime: return fDelayTime;
        case kParameterDelayFeedback: return fDelayFeedback;
        case kParameterDelayMix: return fDelayMix;

        default: return 0.0f;
        }
    }

    void setParameterValue(uint32_t index, float value) override
    {
        // Store parameter value
        switch (index) {
        case kParameterDistortionEnabled:
            fDistortionEnabled = (value >= 0.5f);
            if (fDistortion) fx_distortion_set_enabled(fDistortion, fDistortionEnabled);
            break;
        case kParameterDistortionDrive:
            fDistortionDrive = value;
            if (fDistortion) fx_distortion_set_drive(fDistortion, value);
            break;
        case kParameterDistortionMix:
            fDistortionMix = value;
            if (fDistortion) fx_distortion_set_mix(fDistortion, value);
            break;

        case kParameterFilterEnabled:
            fFilterEnabled = (value >= 0.5f);
            if (fFilter) fx_filter_set_enabled(fFilter, fFilterEnabled);
            break;
        case kParameterFilterCutoff:
            fFilterCutoff = value;
            if (fFilter) fx_filter_set_cutoff(fFilter, value);
            break;
        case kParameterFilterResonance:
            fFilterResonance = value;
            if (fFilter) fx_filter_set_resonance(fFilter, value);
            break;

        case kParameterEQEnabled:
            fEQEnabled = (value >= 0.5f);
            if (fEQ) fx_eq_set_enabled(fEQ, fEQEnabled);
            break;
        case kParameterEQLow:
            fEQLow = value;
            if (fEQ) fx_eq_set_low(fEQ, value);
            break;
        case kParameterEQMid:
            fEQMid = value;
            if (fEQ) fx_eq_set_mid(fEQ, value);
            break;
        case kParameterEQHigh:
            fEQHigh = value;
            if (fEQ) fx_eq_set_high(fEQ, value);
            break;

        case kParameterCompressorEnabled:
            fCompressorEnabled = (value >= 0.5f);
            if (fCompressor) fx_compressor_set_enabled(fCompressor, fCompressorEnabled);
            break;
        case kParameterCompressorThreshold:
            fCompressorThreshold = value;
            if (fCompressor) fx_compressor_set_threshold(fCompressor, value);
            break;
        case kParameterCompressorRatio:
            fCompressorRatio = value;
            if (fCompressor) fx_compressor_set_ratio(fCompressor, value);
            break;
        case kParameterCompressorAttack:
            fCompressorAttack = value;
            if (fCompressor) fx_compressor_set_attack(fCompressor, value);
            break;
        case kParameterCompressorRelease:
            fCompressorRelease = value;
            if (fCompressor) fx_compressor_set_release(fCompressor, value);
            break;
        case kParameterCompressorMakeup:
            fCompressorMakeup = value;
            if (fCompressor) fx_compressor_set_makeup(fCompressor, value);
            break;

        case kParameterDelayEnabled:
            fDelayEnabled = (value >= 0.5f);
            if (fDelay) fx_delay_set_enabled(fDelay, fDelayEnabled);
            break;
        case kParameterDelayTime:
            fDelayTime = value;
            if (fDelay) fx_delay_set_time(fDelay, value);
            break;
        case kParameterDelayFeedback:
            fDelayFeedback = value;
            if (fDelay) fx_delay_set_feedback(fDelay, value);
            break;
        case kParameterDelayMix:
            fDelayMix = value;
            if (fDelay) fx_delay_set_mix(fDelay, value);
            break;
        }
    }

    void initState(uint32_t index, State& state) override
    {
        switch (index) {
        case 0: state.key = "dist_en"; state.defaultValue = "0"; break;
        case 1: state.key = "dist_drive"; state.defaultValue = "0.5"; break;
        case 2: state.key = "dist_mix"; state.defaultValue = "0.5"; break;
        case 3: state.key = "filt_en"; state.defaultValue = "0"; break;
        case 4: state.key = "filt_cutoff"; state.defaultValue = "0.8"; break;
        case 5: state.key = "filt_res"; state.defaultValue = "0.3"; break;
        case 6: state.key = "eq_en"; state.defaultValue = "0"; break;
        case 7: state.key = "eq_low"; state.defaultValue = "0.5"; break;
        case 8: state.key = "eq_mid"; state.defaultValue = "0.5"; break;
        case 9: state.key = "eq_high"; state.defaultValue = "0.5"; break;
        case 10: state.key = "comp_en"; state.defaultValue = "0"; break;
        case 11: state.key = "comp_thresh"; state.defaultValue = "0.4"; break;
        case 12: state.key = "comp_ratio"; state.defaultValue = "0.4"; break;
        case 13: state.key = "comp_attack"; state.defaultValue = "0.05"; break;
        case 14: state.key = "comp_release"; state.defaultValue = "0.5"; break;
        case 15: state.key = "comp_makeup"; state.defaultValue = "0.65"; break;
        case 16: state.key = "delay_en"; state.defaultValue = "0"; break;
        case 17: state.key = "delay_time"; state.defaultValue = "0.5"; break;
        case 18: state.key = "delay_fb"; state.defaultValue = "0.4"; break;
        case 19: state.key = "delay_mix"; state.defaultValue = "0.3"; break;
        }
        state.hints = kStateIsOnlyForDSP;
    }

    void setState(const char* key, const char* value) override
    {
        float fValue = std::atof(value);

        if (std::strcmp(key, "dist_en") == 0) {
            fDistortionEnabled = (fValue >= 0.5f);
            if (fDistortion) fx_distortion_set_enabled(fDistortion, fDistortionEnabled);
        }
        else if (std::strcmp(key, "dist_drive") == 0) {
            fDistortionDrive = fValue;
            if (fDistortion) fx_distortion_set_drive(fDistortion, fDistortionDrive);
        }
        else if (std::strcmp(key, "dist_mix") == 0) {
            fDistortionMix = fValue;
            if (fDistortion) fx_distortion_set_mix(fDistortion, fDistortionMix);
        }
        else if (std::strcmp(key, "filt_en") == 0) {
            fFilterEnabled = (fValue >= 0.5f);
            if (fFilter) fx_filter_set_enabled(fFilter, fFilterEnabled);
        }
        else if (std::strcmp(key, "filt_cutoff") == 0) {
            fFilterCutoff = fValue;
            if (fFilter) fx_filter_set_cutoff(fFilter, fFilterCutoff);
        }
        else if (std::strcmp(key, "filt_res") == 0) {
            fFilterResonance = fValue;
            if (fFilter) fx_filter_set_resonance(fFilter, fFilterResonance);
        }
        else if (std::strcmp(key, "eq_en") == 0) {
            fEQEnabled = (fValue >= 0.5f);
            if (fEQ) fx_eq_set_enabled(fEQ, fEQEnabled);
        }
        else if (std::strcmp(key, "eq_low") == 0) {
            fEQLow = fValue;
            if (fEQ) fx_eq_set_low(fEQ, fEQLow);
        }
        else if (std::strcmp(key, "eq_mid") == 0) {
            fEQMid = fValue;
            if (fEQ) fx_eq_set_mid(fEQ, fEQMid);
        }
        else if (std::strcmp(key, "eq_high") == 0) {
            fEQHigh = fValue;
            if (fEQ) fx_eq_set_high(fEQ, fEQHigh);
        }
        else if (std::strcmp(key, "comp_en") == 0) {
            fCompressorEnabled = (fValue >= 0.5f);
            if (fCompressor) fx_compressor_set_enabled(fCompressor, fCompressorEnabled);
        }
        else if (std::strcmp(key, "comp_thresh") == 0) {
            fCompressorThreshold = fValue;
            if (fCompressor) fx_compressor_set_threshold(fCompressor, fCompressorThreshold);
        }
        else if (std::strcmp(key, "comp_ratio") == 0) {
            fCompressorRatio = fValue;
            if (fCompressor) fx_compressor_set_ratio(fCompressor, fCompressorRatio);
        }
        else if (std::strcmp(key, "comp_attack") == 0) {
            fCompressorAttack = fValue;
            if (fCompressor) fx_compressor_set_attack(fCompressor, fCompressorAttack);
        }
        else if (std::strcmp(key, "comp_release") == 0) {
            fCompressorRelease = fValue;
            if (fCompressor) fx_compressor_set_release(fCompressor, fCompressorRelease);
        }
        else if (std::strcmp(key, "comp_makeup") == 0) {
            fCompressorMakeup = fValue;
            if (fCompressor) fx_compressor_set_makeup(fCompressor, fCompressorMakeup);
        }
        else if (std::strcmp(key, "delay_en") == 0) {
            fDelayEnabled = (fValue >= 0.5f);
            if (fDelay) fx_delay_set_enabled(fDelay, fDelayEnabled);
        }
        else if (std::strcmp(key, "delay_time") == 0) {
            fDelayTime = fValue;
            if (fDelay) fx_delay_set_time(fDelay, fDelayTime);
        }
        else if (std::strcmp(key, "delay_fb") == 0) {
            fDelayFeedback = fValue;
            if (fDelay) fx_delay_set_feedback(fDelay, fDelayFeedback);
        }
        else if (std::strcmp(key, "delay_mix") == 0) {
            fDelayMix = fValue;
            if (fDelay) fx_delay_set_mix(fDelay, fDelayMix);
        }
    }

    String getState(const char* key) const override
    {
        char buf[32];

        if (std::strcmp(key, "dist_en") == 0) {
            std::snprintf(buf, sizeof(buf), "%d", fDistortionEnabled ? 1 : 0);
            return String(buf);
        }
        if (std::strcmp(key, "dist_drive") == 0) {
            std::snprintf(buf, sizeof(buf), "%.6f", fDistortionDrive);
            return String(buf);
        }
        if (std::strcmp(key, "dist_mix") == 0) {
            std::snprintf(buf, sizeof(buf), "%.6f", fDistortionMix);
            return String(buf);
        }
        if (std::strcmp(key, "filt_en") == 0) {
            std::snprintf(buf, sizeof(buf), "%d", fFilterEnabled ? 1 : 0);
            return String(buf);
        }
        if (std::strcmp(key, "filt_cutoff") == 0) {
            std::snprintf(buf, sizeof(buf), "%.6f", fFilterCutoff);
            return String(buf);
        }
        if (std::strcmp(key, "filt_res") == 0) {
            std::snprintf(buf, sizeof(buf), "%.6f", fFilterResonance);
            return String(buf);
        }
        if (std::strcmp(key, "eq_en") == 0) {
            std::snprintf(buf, sizeof(buf), "%d", fEQEnabled ? 1 : 0);
            return String(buf);
        }
        if (std::strcmp(key, "eq_low") == 0) {
            std::snprintf(buf, sizeof(buf), "%.6f", fEQLow);
            return String(buf);
        }
        if (std::strcmp(key, "eq_mid") == 0) {
            std::snprintf(buf, sizeof(buf), "%.6f", fEQMid);
            return String(buf);
        }
        if (std::strcmp(key, "eq_high") == 0) {
            std::snprintf(buf, sizeof(buf), "%.6f", fEQHigh);
            return String(buf);
        }
        if (std::strcmp(key, "comp_en") == 0) {
            std::snprintf(buf, sizeof(buf), "%d", fCompressorEnabled ? 1 : 0);
            return String(buf);
        }
        if (std::strcmp(key, "comp_thresh") == 0) {
            std::snprintf(buf, sizeof(buf), "%.6f", fCompressorThreshold);
            return String(buf);
        }
        if (std::strcmp(key, "comp_ratio") == 0) {
            std::snprintf(buf, sizeof(buf), "%.6f", fCompressorRatio);
            return String(buf);
        }
        if (std::strcmp(key, "comp_attack") == 0) {
            std::snprintf(buf, sizeof(buf), "%.6f", fCompressorAttack);
            return String(buf);
        }
        if (std::strcmp(key, "comp_release") == 0) {
            std::snprintf(buf, sizeof(buf), "%.6f", fCompressorRelease);
            return String(buf);
        }
        if (std::strcmp(key, "comp_makeup") == 0) {
            std::snprintf(buf, sizeof(buf), "%.6f", fCompressorMakeup);
            return String(buf);
        }
        if (std::strcmp(key, "delay_en") == 0) {
            std::snprintf(buf, sizeof(buf), "%d", fDelayEnabled ? 1 : 0);
            return String(buf);
        }
        if (std::strcmp(key, "delay_time") == 0) {
            std::snprintf(buf, sizeof(buf), "%.6f", fDelayTime);
            return String(buf);
        }
        if (std::strcmp(key, "delay_fb") == 0) {
            std::snprintf(buf, sizeof(buf), "%.6f", fDelayFeedback);
            return String(buf);
        }
        if (std::strcmp(key, "delay_mix") == 0) {
            std::snprintf(buf, sizeof(buf), "%.6f", fDelayMix);
            return String(buf);
        }

        return String("0");
    }

    void activate() override
    {
        // Reset all effects
        if (fDistortion) {
            fx_distortion_reset(fDistortion);
            fx_distortion_set_enabled(fDistortion, fDistortionEnabled);
            fx_distortion_set_drive(fDistortion, fDistortionDrive);
            fx_distortion_set_mix(fDistortion, fDistortionMix);
        }
        if (fFilter) {
            fx_filter_reset(fFilter);
            fx_filter_set_enabled(fFilter, fFilterEnabled);
            fx_filter_set_cutoff(fFilter, fFilterCutoff);
            fx_filter_set_resonance(fFilter, fFilterResonance);
        }
        if (fEQ) {
            fx_eq_reset(fEQ);
            fx_eq_set_enabled(fEQ, fEQEnabled);
            fx_eq_set_low(fEQ, fEQLow);
            fx_eq_set_mid(fEQ, fEQMid);
            fx_eq_set_high(fEQ, fEQHigh);
        }
        if (fCompressor) {
            fx_compressor_reset(fCompressor);
            fx_compressor_set_enabled(fCompressor, fCompressorEnabled);
            fx_compressor_set_threshold(fCompressor, fCompressorThreshold);
            fx_compressor_set_ratio(fCompressor, fCompressorRatio);
            fx_compressor_set_attack(fCompressor, fCompressorAttack);
            fx_compressor_set_release(fCompressor, fCompressorRelease);
            fx_compressor_set_makeup(fCompressor, fCompressorMakeup);
        }
        if (fDelay) {
            fx_delay_reset(fDelay);
            fx_delay_set_enabled(fDelay, fDelayEnabled);
            fx_delay_set_time(fDelay, fDelayTime);
            fx_delay_set_feedback(fDelay, fDelayFeedback);
            fx_delay_set_mix(fDelay, fDelayMix);
        }
    }

    void run(const float** inputs, float** outputs, uint32_t frames) override
    {
        // Create interleaved stereo buffer for processing
        float* buffer = new float[frames * 2];

        // Interleave input channels
        for (uint32_t i = 0; i < frames; i++) {
            buffer[i * 2]     = inputs[0][i];
            buffer[i * 2 + 1] = inputs[1][i];
        }

        int sample_rate = (int)getSampleRate();

        // Process through effect chain in order (like regroove_effects.c)
        if (fDistortion) {
            fx_distortion_process_f32(fDistortion, buffer, frames, sample_rate);
        }
        if (fFilter) {
            fx_filter_process_f32(fFilter, buffer, frames, sample_rate);
        }
        if (fEQ) {
            fx_eq_process_f32(fEQ, buffer, frames, sample_rate);
        }
        if (fCompressor) {
            fx_compressor_process_f32(fCompressor, buffer, frames, sample_rate);
        }
        if (fDelay) {
            fx_delay_process_f32(fDelay, buffer, frames, sample_rate);
        }

        // Deinterleave output channels
        for (uint32_t i = 0; i < frames; i++) {
            outputs[0][i] = buffer[i * 2];
            outputs[1][i] = buffer[i * 2 + 1];
        }

        delete[] buffer;
    }

private:
    // Individual effect modules (like RegrooveM1)
    FXDistortion* fDistortion;
    FXFilter* fFilter;
    FXEqualizer* fEQ;
    FXCompressor* fCompressor;
    FXDelay* fDelay;

    // Store parameters to persist across activate/deactivate
    bool fDistortionEnabled;
    float fDistortionDrive;
    float fDistortionMix;

    bool fFilterEnabled;
    float fFilterCutoff;
    float fFilterResonance;

    bool fEQEnabled;
    float fEQLow;
    float fEQMid;
    float fEQHigh;

    bool fCompressorEnabled;
    float fCompressorThreshold;
    float fCompressorRatio;
    float fCompressorAttack;
    float fCompressorRelease;
    float fCompressorMakeup;

    bool fDelayEnabled;
    float fDelayTime;
    float fDelayFeedback;
    float fDelayMix;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RegrooveFXPlugin)
};

Plugin* createPlugin()
{
    return new RegrooveFXPlugin();
}

END_NAMESPACE_DISTRHO
