#include "DistrhoPlugin.hpp"
#include "../../synth/synth_oscillator.h"
#include "../../synth/synth_envelope.h"
#include "../../synth/synth_filter.h"
#include "../../synth/synth_lfo.h"
#include "../../synth/synth_voice_manager.h"
#include <cstring>
#include <cmath>

START_NAMESPACE_DISTRHO

#define MAX_VOICES 8
#define NUM_WAVEFORMS 8

struct K1Voice {
    // Dual DCO
    SynthOscillator* dco1;
    SynthOscillator* dco2;

    // Dual DCF
    SynthFilter* dcf1;
    SynthFilter* dcf2;

    // Envelopes
    SynthEnvelope* filt_env;  // Filter/Pitch envelope
    SynthEnvelope* amp_env;   // Amplitude envelope

    int note;
    int velocity;
    bool active;

    float phase1;
    float phase2;
};

class RGK1_SynthPlugin : public Plugin
{
public:
    RGK1_SynthPlugin()
        : Plugin(kParameterCount, 0, 0)
    {
        // Initialize waveforms (K1-style ROM PCM waves)
        initWaveforms();

        // Initialize default parameters
        initDefaults();

        // Create voice manager and LFO
        fVoiceManager = synth_voice_manager_create(MAX_VOICES);
        fLFO = synth_lfo_create();

        // Initialize voices
        for (int i = 0; i < MAX_VOICES; i++) {
            fVoices[i].dco1 = synth_oscillator_create();
            fVoices[i].dco2 = synth_oscillator_create();
            fVoices[i].dcf1 = synth_filter_create();
            fVoices[i].dcf2 = synth_filter_create();
            fVoices[i].filt_env = synth_envelope_create();
            fVoices[i].amp_env = synth_envelope_create();
            fVoices[i].note = -1;
            fVoices[i].velocity = 0;
            fVoices[i].active = false;
            fVoices[i].phase1 = 0.0f;
            fVoices[i].phase2 = 0.0f;
        }

        updateEnvelopes();
        updateLFO();
    }

    ~RGK1_SynthPlugin() override
    {
        for (int i = 0; i < MAX_VOICES; i++) {
            if (fVoices[i].dco1) synth_oscillator_destroy(fVoices[i].dco1);
            if (fVoices[i].dco2) synth_oscillator_destroy(fVoices[i].dco2);
            if (fVoices[i].dcf1) synth_filter_destroy(fVoices[i].dcf1);
            if (fVoices[i].dcf2) synth_filter_destroy(fVoices[i].dcf2);
            if (fVoices[i].filt_env) synth_envelope_destroy(fVoices[i].filt_env);
            if (fVoices[i].amp_env) synth_envelope_destroy(fVoices[i].amp_env);
        }
        if (fVoiceManager) synth_voice_manager_destroy(fVoiceManager);
        if (fLFO) synth_lfo_destroy(fLFO);
    }

protected:
    const char* getLabel() const override { return RGK1_DISPLAY_NAME; }
    const char* getDescription() const override { return RGK1_DESCRIPTION; }
    const char* getMaker() const override { return "Regroove"; }
    const char* getHomePage() const override { return "https://music.gbraad.nl/regrooved/"; }
    const char* getLicense() const override { return "GPL-3.0"; }
    uint32_t getVersion() const override { return d_version(1, 0, 0); }
    int64_t getUniqueId() const override { return d_cconst('R', 'G', 'K', '1'); }

