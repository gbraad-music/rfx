#include "DistrhoPlugin.hpp"
#include "../../synth/synth_fm_opl2.h"
#include "../../synth/synth_voice_manager.h"
#include "../../synth/synth_lfo.h"
#include <cstring>
#include <cmath>

START_NAMESPACE_DISTRHO

#define FM_VOICES 9

struct FMVoice {
    SynthFMOperator* op1;  // Carrier
    SynthFMOperator* op2;  // Modulator
    bool active;
};

class RG560_SynthPlugin : public Plugin
{
public:
    RG560_SynthPlugin()
        : Plugin(kParameterCount, 0, 0)
        , fAlgorithm(0.0f)
        , fOp1Multiplier(1.0f), fOp1Level(1.0f), fOp1Attack(0.01f), fOp1Decay(0.3f), fOp1Sustain(0.7f), fOp1Release(0.3f), fOp1Waveform(0.0f)
        , fOp2Multiplier(1.0f), fOp2Level(0.7f), fOp2Attack(0.01f), fOp2Decay(0.2f), fOp2Sustain(0.5f), fOp2Release(0.2f), fOp2Waveform(0.0f)
        , fFeedback(0.0f), fDetune(0.0f), fLFORate(5.0f), fLFOPitchDepth(0.0f), fLFOAmpDepth(0.0f), fVelocitySensitivity(0.5f), fVolume(0.3f)
    {
        fVoiceManager = synth_voice_manager_create(FM_VOICES);
        fLFO = synth_lfo_create();

        for (int i = 0; i < FM_VOICES; i++) {
            fVoices[i].op1 = synth_fm_operator_create();
            fVoices[i].op2 = synth_fm_operator_create();
            fVoices[i].active = false;
            updateOperators(i);
        }

        if (fLFO) {
            synth_lfo_set_waveform(fLFO, SYNTH_LFO_SINE);
            synth_lfo_set_frequency(fLFO, fLFORate);
        }
    }

    ~RG560_SynthPlugin() override
    {
        for (int i = 0; i < FM_VOICES; i++) {
            if (fVoices[i].op1) synth_fm_operator_destroy(fVoices[i].op1);
            if (fVoices[i].op2) synth_fm_operator_destroy(fVoices[i].op2);
        }
        if (fVoiceManager) synth_voice_manager_destroy(fVoiceManager);
        if (fLFO) synth_lfo_destroy(fLFO);
    }

protected:
    const char* getLabel() const override { return RG560_DISPLAY_NAME; }
    const char* getDescription() const override { return RG560_DESCRIPTION; }
    const char* getMaker() const override { return "Regroove"; }
    const char* getHomePage() const override { return "https://music.gbraad.nl/regrooved/"; }
    const char* getLicense() const override { return "GPL-3.0"; }
    uint32_t getVersion() const override { return d_version(1, 0, 0); }
    int64_t getUniqueId() const override { return d_cconst('R', 'G', '5', '6'); }

