#include "DistrhoPlugin.hpp"
#include "../../synth/synth_oscillator.h"
#include "../../synth/synth_noise.h"
#include "../../synth/synth_filter.h"
#include "../../synth/synth_envelope.h"
#include "../../synth/synth_voice_manager.h"
#include <cstring>
#include <cmath>

START_NAMESPACE_DISTRHO

#define MAX_DRUM_VOICES 7

enum DrumType {
    DRUM_BD = 0,
    DRUM_SD,
    DRUM_LT,
    DRUM_HT,
    DRUM_CH,
    DRUM_OH,
    DRUM_CY
};

struct DrumVoice {
    DrumType type;
    SynthOscillator* osc1;
    SynthOscillator* osc2;
    SynthNoise* noise;
    SynthFilter* filter;
    SynthEnvelope* env;
    SynthEnvelope* pitch_env;
    bool active;
};

class RG606_DrumPlugin : public Plugin
{
public:
    RG606_DrumPlugin()
        : Plugin(kParameterCount, 0, 0)
        , fBDLevel(0.7f), fBDTone(0.5f), fBDDecay(0.4f)
        , fSDLevel(0.6f), fSDTone(0.5f), fSDSnappy(0.5f)
        , fLTLevel(0.6f), fLTTuning(0.5f)
        , fHTLevel(0.6f), fHTTuning(0.5f)
        , fCHLevel(0.5f)
        , fOHLevel(0.5f), fOHDecay(0.5f)
        , fCYLevel(0.5f), fCYTone(0.5f)
        , fMasterVolume(0.5f)
    {
        fVoiceManager = synth_voice_manager_create(MAX_DRUM_VOICES);

        for (int i = 0; i < MAX_DRUM_VOICES; i++) {
            fVoices[i].osc1 = synth_oscillator_create();
            fVoices[i].osc2 = synth_oscillator_create();
            fVoices[i].noise = synth_noise_create();
            fVoices[i].filter = synth_filter_create();
            fVoices[i].env = synth_envelope_create();
            fVoices[i].pitch_env = synth_envelope_create();
            fVoices[i].active = false;
        }
    }

    ~RG606_DrumPlugin() override
    {
        for (int i = 0; i < MAX_DRUM_VOICES; i++) {
            if (fVoices[i].osc1) synth_oscillator_destroy(fVoices[i].osc1);
            if (fVoices[i].osc2) synth_oscillator_destroy(fVoices[i].osc2);
            if (fVoices[i].noise) synth_noise_destroy(fVoices[i].noise);
            if (fVoices[i].filter) synth_filter_destroy(fVoices[i].filter);
            if (fVoices[i].env) synth_envelope_destroy(fVoices[i].env);
            if (fVoices[i].pitch_env) synth_envelope_destroy(fVoices[i].pitch_env);
        }
        if (fVoiceManager) synth_voice_manager_destroy(fVoiceManager);
    }

protected:
    const char* getLabel() const override { return RG606_DISPLAY_NAME; }
    const char* getDescription() const override { return RG606_DESCRIPTION; }
    const char* getMaker() const override { return "Regroove"; }
    const char* getHomePage() const override { return "https://music.gbraad.nl/regrooved/"; }
    const char* getLicense() const override { return "GPL-3.0"; }
    uint32_t getVersion() const override { return d_version(1, 0, 0); }
    int64_t getUniqueId() const override { return d_cconst('R', 'G', '6', '6'); }

