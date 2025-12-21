#include "DistrhoPlugin.hpp"
#include "../../synth/synth_karplus.h"
#include "../../synth/synth_voice_manager.h"
#include <cstring>
#include <cmath>

START_NAMESPACE_DISTRHO

#define KS_VOICES 8

struct KSVoice {
    SynthKarplus* ks;
    bool active;
};

class RGKSSynthPlugin : public Plugin
{
public:
    RGKSSynthPlugin()
        : Plugin(kParameterCount, 0, 0)
        , fDamping(0.5f)
        , fBrightness(0.5f)
        , fStretch(0.0f)
        , fPickPosition(0.5f)
        , fVolume(0.5f)
    {
        fVoiceManager = synth_voice_manager_create(KS_VOICES);

        for (int i = 0; i < KS_VOICES; i++) {
            fVoices[i].ks = synth_karplus_create();
            fVoices[i].active = false;
            updateVoice(i);
        }
    }

    ~RGKSSynthPlugin() override
    {
        for (int i = 0; i < KS_VOICES; i++) {
            if (fVoices[i].ks) synth_karplus_destroy(fVoices[i].ks);
        }
        if (fVoiceManager) synth_voice_manager_destroy(fVoiceManager);
    }

protected:
    const char* getLabel() const override { return RGKS_DISPLAY_NAME; }
    const char* getDescription() const override { return RGKS_DESCRIPTION; }
    const char* getMaker() const override { return "Regroove"; }
    const char* getHomePage() const override { return "https://music.gbraad.nl/regrooved/"; }
    const char* getLicense() const override { return "GPL-3.0"; }
    uint32_t getVersion() const override { return d_version(1, 0, 0); }
    int64_t getUniqueId() const override { return d_cconst('R', 'G', 'K', 'S'); }

    void initParameter(uint32_t index, Parameter& param) override
    {
        param.hints = kParameterIsAutomatable;
        param.ranges.min = 0.0f;
        param.ranges.max = 1.0f;
        param.ranges.def = 0.5f;

        switch (index) {
        case kParameterDamping:
            param.name = "Damping";
            param.symbol = "damping";
            param.ranges.def = 0.5f;
            break;
        case kParameterBrightness:
            param.name = "Brightness";
            param.symbol = "brightness";
            param.ranges.def = 0.5f;
            break;
        case kParameterStretch:
            param.name = "Stretch";
            param.symbol = "stretch";
            param.ranges.def = 0.0f;
            break;
        case kParameterPickPosition:
            param.name = "Pick Position";
            param.symbol = "pick_pos";
            param.ranges.def = 0.5f;
            break;
        case kParameterVolume:
            param.name = "Volume";
            param.symbol = "volume";
            param.ranges.def = 0.5f;
            break;
        }
    }

    float getParameterValue(uint32_t index) const override
    {
        switch (index) {
        case kParameterDamping: return fDamping;
        case kParameterBrightness: return fBrightness;
        case kParameterStretch: return fStretch;
        case kParameterPickPosition: return fPickPosition;
        case kParameterVolume: return fVolume;
        default: return 0.0f;
        }
    }

    void setParameterValue(uint32_t index, float value) override
    {
        switch (index) {
        case kParameterDamping: fDamping = value; updateAllVoices(); break;
        case kParameterBrightness: fBrightness = value; updateAllVoices(); break;
        case kParameterStretch: fStretch = value; updateAllVoices(); break;
        case kParameterPickPosition: fPickPosition = value; updateAllVoices(); break;
        case kParameterVolume: fVolume = value; break;
        }
    }

    void run(const float**, float** outputs, uint32_t frames, const MidiEvent* midiEvents, uint32_t midiEventCount) override
    {
        float* outL = outputs[0];
        float* outR = outputs[1];
        uint32_t framePos = 0;
        const int sampleRate = (int)getSampleRate();

        for (uint32_t i = 0; i < midiEventCount; ++i) {
            const MidiEvent& event = midiEvents[i];
            while (framePos < event.frame) {
                renderFrame(outL, outR, framePos, sampleRate);
                framePos++;
            }
            if (event.size != 3) continue;
            const uint8_t status = event.data[0] & 0xF0;
            const uint8_t note = event.data[1];
            const uint8_t velocity = event.data[2];
            if (status == 0x90 && velocity > 0) handleNoteOn(note, velocity);
            else if (status == 0x80 || (status == 0x90 && velocity == 0)) handleNoteOff(note);
        }
        while (framePos < frames) {
            renderFrame(outL, outR, framePos, sampleRate);
            framePos++;
        }
    }

private:
    void updateVoice(int idx)
    {
        if (idx < 0 || idx >= KS_VOICES) return;
        synth_karplus_set_damping(fVoices[idx].ks, fDamping);
        synth_karplus_set_brightness(fVoices[idx].ks, fBrightness);
        synth_karplus_set_stretch(fVoices[idx].ks, fStretch);
        synth_karplus_set_pick_position(fVoices[idx].ks, fPickPosition);
    }

    void updateAllVoices()
    {
        for (int i = 0; i < KS_VOICES; i++) updateVoice(i);
    }

    void handleNoteOn(uint8_t note, uint8_t velocity)
    {
        int voice_idx = synth_voice_manager_allocate(fVoiceManager, note, velocity);
        if (voice_idx < 0 || voice_idx >= KS_VOICES) return;

        float freq = 440.0f * powf(2.0f, (note - 69) / 12.0f);
        float vel = velocity / 127.0f;

        synth_karplus_trigger(fVoices[voice_idx].ks, freq, vel, (int)getSampleRate());
        fVoices[voice_idx].active = true;
    }

    void handleNoteOff(uint8_t note)
    {
        int voice_idx = synth_voice_manager_release(fVoiceManager, note);
        if (voice_idx < 0 || voice_idx >= KS_VOICES) return;
        synth_karplus_release(fVoices[voice_idx].ks);
    }

    void renderFrame(float* outL, float* outR, uint32_t framePos, int sampleRate)
    {
        float mixL = 0.0f, mixR = 0.0f;

        for (int i = 0; i < KS_VOICES; i++) {
            const VoiceMeta* meta = synth_voice_manager_get_voice(fVoiceManager, i);
            if (!meta || meta->state == VOICE_INACTIVE) {
                fVoices[i].active = false;
                continue;
            }
            if (!fVoices[i].active) continue;

            float sample = synth_karplus_process(fVoices[i].ks, sampleRate);

            if (!synth_karplus_is_active(fVoices[i].ks)) {
                synth_voice_manager_stop_voice(fVoiceManager, i);
                fVoices[i].active = false;
            }

            mixL += sample;
            mixR += sample;
        }

        // Per-voice reduction for 8 voices + volume control
        mixL *= 0.15f * fVolume;
        mixR *= 0.15f * fVolume;

        // Soft clipping
        if (mixL > 1.0f) mixL = 1.0f;
        if (mixL < -1.0f) mixL = -1.0f;
        if (mixR > 1.0f) mixR = 1.0f;
        if (mixR < -1.0f) mixR = -1.0f;

        outL[framePos] = mixL;
        outR[framePos] = mixR;
    }

    SynthVoiceManager* fVoiceManager;
    KSVoice fVoices[KS_VOICES];

    float fDamping, fBrightness, fStretch, fPickPosition, fVolume;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RGKSSynthPlugin)
};

Plugin* createPlugin() { return new RGKSSynthPlugin(); }

END_NAMESPACE_DISTRHO