    void initParameter(uint32_t index, Parameter& param) override
    {
        param.hints = kParameterIsAutomatable;
        param.ranges.min = 0.0f;
        param.ranges.max = 1.0f;
        param.ranges.def = 0.5f;

        switch (index) {
        // DCO 1
        case kParameterDCO1Wave:
            param.name = "DCO1 Wave";
            param.symbol = "dco1_wave";
            param.ranges.def = 0.0f;
            break;
        case kParameterDCO1Octave:
            param.name = "DCO1 Octave";
            param.symbol = "dco1_octave";
            param.ranges.min = -2.0f;
            param.ranges.max = 2.0f;
            param.ranges.def = 0.0f;
            break;
        case kParameterDCO1Level:
            param.name = "DCO1 Level";
            param.symbol = "dco1_level";
            param.ranges.def = 0.7f;
            break;
        case kParameterDCO1Detune:
            param.name = "DCO1 Detune";
            param.symbol = "dco1_detune";
            param.ranges.min = -1.0f;
            param.ranges.max = 1.0f;
            param.ranges.def = 0.0f;
            break;

        // DCO 2
        case kParameterDCO2Wave:
            param.name = "DCO2 Wave";
            param.symbol = "dco2_wave";
            param.ranges.def = 0.125f;
            break;
        case kParameterDCO2Octave:
            param.name = "DCO2 Octave";
            param.symbol = "dco2_octave";
            param.ranges.min = -2.0f;
            param.ranges.max = 2.0f;
            param.ranges.def = 0.0f;
            break;
        case kParameterDCO2Level:
            param.name = "DCO2 Level";
            param.symbol = "dco2_level";
            param.ranges.def = 0.7f;
            break;
        case kParameterDCO2Detune:
            param.name = "DCO2 Detune";
            param.symbol = "dco2_detune";
            param.ranges.min = -1.0f;
            param.ranges.max = 1.0f;
            param.ranges.def = 0.0f;
            break;

        // DCF 1
        case kParameterDCF1Cutoff:
            param.name = "DCF1 Cutoff";
            param.symbol = "dcf1_cutoff";
            param.ranges.def = 0.7f;
            break;
        case kParameterDCF1Resonance:
            param.name = "DCF1 Resonance";
            param.symbol = "dcf1_resonance";
            param.ranges.def = 0.3f;
            break;
        case kParameterDCF1EnvDepth:
            param.name = "DCF1 Env";
            param.symbol = "dcf1_env";
            param.ranges.def = 0.5f;
            break;
        case kParameterDCF1KeyTrack:
            param.name = "DCF1 KeyTrk";
            param.symbol = "dcf1_keytrack";
            param.ranges.def = 0.3f;
            break;

        // DCF 2
        case kParameterDCF2Cutoff:
            param.name = "DCF2 Cutoff";
            param.symbol = "dcf2_cutoff";
            param.ranges.def = 0.7f;
            break;
        case kParameterDCF2Resonance:
            param.name = "DCF2 Resonance";
            param.symbol = "dcf2_resonance";
            param.ranges.def = 0.3f;
            break;
        case kParameterDCF2EnvDepth:
            param.name = "DCF2 Env";
            param.symbol = "dcf2_env";
            param.ranges.def = 0.5f;
            break;
        case kParameterDCF2KeyTrack:
            param.name = "DCF2 KeyTrk";
            param.symbol = "dcf2_keytrack";
            param.ranges.def = 0.3f;
            break;

        // Filter Envelope
        case kParameterFiltAttack:
            param.name = "Filt Attack";
            param.symbol = "filt_attack";
            param.ranges.def = 0.01f;
            break;
        case kParameterFiltDecay:
            param.name = "Filt Decay";
            param.symbol = "filt_decay";
            param.ranges.def = 0.3f;
            break;
        case kParameterFiltSustain:
            param.name = "Filt Sustain";
            param.symbol = "filt_sustain";
            param.ranges.def = 0.5f;
            break;
        case kParameterFiltRelease:
            param.name = "Filt Release";
            param.symbol = "filt_release";
            param.ranges.def = 0.5f;
            break;

        // Amp Envelope
        case kParameterAmpAttack:
            param.name = "Amp Attack";
            param.symbol = "amp_attack";
            param.ranges.def = 0.01f;
            break;
        case kParameterAmpDecay:
            param.name = "Amp Decay";
            param.symbol = "amp_decay";
            param.ranges.def = 0.3f;
            break;
        case kParameterAmpSustain:
            param.name = "Amp Sustain";
            param.symbol = "amp_sustain";
            param.ranges.def = 0.7f;
            break;
        case kParameterAmpRelease:
            param.name = "Amp Release";
            param.symbol = "amp_release";
            param.ranges.def = 0.5f;
            break;

        // LFO
        case kParameterLFOWave:
            param.name = "LFO Wave";
            param.symbol = "lfo_wave";
            param.ranges.def = 0.0f;
            break;
        case kParameterLFORate:
            param.name = "LFO Rate";
            param.symbol = "lfo_rate";
            param.ranges.min = 0.1f;
            param.ranges.max = 20.0f;
            param.ranges.def = 5.0f;
            break;
        case kParameterLFOPitchDepth:
            param.name = "LFO Pitch";
            param.symbol = "lfo_pitch";
            param.ranges.def = 0.0f;
            break;
        case kParameterLFOFilterDepth:
            param.name = "LFO Filter";
            param.symbol = "lfo_filter";
            param.ranges.def = 0.0f;
            break;
        case kParameterLFOAmpDepth:
            param.name = "LFO Amp";
            param.symbol = "lfo_amp";
            param.ranges.def = 0.0f;
            break;

        // Master
        case kParameterVelocitySensitivity:
            param.name = "Velocity";
            param.symbol = "velocity";
            param.ranges.def = 0.5f;
            break;
        case kParameterVolume:
            param.name = "Volume";
            param.symbol = "volume";
            param.ranges.def = 0.7f;
            break;
        }
    }