    void initParameter(uint32_t index, Parameter& param) override
    {
        param.hints = kParameterIsAutomatable;
        param.ranges.min = 0.0f;
        param.ranges.max = 1.0f;
        param.ranges.def = 0.5f;

        switch (index) {
        case kParameterBDLevel: param.name = "BD Level"; param.symbol = "bd_level"; param.ranges.def = 0.7f; break;
        case kParameterBDTone: param.name = "BD Tone"; param.symbol = "bd_tone"; break;
        case kParameterBDDecay: param.name = "BD Decay"; param.symbol = "bd_decay"; param.ranges.def = 0.4f; break;

        case kParameterSDLevel: param.name = "SD Level"; param.symbol = "sd_level"; param.ranges.def = 0.6f; break;
        case kParameterSDTone: param.name = "SD Tone"; param.symbol = "sd_tone"; break;
        case kParameterSDSnappy: param.name = "SD Snappy"; param.symbol = "sd_snappy"; break;

        case kParameterLTLevel: param.name = "LT Level"; param.symbol = "lt_level"; param.ranges.def = 0.6f; break;
        case kParameterLTTuning: param.name = "LT Tuning"; param.symbol = "lt_tuning"; break;

        case kParameterHTLevel: param.name = "HT Level"; param.symbol = "ht_level"; param.ranges.def = 0.6f; break;
        case kParameterHTTuning: param.name = "HT Tuning"; param.symbol = "ht_tuning"; break;

        case kParameterCHLevel: param.name = "CH Level"; param.symbol = "ch_level"; break;

        case kParameterOHLevel: param.name = "OH Level"; param.symbol = "oh_level"; break;
        case kParameterOHDecay: param.name = "OH Decay"; param.symbol = "oh_decay"; break;

        case kParameterCYLevel: param.name = "CY Level"; param.symbol = "cy_level"; break;
        case kParameterCYTone: param.name = "CY Tone"; param.symbol = "cy_tone"; break;

        case kParameterMasterVolume: param.name = "Master Volume"; param.symbol = "master"; break;
        }
    }

    float getParameterValue(uint32_t index) const override
    {
        switch (index) {
        case kParameterBDLevel: return fBDLevel;
        case kParameterBDTone: return fBDTone;
        case kParameterBDDecay: return fBDDecay;
        case kParameterSDLevel: return fSDLevel;
        case kParameterSDTone: return fSDTone;
        case kParameterSDSnappy: return fSDSnappy;
        case kParameterLTLevel: return fLTLevel;
        case kParameterLTTuning: return fLTTuning;
        case kParameterHTLevel: return fHTLevel;
        case kParameterHTTuning: return fHTTuning;
        case kParameterCHLevel: return fCHLevel;
        case kParameterOHLevel: return fOHLevel;
        case kParameterOHDecay: return fOHDecay;
        case kParameterCYLevel: return fCYLevel;
        case kParameterCYTone: return fCYTone;
        case kParameterMasterVolume: return fMasterVolume;
        default: return 0.0f;
        }
    }

    void setParameterValue(uint32_t index, float value) override
    {
        switch (index) {
        case kParameterBDLevel: fBDLevel = value; break;
        case kParameterBDTone: fBDTone = value; break;
        case kParameterBDDecay: fBDDecay = value; break;
        case kParameterSDLevel: fSDLevel = value; break;
        case kParameterSDTone: fSDTone = value; break;
        case kParameterSDSnappy: fSDSnappy = value; break;
        case kParameterLTLevel: fLTLevel = value; break;
        case kParameterLTTuning: fLTTuning = value; break;
        case kParameterHTLevel: fHTLevel = value; break;
        case kParameterHTTuning: fHTTuning = value; break;
        case kParameterCHLevel: fCHLevel = value; break;
        case kParameterOHLevel: fOHLevel = value; break;
        case kParameterOHDecay: fOHDecay = value; break;
        case kParameterCYLevel: fCYLevel = value; break;
        case kParameterCYTone: fCYTone = value; break;
        case kParameterMasterVolume: fMasterVolume = value; break;
        }
    }

