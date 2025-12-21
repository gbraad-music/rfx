#include "DistrhoPlugin.hpp"
#include "../../synth/synth_oscillator.h"
#include "../../synth/synth_filter_ladder.h"
#include "../../synth/synth_envelope.h"
#include "../../synth/synth_lfo.h"
#include "../../synth/synth_voice_manager.h"
#include "../../synth/synth_chorus.h"
#include <cstring>
#include <cmath>

START_NAMESPACE_DISTRHO

#define JUNO_VOICES 6

struct Juno106Voice {
    SynthOscillator* osc;
    SynthOscillator* sub_osc;
    SynthFilterLadder* filter;
    SynthEnvelope* envelope;
    bool active;
    int note;
    int velocity;
    float current_freq;
    float target_freq;
    bool sliding;
};

class RG106_SynthPlugin : public Plugin
{
public:
    RG106_SynthPlugin()
        : Plugin(kParameterCount, 0, 0)
        , fPulseWidth(0.5f)
        , fPWM(0.0f)
        , fSubLevel(0.3f)
        , fCutoff(0.5f)
        , fResonance(0.3f)
        , fEnvMod(0.5f)
        , fLFOMod(0.0f)
        , fKeyboardTracking(0.5f)
        , fHPFCutoff(0.0f)
        , fVCALevel(1.0f)
        , fAttack(0.01f)
        , fDecay(0.3f)
        , fSustain(0.7f)
        , fRelease(0.5f)
        , fLFOWaveform(0.0f)
        , fLFORate(5.0f)
        , fLFODelay(0.0f)
        , fLFOPitchDepth(0.0f)
        , fLFOAmpDepth(0.0f)
        , fChorusMode(0.0f)
        , fChorusRate(0.8f)
        , fChorusDepth(0.5f)
        , fVelocitySensitivity(0.5f)
        , fPortamento(0.0f)
        , fVolume(0.4f)
    {
        // Create voice manager
        fVoiceManager = synth_voice_manager_create(JUNO_VOICES);

        // Create shared LFO and chorus
        fLFO = synth_lfo_create();
        fChorus = synth_chorus_create();

        // Initialize voices
        for (int i = 0; i < JUNO_VOICES; i++) {
            fVoices[i].osc = synth_oscillator_create();
            fVoices[i].sub_osc = synth_oscillator_create();
            fVoices[i].filter = synth_filter_ladder_create();
            fVoices[i].envelope = synth_envelope_create();
            fVoices[i].active = false;
            fVoices[i].note = -1;
            fVoices[i].velocity = 0;
            fVoices[i].current_freq = 440.0f;
            fVoices[i].target_freq = 440.0f;
            fVoices[i].sliding = false;

            if (fVoices[i].osc) {
                synth_oscillator_set_waveform(fVoices[i].osc, SYNTH_OSC_SAW);
            }
            if (fVoices[i].sub_osc) {
                synth_oscillator_set_waveform(fVoices[i].sub_osc, SYNTH_OSC_SQUARE);
            }
        }

        updateEnvelope();

        if (fLFO) {
            synth_lfo_set_waveform(fLFO, SYNTH_LFO_TRIANGLE);
            synth_lfo_set_frequency(fLFO, fLFORate);
        }

        if (fChorus) {
            synth_chorus_set_mode(fChorus, CHORUS_OFF);
            synth_chorus_set_rate(fChorus, fChorusRate);
            synth_chorus_set_depth(fChorus, fChorusDepth);
        }
    }

    ~RG106_SynthPlugin() override
    {
        for (int i = 0; i < JUNO_VOICES; i++) {
            if (fVoices[i].osc) synth_oscillator_destroy(fVoices[i].osc);
            if (fVoices[i].sub_osc) synth_oscillator_destroy(fVoices[i].sub_osc);
            if (fVoices[i].filter) synth_filter_ladder_destroy(fVoices[i].filter);
            if (fVoices[i].envelope) synth_envelope_destroy(fVoices[i].envelope);
        }

        if (fVoiceManager) synth_voice_manager_destroy(fVoiceManager);
        if (fLFO) synth_lfo_destroy(fLFO);
        if (fChorus) synth_chorus_destroy(fChorus);
    }

protected:
    const char* getLabel() const override
    {
        return RG106_DISPLAY_NAME;
    }

    const char* getDescription() const override
    {
        return RG106_DESCRIPTION;
    }

    const char* getMaker() const override
    {
        return "Regroove";
    }