    void initParameter(uint32_t index, Parameter& param) override
    {
        param.hints = kParameterIsAutomatable;
        param.ranges.min = 0.0f;
        param.ranges.max = 1.0f;
        param.ranges.def = 0.5f;

        switch (index) {
        case kParameterAlgorithm: param.name = "Algorithm"; param.symbol = "algo"; param.ranges.def = 0.0f; break;
        case kParameterOp1Multiplier: param.name = "Op1 Mult"; param.symbol = "op1_mult"; param.ranges.min = 0.5f; param.ranges.max = 15.0f; param.ranges.def = 1.0f; break;
        case kParameterOp1Level: param.name = "Op1 Level"; param.symbol = "op1_level"; param.ranges.def = 1.0f; break;
        case kParameterOp1Attack: param.name = "Op1 Attack"; param.symbol = "op1_attack"; param.ranges.def = 0.01f; break;
        case kParameterOp1Decay: param.name = "Op1 Decay"; param.symbol = "op1_decay"; param.ranges.def = 0.3f; break;
        case kParameterOp1Sustain: param.name = "Op1 Sustain"; param.symbol = "op1_sustain"; param.ranges.def = 0.7f; break;
        case kParameterOp1Release: param.name = "Op1 Release"; param.symbol = "op1_release"; param.ranges.def = 0.3f; break;
        case kParameterOp1Waveform: param.name = "Op1 Wave"; param.symbol = "op1_wave"; param.ranges.def = 0.0f; break;
        case kParameterOp2Multiplier: param.name = "Op2 Mult"; param.symbol = "op2_mult"; param.ranges.min = 0.5f; param.ranges.max = 15.0f; param.ranges.def = 1.0f; break;
        case kParameterOp2Level: param.name = "Op2 Level"; param.symbol = "op2_level"; param.ranges.def = 0.7f; break;
        case kParameterOp2Attack: param.name = "Op2 Attack"; param.symbol = "op2_attack"; param.ranges.def = 0.01f; break;
        case kParameterOp2Decay: param.name = "Op2 Decay"; param.symbol = "op2_decay"; param.ranges.def = 0.2f; break;
        case kParameterOp2Sustain: param.name = "Op2 Sustain"; param.symbol = "op2_sustain"; param.ranges.def = 0.5f; break;
        case kParameterOp2Release: param.name = "Op2 Release"; param.symbol = "op2_release"; param.ranges.def = 0.2f; break;
        case kParameterOp2Waveform: param.name = "Op2 Wave"; param.symbol = "op2_wave"; param.ranges.def = 0.0f; break;
        case kParameterFeedback: param.name = "Feedback"; param.symbol = "feedback"; param.ranges.def = 0.0f; break;
        case kParameterDetune: param.name = "Detune"; param.symbol = "detune"; param.ranges.def = 0.0f; break;
        case kParameterLFORate: param.name = "LFO Rate"; param.symbol = "lfo_rate"; param.ranges.min = 0.1f; param.ranges.max = 20.0f; param.ranges.def = 5.0f; break;
        case kParameterLFOPitchDepth: param.name = "LFO Pitch"; param.symbol = "lfo_pitch"; param.ranges.def = 0.0f; break;
        case kParameterLFOAmpDepth: param.name = "LFO Amp"; param.symbol = "lfo_amp"; param.ranges.def = 0.0f; break;
        case kParameterVelocitySensitivity: param.name = "Velocity"; param.symbol = "velocity"; param.ranges.def = 0.5f; break;
        case kParameterVolume: param.name = "Volume"; param.symbol = "volume"; param.ranges.def = 0.3f; break;
        }
    }

    float getParameterValue(uint32_t index) const override
    {
        switch (index) {
        case kParameterAlgorithm: return fAlgorithm;
        case kParameterOp1Multiplier: return fOp1Multiplier;
        case kParameterOp1Level: return fOp1Level;
        case kParameterOp1Attack: return fOp1Attack;
        case kParameterOp1Decay: return fOp1Decay;
        case kParameterOp1Sustain: return fOp1Sustain;
        case kParameterOp1Release: return fOp1Release;
        case kParameterOp1Waveform: return fOp1Waveform;
        case kParameterOp2Multiplier: return fOp2Multiplier;
        case kParameterOp2Level: return fOp2Level;
        case kParameterOp2Attack: return fOp2Attack;
        case kParameterOp2Decay: return fOp2Decay;
        case kParameterOp2Sustain: return fOp2Sustain;
        case kParameterOp2Release: return fOp2Release;
        case kParameterOp2Waveform: return fOp2Waveform;
        case kParameterFeedback: return fFeedback;
        case kParameterDetune: return fDetune;
        case kParameterLFORate: return fLFORate;
        case kParameterLFOPitchDepth: return fLFOPitchDepth;
        case kParameterLFOAmpDepth: return fLFOAmpDepth;
        case kParameterVelocitySensitivity: return fVelocitySensitivity;
        case kParameterVolume: return fVolume;
        default: return 0.0f;
        }
    }