    void run(const float**, float** outputs, uint32_t frames, const MidiEvent* midiEvents, uint32_t midiEventCount) override
    {
        float* outL = outputs[0];
        float* outR = outputs[1];
        std::memset(outL, 0, sizeof(float) * frames);
        std::memset(outR, 0, sizeof(float) * frames);

        uint32_t framePos = 0;
        const int sampleRate = (int)getSampleRate();

        for (uint32_t i = 0; i < midiEventCount; ++i) {
            const MidiEvent& event = midiEvents[i];

            while (framePos < event.frame && framePos < frames) {
                renderFrame(outL, outR, framePos, sampleRate);
                framePos++;
            }

            if (event.size != 3) continue;
            const uint8_t status = event.data[0] & 0xF0;
            const uint8_t note = event.data[1];
            const uint8_t velocity = event.data[2];

            if (status == 0x90 && velocity > 0) {
                triggerDrum(note, velocity, sampleRate);
            }
        }

        while (framePos < frames) {
            renderFrame(outL, outR, framePos, sampleRate);
            framePos++;
        }
    }

private:
    void triggerDrum(uint8_t note, uint8_t velocity, int sampleRate)
    {
        DrumType type;
        switch (note) {
        case MIDI_NOTE_BD: type = DRUM_BD; break;
        case MIDI_NOTE_SD: type = DRUM_SD; break;
        case MIDI_NOTE_LT: type = DRUM_LT; break;
        case MIDI_NOTE_HT: type = DRUM_HT; break;
        case MIDI_NOTE_CH: type = DRUM_CH; break;
        case MIDI_NOTE_OH: type = DRUM_OH; break;
        case MIDI_NOTE_CY: type = DRUM_CY; break;
        default: return;
        }

        int voice_idx = synth_voice_manager_allocate(fVoiceManager, note, velocity);
        if (voice_idx < 0 || voice_idx >= MAX_DRUM_VOICES) return;

        DrumVoice* voice = &fVoices[voice_idx];

        // Reset all components
        if (voice->osc1) synth_oscillator_reset(voice->osc1);
        if (voice->osc2) synth_oscillator_reset(voice->osc2);
        if (voice->noise) synth_noise_reset(voice->noise);
        if (voice->filter) synth_filter_reset(voice->filter);

        voice->type = type;
        setupDrumVoice(voice, type, sampleRate);
        voice->active = true;
    }

