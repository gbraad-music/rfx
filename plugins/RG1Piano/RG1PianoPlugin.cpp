#include "DistrhoPlugin.hpp"

extern "C" {
#include "../../synth/synth_modal_piano.h"
#include "../../data/rg1piano/sample_data.h"
}

#include <cstring>
#include <cmath>

START_NAMESPACE_DISTRHO

#define PIANO_VOICES 8

struct PianoVoice {
    ModalPiano* piano;
    bool active;
    uint8_t note;
};

class RG1PianoPlugin : public Plugin
{
public:
    RG1PianoPlugin()
        : Plugin(kParameterCount, 0, 0)
        , fDecay(0.5f)
        , fResonance(0.0f)
        , fBrightness(0.6f)
        , fVelocitySens(0.8f)
        , fVolume(0.83f)
        , fLfoRate(0.3f)
        , fLfoDepth(0.2f)
    {
        // Initialize sample data structure
        fSampleData.attack_data = m1piano_onset;
        fSampleData.attack_length = m1piano_onset_length;
        fSampleData.loop_data = m1piano_tail;
        fSampleData.loop_length = m1piano_tail_length;
        fSampleData.sample_rate = M1PIANO_SAMPLE_RATE;
        fSampleData.root_note = M1PIANO_ROOT_NOTE;

        // Create voices
        for (int i = 0; i < PIANO_VOICES; i++) {
            fVoices[i].piano = modal_piano_create();
            fVoices[i].active = false;
            fVoices[i].note = 0;

            if (fVoices[i].piano) {
                modal_piano_load_sample(fVoices[i].piano, &fSampleData);
                updateVoice(i);
            }
        }
    }

    ~RG1PianoPlugin() override
    {
        for (int i = 0; i < PIANO_VOICES; i++) {
            if (fVoices[i].piano) {
                modal_piano_destroy(fVoices[i].piano);
            }
        }
    }

protected:
    const char* getLabel() const override { return RG1PIANO_DISPLAY_NAME; }
    const char* getDescription() const override { return RG1PIANO_DESCRIPTION; }
    const char* getMaker() const override { return "Regroove"; }
    const char* getHomePage() const override { return "https://music.gbraad.nl/regrooved/"; }
    const char* getLicense() const override { return "GPL-3.0"; }
    uint32_t getVersion() const override { return d_version(1, 0, 0); }
    int64_t getUniqueId() const override { return d_cconst('R', 'G', '1', 'P'); }

    void initParameter(uint32_t index, Parameter& param) override
    {
        param.hints = kParameterIsAutomatable;
        param.ranges.min = 0.0f;
        param.ranges.max = 1.0f;
        param.ranges.def = 0.5f;

        switch (index) {
        case kParameterDecay:
            param.name = "Decay";
            param.symbol = "decay";
            param.ranges.def = 0.5f;
            break;
        case kParameterResonance:
            param.name = "Resonance";
            param.symbol = "resonance";
            param.ranges.def = 0.0f;
            break;
        case kParameterBrightness:
            param.name = "Brightness";
            param.symbol = "brightness";
            param.ranges.def = 0.6f;
            break;
        case kParameterVelocitySens:
            param.name = "Velocity Sens";
            param.symbol = "vel_sens";
            param.ranges.def = 0.8f;
            break;
        case kParameterVolume:
            param.name = "Volume";
            param.symbol = "volume";
            param.ranges.def = 0.83f;
            break;
        case kParameterLfoRate:
            param.name = "LFO Rate";
            param.symbol = "lfo_rate";
            param.ranges.def = 0.3f;
            break;
        case kParameterLfoDepth:
            param.name = "LFO Depth";
            param.symbol = "lfo_depth";
            param.ranges.def = 0.2f;
            break;
        }
    }

    float getParameterValue(uint32_t index) const override
    {
        switch (index) {
        case kParameterDecay: return fDecay;
        case kParameterResonance: return fResonance;
        case kParameterBrightness: return fBrightness;
        case kParameterVelocitySens: return fVelocitySens;
        case kParameterVolume: return fVolume;
        case kParameterLfoRate: return fLfoRate;
        case kParameterLfoDepth: return fLfoDepth;
        default: return 0.0f;
        }
    }

    void setParameterValue(uint32_t index, float value) override
    {
        switch (index) {
        case kParameterDecay:
            fDecay = value;
            updateAllVoices();
            break;
        case kParameterResonance:
            fResonance = value;
            updateAllVoices();
            break;
        case kParameterBrightness:
            fBrightness = value;
            updateAllVoices();
            break;
        case kParameterVelocitySens:
            fVelocitySens = value;
            updateAllVoices();
            break;
        case kParameterVolume:
            fVolume = value;
            break;
        case kParameterLfoRate:
            fLfoRate = value;
            updateAllVoices();
            break;
        case kParameterLfoDepth:
            fLfoDepth = value;
            updateAllVoices();
            break;
        }
    }

