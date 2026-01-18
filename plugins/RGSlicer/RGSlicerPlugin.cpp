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

        // Per-slice defaults (slice 0)
        slice0_pitch_ = 0.0f;
        slice0_time_ = 1.0f;
        slice0_volume_ = 1.0f;
        slice0_pan_ = 0.0f;
        slice0_loop_ = 0.0f;
        slice0_one_shot_ = 0.0f;

        // Random sequencer BPM and note division
        bpm_ = 125.0f;
        note_division_ = 4.0f;  // 16th notes default
        if (slicer_) {
            rgslicer_set_bpm(slicer_, bpm_);
            rgslicer_set_note_division(slicer_, note_division_);
        }

        // Pitch/Time algorithms
        pitch_algorithm_ = 0.0f;  // Simple (rate) default
        time_algorithm_ = 1.0f;   // AKAI/Amiga default
        if (slicer_) rgslicer_set_time_algorithm(slicer_, 1);

        // Output parameters
        playback_pos_ = 0.0f;
        playing_slice_ = -1.0f;

        // Trigger parameter
        trigger_note_ = -1.0f;

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
        // Make ALL parameters automatable!
        parameter.hints = kParameterIsAutomatable;

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
                parameter.ranges.min = 10.0f;
                parameter.ranges.max = 800.0f;
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
                parameter.ranges.min = 10.0f;
                parameter.ranges.max = 800.0f;
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

            case PARAM_SLICE0_LOOP:
                parameter.name = "Slice 0 Loop";
                parameter.symbol = "slice0_loop";
                parameter.ranges.def = 0.0f;
                parameter.ranges.min = 0.0f;
                parameter.ranges.max = 1.0f;
                parameter.hints |= kParameterIsBoolean;
                break;

            case PARAM_SLICE0_ONE_SHOT:
                parameter.name = "Slice 0 One-Shot";
                parameter.symbol = "slice0_one_shot";
                parameter.ranges.def = 0.0f;
                parameter.ranges.min = 0.0f;
                parameter.ranges.max = 1.0f;
                parameter.hints |= kParameterIsBoolean;
                break;

            case PARAM_BPM:
                parameter.name = "BPM";
                parameter.symbol = "bpm";
                parameter.ranges.def = 125.0f;
                parameter.ranges.min = 20.0f;
                parameter.ranges.max = 300.0f;
                parameter.unit = "BPM";
                break;

            case PARAM_NOTE_DIVISION:
                parameter.name = "Note Division";
                parameter.symbol = "note_division";
                parameter.ranges.def = 4.0f;  // 16th notes default
                parameter.ranges.min = 1.0f;
                parameter.ranges.max = 8.0f;
                parameter.hints = kParameterIsAutomatable | kParameterIsInteger;
                parameter.enumValues.count = 4;
                parameter.enumValues.restrictedMode = true;
                {
                    ParameterEnumerationValue* const values = new ParameterEnumerationValue[4];
                    values[0].label = "Quarter";
                    values[0].value = 1.0f;
                    values[1].label = "8th";
                    values[1].value = 2.0f;
                    values[2].label = "16th";
                    values[2].value = 4.0f;
                    values[3].label = "32nd";
                    values[3].value = 8.0f;
                    parameter.enumValues.values = values;
                }
                break;

            case PARAM_PITCH_ALGORITHM:
                parameter.name = "Pitch Algorithm";
                parameter.symbol = "pitch_algorithm";
                parameter.ranges.def = 0.0f;  // Simple default
                parameter.ranges.min = 0.0f;
                parameter.ranges.max = 1.0f;
                parameter.hints = kParameterIsAutomatable | kParameterIsInteger;
                parameter.enumValues.count = 2;
                parameter.enumValues.restrictedMode = true;
                {
                    ParameterEnumerationValue* const values = new ParameterEnumerationValue[2];
                    values[0].label = "Simple (Rate)";
                    values[0].value = 0.0f;
                    values[1].label = "Time-Preserving";
                    values[1].value = 1.0f;
                    parameter.enumValues.values = values;
                }
                break;

            case PARAM_TIME_ALGORITHM:
                parameter.name = "Time Algorithm";
                parameter.symbol = "time_algorithm";
                parameter.ranges.def = 1.0f;  // AKAI/Amiga default
                parameter.ranges.min = 0.0f;
                parameter.ranges.max = 1.0f;
                parameter.hints = kParameterIsAutomatable | kParameterIsInteger;
                parameter.enumValues.count = 2;
                parameter.enumValues.restrictedMode = true;
                {
                    ParameterEnumerationValue* const values = new ParameterEnumerationValue[2];
                    values[0].label = "Granular";
                    values[0].value = 0.0f;
                    values[1].label = "AKAI/Amiga Rate";
                    values[1].value = 1.0f;
                    parameter.enumValues.values = values;
                }
                break;

            case PARAM_PLAYBACK_POS:
                parameter.name = "Playback Position";
                parameter.symbol = "playback_pos";
                parameter.ranges.def = 0.0f;
                parameter.ranges.min = 0.0f;
                parameter.ranges.max = 1.0f;
                parameter.hints = kParameterIsOutput;  // Output-only for UI
                break;

            case PARAM_PLAYING_SLICE:
                parameter.name = "Playing Slice";
                parameter.symbol = "playing_slice";
                parameter.ranges.def = -1.0f;
                parameter.ranges.min = -2.0f;  // -2 = full sample, -1 = none
                parameter.ranges.max = 63.0f;
                parameter.hints = kParameterIsOutput | kParameterIsInteger;
                break;

            case PARAM_TRIGGER_NOTE:
                parameter.name = "Trigger Note";
                parameter.symbol = "trigger_note";
                parameter.ranges.def = 0.0f;
                parameter.ranges.min = 0.0f;
                parameter.ranges.max = 255.0f;  // note (0-127) + toggle bit (0 or 128)
                parameter.hints = kParameterIsInteger | kParameterIsHidden;  // Hidden, not automatable
                break;
        }
    }

    // ========================================================================
    // Parameters
    // ========================================================================

    float getParameterValue(uint32_t index) const override {
        switch (index) {
            case PARAM_MASTER_VOLUME:    return master_volume_ * 100.0f;
            case PARAM_MASTER_PITCH:     return master_pitch_;
            case PARAM_MASTER_TIME:      return master_time_ * 100.0f;
            case PARAM_SLICE0_PITCH:     return slice0_pitch_;
            case PARAM_SLICE0_TIME:      return slice0_time_ * 100.0f;
            case PARAM_SLICE0_VOLUME:    return slice0_volume_ * 100.0f;
            case PARAM_SLICE0_PAN:       return slice0_pan_ * 100.0f;
            case PARAM_SLICE0_LOOP:      return slice0_loop_;
            case PARAM_SLICE0_ONE_SHOT:  return slice0_one_shot_;
            case PARAM_BPM:              return bpm_;
            case PARAM_NOTE_DIVISION:    return note_division_;
            case PARAM_PITCH_ALGORITHM:  return pitch_algorithm_;
            case PARAM_TIME_ALGORITHM:   return time_algorithm_;
            case PARAM_PLAYBACK_POS:     return playback_pos_;
            case PARAM_PLAYING_SLICE:    return playing_slice_;
            case PARAM_TRIGGER_NOTE:     return trigger_note_;
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

            case PARAM_SLICE0_LOOP:
                slice0_loop_ = value;
                if (slicer_) rgslicer_set_slice_loop(slicer_, 0, value > 0.5f);
                break;

            case PARAM_SLICE0_ONE_SHOT:
                slice0_one_shot_ = value;
                if (slicer_) rgslicer_set_slice_one_shot(slicer_, 0, value > 0.5f);
                break;

            case PARAM_BPM:
                bpm_ = value;
                if (slicer_) rgslicer_set_bpm(slicer_, bpm_);
                break;

            case PARAM_NOTE_DIVISION:
                note_division_ = value;
                if (slicer_) rgslicer_set_note_division(slicer_, note_division_);
                break;

            case PARAM_PITCH_ALGORITHM:
                pitch_algorithm_ = value;
                if (slicer_) rgslicer_set_pitch_algorithm(slicer_, (int)value);
                break;

            case PARAM_TIME_ALGORITHM:
                time_algorithm_ = value;
                if (slicer_) rgslicer_set_time_algorithm(slicer_, (int)value);
                break;

            case PARAM_TRIGGER_NOTE:
                trigger_note_ = value;
                // Extract MIDI note using modulo 128
                if (slicer_ && trigger_note_ > 0.0f) {
                    uint8_t note = (uint8_t)((int)trigger_note_ % 128);
                    rgslicer_note_on(slicer_, note, 100);
                }
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
            samplePath_ = value;  // SAVE the path for getState()!
            if (slicer_) {
                // Load sample - preserves CUE point slicing!
                rgslicer_load_sample(slicer_, value);
                // DO NOT call triggerAutoSlice() - it overwrites CUE points!
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

            // Update playback position and active slice for UI visualization
            float newPos = 0.0f;
            float newSlice = -1.0f;
            if (rgslicer_has_sample(slicer_)) {
                for (int v = 0; v < 16; v++) {  // RGSLICER_MAX_VOICES
                    if (slicer_->voices[v].active) {
                        newPos = slicer_->voices[v].playback_pos / (float)slicer_->sample_length;
                        // Note 37 (C#2) plays full sample - use special value -2
                        newSlice = (slicer_->voices[v].note == 37) ? -2.0f : (float)slicer_->voices[v].slice_index;
                        break;  // Use first active voice
                    }
                }
            }

            if (newPos != playback_pos_) {
                playback_pos_ = newPos;
                setParameterValue(PARAM_PLAYBACK_POS, playback_pos_);
            }
            if (newSlice != playing_slice_) {
                playing_slice_ = newSlice;
                setParameterValue(PARAM_PLAYING_SLICE, playing_slice_);
            }
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
    float slice0_pitch_;
    float slice0_time_;
    float slice0_volume_;
    float slice0_pan_;
    float slice0_loop_;
    float slice0_one_shot_;
    float bpm_;
    float note_division_;    // 1.0 = quarter, 2.0 = 8th, 4.0 = 16th, 8.0 = 32nd
    float pitch_algorithm_;  // 0 = Simple (rate), 1 = Time-Preserving
    float time_algorithm_;   // 0 = Granular, 1 = Amiga Offset
    float playback_pos_;     // Output parameter for UI visualization
    float playing_slice_;    // Currently playing slice index
    float trigger_note_;     // Trigger parameter for UI to send note-on (-1 = none)

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RGSlicerPlugin)
};

// ============================================================================
// Plugin Entry Point
// ============================================================================

Plugin* createPlugin() {
    return new RGSlicerPlugin();
}

END_NAMESPACE_DISTRHO