    void setupDrumVoice(DrumVoice* voice, DrumType type, int sampleRate)
    {
        (void)sampleRate;

        switch (type) {
        case DRUM_BD: // Bass Drum - punchy, short
            {
                float tone_freq = 50.0f + fBDTone * 30.0f;
                synth_oscillator_set_waveform(voice->osc1, SYNTH_OSC_SINE);
                synth_oscillator_set_frequency(voice->osc1, tone_freq);

                float decay = 0.08f + fBDDecay * 0.15f;
                synth_envelope_set_attack(voice->env, 0.001f);
                synth_envelope_set_decay(voice->env, decay);
                synth_envelope_set_sustain(voice->env, 0.0f);
                synth_envelope_set_release(voice->env, 0.01f);
                synth_envelope_trigger(voice->env);

                synth_envelope_set_attack(voice->pitch_env, 0.001f);
                synth_envelope_set_decay(voice->pitch_env, 0.05f);
                synth_envelope_set_sustain(voice->pitch_env, 0.0f);
                synth_envelope_set_release(voice->pitch_env, 0.01f);
                synth_envelope_trigger(voice->pitch_env);
            }
            break;

        case DRUM_SD: // Snare - dual tone + noise
            {
                float tone_freq = 180.0f + fSDTone * 100.0f;
                synth_oscillator_set_waveform(voice->osc1, SYNTH_OSC_SINE);
                synth_oscillator_set_frequency(voice->osc1, tone_freq);

                synth_oscillator_set_waveform(voice->osc2, SYNTH_OSC_SINE);
                synth_oscillator_set_frequency(voice->osc2, tone_freq * 1.6f);

                synth_filter_set_type(voice->filter, SYNTH_FILTER_HPF);
                synth_filter_set_cutoff(voice->filter, 0.2f + fSDSnappy * 0.6f);
                synth_filter_set_resonance(voice->filter, 0.3f);

                synth_envelope_set_attack(voice->env, 0.001f);
                synth_envelope_set_decay(voice->env, 0.15f);
                synth_envelope_set_sustain(voice->env, 0.0f);
                synth_envelope_set_release(voice->env, 0.01f);
                synth_envelope_trigger(voice->env);
            }
            break;

        case DRUM_LT: // Low Tom
            {
                float freq = 80.0f + fLTTuning * 60.0f;
                synth_oscillator_set_waveform(voice->osc1, SYNTH_OSC_SINE);
                synth_oscillator_set_frequency(voice->osc1, freq);

                synth_envelope_set_attack(voice->env, 0.001f);
                synth_envelope_set_decay(voice->env, 0.2f);
                synth_envelope_set_sustain(voice->env, 0.0f);
                synth_envelope_set_release(voice->env, 0.05f);
                synth_envelope_trigger(voice->env);

                synth_envelope_set_attack(voice->pitch_env, 0.001f);
                synth_envelope_set_decay(voice->pitch_env, 0.08f);
                synth_envelope_set_sustain(voice->pitch_env, 0.0f);
                synth_envelope_set_release(voice->pitch_env, 0.01f);
                synth_envelope_trigger(voice->pitch_env);
            }
            break;

        case DRUM_HT: // High Tom
            {
                float freq = 140.0f + fHTTuning * 100.0f;
                synth_oscillator_set_waveform(voice->osc1, SYNTH_OSC_SINE);
                synth_oscillator_set_frequency(voice->osc1, freq);

                synth_envelope_set_attack(voice->env, 0.001f);
                synth_envelope_set_decay(voice->env, 0.15f);
                synth_envelope_set_sustain(voice->env, 0.0f);
                synth_envelope_set_release(voice->env, 0.05f);
                synth_envelope_trigger(voice->env);

                synth_envelope_set_attack(voice->pitch_env, 0.001f);
                synth_envelope_set_decay(voice->pitch_env, 0.06f);
                synth_envelope_set_sustain(voice->pitch_env, 0.0f);
                synth_envelope_set_release(voice->pitch_env, 0.01f);
                synth_envelope_trigger(voice->pitch_env);
            }
            break;

        case DRUM_CH: // Closed Hi-Hat
            {
                synth_filter_set_type(voice->filter, SYNTH_FILTER_HPF);
                synth_filter_set_cutoff(voice->filter, 0.8f);
                synth_filter_set_resonance(voice->filter, 0.5f);

                synth_envelope_set_attack(voice->env, 0.001f);
                synth_envelope_set_decay(voice->env, 0.05f);
                synth_envelope_set_sustain(voice->env, 0.0f);
                synth_envelope_set_release(voice->env, 0.01f);
                synth_envelope_trigger(voice->env);
            }
            break;

        case DRUM_OH: // Open Hi-Hat
            {
                synth_filter_set_type(voice->filter, SYNTH_FILTER_HPF);
                synth_filter_set_cutoff(voice->filter, 0.7f);
                synth_filter_set_resonance(voice->filter, 0.7f);

                float decay = 0.2f + fOHDecay * 0.5f;
                synth_envelope_set_attack(voice->env, 0.001f);
                synth_envelope_set_decay(voice->env, decay);
                synth_envelope_set_sustain(voice->env, 0.0f);
                synth_envelope_set_release(voice->env, 0.05f);
                synth_envelope_trigger(voice->env);
            }
            break;

        case DRUM_CY: // Cymbal
            {
                synth_filter_set_type(voice->filter, SYNTH_FILTER_BPF);
                synth_filter_set_cutoff(voice->filter, 0.4f + fCYTone * 0.4f);
                synth_filter_set_resonance(voice->filter, 0.6f);

                synth_envelope_set_attack(voice->env, 0.001f);
                synth_envelope_set_decay(voice->env, 0.4f);
                synth_envelope_set_sustain(voice->env, 0.0f);
                synth_envelope_set_release(voice->env, 0.1f);
                synth_envelope_trigger(voice->env);
            }
            break;
        }
    }

