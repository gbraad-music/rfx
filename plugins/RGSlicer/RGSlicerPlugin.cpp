/*
 * RGSlicer Plugin - VST3/LV2/Standalone
 * Slicing Sampler with Pitch/Time Effects
 */

#include "DistrhoPlugin.hpp"

extern "C" {
#include "../../synth/rgslicer.h"
}

START_NAMESPACE_DISTRHO

// ============================================================================
// RGSlicer Plugin
// ============================================================================

class RGSlicerPlugin : public Plugin {
public:
    RGSlicerPlugin()
        : Plugin(PARAM_COUNT, 1, 1)  // params, programs, states
    {
        slicer_ = rgslicer_create(getSampleRate());

        // Initialize default parameters
        master_volume_ = 1.0f;
        master_pitch_ = 0.0f;
        master_time_ = 1.0f;
        slice_mode_ = 0;  // Transient
        num_slices_ = 16;
        sensitivity_ = 0.5f;

        // Per-slice defaults (slice 0)
        slice0_pitch_ = 0.0f;
        slice0_time_ = 1.0f;
        slice0_volume_ = 1.0f;
        slice0_pan_ = 0.0f;
        slice0_reverse_ = 0.0f;
        slice0_loop_ = 0.0f;

        // Sample will be loaded via UI file browser
    }

    ~RGSlicerPlugin() override {
        if (slicer_) {
            rgslicer_destroy(slicer_);
        }
    }

protected:
    // ========================================================================
    // Plugin Info
    // ========================================================================

    const char* getLabel() const override {
        return "RGSlicer";
    }

    const char* getDescription() const override {
        return "Slicing Sampler with Pitch/Time Effects";
    }

    const char* getMaker() const override {
        return "Regroove";
    }

    const char* getHomePage() const override {
        return "https://regroove.org/plugins/rgslicer";
    }

    const char* getLicense() const override {
        return "BSD-3-Clause";
    }

    uint32_t getVersion() const override {
        return d_version(1, 0, 0);
    }

    int64_t getUniqueId() const override {
        return d_cconst('R', 'G', 'S', 'L');
    }

    // ========================================================================
    // Init
    // ========================================================================

    void initParameter(uint32_t index, Parameter& parameter) override {
        switch (index) {
            case PARAM_MASTER_VOLUME:
                parameter.name = "Master Volume";
                parameter.symbol = "master_volume";
                parameter.ranges.def = 100.0f;
                parameter.ranges.min = 0.0f;
                parameter.ranges.max = 200.0f;
                parameter.unit = "%";
                break;

            case PARAM_MASTER_PITCH:
                parameter.name = "Master Pitch";
                parameter.symbol = "master_pitch";
                parameter.ranges.def = 0.0f;
                parameter.ranges.min = -12.0f;
                parameter.ranges.max = 12.0f;
                parameter.unit = "st";
                break;

            case PARAM_MASTER_TIME:
                parameter.name = "Master Time";
                parameter.symbol = "master_time";
                parameter.ranges.def = 100.0f;
                parameter.ranges.min = 50.0f;
                parameter.ranges.max = 200.0f;
                parameter.unit = "%";
                break;

            case PARAM_SLICE_MODE:
                parameter.name = "Slice Mode";
                parameter.symbol = "slice_mode";
                parameter.ranges.def = 0.0f;
                parameter.ranges.min = 0.0f;
                parameter.ranges.max = 3.0f;
                parameter.enumValues.count = 4;
                parameter.enumValues.restrictedMode = true;
                {
                    ParameterEnumerationValue* const values = new ParameterEnumerationValue[4];
                    parameter.enumValues.values = values;
                    values[0].label = "Transient";
                    values[0].value = 0.0f;
                    values[1].label = "Zero-Cross";
                    values[1].value = 1.0f;
                    values[2].label = "Fixed Grid";
                    values[2].value = 2.0f;
                    values[3].label = "BPM Sync";
                    values[3].value = 3.0f;
                }
                break;

            case PARAM_NUM_SLICES:
                parameter.name = "Num Slices";
                parameter.symbol = "num_slices";
                parameter.ranges.def = 16.0f;
                parameter.ranges.min = 1.0f;
                parameter.ranges.max = 64.0f;
                parameter.hints = kParameterIsInteger;
                break;

            case PARAM_SENSITIVITY:
                parameter.name = "Sensitivity";
                parameter.symbol = "sensitivity";
                parameter.ranges.def = 50.0f;
                parameter.ranges.min = 0.0f;
                parameter.ranges.max = 100.0f;
                parameter.unit = "%";
                break;

            case PARAM_SLICE0_PITCH:
                parameter.name = "Slice 0 Pitch";
                parameter.symbol = "slice0_pitch";
                parameter.ranges.def = 0.0f;
                parameter.ranges.min = -12.0f;
                parameter.ranges.max = 12.0f;
                parameter.unit = "st";
                break;

            case PARAM_SLICE0_TIME:
                parameter.name = "Slice 0 Time";
                parameter.symbol = "slice0_time";
                parameter.ranges.def = 100.0f;
                parameter.ranges.min = 50.0f;
                parameter.ranges.max = 200.0f;
                parameter.unit = "%";
                break;

            case PARAM_SLICE0_VOLUME:
                parameter.name = "Slice 0 Volume";
                parameter.symbol = "slice0_volume";
                parameter.ranges.def = 100.0f;
                parameter.ranges.min = 0.0f;
                parameter.ranges.max = 200.0f;
                parameter.unit = "%";
                break;

            case PARAM_SLICE0_PAN:
                parameter.name = "Slice 0 Pan";
                parameter.symbol = "slice0_pan";
                parameter.ranges.def = 0.0f;
                parameter.ranges.min = -100.0f;
                parameter.ranges.max = 100.0f;
                parameter.unit = "%";
                break;

            case PARAM_SLICE0_REVERSE:
                parameter.name = "Slice 0 Reverse";
                parameter.symbol = "slice0_reverse";
                parameter.ranges.def = 0.0f;
                parameter.ranges.min = 0.0f;
                parameter.ranges.max = 1.0f;
                parameter.hints = kParameterIsBoolean;
                break;

            case PARAM_SLICE0_LOOP:
                parameter.name = "Slice 0 Loop";
                parameter.symbol = "slice0_loop";
                parameter.ranges.def = 0.0f;
                parameter.ranges.min = 0.0f;
                parameter.ranges.max = 1.0f;
                parameter.hints = kParameterIsBoolean;
                break;
        }
    }