    void run(const float**, float** outputs, uint32_t frames,
             const MidiEvent* midiEvents, uint32_t midiEventCount) override
    {
        float* outL = outputs[0];
        float* outR = outputs[1];
        uint32_t framePos = 0;
        const int sampleRate = (int)getSampleRate();

        // Process MIDI events
        for (uint32_t i = 0; i < midiEventCount; ++i) {
            const MidiEvent& event = midiEvents[i];

            // Render frames up to this event
            while (framePos < event.frame) {
                renderFrame(outL, outR, framePos, sampleRate);
                framePos++;
            }

            if (event.size != 3) continue;

            const uint8_t status = event.data[0] & 0xF0;
            const uint8_t note = event.data[1];
            const uint8_t velocity = event.data[2];

            if (status == 0x90 && velocity > 0) {
                handleNoteOn(note, velocity);
            } else if (status == 0x80 || (status == 0x90 && velocity == 0)) {
                handleNoteOff(note);
            }
        }

        // Render remaining frames
        while (framePos < frames) {
            renderFrame(outL, outR, framePos, sampleRate);
            framePos++;
        }
    }

private:
    void updateVoice(int idx)
    {
        if (idx < 0 || idx >= PIANO_VOICES || !fVoices[idx].piano) return;

        // Map decay parameter to 0.5s - 8s range
        float decay_time = 0.5f + fDecay * 7.5f;
        modal_piano_set_decay(fVoices[idx].piano, decay_time);

        // Set resonance amount
        modal_piano_set_resonance(fVoices[idx].piano, fResonance);

        // Set filter envelope (brightness controls sustain level)
        modal_piano_set_filter_envelope(fVoices[idx].piano, 0.01f, 0.3f, fBrightness);

        // Set velocity sensitivity
        modal_piano_set_velocity_sensitivity(fVoices[idx].piano, fVelocitySens);

        // Map LFO rate: 0-1 -> 0.5Hz-8Hz
        float lfo_freq = 0.5f + fLfoRate * 7.5f;
        modal_piano_set_lfo(fVoices[idx].piano, lfo_freq, fLfoDepth);
    }

    void updateAllVoices()
    {
        for (int i = 0; i < PIANO_VOICES; i++) {
            updateVoice(i);
        }
    }

    int findFreeVoice()
    {
        // First try to find an inactive voice
        for (int i = 0; i < PIANO_VOICES; i++) {
            if (!fVoices[i].active) return i;
        }

        // Otherwise steal oldest voice (voice 0)
        return 0;
    }

    void handleNoteOn(uint8_t note, uint8_t velocity)
    {
        int voice_idx = findFreeVoice();
        if (voice_idx < 0 || !fVoices[voice_idx].piano) return;

        // Modal piano handles velocity sensitivity internally
        modal_piano_trigger(fVoices[voice_idx].piano, note, velocity);
        fVoices[voice_idx].active = true;
        fVoices[voice_idx].note = note;
    }

    void handleNoteOff(uint8_t note)
    {
        for (int i = 0; i < PIANO_VOICES; i++) {
            if (fVoices[i].active && fVoices[i].note == note) {
                modal_piano_release(fVoices[i].piano);
            }
        }
    }

    void renderFrame(float* outL, float* outR, uint32_t framePos, int sampleRate)
    {
        float mixL = 0.0f, mixR = 0.0f;

        for (int i = 0; i < PIANO_VOICES; i++) {
            if (!fVoices[i].active || !fVoices[i].piano) continue;

            // Modal piano handles filter envelope internally
            float sample = modal_piano_process(fVoices[i].piano, sampleRate);

            if (!modal_piano_is_active(fVoices[i].piano)) {
                fVoices[i].active = false;
                continue;
            }

            mixL += sample;
            mixR += sample;
        }

        // Per-voice reduction for 8 voices + volume control
        mixL *= 0.2f * fVolume;
        mixR *= 0.2f * fVolume;

        // Soft clipping
        if (mixL > 1.0f) {
            mixL = 1.0f - expf(-(mixL - 1.0f));
        } else if (mixL < -1.0f) {
            mixL = -1.0f + expf(mixL + 1.0f);
        }

        if (mixR > 1.0f) {
            mixR = 1.0f - expf(-(mixR - 1.0f));
        } else if (mixR < -1.0f) {
            mixR = -1.0f + expf(mixR + 1.0f);
        }

        outL[framePos] = mixL;
        outR[framePos] = mixR;
    }

    SampleData fSampleData;
    PianoVoice fVoices[PIANO_VOICES];

    float fDecay, fResonance, fBrightness, fVelocitySens, fVolume;
    float fLfoRate, fLfoDepth;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RG1PianoPlugin)
};

Plugin* createPlugin() { return new RG1PianoPlugin(); }

END_NAMESPACE_DISTRHO