    void renderFrame(float* outL, float* outR, uint32_t framePos, int sampleRate)
    {
        float mixL = 0.0f, mixR = 0.0f;

        for (int i = 0; i < MAX_DRUM_VOICES; i++) {
            if (!fVoices[i].active) continue;

            DrumVoice* voice = &fVoices[i];
            float sample = 0.0f;
            float env_value = synth_envelope_process(voice->env, sampleRate);

            if (env_value <= 0.0f) {
                synth_voice_manager_stop_voice(fVoiceManager, i);
                voice->active = false;
                continue;
            }

            switch (voice->type) {
            case DRUM_BD: // Bass Drum
                {
                    float pitch_env = synth_envelope_process(voice->pitch_env, sampleRate);
                    float base_freq = 50.0f + fBDTone * 30.0f;
                    float pitch_mod = base_freq * (1.0f + pitch_env * 0.5f);
                    synth_oscillator_set_frequency(voice->osc1, pitch_mod);

                    sample = synth_oscillator_process(voice->osc1, sampleRate);
                    sample *= env_value * fBDLevel * 0.6f;
                }
                break;

            case DRUM_SD: // Snare
                {
                    float tone1 = synth_oscillator_process(voice->osc1, sampleRate);
                    float tone2 = synth_oscillator_process(voice->osc2, sampleRate);
                    float noise = synth_noise_process(voice->noise);
                    noise = synth_filter_process(voice->filter, noise, sampleRate);

                    sample = (tone1 + tone2) * 0.3f + noise * 0.7f;
                    sample *= env_value * fSDLevel * 0.5f;
                }
                break;

            case DRUM_LT: // Low Tom
                {
                    float pitch_env = synth_envelope_process(voice->pitch_env, sampleRate);
                    float base_freq = 80.0f + fLTTuning * 60.0f;
                    float pitch_mod = base_freq * (1.0f + pitch_env * 0.3f);
                    synth_oscillator_set_frequency(voice->osc1, pitch_mod);

                    sample = synth_oscillator_process(voice->osc1, sampleRate);
                    sample *= env_value * fLTLevel * 0.5f;
                }
                break;

            case DRUM_HT: // High Tom
                {
                    float pitch_env = synth_envelope_process(voice->pitch_env, sampleRate);
                    float base_freq = 140.0f + fHTTuning * 100.0f;
                    float pitch_mod = base_freq * (1.0f + pitch_env * 0.3f);
                    synth_oscillator_set_frequency(voice->osc1, pitch_mod);

                    sample = synth_oscillator_process(voice->osc1, sampleRate);
                    sample *= env_value * fHTLevel * 0.5f;
                }
                break;

            case DRUM_CH: // Closed Hi-Hat
                {
                    float noise = synth_noise_process(voice->noise);
                    sample = synth_filter_process(voice->filter, noise, sampleRate);
                    sample *= env_value * fCHLevel * 0.4f;
                }
                break;

            case DRUM_OH: // Open Hi-Hat
                {
                    float noise = synth_noise_process(voice->noise);
                    sample = synth_filter_process(voice->filter, noise, sampleRate);
                    sample *= env_value * fOHLevel * 0.4f;
                }
                break;

            case DRUM_CY: // Cymbal
                {
                    float noise = synth_noise_process(voice->noise);
                    sample = synth_filter_process(voice->filter, noise, sampleRate);
                    sample *= env_value * fCYLevel * 0.4f;
                }
                break;
            }

            mixL += sample;
            mixR += sample;
        }

        mixL *= fMasterVolume;
        mixR *= fMasterVolume;

        if (mixL > 1.0f) mixL = 1.0f;
        if (mixL < -1.0f) mixL = -1.0f;
        if (mixR > 1.0f) mixR = 1.0f;
        if (mixR < -1.0f) mixR = -1.0f;

        outL[framePos] += mixL;
        outR[framePos] += mixR;
    }

    SynthVoiceManager* fVoiceManager;
    DrumVoice fVoices[MAX_DRUM_VOICES];

    float fBDLevel, fBDTone, fBDDecay;
    float fSDLevel, fSDTone, fSDSnappy;
    float fLTLevel, fLTTuning;
    float fHTLevel, fHTTuning;
    float fCHLevel;
    float fOHLevel, fOHDecay;
    float fCYLevel, fCYTone;
    float fMasterVolume;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RG606_DrumPlugin)
};

Plugin* createPlugin() { return new RG606_DrumPlugin(); }

END_NAMESPACE_DISTRHO
