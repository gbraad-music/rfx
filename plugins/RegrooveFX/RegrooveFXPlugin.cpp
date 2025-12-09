/*
 * RegrooveFX Plugin - CLEAN VERSION (5 effects only)
 * Copyright (C) 2024
 */

#include "DistrhoPlugin.hpp"
#include "regroove_effects.h"
#include <cstring>

START_NAMESPACE_DISTRHO

class RegrooveFXPlugin : public Plugin
{
public:
    RegrooveFXPlugin()
        : Plugin(kParameterCount, 0, 0)
    {
        fEffects = regroove_effects_create();
    }

    ~RegrooveFXPlugin() override
    {
        if (fEffects) regroove_effects_destroy(fEffects);
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
        if (!fEffects) return 0.0f;

        switch (index) {
        case kParameterDistortionEnabled: return regroove_effects_get_distortion_enabled(fEffects) ? 1.0f : 0.0f;
        case kParameterDistortionDrive: return regroove_effects_get_distortion_drive(fEffects);
        case kParameterDistortionMix: return regroove_effects_get_distortion_mix(fEffects);

        case kParameterFilterEnabled: return regroove_effects_get_filter_enabled(fEffects) ? 1.0f : 0.0f;
        case kParameterFilterCutoff: return regroove_effects_get_filter_cutoff(fEffects);
        case kParameterFilterResonance: return regroove_effects_get_filter_resonance(fEffects);

        case kParameterEQEnabled: return regroove_effects_get_eq_enabled(fEffects) ? 1.0f : 0.0f;
        case kParameterEQLow: return regroove_effects_get_eq_low(fEffects);
        case kParameterEQMid: return regroove_effects_get_eq_mid(fEffects);
        case kParameterEQHigh: return regroove_effects_get_eq_high(fEffects);

        case kParameterCompressorEnabled: return regroove_effects_get_compressor_enabled(fEffects) ? 1.0f : 0.0f;
        case kParameterCompressorThreshold: return regroove_effects_get_compressor_threshold(fEffects);
        case kParameterCompressorRatio: return regroove_effects_get_compressor_ratio(fEffects);
        case kParameterCompressorAttack: return regroove_effects_get_compressor_attack(fEffects);
        case kParameterCompressorRelease: return regroove_effects_get_compressor_release(fEffects);
        case kParameterCompressorMakeup: return regroove_effects_get_compressor_makeup(fEffects);

        case kParameterDelayEnabled: return regroove_effects_get_delay_enabled(fEffects) ? 1.0f : 0.0f;
        case kParameterDelayTime: return regroove_effects_get_delay_time(fEffects);
        case kParameterDelayFeedback: return regroove_effects_get_delay_feedback(fEffects);
        case kParameterDelayMix: return regroove_effects_get_delay_mix(fEffects);

        default: return 0.0f;
        }
    }

    void setParameterValue(uint32_t index, float value) override
    {
        if (!fEffects) return;

        switch (index) {
        case kParameterDistortionEnabled: regroove_effects_set_distortion_enabled(fEffects, value >= 0.5f); break;
        case kParameterDistortionDrive: regroove_effects_set_distortion_drive(fEffects, value); break;
        case kParameterDistortionMix: regroove_effects_set_distortion_mix(fEffects, value); break;

        case kParameterFilterEnabled: regroove_effects_set_filter_enabled(fEffects, value >= 0.5f); break;
        case kParameterFilterCutoff: regroove_effects_set_filter_cutoff(fEffects, value); break;
        case kParameterFilterResonance: regroove_effects_set_filter_resonance(fEffects, value); break;

        case kParameterEQEnabled: regroove_effects_set_eq_enabled(fEffects, value >= 0.5f); break;
        case kParameterEQLow: regroove_effects_set_eq_low(fEffects, value); break;
        case kParameterEQMid: regroove_effects_set_eq_mid(fEffects, value); break;
        case kParameterEQHigh: regroove_effects_set_eq_high(fEffects, value); break;

        case kParameterCompressorEnabled: regroove_effects_set_compressor_enabled(fEffects, value >= 0.5f); break;
        case kParameterCompressorThreshold: regroove_effects_set_compressor_threshold(fEffects, value); break;
        case kParameterCompressorRatio: regroove_effects_set_compressor_ratio(fEffects, value); break;
        case kParameterCompressorAttack: regroove_effects_set_compressor_attack(fEffects, value); break;
        case kParameterCompressorRelease: regroove_effects_set_compressor_release(fEffects, value); break;
        case kParameterCompressorMakeup: regroove_effects_set_compressor_makeup(fEffects, value); break;

        case kParameterDelayEnabled: regroove_effects_set_delay_enabled(fEffects, value >= 0.5f); break;
        case kParameterDelayTime: regroove_effects_set_delay_time(fEffects, value); break;
        case kParameterDelayFeedback: regroove_effects_set_delay_feedback(fEffects, value); break;
        case kParameterDelayMix: regroove_effects_set_delay_mix(fEffects, value); break;
        }
    }

    void activate() override
    {
        if (fEffects) regroove_effects_reset(fEffects);
    }

    void run(const float** inputs, float** outputs, uint32_t frames) override
    {
        if (!fEffects) {
            std::memcpy(outputs[0], inputs[0], sizeof(float) * frames);
            std::memcpy(outputs[1], inputs[1], sizeof(float) * frames);
            return;
        }

        // Convert float to int16 for regroove_effects
        int16_t* buffer = new int16_t[frames * 2];

        for (uint32_t i = 0; i < frames; i++) {
            buffer[i * 2]     = (int16_t)(inputs[0][i] * 32767.0f);
            buffer[i * 2 + 1] = (int16_t)(inputs[1][i] * 32767.0f);
        }

        // Process
        regroove_effects_process(fEffects, buffer, frames, (int)getSampleRate());

        // Convert back to float
        for (uint32_t i = 0; i < frames; i++) {
            outputs[0][i] = buffer[i * 2]     / 32768.0f;
            outputs[1][i] = buffer[i * 2 + 1] / 32768.0f;
        }

        delete[] buffer;
    }

private:
    RegrooveEffects* fEffects;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RegrooveFXPlugin)
};

Plugin* createPlugin()
{
    return new RegrooveFXPlugin();
}

END_NAMESPACE_DISTRHO