    const char* getHomePage() const override
    {
        return "https://music.gbraad.nl/regrooved/";
    }

    const char* getLicense() const override
    {
        return "GPL-3.0";
    }

    uint32_t getVersion() const override
    {
        return d_version(1, 0, 0);
    }

    int64_t getUniqueId() const override
    {
        return d_cconst('R', 'G', '1', '6');
    }

    void initParameter(uint32_t index, Parameter& param) override
    {
        param.hints = kParameterIsAutomatable;
        param.ranges.min = 0.0f;
        param.ranges.max = 1.0f;
        param.ranges.def = 0.5f;

        switch (index) {
        case kParameterPulseWidth:
            param.name = "Pulse Width";
            param.symbol = "pw";
            param.ranges.def = 0.5f;
            break;
        case kParameterPWM:
            param.name = "PWM";
            param.symbol = "pwm";
            param.ranges.def = 0.0f;
            break;
        case kParameterSubLevel:
            param.name = "Sub Level";
            param.symbol = "sub_level";
            param.ranges.def = 0.3f;
            break;
        case kParameterCutoff:
            param.name = "Cutoff";
            param.symbol = "cutoff";
            param.ranges.def = 0.5f;
            break;
        case kParameterResonance:
            param.name = "Resonance";
            param.symbol = "resonance";
            param.ranges.def = 0.3f;
            break;
        case kParameterEnvMod:
            param.name = "Env Mod";
            param.symbol = "env_mod";
            param.ranges.def = 0.5f;
            break;
        case kParameterLFOMod:
            param.name = "LFO Mod";
            param.symbol = "lfo_mod";
            param.ranges.def = 0.0f;
            break;
        case kParameterKeyboardTracking:
            param.name = "Kbd Track";
            param.symbol = "kbd_track";
            param.ranges.def = 0.5f;
            break;
        case kParameterHPFCutoff:
            param.name = "HPF Cutoff";
            param.symbol = "hpf_cutoff";
            param.ranges.def = 0.0f;
            break;
        case kParameterVCALevel:
            param.name = "VCA Level";
            param.symbol = "vca_level";
            param.ranges.def = 1.0f;
            break;
        case kParameterAttack:
            param.name = "Attack";
            param.symbol = "attack";
            param.ranges.def = 0.01f;
            break;
        case kParameterDecay:
            param.name = "Decay";
            param.symbol = "decay";
            param.ranges.def = 0.3f;
            break;
        case kParameterSustain:
            param.name = "Sustain";
            param.symbol = "sustain";
            param.ranges.def = 0.7f;
            break;
        case kParameterRelease:
            param.name = "Release";
            param.symbol = "release";
            param.ranges.def = 0.5f;
            break;
        case kParameterLFORate:
            param.name = "LFO Rate";
            param.symbol = "lfo_rate";
            param.ranges.min = 0.1f;
            param.ranges.max = 20.0f;
            param.ranges.def = 5.0f;
            break;
        case kParameterLFODelay:
            param.name = "LFO Delay";
            param.symbol = "lfo_delay";
            param.ranges.def = 0.0f;
            break;
        case kParameterLFOWaveform:
            param.name = "LFO Wave";
            param.symbol = "lfo_wave";
            param.ranges.def = 0.0f;
            break;
        case kParameterLFOPitchDepth:
            param.name = "LFO Pitch";
            param.symbol = "lfo_pitch";
            param.ranges.def = 0.0f;
            break;
        case kParameterLFOAmpDepth:
            param.name = "LFO Amp";
            param.symbol = "lfo_amp";
            param.ranges.def = 0.0f;
            break;
        case kParameterChorusMode:
            param.name = "Chorus Mode";
            param.symbol = "chorus_mode";
            param.ranges.def = 0.0f;
            break;
        case kParameterChorusRate:
            param.name = "Chorus Rate";
            param.symbol = "chorus_rate";
            param.ranges.min = 0.1f;
            param.ranges.max = 10.0f;
            param.ranges.def = 0.8f;
            break;
        case kParameterChorusDepth:
            param.name = "Chorus Depth";
            param.symbol = "chorus_depth";
            param.ranges.def = 0.5f;
            break;
        case kParameterVelocitySensitivity:
            param.name = "Velocity";
            param.symbol = "velocity";
            param.ranges.def = 0.5f;
            break;
        case kParameterPortamento:
            param.name = "Portamento";
            param.symbol = "portamento";
            param.ranges.def = 0.0f;
            break;
        case kParameterVolume:
            param.name = "Volume";
            param.symbol = "volume";
            param.ranges.def = 0.4f;
            break;
        }
    }

