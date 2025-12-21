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
    DRUM_MT,
    DRUM_HT,
    DRUM_RS,
    DRUM_HC
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
    int clap_stage;  // For hand clap multi-burst
    float clap_timer;
};

class RG909_DrumPlugin : public Plugin
{
public:
    RG909_DrumPlugin()
        : Plugin(kParameterCount, 0, 0)
        , fBDLevel(0.8f), fBDTune(0.5f), fBDDecay(0.5f), fBDAttack(0.0f)
        , fSDLevel(0.7f), fSDTone(0.5f), fSDSnappy(0.5f), fSDTuning(0.5f)
        , fLTLevel(0.7f), fLTTuning(0.5f), fLTDecay(0.5f)
        , fMTLevel(0.7f), fMTTuning(0.5f), fMTDecay(0.5f)
        , fHTLevel(0.7f), fHTTuning(0.5f), fHTDecay(0.5f)
        , fRSLevel(0.6f), fRSTuning(0.5f)
        , fHCLevel(0.6f), fHCTone(0.5f)
        , fMasterVolume(0.6f)
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
            fVoices[i].clap_stage = 0;
            fVoices[i].clap_timer = 0.0f;
        }
    }

    ~RG909_DrumPlugin() override
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
    const char* getLabel() const override { return RG909_DISPLAY_NAME; }
    const char* getDescription() const override { return RG909_DESCRIPTION; }
    const char* getMaker() const override { return "Regroove"; }
    const char* getHomePage() const override { return "https://music.gbraad.nl/regrooved/"; }
    const char* getLicense() const override { return "GPL-3.0"; }
    uint32_t getVersion() const override { return d_version(1, 0, 0); }
    int64_t getUniqueId() const override { return d_cconst('R', 'G', '9', '9'); }

    void initParameter(uint32_t index, Parameter& param) override
    {
        param.hints = kParameterIsAutomatable;
        param.ranges.min = 0.0f;
        param.ranges.max = 1.0f;
        param.ranges.def = 0.5f;

        switch (index) {
        case kParameterBDLevel: param.name = "BD Level"; param.symbol = "bd_level"; param.ranges.def = 0.8f; break;
        case kParameterBDTune: param.name = "BD Tune"; param.symbol = "bd_tune"; break;
        case kParameterBDDecay: param.name = "BD Decay"; param.symbol = "bd_decay"; break;
        case kParameterBDAttack: param.name = "BD Attack"; param.symbol = "bd_attack"; param.ranges.def = 0.0f; break;

        case kParameterSDLevel: param.name = "SD Level"; param.symbol = "sd_level"; param.ranges.def = 0.7f; break;
        case kParameterSDTone: param.name = "SD Tone"; param.symbol = "sd_tone"; break;
        case kParameterSDSnappy: param.name = "SD Snappy"; param.symbol = "sd_snappy"; break;
        case kParameterSDTuning: param.name = "SD Tuning"; param.symbol = "sd_tuning"; break;

        case kParameterLTLevel: param.name = "LT Level"; param.symbol = "lt_level"; param.ranges.def = 0.7f; break;
        case kParameterLTTuning: param.name = "LT Tuning"; param.symbol = "lt_tuning"; break;
        case kParameterLTDecay: param.name = "LT Decay"; param.symbol = "lt_decay"; break;

        case kParameterMTLevel: param.name = "MT Level"; param.symbol = "mt_level"; param.ranges.def = 0.7f; break;
        case kParameterMTTuning: param.name = "MT Tuning"; param.symbol = "mt_tuning"; break;
        case kParameterMTDecay: param.name = "MT Decay"; param.symbol = "mt_decay"; break;

        case kParameterHTLevel: param.name = "HT Level"; param.symbol = "ht_level"; param.ranges.def = 0.7f; break;
        case kParameterHTTuning: param.name = "HT Tuning"; param.symbol = "ht_tuning"; break;
        case kParameterHTDecay: param.name = "HT Decay"; param.symbol = "ht_decay"; break;

        case kParameterRSLevel: param.name = "RS Level"; param.symbol = "rs_level"; param.ranges.def = 0.6f; break;
        case kParameterRSTuning: param.name = "RS Tuning"; param.symbol = "rs_tuning"; break;

        case kParameterHCLevel: param.name = "HC Level"; param.symbol = "hc_level"; param.ranges.def = 0.6f; break;
        case kParameterHCTone: param.name = "HC Tone"; param.symbol = "hc_tone"; break;

        case kParameterMasterVolume: param.name = "Master"; param.symbol = "master"; param.ranges.def = 0.6f; break;
        }
    }

    float getParameterValue(uint32_t index) const override
    {
        switch (index) {
        case kParameterBDLevel: return fBDLevel;
        case kParameterBDTune: return fBDTune;
        case kParameterBDDecay: return fBDDecay;
        case kParameterBDAttack: return fBDAttack;
        case kParameterSDLevel: return fSDLevel;
        case kParameterSDTone: return fSDTone;
        case kParameterSDSnappy: return fSDSnappy;
        case kParameterSDTuning: return fSDTuning;
        case kParameterLTLevel: return fLTLevel;
        case kParameterLTTuning: return fLTTuning;
        case kParameterLTDecay: return fLTDecay;
        case kParameterMTLevel: return fMTLevel;
        case kParameterMTTuning: return fMTTuning;
        case kParameterMTDecay: return fMTDecay;
        case kParameterHTLevel: return fHTLevel;
        case kParameterHTTuning: return fHTTuning;
        case kParameterHTDecay: return fHTDecay;
        case kParameterRSLevel: return fRSLevel;
        case kParameterRSTuning: return fRSTuning;
        case kParameterHCLevel: return fHCLevel;
        case kParameterHCTone: return fHCTone;
        case kParameterMasterVolume: return fMasterVolume;
        default: return 0.0f;
        }
    }

    void setParameterValue(uint32_t index, float value) override
    {
        switch (index) {
        case kParameterBDLevel: fBDLevel = value; break;
        case kParameterBDTune: fBDTune = value; break;
        case kParameterBDDecay: fBDDecay = value; break;
        case kParameterBDAttack: fBDAttack = value; break;
        case kParameterSDLevel: fSDLevel = value; break;
        case kParameterSDTone: fSDTone = value; break;
        case kParameterSDSnappy: fSDSnappy = value; break;
        case kParameterSDTuning: fSDTuning = value; break;
        case kParameterLTLevel: fLTLevel = value; break;
        case kParameterLTTuning: fLTTuning = value; break;
        case kParameterLTDecay: fLTDecay = value; break;
        case kParameterMTLevel: fMTLevel = value; break;
        case kParameterMTTuning: fMTTuning = value; break;
        case kParameterMTDecay: fMTDecay = value; break;
        case kParameterHTLevel: fHTLevel = value; break;
        case kParameterHTTuning: fHTTuning = value; break;
        case kParameterHTDecay: fHTDecay = value; break;
        case kParameterRSLevel: fRSLevel = value; break;
        case kParameterRSTuning: fRSTuning = value; break;
        case kParameterHCLevel: fHCLevel = value; break;
        case kParameterHCTone: fHCTone = value; break;
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
        case MIDI_NOTE_MT: type = DRUM_MT; break;
        case MIDI_NOTE_HT: type = DRUM_HT; break;
        case MIDI_NOTE_RS: type = DRUM_RS; break;
        case MIDI_NOTE_HC: type = DRUM_HC; break;
        default: return;
        }

        int voice_idx = synth_voice_manager_allocate(fVoiceManager, note, velocity);
        if (voice_idx < 0 || voice_idx >= MAX_DRUM_VOICES) return;

        DrumVoice* voice = &fVoices[voice_idx];

        // Reset components
        if (voice->osc1) synth_oscillator_reset(voice->osc1);
        if (voice->osc2) synth_oscillator_reset(voice->osc2);
        if (voice->noise) synth_noise_reset(voice->noise);
        if (voice->filter) synth_filter_reset(voice->filter);

        voice->type = type;
        voice->clap_stage = 0;
        voice->clap_timer = 0.0f;
        setupDrumVoice(voice, type, sampleRate);
        voice->active = true;
    }

    void setupDrumVoice(DrumVoice* voice, DrumType type, int sampleRate)
    {
        (void)sampleRate;

        switch (type) {
        case DRUM_BD: // Bass Drum - deep punchy kick
            {
                float tune = 50.0f + fBDTune * 40.0f;  // 50-90 Hz
                synth_oscillator_set_waveform(voice->osc1, SYNTH_OSC_SINE);
                synth_oscillator_set_frequency(voice->osc1, tune);

                float attack = 0.001f + fBDAttack * 0.01f;
                float decay = 0.15f + fBDDecay * 0.35f;
                synth_envelope_set_attack(voice->env, attack);
                synth_envelope_set_decay(voice->env, decay);
                synth_envelope_set_sustain(voice->env, 0.0f);
                synth_envelope_set_release(voice->env, 0.01f);
                synth_envelope_trigger(voice->env);

                // Pitch envelope for initial click
                synth_envelope_set_attack(voice->pitch_env, 0.001f);
                synth_envelope_set_decay(voice->pitch_env, 0.03f);
                synth_envelope_set_sustain(voice->pitch_env, 0.0f);
                synth_envelope_set_release(voice->pitch_env, 0.01f);
                synth_envelope_trigger(voice->pitch_env);
            }
            break;

        case DRUM_SD: // Snare Drum - dual tone + noise
            {
                float tune = 160.0f + fSDTuning * 120.0f;  // 160-280 Hz
                float tone_offset = 100.0f + fSDTone * 100.0f;

                synth_oscillator_set_waveform(voice->osc1, SYNTH_OSC_SINE);
                synth_oscillator_set_frequency(voice->osc1, tune);

                synth_oscillator_set_waveform(voice->osc2, SYNTH_OSC_SINE);
                synth_oscillator_set_frequency(voice->osc2, tune + tone_offset);

                synth_filter_set_type(voice->filter, SYNTH_FILTER_HPF);
                synth_filter_set_cutoff(voice->filter, 0.3f + fSDSnappy * 0.5f);
                synth_filter_set_resonance(voice->filter, 0.4f);

                synth_envelope_set_attack(voice->env, 0.001f);
                synth_envelope_set_decay(voice->env, 0.12f);
                synth_envelope_set_sustain(voice->env, 0.0f);
                synth_envelope_set_release(voice->env, 0.05f);
                synth_envelope_trigger(voice->env);
            }
            break;

        case DRUM_LT: // Low Tom
            {
                float freq = 65.0f + fLTTuning * 80.0f;  // 65-145 Hz
                synth_oscillator_set_waveform(voice->osc1, SYNTH_OSC_SINE);
                synth_oscillator_set_frequency(voice->osc1, freq);

                float decay = 0.2f + fLTDecay * 0.5f;
                synth_envelope_set_attack(voice->env, 0.001f);
                synth_envelope_set_decay(voice->env, decay);
                synth_envelope_set_sustain(voice->env, 0.0f);
                synth_envelope_set_release(voice->env, 0.05f);
                synth_envelope_trigger(voice->env);

                synth_envelope_set_attack(voice->pitch_env, 0.001f);
                synth_envelope_set_decay(voice->pitch_env, 0.05f);
                synth_envelope_set_sustain(voice->pitch_env, 0.0f);
                synth_envelope_set_release(voice->pitch_env, 0.01f);
                synth_envelope_trigger(voice->pitch_env);
            }
            break;

        case DRUM_MT: // Mid Tom
            {
                float freq = 100.0f + fMTTuning * 100.0f;  // 100-200 Hz
                synth_oscillator_set_waveform(voice->osc1, SYNTH_OSC_SINE);
                synth_oscillator_set_frequency(voice->osc1, freq);

                float decay = 0.15f + fMTDecay * 0.4f;
                synth_envelope_set_attack(voice->env, 0.001f);
                synth_envelope_set_decay(voice->env, decay);
                synth_envelope_set_sustain(voice->env, 0.0f);
                synth_envelope_set_release(voice->env, 0.05f);
                synth_envelope_trigger(voice->env);

                synth_envelope_set_attack(voice->pitch_env, 0.001f);
                synth_envelope_set_decay(voice->pitch_env, 0.04f);
                synth_envelope_set_sustain(voice->pitch_env, 0.0f);
                synth_envelope_set_release(voice->pitch_env, 0.01f);
                synth_envelope_trigger(voice->pitch_env);
            }
            break;

        case DRUM_HT: // High Tom
            {
                float freq = 150.0f + fHTTuning * 150.0f;  // 150-300 Hz
                synth_oscillator_set_waveform(voice->osc1, SYNTH_OSC_SINE);
                synth_oscillator_set_frequency(voice->osc1, freq);

                float decay = 0.12f + fHTDecay * 0.35f;
                synth_envelope_set_attack(voice->env, 0.001f);
                synth_envelope_set_decay(voice->env, decay);
                synth_envelope_set_sustain(voice->env, 0.0f);
                synth_envelope_set_release(voice->env, 0.05f);
                synth_envelope_trigger(voice->env);

                synth_envelope_set_attack(voice->pitch_env, 0.001f);
                synth_envelope_set_decay(voice->pitch_env, 0.03f);
                synth_envelope_set_sustain(voice->pitch_env, 0.0f);
                synth_envelope_set_release(voice->pitch_env, 0.01f);
                synth_envelope_trigger(voice->pitch_env);
            }
            break;

        case DRUM_RS: // Rimshot - metallic click
            {
                float freq = 1000.0f + fRSTuning * 2000.0f;  // 1-3 kHz
                synth_oscillator_set_waveform(voice->osc1, SYNTH_OSC_SQUARE);
                synth_oscillator_set_frequency(voice->osc1, freq);

                synth_oscillator_set_waveform(voice->osc2, SYNTH_OSC_SQUARE);
                synth_oscillator_set_frequency(voice->osc2, freq * 1.4f);

                synth_envelope_set_attack(voice->env, 0.0001f);
                synth_envelope_set_decay(voice->env, 0.008f);
                synth_envelope_set_sustain(voice->env, 0.0f);
                synth_envelope_set_release(voice->env, 0.002f);
                synth_envelope_trigger(voice->env);
            }
            break;

        case DRUM_HC: // Hand Clap - filtered noise bursts
            {
                synth_filter_set_type(voice->filter, SYNTH_FILTER_BPF);
                synth_filter_set_cutoff(voice->filter, 0.4f + fHCTone * 0.4f);
                synth_filter_set_resonance(voice->filter, 0.7f);

                synth_envelope_set_attack(voice->env, 0.001f);
                synth_envelope_set_decay(voice->env, 0.05f);
                synth_envelope_set_sustain(voice->env, 0.0f);
                synth_envelope_set_release(voice->env, 0.02f);
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

            if (env_value <= 0.0f && voice->type != DRUM_HC) {
                synth_voice_manager_stop_voice(fVoiceManager, i);
                voice->active = false;
                continue;
            }

            switch (voice->type) {
            case DRUM_BD:
                {
                    float pitch_env = synth_envelope_process(voice->pitch_env, sampleRate);
                    float base_freq = 50.0f + fBDTune * 40.0f;
                    float pitch_mod = base_freq * (1.0f + pitch_env * 1.5f);
                    synth_oscillator_set_frequency(voice->osc1, pitch_mod);

                    sample = synth_oscillator_process(voice->osc1, sampleRate);
                    sample *= env_value * fBDLevel * 0.7f;
                }
                break;

            case DRUM_SD:
                {
                    float tone1 = synth_oscillator_process(voice->osc1, sampleRate);
                    float tone2 = synth_oscillator_process(voice->osc2, sampleRate);
                    float noise = synth_noise_process(voice->noise);
                    noise = synth_filter_process(voice->filter, noise, sampleRate);

                    sample = (tone1 + tone2) * 0.25f + noise * 0.75f;
                    sample *= env_value * fSDLevel * 0.6f;
                }
                break;

            case DRUM_LT:
                {
                    float pitch_env = synth_envelope_process(voice->pitch_env, sampleRate);
                    float base_freq = 65.0f + fLTTuning * 80.0f;
                    float pitch_mod = base_freq * (1.0f + pitch_env * 0.3f);
                    synth_oscillator_set_frequency(voice->osc1, pitch_mod);

                    sample = synth_oscillator_process(voice->osc1, sampleRate);
                    sample *= env_value * fLTLevel * 0.6f;
                }
                break;

            case DRUM_MT:
                {
                    float pitch_env = synth_envelope_process(voice->pitch_env, sampleRate);
                    float base_freq = 100.0f + fMTTuning * 100.0f;
                    float pitch_mod = base_freq * (1.0f + pitch_env * 0.3f);
                    synth_oscillator_set_frequency(voice->osc1, pitch_mod);

                    sample = synth_oscillator_process(voice->osc1, sampleRate);
                    sample *= env_value * fMTLevel * 0.6f;
                }
                break;

            case DRUM_HT:
                {
                    float pitch_env = synth_envelope_process(voice->pitch_env, sampleRate);
                    float base_freq = 150.0f + fHTTuning * 150.0f;
                    float pitch_mod = base_freq * (1.0f + pitch_env * 0.3f);
                    synth_oscillator_set_frequency(voice->osc1, pitch_mod);

                    sample = synth_oscillator_process(voice->osc1, sampleRate);
                    sample *= env_value * fHTLevel * 0.6f;
                }
                break;

            case DRUM_RS:
                {
                    float osc1 = synth_oscillator_process(voice->osc1, sampleRate);
                    float osc2 = synth_oscillator_process(voice->osc2, sampleRate);
                    sample = (osc1 + osc2) * 0.5f;
                    sample *= env_value * fRSLevel * 0.4f;
                }
                break;

            case DRUM_HC:
                {
                    // Multi-burst clap simulation
                    const float burst_times[] = {0.0f, 0.02f, 0.04f};
                    voice->clap_timer += 1.0f / sampleRate;

                    if (voice->clap_stage < 3 && voice->clap_timer >= burst_times[voice->clap_stage]) {
                        synth_envelope_trigger(voice->env);
                        voice->clap_stage++;
                    }

                    if (voice->clap_stage >= 3 && env_value <= 0.0f) {
                        synth_voice_manager_stop_voice(fVoiceManager, i);
                        voice->active = false;
                        continue;
                    }

                    float noise = synth_noise_process(voice->noise);
                    sample = synth_filter_process(voice->filter, noise, sampleRate);
                    sample *= env_value * fHCLevel * 0.5f;
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

    float fBDLevel, fBDTune, fBDDecay, fBDAttack;
    float fSDLevel, fSDTone, fSDSnappy, fSDTuning;
    float fLTLevel, fLTTuning, fLTDecay;
    float fMTLevel, fMTTuning, fMTDecay;
    float fHTLevel, fHTTuning, fHTDecay;
    float fRSLevel, fRSTuning;
    float fHCLevel, fHCTone;
    float fMasterVolume;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RG909_DrumPlugin)
};

Plugin* createPlugin() { return new RG909_DrumPlugin(); }

END_NAMESPACE_DISTRHO