    float getParameterValue(uint32_t index) const override
    {
        if (index < kParameterCount)
            return fParameters[index];
        return 0.0f;
    }

    void setParameterValue(uint32_t index, float value) override
    {
        if (index < kParameterCount) {
            fParameters[index] = value;

            // Update envelopes when changed
            if (index >= kParameterFiltAttack && index <= kParameterAmpRelease) {
                updateEnvelopes();
            }

            // Update LFO when changed
            if (index >= kParameterLFOWave && index <= kParameterLFORate) {
                updateLFO();
            }
        }
    }

    void run(const float**, float** outputs, uint32_t frames,
             const MidiEvent* midiEvents, uint32_t midiEventCount) override
    {
        float* outL = outputs[0];
        float* outR = outputs[1];

        std::memset(outL, 0, sizeof(float) * frames);
        std::memset(outR, 0, sizeof(float) * frames);

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

            if (status == 0x90 && velocity > 0) {
                handleNoteOn(note, velocity, sampleRate);
            } else if (status == 0x80 || (status == 0x90 && velocity == 0)) {
                handleNoteOff(note);
            }
        }

        while (framePos < frames) {
            renderFrame(outL, outR, framePos, sampleRate);
            framePos++;
        }
    }

private:
    void initWaveforms()
    {
        // K1-style PCM waveforms (simplified versions)
        // Waveform 0: Sine
        for (int i = 0; i < WAVETABLE_SIZE; i++) {
            fWaveforms[0][i] = sinf((float)i / WAVETABLE_SIZE * 2.0f * M_PI);
        }

        // Waveform 1: Saw
        for (int i = 0; i < WAVETABLE_SIZE; i++) {
            fWaveforms[1][i] = 2.0f * ((float)i / WAVETABLE_SIZE) - 1.0f;
        }

        // Waveform 2: Square
        for (int i = 0; i < WAVETABLE_SIZE; i++) {
            fWaveforms[2][i] = (i < WAVETABLE_SIZE/2) ? 1.0f : -1.0f;
        }

        // Waveform 3: Triangle
        for (int i = 0; i < WAVETABLE_SIZE; i++) {
            float t = (float)i / WAVETABLE_SIZE;
            fWaveforms[3][i] = (t < 0.5f) ? (4.0f * t - 1.0f) : (3.0f - 4.0f * t);
        }

        // Waveform 4: Pulse 25%
        for (int i = 0; i < WAVETABLE_SIZE; i++) {
            fWaveforms[4][i] = (i < WAVETABLE_SIZE/4) ? 1.0f : -1.0f;
        }

        // Waveform 5: Organ-like (additive harmonics)
        for (int i = 0; i < WAVETABLE_SIZE; i++) {
            float t = (float)i / WAVETABLE_SIZE * 2.0f * M_PI;
            fWaveforms[5][i] = sinf(t) + 0.5f*sinf(2.0f*t) + 0.25f*sinf(3.0f*t);
            fWaveforms[5][i] /= 1.75f;  // Normalize
        }

        // Waveform 6: Harmonic sweep
        for (int i = 0; i < WAVETABLE_SIZE; i++) {
            float t = (float)i / WAVETABLE_SIZE * 2.0f * M_PI;
            fWaveforms[6][i] = sinf(t) + 0.3f*sinf(4.0f*t) + 0.2f*sinf(7.0f*t);
            fWaveforms[6][i] /= 1.5f;
        }

        // Waveform 7: Formant-like
        for (int i = 0; i < WAVETABLE_SIZE; i++) {
            float t = (float)i / WAVETABLE_SIZE * 2.0f * M_PI;
            fWaveforms[7][i] = sinf(t) * (1.0f + 0.5f*sinf(5.0f*t));
            fWaveforms[7][i] /= 1.5f;
        }
    }

    void initDefaults()
    {
        for (int i = 0; i < kParameterCount; i++) {
            fParameters[i] = 0.5f;
        }

        fParameters[kParameterDCO1Wave] = 0.0f;
        fParameters[kParameterDCO1Octave] = 0.0f;
        fParameters[kParameterDCO1Level] = 0.7f;
        fParameters[kParameterDCO1Detune] = 0.0f;

        fParameters[kParameterDCO2Wave] = 0.125f;
        fParameters[kParameterDCO2Octave] = 0.0f;
        fParameters[kParameterDCO2Level] = 0.7f;
        fParameters[kParameterDCO2Detune] = 0.0f;

        fParameters[kParameterDCF1Cutoff] = 0.7f;
        fParameters[kParameterDCF1Resonance] = 0.3f;
        fParameters[kParameterDCF1EnvDepth] = 0.5f;
        fParameters[kParameterDCF1KeyTrack] = 0.3f;

        fParameters[kParameterDCF2Cutoff] = 0.7f;
        fParameters[kParameterDCF2Resonance] = 0.3f;
        fParameters[kParameterDCF2EnvDepth] = 0.5f;
        fParameters[kParameterDCF2KeyTrack] = 0.3f;

        fParameters[kParameterFiltAttack] = 0.01f;
        fParameters[kParameterFiltDecay] = 0.3f;
        fParameters[kParameterFiltSustain] = 0.5f;
        fParameters[kParameterFiltRelease] = 0.5f;

        fParameters[kParameterAmpAttack] = 0.01f;
        fParameters[kParameterAmpDecay] = 0.3f;
        fParameters[kParameterAmpSustain] = 0.7f;
        fParameters[kParameterAmpRelease] = 0.5f;

        fParameters[kParameterLFOWave] = 0.0f;
        fParameters[kParameterLFORate] = 5.0f;
        fParameters[kParameterLFOPitchDepth] = 0.0f;
        fParameters[kParameterLFOFilterDepth] = 0.0f;
        fParameters[kParameterLFOAmpDepth] = 0.0f;

        fParameters[kParameterVelocitySensitivity] = 0.5f;
        fParameters[kParameterVolume] = 0.7f;
    }

    void updateEnvelopes()
    {
        for (int i = 0; i < MAX_VOICES; i++) {
            if (fVoices[i].filt_env) {
                synth_envelope_set_attack(fVoices[i].filt_env, 0.001f + fParameters[kParameterFiltAttack] * 2.0f);
                synth_envelope_set_decay(fVoices[i].filt_env, 0.01f + fParameters[kParameterFiltDecay] * 3.0f);
                synth_envelope_set_sustain(fVoices[i].filt_env, fParameters[kParameterFiltSustain]);
                synth_envelope_set_release(fVoices[i].filt_env, 0.01f + fParameters[kParameterFiltRelease] * 5.0f);
            }
            if (fVoices[i].amp_env) {
                synth_envelope_set_attack(fVoices[i].amp_env, 0.001f + fParameters[kParameterAmpAttack] * 2.0f);
                synth_envelope_set_decay(fVoices[i].amp_env, 0.01f + fParameters[kParameterAmpDecay] * 3.0f);
                synth_envelope_set_sustain(fVoices[i].amp_env, fParameters[kParameterAmpSustain]);
                synth_envelope_set_release(fVoices[i].amp_env, 0.01f + fParameters[kParameterAmpRelease] * 5.0f);
            }
        }
    }

    void updateLFO()
    {
        if (fLFO) {
            int waveform = (int)(fParameters[kParameterLFOWave] * 4.0f);
            if (waveform > 4) waveform = 4;
            synth_lfo_set_waveform(fLFO, (SynthLFOWaveform)waveform);
            synth_lfo_set_frequency(fLFO, fParameters[kParameterLFORate]);
        }
    }

    void handleNoteOn(uint8_t note, uint8_t velocity, int)
    {
        if (!fVoiceManager) return;

        int voice_idx = synth_voice_manager_allocate(fVoiceManager, note, velocity);
        if (voice_idx < 0 || voice_idx >= MAX_VOICES) return;

        K1Voice* voice = &fVoices[voice_idx];

        voice->note = note;
        voice->velocity = velocity;
        voice->phase1 = 0.0f;
        voice->phase2 = 0.0f;
        voice->active = true;

        if (voice->filt_env) synth_envelope_trigger(voice->filt_env);
        if (voice->amp_env) synth_envelope_trigger(voice->amp_env);
    }

    void handleNoteOff(uint8_t note)
    {
        if (!fVoiceManager) return;

        int voice_idx = synth_voice_manager_release(fVoiceManager, note);
        if (voice_idx < 0 || voice_idx >= MAX_VOICES) return;

        K1Voice* voice = &fVoices[voice_idx];
        if (voice->filt_env) synth_envelope_release(voice->filt_env);
        if (voice->amp_env) synth_envelope_release(voice->amp_env);
    }

    void renderFrame(float* outL, float* outR, uint32_t framePos, int sampleRate)
    {
        float mixL = 0.0f;
        float mixR = 0.0f;

        // Get LFO value
        float lfo_value = fLFO ? synth_lfo_process(fLFO, sampleRate) : 0.0f;

        for (int i = 0; i < MAX_VOICES; i++) {
            const VoiceMeta* meta = synth_voice_manager_get_voice(fVoiceManager, i);
            if (!meta || meta->state == VOICE_INACTIVE) {
                fVoices[i].active = false;
                continue;
            }

            if (!fVoices[i].active) continue;

            K1Voice* voice = &fVoices[i];

            float filt_env = synth_envelope_process(voice->filt_env, sampleRate);
            float amp_env = synth_envelope_process(voice->amp_env, sampleRate);

            if (amp_env <= 0.0f && meta->state == VOICE_RELEASING) {
                synth_voice_manager_stop_voice(fVoiceManager, i);
                voice->active = false;
                continue;
            }

            synth_voice_manager_update_amplitude(fVoiceManager, i, amp_env);

            // Calculate base frequency
            float base_freq = 440.0f * powf(2.0f, (voice->note - 69) / 12.0f);

            // DCO1 frequency with octave, detune, and LFO
            float dco1_freq = base_freq * powf(2.0f, fParameters[kParameterDCO1Octave]);
            dco1_freq *= powf(2.0f, fParameters[kParameterDCO1Detune] / 12.0f);
            dco1_freq *= powf(2.0f, lfo_value * fParameters[kParameterLFOPitchDepth] * 0.05f);

            // DCO2 frequency with octave, detune, and LFO
            float dco2_freq = base_freq * powf(2.0f, fParameters[kParameterDCO2Octave]);
            dco2_freq *= powf(2.0f, fParameters[kParameterDCO2Detune] / 12.0f);
            dco2_freq *= powf(2.0f, lfo_value * fParameters[kParameterLFOPitchDepth] * 0.05f);

            // Read from wavetables
            int wave1_idx = (int)(fParameters[kParameterDCO1Wave] * (NUM_WAVEFORMS - 1));
            int wave2_idx = (int)(fParameters[kParameterDCO2Wave] * (NUM_WAVEFORMS - 1));

            float sample1 = readWavetable(wave1_idx, voice->phase1);
            float sample2 = readWavetable(wave2_idx, voice->phase2);

            voice->phase1 += (dco1_freq / sampleRate) * WAVETABLE_SIZE;
            voice->phase2 += (dco2_freq / sampleRate) * WAVETABLE_SIZE;
            while (voice->phase1 >= WAVETABLE_SIZE) voice->phase1 -= WAVETABLE_SIZE;
            while (voice->phase2 >= WAVETABLE_SIZE) voice->phase2 -= WAVETABLE_SIZE;

            // DCF1 - Filter for DCO1
            float dcf1_cutoff = fParameters[kParameterDCF1Cutoff];
            dcf1_cutoff += fParameters[kParameterDCF1EnvDepth] * filt_env;
            dcf1_cutoff += (voice->note - 60) / 60.0f * fParameters[kParameterDCF1KeyTrack] * 0.5f;
            dcf1_cutoff += lfo_value * fParameters[kParameterLFOFilterDepth] * 0.3f;
            dcf1_cutoff = (dcf1_cutoff < 0.0f) ? 0.0f : (dcf1_cutoff > 1.0f) ? 1.0f : dcf1_cutoff;

            if (voice->dcf1) {
                synth_filter_set_cutoff(voice->dcf1, dcf1_cutoff);
                synth_filter_set_resonance(voice->dcf1, fParameters[kParameterDCF1Resonance]);
                sample1 = synth_filter_process(voice->dcf1, sample1, sampleRate);
            }

            // DCF2 - Filter for DCO2
            float dcf2_cutoff = fParameters[kParameterDCF2Cutoff];
            dcf2_cutoff += fParameters[kParameterDCF2EnvDepth] * filt_env;
            dcf2_cutoff += (voice->note - 60) / 60.0f * fParameters[kParameterDCF2KeyTrack] * 0.5f;
            dcf2_cutoff += lfo_value * fParameters[kParameterLFOFilterDepth] * 0.3f;
            dcf2_cutoff = (dcf2_cutoff < 0.0f) ? 0.0f : (dcf2_cutoff > 1.0f) ? 1.0f : dcf2_cutoff;

            if (voice->dcf2) {
                synth_filter_set_cutoff(voice->dcf2, dcf2_cutoff);
                synth_filter_set_resonance(voice->dcf2, fParameters[kParameterDCF2Resonance]);
                sample2 = synth_filter_process(voice->dcf2, sample2, sampleRate);
            }

            // Mix DCO1 and DCO2
            float sample = sample1 * fParameters[kParameterDCO1Level] +
                          sample2 * fParameters[kParameterDCO2Level];

            // Apply amp envelope
            sample *= amp_env;

            // LFO amplitude modulation
            sample *= 1.0f + lfo_value * fParameters[kParameterLFOAmpDepth] * 0.5f;

            // Velocity sensitivity
            float vel_scale = 1.0f - fParameters[kParameterVelocitySensitivity] +
                             (fParameters[kParameterVelocitySensitivity] * (voice->velocity / 127.0f));
            sample *= vel_scale;

            mixL += sample;
            mixR += sample;
        }

        // Reduce per-voice level for polyphony
        mixL *= 0.25f;
        mixR *= 0.25f;

        // Apply master volume
        mixL *= fParameters[kParameterVolume];
        mixR *= fParameters[kParameterVolume];

        // Soft clipping
        if (mixL > 1.0f) mixL = 1.0f;
        if (mixL < -1.0f) mixL = -1.0f;
        if (mixR > 1.0f) mixR = 1.0f;
        if (mixR < -1.0f) mixR = -1.0f;

        outL[framePos] = mixL;
        outR[framePos] = mixR;
    }

    float readWavetable(int waveform, float phase)
    {
        if (waveform < 0 || waveform >= NUM_WAVEFORMS) waveform = 0;

        float int_part;
        float frac = modff(phase, &int_part);
        int idx1 = (int)int_part % WAVETABLE_SIZE;
        int idx2 = (idx1 + 1) % WAVETABLE_SIZE;

        return fWaveforms[waveform][idx1] * (1.0f - frac) +
               fWaveforms[waveform][idx2] * frac;
    }

    // Voice management
    SynthVoiceManager* fVoiceManager;
    K1Voice fVoices[MAX_VOICES];
    SynthLFO* fLFO;

    // Waveforms
    float fWaveforms[NUM_WAVEFORMS][WAVETABLE_SIZE];

    // Parameters
    float fParameters[kParameterCount];

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RGK1_SynthPlugin)
};

Plugin* createPlugin()
{
    return new RGK1_SynthPlugin();
}

END_NAMESPACE_DISTRHO