    float getParameterValue(uint32_t index) const override
    {
        switch (index) {
        case kParameterPulseWidth: return fPulseWidth;
        case kParameterPWM: return fPWM;
        case kParameterSubLevel: return fSubLevel;
        case kParameterCutoff: return fCutoff;
        case kParameterResonance: return fResonance;
        case kParameterEnvMod: return fEnvMod;
        case kParameterLFOMod: return fLFOMod;
        case kParameterKeyboardTracking: return fKeyboardTracking;
        case kParameterHPFCutoff: return fHPFCutoff;
        case kParameterVCALevel: return fVCALevel;
        case kParameterAttack: return fAttack;
        case kParameterDecay: return fDecay;
        case kParameterSustain: return fSustain;
        case kParameterRelease: return fRelease;
        case kParameterLFOWaveform: return fLFOWaveform;
        case kParameterLFORate: return fLFORate;
        case kParameterLFODelay: return fLFODelay;
        case kParameterLFOPitchDepth: return fLFOPitchDepth;
        case kParameterLFOAmpDepth: return fLFOAmpDepth;
        case kParameterChorusMode: return fChorusMode;
        case kParameterChorusRate: return fChorusRate;
        case kParameterChorusDepth: return fChorusDepth;
        case kParameterVelocitySensitivity: return fVelocitySensitivity;
        case kParameterPortamento: return fPortamento;
        case kParameterVolume: return fVolume;
        default: return 0.0f;
        }
    }