    // ========================================================================
    // Parameters
    // ========================================================================

    float getParameterValue(uint32_t index) const override {
        switch (index) {
            case PARAM_MASTER_VOLUME:  return master_volume_ * 100.0f;
            case PARAM_MASTER_PITCH:   return master_pitch_;
            case PARAM_MASTER_TIME:    return master_time_ * 100.0f;
            case PARAM_SLICE_MODE:     return (float)slice_mode_;
            case PARAM_NUM_SLICES:     return (float)num_slices_;
            case PARAM_SENSITIVITY:    return sensitivity_ * 100.0f;
            case PARAM_SLICE0_PITCH:   return slice0_pitch_;
            case PARAM_SLICE0_TIME:    return slice0_time_ * 100.0f;
            case PARAM_SLICE0_VOLUME:  return slice0_volume_ * 100.0f;
            case PARAM_SLICE0_PAN:     return slice0_pan_ * 100.0f;
            case PARAM_SLICE0_REVERSE: return slice0_reverse_;
            case PARAM_SLICE0_LOOP:    return slice0_loop_;
            default: return 0.0f;
        }
    }

    void setParameterValue(uint32_t index, float value) override {
        switch (index) {
            case PARAM_MASTER_VOLUME:
                master_volume_ = value / 100.0f;
                if (slicer_) rgslicer_set_global_volume(slicer_, master_volume_);
                break;

            case PARAM_MASTER_PITCH:
                master_pitch_ = value;
                if (slicer_) rgslicer_set_global_pitch(slicer_, master_pitch_);
                break;

            case PARAM_MASTER_TIME:
                master_time_ = value / 100.0f;
                if (slicer_) rgslicer_set_global_time(slicer_, master_time_);
                break;

            case PARAM_SLICE_MODE:
                slice_mode_ = (uint8_t)value;
                triggerAutoSlice();
                break;

            case PARAM_NUM_SLICES:
                num_slices_ = (uint8_t)value;
                triggerAutoSlice();
                break;

            case PARAM_SENSITIVITY:
                sensitivity_ = value / 100.0f;
                triggerAutoSlice();
                break;

            case PARAM_SLICE0_PITCH:
                slice0_pitch_ = value;
                if (slicer_) rgslicer_set_slice_pitch(slicer_, 0, slice0_pitch_);
                break;

            case PARAM_SLICE0_TIME:
                slice0_time_ = value / 100.0f;
                if (slicer_) rgslicer_set_slice_time(slicer_, 0, slice0_time_);
                break;

            case PARAM_SLICE0_VOLUME:
                slice0_volume_ = value / 100.0f;
                if (slicer_) rgslicer_set_slice_volume(slicer_, 0, slice0_volume_);
                break;

            case PARAM_SLICE0_PAN:
                slice0_pan_ = value / 100.0f;
                if (slicer_) rgslicer_set_slice_pan(slicer_, 0, slice0_pan_);
                break;

            case PARAM_SLICE0_REVERSE:
                slice0_reverse_ = value;
                if (slicer_) rgslicer_set_slice_reverse(slicer_, 0, value > 0.5f);
                break;

            case PARAM_SLICE0_LOOP:
                slice0_loop_ = value;
                if (slicer_) rgslicer_set_slice_loop(slicer_, 0, value > 0.5f);
                break;
        }
    }