    void setParameterValue(uint32_t index, float value) override
    {
        switch (index) {
        case kParameterAlgorithm: fAlgorithm = value; break;
        case kParameterOp1Multiplier: fOp1Multiplier = value; updateAllOperators(); break;
        case kParameterOp1Level: fOp1Level = value; updateAllOperators(); break;
        case kParameterOp1Attack: fOp1Attack = value; updateAllOperators(); break;
        case kParameterOp1Decay: fOp1Decay = value; updateAllOperators(); break;
        case kParameterOp1Sustain: fOp1Sustain = value; updateAllOperators(); break;
        case kParameterOp1Release: fOp1Release = value; updateAllOperators(); break;
        case kParameterOp1Waveform: fOp1Waveform = value; updateAllOperators(); break;
        case kParameterOp2Multiplier: fOp2Multiplier = value; updateAllOperators(); break;
        case kParameterOp2Level: fOp2Level = value; updateAllOperators(); break;
        case kParameterOp2Attack: fOp2Attack = value; updateAllOperators(); break;
        case kParameterOp2Decay: fOp2Decay = value; updateAllOperators(); break;
        case kParameterOp2Sustain: fOp2Sustain = value; updateAllOperators(); break;
        case kParameterOp2Release: fOp2Release = value; updateAllOperators(); break;
        case kParameterOp2Waveform: fOp2Waveform = value; updateAllOperators(); break;
        case kParameterFeedback: fFeedback = value; break;
        case kParameterDetune: fDetune = value; break;
        case kParameterLFORate: fLFORate = value; if (fLFO) synth_lfo_set_frequency(fLFO, fLFORate); break;
        case kParameterLFOPitchDepth: fLFOPitchDepth = value; break;
        case kParameterLFOAmpDepth: fLFOAmpDepth = value; break;
        case kParameterVelocitySensitivity: fVelocitySensitivity = value; break;
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
    void updateOperators(int idx)
    {
        if (idx < 0 || idx >= FM_VOICES) return;
        synth_fm_operator_set_multiplier(fVoices[idx].op1, fOp1Multiplier);
        synth_fm_operator_set_level(fVoices[idx].op1, fOp1Level);
        synth_fm_operator_set_attack(fVoices[idx].op1, 0.001f + fOp1Attack * 2.0f);
        synth_fm_operator_set_decay(fVoices[idx].op1, 0.01f + fOp1Decay * 2.0f);
        synth_fm_operator_set_sustain(fVoices[idx].op1, fOp1Sustain);
        synth_fm_operator_set_release(fVoices[idx].op1, 0.01f + fOp1Release * 2.0f);
        synth_fm_operator_set_waveform(fVoices[idx].op1, (OPL2Waveform)((int)(fOp1Waveform * 7.0f)));

        synth_fm_operator_set_multiplier(fVoices[idx].op2, fOp2Multiplier);
        synth_fm_operator_set_level(fVoices[idx].op2, fOp2Level);
        synth_fm_operator_set_attack(fVoices[idx].op2, 0.001f + fOp2Attack * 2.0f);
        synth_fm_operator_set_decay(fVoices[idx].op2, 0.01f + fOp2Decay * 2.0f);
        synth_fm_operator_set_sustain(fVoices[idx].op2, fOp2Sustain);
        synth_fm_operator_set_release(fVoices[idx].op2, 0.01f + fOp2Release * 2.0f);
        synth_fm_operator_set_waveform(fVoices[idx].op2, (OPL2Waveform)((int)(fOp2Waveform * 7.0f)));
    }

    void updateAllOperators()
    {
        for (int i = 0; i < FM_VOICES; i++) updateOperators(i);
    }

    void handleNoteOn(uint8_t note, uint8_t velocity)
    {
        int voice_idx = synth_voice_manager_allocate(fVoiceManager, note, velocity);
        if (voice_idx < 0 || voice_idx >= FM_VOICES) return;

        // Store velocity for this voice
        fVoiceVelocity[voice_idx] = velocity / 127.0f;

        synth_fm_operator_trigger(fVoices[voice_idx].op1);
        synth_fm_operator_trigger(fVoices[voice_idx].op2);
        fVoices[voice_idx].active = true;
    }

    void handleNoteOff(uint8_t note)
    {
        int voice_idx = synth_voice_manager_release(fVoiceManager, note);
        if (voice_idx < 0 || voice_idx >= FM_VOICES) return;
        synth_fm_operator_release(fVoices[voice_idx].op1);
        synth_fm_operator_release(fVoices[voice_idx].op2);
    }

    void renderFrame(float* outL, float* outR, uint32_t framePos, int sampleRate)
    {
        float mixL = 0.0f, mixR = 0.0f;
        float lfo_value = fLFO ? synth_lfo_process(fLFO, sampleRate) : 0.0f;

        for (int i = 0; i < FM_VOICES; i++) {
            const VoiceMeta* meta = synth_voice_manager_get_voice(fVoiceManager, i);
            if (!meta || meta->state == VOICE_INACTIVE) {
                fVoices[i].active = false;
                continue;
            }
            if (!fVoices[i].active) continue;

            // Calculate base frequency with detune
            float freq1 = 440.0f * powf(2.0f, (meta->note - 69) / 12.0f);
            float freq2 = freq1 * (1.0f + fDetune * 0.05f);  // Detune operator 2

            // Apply LFO to pitch
            freq1 *= 1.0f + lfo_value * fLFOPitchDepth * 0.05f;
            freq2 *= 1.0f + lfo_value * fLFOPitchDepth * 0.05f;

            // Process operators
            float mod_out = synth_fm_operator_process(fVoices[i].op2, freq2, 0.0f, sampleRate);
            float carrier_in = (fAlgorithm < 0.5f) ? mod_out * 2.0f : 0.0f;  // FM vs Additive
            float carrier_out = synth_fm_operator_process(fVoices[i].op1, freq1, carrier_in + fFeedback * fLastOutput[i], sampleRate);

            float sample = (fAlgorithm < 0.5f) ? carrier_out : (carrier_out + mod_out) * 0.5f;
            fLastOutput[i] = sample * fFeedback;

            // Apply velocity sensitivity
            float vel_scale = 1.0f - fVelocitySensitivity + (fVelocitySensitivity * fVoiceVelocity[i]);
            sample *= vel_scale;

            // Apply LFO amplitude modulation
            sample *= 1.0f + lfo_value * fLFOAmpDepth * 0.5f;

            if (!synth_fm_operator_is_active(fVoices[i].op1) && !synth_fm_operator_is_active(fVoices[i].op2)) {
                synth_voice_manager_stop_voice(fVoiceManager, i);
                fVoices[i].active = false;
            }

            mixL += sample;
            mixR += sample;
        }

        mixL *= 0.15f * fVolume;
        mixR *= 0.15f * fVolume;
        if (mixL > 1.0f) mixL = 1.0f; if (mixL < -1.0f) mixL = -1.0f;
        if (mixR > 1.0f) mixR = 1.0f; if (mixR < -1.0f) mixR = -1.0f;
        outL[framePos] = mixL;
        outR[framePos] = mixR;
    }

    SynthVoiceManager* fVoiceManager;
    FMVoice fVoices[FM_VOICES];
    SynthLFO* fLFO;
    float fLastOutput[FM_VOICES] = {0};
    float fVoiceVelocity[FM_VOICES] = {1.0f};

    float fAlgorithm, fOp1Multiplier, fOp1Level, fOp1Attack, fOp1Decay, fOp1Sustain, fOp1Release, fOp1Waveform;
    float fOp2Multiplier, fOp2Level, fOp2Attack, fOp2Decay, fOp2Sustain, fOp2Release, fOp2Waveform;
    float fFeedback, fDetune, fLFORate, fLFOPitchDepth, fLFOAmpDepth, fVelocitySensitivity, fVolume;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RG560_SynthPlugin)
};

Plugin* createPlugin() { return new RG560_SynthPlugin(); }

END_NAMESPACE_DISTRHO