    void setParameterValue(uint32_t index, float value) override
    {
        switch (index) {
        case kParameterPulseWidth: fPulseWidth = value; break;
        case kParameterPWM: fPWM = value; break;
        case kParameterSubLevel: fSubLevel = value; break;
        case kParameterCutoff: fCutoff = value; break;
        case kParameterResonance: fResonance = value; break;
        case kParameterEnvMod: fEnvMod = value; break;
        case kParameterLFOMod: fLFOMod = value; break;
        case kParameterKeyboardTracking: fKeyboardTracking = value; break;
        case kParameterHPFCutoff: fHPFCutoff = value; break;
        case kParameterVCALevel: fVCALevel = value; break;
        case kParameterAttack: fAttack = value; updateEnvelope(); break;
        case kParameterDecay: fDecay = value; updateEnvelope(); break;
        case kParameterSustain: fSustain = value; updateEnvelope(); break;
        case kParameterRelease: fRelease = value; updateEnvelope(); break;
        case kParameterLFORate:
            fLFORate = value;
            if (fLFO) synth_lfo_set_frequency(fLFO, fLFORate);
            break;
        case kParameterLFODelay: fLFODelay = value; break;
        case kParameterLFOWaveform:
            fLFOWaveform = value;
            if (fLFO) {
                int waveform = (int)(value * 4.0f);  // 0-4 = sine, tri, saw, square, S&H
                if (waveform > 4) waveform = 4;
                synth_lfo_set_waveform(fLFO, (SynthLFOWaveform)waveform);
            }
            break;
        case kParameterLFOPitchDepth: fLFOPitchDepth = value; break;
        case kParameterLFOAmpDepth: fLFOAmpDepth = value; break;
        case kParameterChorusMode:
            fChorusMode = value;
            if (fChorus) {
                ChorusMode mode = CHORUS_OFF;
                if (value < 0.33f) mode = CHORUS_OFF;
                else if (value < 0.66f) mode = CHORUS_I;
                else mode = CHORUS_II;
                synth_chorus_set_mode(fChorus, mode);
            }
            break;
        case kParameterChorusRate:
            fChorusRate = value;
            if (fChorus) synth_chorus_set_rate(fChorus, fChorusRate);
            break;
        case kParameterChorusDepth:
            fChorusDepth = value;
            if (fChorus) synth_chorus_set_depth(fChorus, fChorusDepth);
            break;
        case kParameterVelocitySensitivity: fVelocitySensitivity = value; break;
        case kParameterPortamento: fPortamento = value; break;
        case kParameterVolume: fVolume = value; break;
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

            // Render audio up to this event
            while (framePos < event.frame) {
                renderFrame(outL, outR, framePos, sampleRate);
                framePos++;
            }

            // Handle MIDI event
            if (event.size != 3)
                continue;

            const uint8_t status = event.data[0] & 0xF0;
            const uint8_t note = event.data[1];
            const uint8_t velocity = event.data[2];

            if (status == 0x90 && velocity > 0) {
                handleNoteOn(note, velocity, sampleRate);
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
    void updateEnvelope()
    {
        for (int i = 0; i < JUNO_VOICES; i++) {
            if (!fVoices[i].envelope) continue;
            synth_envelope_set_attack(fVoices[i].envelope, 0.001f + fAttack * 3.0f);
            synth_envelope_set_decay(fVoices[i].envelope, 0.01f + fDecay * 3.0f);
            synth_envelope_set_sustain(fVoices[i].envelope, fSustain);
            synth_envelope_set_release(fVoices[i].envelope, 0.01f + fRelease * 5.0f);
        }
    }

    void handleNoteOn(uint8_t note, uint8_t velocity, int sampleRate)
    {
        if (!fVoiceManager) return;

        int voice_idx = synth_voice_manager_allocate(fVoiceManager, note, velocity);
        if (voice_idx < 0 || voice_idx >= JUNO_VOICES) return;

        Juno106Voice* voice = &fVoices[voice_idx];
        if (!voice->osc || !voice->envelope) return;

        float new_freq = 440.0f * powf(2.0f, (note - 69) / 12.0f);

        // Portamento - check if this voice was already playing
        bool shouldSlide = voice->active && fPortamento > 0.0f;

        voice->note = note;
        voice->velocity = velocity;

        if (shouldSlide) {
            // Slide to new note
            voice->target_freq = new_freq;
            voice->sliding = true;
        } else {
            // Fresh note
            voice->current_freq = new_freq;
            voice->target_freq = new_freq;
            voice->sliding = false;

            synth_oscillator_set_frequency(voice->osc, new_freq);
            synth_oscillator_set_frequency(voice->sub_osc, new_freq * 0.5f);

            synth_envelope_trigger(voice->envelope);
        }

        voice->active = true;
    }

    void handleNoteOff(uint8_t note)
    {
        if (!fVoiceManager) return;

        int voice_idx = synth_voice_manager_release(fVoiceManager, note);
        if (voice_idx < 0 || voice_idx >= JUNO_VOICES) return;

        Juno106Voice* voice = &fVoices[voice_idx];
        if (voice->envelope) {
            synth_envelope_release(voice->envelope);
        }
    }

    void renderFrame(float* outL, float* outR, uint32_t framePos, int sampleRate)
    {
        float mixL = 0.0f;
        float mixR = 0.0f;

        // Get LFO value
        float lfo_value = fLFO ? synth_lfo_process(fLFO, sampleRate) : 0.0f;

        // PWM
        float pw = fPulseWidth;
        if (fPWM > 0.0f) {
            pw += lfo_value * fPWM * 0.4f;
            if (pw < 0.1f) pw = 0.1f;
            if (pw > 0.9f) pw = 0.9f;
        }

        // Render all voices
        for (int i = 0; i < JUNO_VOICES; i++) {
            const VoiceMeta* meta = synth_voice_manager_get_voice(fVoiceManager, i);
            if (!meta || meta->state == VOICE_INACTIVE) {
                fVoices[i].active = false;
                continue;
            }

            if (!fVoices[i].active) continue;

            Juno106Voice* voice = &fVoices[i];

            // Handle portamento
            if (voice->sliding && fPortamento > 0.0f) {
                float slide_time = 0.001f + fPortamento * 0.5f; // 1ms to 500ms
                float slide_rate = (voice->target_freq - voice->current_freq) / (slide_time * sampleRate);

                voice->current_freq += slide_rate;

                if ((slide_rate > 0.0f && voice->current_freq >= voice->target_freq) ||
                    (slide_rate < 0.0f && voice->current_freq <= voice->target_freq)) {
                    voice->current_freq = voice->target_freq;
                    voice->sliding = false;
                }

                synth_oscillator_set_frequency(voice->osc, voice->current_freq);
                synth_oscillator_set_frequency(voice->sub_osc, voice->current_freq * 0.5f);
            }

            // LFO to pitch (vibrato)
            if (fLFOPitchDepth > 0.0f) {
                float pitch_mod = 1.0f + (lfo_value * fLFOPitchDepth * 0.05f); // ±5% max
                float current_freq = voice->sliding ? voice->current_freq : (440.0f * powf(2.0f, (voice->note - 69) / 12.0f));
                synth_oscillator_set_frequency(voice->osc, current_freq * pitch_mod);
                synth_oscillator_set_frequency(voice->sub_osc, current_freq * 0.5f * pitch_mod);
            }

            // Set pulse width
            synth_oscillator_set_pulse_width(voice->osc, pw);

            // Generate oscillators (Juno: saw + square mixed internally)
            float saw_sample = synth_oscillator_process(voice->osc, sampleRate);

            synth_oscillator_set_waveform(voice->osc, SYNTH_OSC_SQUARE);
            float square_sample = synth_oscillator_process(voice->osc, sampleRate);
            synth_oscillator_set_waveform(voice->osc, SYNTH_OSC_SAW);

            float sub_sample = synth_oscillator_process(voice->sub_osc, sampleRate) * fSubLevel;

            // Mix oscillators (Juno character)
            float sample = saw_sample * 0.5f + square_sample * 0.5f + sub_sample;

            // Envelope
            float env_value = synth_envelope_process(voice->envelope, sampleRate);

            // Check if voice finished
            if (env_value <= 0.0f && meta->state == VOICE_RELEASING) {
                synth_voice_manager_stop_voice(fVoiceManager, i);
                voice->active = false;
                continue;
            }

            // Update amplitude for voice stealing
            synth_voice_manager_update_amplitude(fVoiceManager, i, env_value);

            // Filter cutoff with modulation
            float cutoff = fCutoff;
            cutoff += fEnvMod * env_value;
            cutoff += fLFOMod * lfo_value * 0.3f;

            // Keyboard tracking
            if (fKeyboardTracking > 0.0f) {
                float note_offset = (meta->note - 60) / 60.0f;
                cutoff += note_offset * fKeyboardTracking * 0.5f;
            }

            if (cutoff > 1.0f) cutoff = 1.0f;
            if (cutoff < 0.0f) cutoff = 0.0f;

            synth_filter_ladder_set_cutoff(voice->filter, cutoff);
            synth_filter_ladder_set_resonance(voice->filter, fResonance);

            sample = synth_filter_ladder_process(voice->filter, sample, sampleRate);

            // Apply envelope and VCA
            sample *= env_value * fVCALevel;

            // LFO to amplitude (tremolo)
            if (fLFOAmpDepth > 0.0f) {
                sample *= 1.0f + lfo_value * fLFOAmpDepth * 0.5f;
            }

            // Velocity sensitivity
            if (fVelocitySensitivity > 0.0f) {
                float vel_scale = 1.0f - fVelocitySensitivity + (fVelocitySensitivity * (voice->velocity / 127.0f));
                sample *= vel_scale;
            }

            mixL += sample;
            mixR += sample;
        }

        // Reduce per-voice level for polyphony
        mixL *= 0.2f;
        mixR *= 0.2f;

        // Apply chorus
        if (fChorus) {
            float chorusL, chorusR;
            synth_chorus_process(fChorus, (mixL + mixR) * 0.5f, &chorusL, &chorusR, sampleRate);
            mixL = chorusL;
            mixR = chorusR;
        }

        // Master volume
        mixL *= fVolume;
        mixR *= fVolume;

        // Soft clipping
        if (mixL > 1.0f) mixL = 1.0f;
        if (mixL < -1.0f) mixL = -1.0f;
        if (mixR > 1.0f) mixR = 1.0f;
        if (mixR < -1.0f) mixR = -1.0f;

        outL[framePos] = mixL;
        outR[framePos] = mixR;
    }

    // Voice management
    SynthVoiceManager* fVoiceManager;
    Juno106Voice fVoices[JUNO_VOICES];

    // Shared components
    SynthLFO* fLFO;
    SynthChorus* fChorus;

    // Parameters
    float fPulseWidth;
    float fPWM;
    float fSubLevel;
    float fCutoff;
    float fResonance;
    float fEnvMod;
    float fLFOMod;
    float fKeyboardTracking;
    float fHPFCutoff;
    float fVCALevel;
    float fAttack;
    float fDecay;
    float fSustain;
    float fRelease;
    float fLFOWaveform;
    float fLFORate;
    float fLFODelay;
    float fLFOPitchDepth;
    float fLFOAmpDepth;
    float fChorusMode;
    float fChorusRate;
    float fChorusDepth;
    float fVelocitySensitivity;
    float fPortamento;
    float fVolume;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RG106_SynthPlugin)
};

Plugin* createPlugin()
{
    return new RG106_SynthPlugin();
}

END_NAMESPACE_DISTRHO