    // ========================================================================
    // Programs
    // ========================================================================

    void initProgramName(uint32_t index, String& programName) override {
        programName = "Default";
    }

    // ========================================================================
    // State (for loading samples)
    // ========================================================================

    void initState(uint32_t index, State& state) override {
        if (index == 0) {
            state.key = "samplePath";
            state.label = "Sample Path";
            state.defaultValue = "";
            state.hints = kStateIsFilenamePath;
        }
    }

    void setState(const char* key, const char* value) override {
        if (std::strcmp(key, "samplePath") == 0 && value && value[0] != '\0') {
            if (slicer_) {
                rgslicer_load_sample(slicer_, value);
                triggerAutoSlice();
            }
        }
    }

    String getState(const char* key) const override {
        if (std::strcmp(key, "samplePath") == 0) {
            return samplePath_;
        }
        return String();
    }

    // ========================================================================
    // Audio/MIDI Processing
    // ========================================================================

    void activate() override {
        if (slicer_) {
            rgslicer_reset(slicer_);
        }
    }

    void run(const float**, float** outputs, uint32_t frames,
             const MidiEvent* midiEvents, uint32_t midiEventCount) override {
        // Process MIDI events
        for (uint32_t i = 0; i < midiEventCount; ++i) {
            const MidiEvent& event = midiEvents[i];

            if (event.size != 3)
                continue;

            const uint8_t* data = event.data;
            const uint8_t status = data[0] & 0xF0;
            const uint8_t channel = data[0] & 0x0F;

            (void)channel;  // Ignore channel for now

            switch (status) {
                case 0x90:  // Note On
                    if (data[2] > 0) {
                        if (slicer_) rgslicer_note_on(slicer_, data[1], data[2]);
                    } else {
                        if (slicer_) rgslicer_note_off(slicer_, data[1]);
                    }
                    break;

                case 0x80:  // Note Off
                    if (slicer_) rgslicer_note_off(slicer_, data[1]);
                    break;

                case 0xB0:  // Control Change
                    if (data[1] == 123) {  // All Notes Off
                        if (slicer_) rgslicer_all_notes_off(slicer_);
                    }
                    break;
            }
        }

        // Process audio (stereo interleaved)
        float* interleavedBuffer = new float[frames * 2];

        if (slicer_) {
            rgslicer_process_f32(slicer_, interleavedBuffer, frames);
        } else {
            std::memset(interleavedBuffer, 0, frames * 2 * sizeof(float));
        }

        // De-interleave to output buffers
        for (uint32_t i = 0; i < frames; ++i) {
            outputs[0][i] = interleavedBuffer[i * 2];
            outputs[1][i] = interleavedBuffer[i * 2 + 1];
        }

        delete[] interleavedBuffer;
    }

private:
    RGSlicer* slicer_;
    String samplePath_;

    // Parameters
    float master_volume_;
    float master_pitch_;
    float master_time_;
    uint8_t slice_mode_;
    uint8_t num_slices_;
    float sensitivity_;
    float slice0_pitch_;
    float slice0_time_;
    float slice0_volume_;
    float slice0_pan_;
    float slice0_reverse_;
    float slice0_loop_;

    void triggerAutoSlice() {
        if (slicer_ && rgslicer_has_sample(slicer_)) {
            SliceMode mode = (SliceMode)slice_mode_;
            rgslicer_auto_slice(slicer_, mode, num_slices_, sensitivity_);
        }
    }

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RGSlicerPlugin)
};

// ============================================================================
// Plugin Entry Point
// ============================================================================

Plugin* createPlugin() {
    return new RGSlicerPlugin();
}

END_NAMESPACE_DISTRHO
