#include "DistrhoPlugin.hpp"
#include "../../synth/synth_oscillator.h"
#include "../../synth/synth_filter_ms20.h"
#include "../../synth/synth_envelope.h"
#include "../../synth/synth_lfo.h"
#include "../../synth/synth_noise.h"
#include <cstring>
#include <cmath>

START_NAMESPACE_DISTRHO

struct RG20Voice {
    SynthOscillator* vco1;
    SynthOscillator* vco2;
    SynthFilterMS20* filter;
    SynthEnvelope* amp_env;
    SynthEnvelope* filter_env;
    SynthLFO* lfo;
    SynthNoise* noise;

    int note;
    int velocity;
    bool active;
    bool gate;

    // Portamento state
    float current_freq;
    float target_freq;
    bool sliding;
};

class RG20_SynthPlugin : public Plugin
{
public:
    RG20_SynthPlugin()
        : Plugin(kParameterCount, 0, 0)
        , fVCO1Waveform(0.0f)  // 0=saw, 0.5=square, 1=triangle
        , fVCO1Octave(0.5f)    // 0=16', 0.5=8', 1=4'
        , fVCO1Tune(0.5f)
        , fVCO1Level(0.7f)
        , fVCO2Waveform(0.0f)
        , fVCO2Octave(0.5f)
        , fVCO2Tune(0.5f)
        , fVCO2Level(0.5f)
        , fVCO2PulseWidth(0.5f)
        , fVCO2PWMDepth(0.0f)
        , fVCOSync(0.0f)
        , fNoiseLevel(0.0f)
        , fRingModLevel(0.0f)
        , fHPFCutoff(0.1f)
        , fHPFPeak(0.0f)
        , fLPFCutoff(0.8f)
        , fLPFPeak(0.3f)
        , fKeyboardTracking(0.5f)
        , fFilterAttack(0.01f)
        , fFilterDecay(0.3f)
        , fFilterRelease(0.1f)
        , fFilterEnvAmount(0.5f)
        , fAmpAttack(0.01f)
        , fAmpDecay(0.3f)
        , fAmpRelease(0.1f)
        , fLFOWaveform(0.0f)
        , fLFORate(5.0f)
        , fLFOPitchDepth(0.0f)
        , fLFOFilterDepth(0.0f)
        , fVelocitySensitivity(0.5f)
        , fPortamento(0.0f)
        , fVolume(0.5f)
    {
        // Initialize voice
        fVoice.vco1 = synth_oscillator_create();
        fVoice.vco2 = synth_oscillator_create();
        fVoice.filter = synth_filter_ms20_create();
        fVoice.amp_env = synth_envelope_create();
        fVoice.filter_env = synth_envelope_create();
        fVoice.lfo = synth_lfo_create();
        fVoice.noise = synth_noise_create();

        fVoice.note = -1;
        fVoice.velocity = 0;
        fVoice.active = false;
        fVoice.gate = false;
        fVoice.current_freq = 440.0f;
        fVoice.target_freq = 440.0f;
        fVoice.sliding = false;

        // Check if all components created successfully
        if (!fVoice.vco1 || !fVoice.vco2 || !fVoice.filter ||
            !fVoice.amp_env || !fVoice.filter_env || !fVoice.lfo || !fVoice.noise) {
            // Cleanup
            if (fVoice.vco1) synth_oscillator_destroy(fVoice.vco1);
            if (fVoice.vco2) synth_oscillator_destroy(fVoice.vco2);
            if (fVoice.filter) synth_filter_ms20_destroy(fVoice.filter);
            if (fVoice.amp_env) synth_envelope_destroy(fVoice.amp_env);
            if (fVoice.filter_env) synth_envelope_destroy(fVoice.filter_env);
            if (fVoice.lfo) synth_lfo_destroy(fVoice.lfo);
            if (fVoice.noise) synth_noise_destroy(fVoice.noise);
            return;
        }

        // Setup components
        updateWaveforms();
        updateEnvelopes();

        // Setup LFO (triangle wave default)
        synth_lfo_set_waveform(fVoice.lfo, SYNTH_LFO_TRIANGLE);
        synth_lfo_set_frequency(fVoice.lfo, fLFORate);

        // Setup filter
        synth_filter_ms20_set_hpf_cutoff(fVoice.filter, fHPFCutoff);
        synth_filter_ms20_set_hpf_peak(fVoice.filter, fHPFPeak);
        synth_filter_ms20_set_lpf_cutoff(fVoice.filter, fLPFCutoff);
        synth_filter_ms20_set_lpf_peak(fVoice.filter, fLPFPeak);
    }

    ~RG20_SynthPlugin() override
    {
        if (fVoice.vco1) synth_oscillator_destroy(fVoice.vco1);
        if (fVoice.vco2) synth_oscillator_destroy(fVoice.vco2);
        if (fVoice.filter) synth_filter_ms20_destroy(fVoice.filter);
        if (fVoice.amp_env) synth_envelope_destroy(fVoice.amp_env);
        if (fVoice.filter_env) synth_envelope_destroy(fVoice.filter_env);
        if (fVoice.lfo) synth_lfo_destroy(fVoice.lfo);
        if (fVoice.noise) synth_noise_destroy(fVoice.noise);
    }

protected:
    const char* getLabel() const override
    {
        return RG20_DISPLAY_NAME;
    }

    const char* getDescription() const override
    {
        return RG20_DESCRIPTION;
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
        return d_cconst('R', 'G', '2', '0');
    }

    void initParameter(uint32_t index, Parameter& param) override
    {
        param.hints = kParameterIsAutomatable;
        param.ranges.min = 0.0f;
        param.ranges.max = 1.0f;
        param.ranges.def = 0.5f;

        switch (index) {
        // VCO 1
        case kParameterVCO1Waveform:
            param.name = "VCO1 Wave";
            param.symbol = "vco1_wave";
            param.ranges.def = 0.0f;
            break;
        case kParameterVCO1Octave:
            param.name = "VCO1 Octave";
            param.symbol = "vco1_octave";
            param.ranges.def = 0.5f;
            break;
        case kParameterVCO1Tune:
            param.name = "VCO1 Tune";
            param.symbol = "vco1_tune";
            param.ranges.def = 0.5f;
            break;
        case kParameterVCO1Level:
            param.name = "VCO1 Level";
            param.symbol = "vco1_level";
            param.ranges.def = 0.7f;
            break;

        // VCO 2
        case kParameterVCO2Waveform:
            param.name = "VCO2 Wave";
            param.symbol = "vco2_wave";
            param.ranges.def = 0.0f;
            break;
        case kParameterVCO2Octave:
            param.name = "VCO2 Octave";
            param.symbol = "vco2_octave";
            param.ranges.def = 0.5f;
            break;
        case kParameterVCO2Tune:
            param.name = "VCO2 Tune";
            param.symbol = "vco2_tune";
            param.ranges.def = 0.5f;
            break;
        case kParameterVCO2Level:
            param.name = "VCO2 Level";
            param.symbol = "vco2_level";
            param.ranges.def = 0.5f;
            break;
        case kParameterVCO2PulseWidth:
            param.name = "VCO2 PW";
            param.symbol = "vco2_pw";
            param.ranges.def = 0.5f;
            break;
        case kParameterVCO2PWMDepth:
            param.name = "VCO2 PWM";
            param.symbol = "vco2_pwm";
            param.ranges.def = 0.0f;
            break;
        case kParameterVCOSync:
            param.name = "VCO Sync";
            param.symbol = "vco_sync";
            param.ranges.def = 0.0f;
            break;

        // Mixer
        case kParameterNoiseLevel:
            param.name = "Noise Level";
            param.symbol = "noise_level";
            param.ranges.def = 0.0f;
            break;
        case kParameterRingModLevel:
            param.name = "Ring Mod";
            param.symbol = "ring_mod";
            param.ranges.def = 0.0f;
            break;

        // HPF
        case kParameterHPFCutoff:
            param.name = "HPF Cutoff";
            param.symbol = "hpf_cutoff";
            param.ranges.def = 0.1f;
            break;
        case kParameterHPFPeak:
            param.name = "HPF Peak";
            param.symbol = "hpf_peak";
            param.ranges.def = 0.0f;
            break;

        // LPF
        case kParameterLPFCutoff:
            param.name = "LPF Cutoff";
            param.symbol = "lpf_cutoff";
            param.ranges.def = 0.8f;
            break;
        case kParameterLPFPeak:
            param.name = "LPF Peak";
            param.symbol = "lpf_peak";
            param.ranges.def = 0.3f;
            break;
        case kParameterKeyboardTracking:
            param.name = "Kbd Track";
            param.symbol = "kbd_track";
            param.ranges.def = 0.5f;
            break;

        // Filter Envelope (ADR)
        case kParameterFilterAttack:
            param.name = "Filt Attack";
            param.symbol = "filt_attack";
            param.ranges.def = 0.01f;
            break;
        case kParameterFilterDecay:
            param.name = "Filt Decay";
            param.symbol = "filt_decay";
            param.ranges.def = 0.3f;
            break;
        case kParameterFilterRelease:
            param.name = "Filt Release";
            param.symbol = "filt_release";
            param.ranges.def = 0.1f;
            break;
        case kParameterFilterEnvAmount:
            param.name = "Filt Env Amt";
            param.symbol = "filt_env_amt";
            param.ranges.def = 0.5f;
            break;

        // Amp Envelope (ADR)
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
        case kParameterAmpRelease:
            param.name = "Amp Release";
            param.symbol = "amp_release";
            param.ranges.def = 0.1f;
            break;

        // Modulation
        case kParameterLFOWaveform:
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

        // Performance
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
            param.ranges.def = 0.5f;
            break;
        }
    }

    float getParameterValue(uint32_t index) const override
    {
        switch (index) {
        case kParameterVCO1Waveform: return fVCO1Waveform;
        case kParameterVCO1Octave: return fVCO1Octave;
        case kParameterVCO1Tune: return fVCO1Tune;
        case kParameterVCO1Level: return fVCO1Level;
        case kParameterVCO2Waveform: return fVCO2Waveform;
        case kParameterVCO2Octave: return fVCO2Octave;
        case kParameterVCO2Tune: return fVCO2Tune;
        case kParameterVCO2Level: return fVCO2Level;
        case kParameterVCO2PulseWidth: return fVCO2PulseWidth;
        case kParameterVCO2PWMDepth: return fVCO2PWMDepth;
        case kParameterVCOSync: return fVCOSync;
        case kParameterNoiseLevel: return fNoiseLevel;
        case kParameterRingModLevel: return fRingModLevel;
        case kParameterHPFCutoff: return fHPFCutoff;
        case kParameterHPFPeak: return fHPFPeak;
        case kParameterLPFCutoff: return fLPFCutoff;
        case kParameterLPFPeak: return fLPFPeak;
        case kParameterKeyboardTracking: return fKeyboardTracking;
        case kParameterFilterAttack: return fFilterAttack;
        case kParameterFilterDecay: return fFilterDecay;
        case kParameterFilterRelease: return fFilterRelease;
        case kParameterFilterEnvAmount: return fFilterEnvAmount;
        case kParameterAmpAttack: return fAmpAttack;
        case kParameterAmpDecay: return fAmpDecay;
        case kParameterAmpRelease: return fAmpRelease;
        case kParameterLFOWaveform: return fLFOWaveform;
        case kParameterLFORate: return fLFORate;
        case kParameterLFOPitchDepth: return fLFOPitchDepth;
        case kParameterLFOFilterDepth: return fLFOFilterDepth;
        case kParameterVelocitySensitivity: return fVelocitySensitivity;
        case kParameterPortamento: return fPortamento;
        case kParameterVolume: return fVolume;
        default: return 0.0f;
        }
    }

    void setParameterValue(uint32_t index, float value) override
    {
        switch (index) {
        case kParameterVCO1Waveform: fVCO1Waveform = value; updateWaveforms(); break;
        case kParameterVCO1Octave: fVCO1Octave = value; break;
        case kParameterVCO1Tune: fVCO1Tune = value; break;
        case kParameterVCO1Level: fVCO1Level = value; break;
        case kParameterVCO2Waveform: fVCO2Waveform = value; updateWaveforms(); break;
        case kParameterVCO2Octave: fVCO2Octave = value; break;
        case kParameterVCO2Tune: fVCO2Tune = value; break;
        case kParameterVCO2Level: fVCO2Level = value; break;
        case kParameterVCO2PulseWidth: fVCO2PulseWidth = value; break;
        case kParameterVCO2PWMDepth: fVCO2PWMDepth = value; break;
        case kParameterVCOSync: fVCOSync = value; break;
        case kParameterNoiseLevel: fNoiseLevel = value; break;
        case kParameterRingModLevel: fRingModLevel = value; break;
        case kParameterHPFCutoff: fHPFCutoff = value; break;
        case kParameterHPFPeak: fHPFPeak = value; break;
        case kParameterLPFCutoff: fLPFCutoff = value; break;
        case kParameterLPFPeak: fLPFPeak = value; break;
        case kParameterKeyboardTracking: fKeyboardTracking = value; break;
        case kParameterFilterAttack: fFilterAttack = value; updateEnvelopes(); break;
        case kParameterFilterDecay: fFilterDecay = value; updateEnvelopes(); break;
        case kParameterFilterRelease: fFilterRelease = value; updateEnvelopes(); break;
        case kParameterFilterEnvAmount: fFilterEnvAmount = value; break;
        case kParameterAmpAttack: fAmpAttack = value; updateEnvelopes(); break;
        case kParameterAmpDecay: fAmpDecay = value; updateEnvelopes(); break;
        case kParameterAmpRelease: fAmpRelease = value; updateEnvelopes(); break;
        case kParameterLFOWaveform:
            fLFOWaveform = value;
            if (fVoice.lfo) {
                int waveform = (int)(value * 4.0f);  // 0-4 = sine, tri, saw, square, S&H
                if (waveform > 4) waveform = 4;
                synth_lfo_set_waveform(fVoice.lfo, (SynthLFOWaveform)waveform);
            }
            break;
        case kParameterLFORate:
            fLFORate = value;
            if (fVoice.lfo) synth_lfo_set_frequency(fVoice.lfo, fLFORate);
            break;
        case kParameterLFOPitchDepth: fLFOPitchDepth = value; break;
        case kParameterLFOFilterDepth: fLFOFilterDepth = value; break;
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
                float sample = renderVoice(sampleRate);
                outL[framePos] = sample;
                outR[framePos] = sample;
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
            float sample = renderVoice(sampleRate);
            outL[framePos] = sample;
            outR[framePos] = sample;
            framePos++;
        }
    }

private:
    void updateWaveforms()
    {
        if (!fVoice.vco1 || !fVoice.vco2) return;

        // VCO1 waveform (0=saw, 0.5=square, 1=triangle)
        if (fVCO1Waveform < 0.33f) {
            synth_oscillator_set_waveform(fVoice.vco1, SYNTH_OSC_SAW);
        } else if (fVCO1Waveform < 0.66f) {
            synth_oscillator_set_waveform(fVoice.vco1, SYNTH_OSC_SQUARE);
        } else {
            synth_oscillator_set_waveform(fVoice.vco1, SYNTH_OSC_TRIANGLE);
        }

        // VCO2 waveform
        if (fVCO2Waveform < 0.33f) {
            synth_oscillator_set_waveform(fVoice.vco2, SYNTH_OSC_SAW);
        } else if (fVCO2Waveform < 0.66f) {
            synth_oscillator_set_waveform(fVoice.vco2, SYNTH_OSC_SQUARE);
        } else {
            synth_oscillator_set_waveform(fVoice.vco2, SYNTH_OSC_TRIANGLE);
        }
    }

    void updateEnvelopes()
    {
        if (!fVoice.amp_env || !fVoice.filter_env) return;

        // MS-20 ADR envelopes (Attack, Decay, Release - no sustain)
        // Amp envelope
        synth_envelope_set_attack(fVoice.amp_env, 0.001f + fAmpAttack * 2.0f);
        synth_envelope_set_decay(fVoice.amp_env, 0.01f + fAmpDecay * 3.0f);
        synth_envelope_set_sustain(fVoice.amp_env, 0.0f);  // No sustain (ADR)
        synth_envelope_set_release(fVoice.amp_env, 0.01f + fAmpRelease * 3.0f);

        // Filter envelope
        synth_envelope_set_attack(fVoice.filter_env, 0.001f + fFilterAttack * 2.0f);
        synth_envelope_set_decay(fVoice.filter_env, 0.01f + fFilterDecay * 3.0f);
        synth_envelope_set_sustain(fVoice.filter_env, 0.0f);  // No sustain (ADR)
        synth_envelope_set_release(fVoice.filter_env, 0.01f + fFilterRelease * 3.0f);
    }

    void handleNoteOn(uint8_t note, uint8_t velocity, int sampleRate)
    {
        if (!fVoice.vco1 || !fVoice.vco2 || !fVoice.amp_env || !fVoice.filter_env) return;

        // Calculate base frequency
        float new_freq = 440.0f * powf(2.0f, (note - 69) / 12.0f);

        // MS-20 style portamento
        bool shouldSlide = fVoice.gate && fVoice.active && fPortamento > 0.0f;

        fVoice.note = note;
        fVoice.velocity = velocity;
        fVoice.active = true;
        fVoice.gate = true;

        if (shouldSlide) {
            fVoice.target_freq = new_freq;
            fVoice.sliding = true;
        } else {
            fVoice.current_freq = new_freq;
            fVoice.target_freq = new_freq;
            fVoice.sliding = false;

            // Trigger envelopes
            synth_envelope_trigger(fVoice.amp_env);
            synth_envelope_trigger(fVoice.filter_env);
        }
    }

    void handleNoteOff(uint8_t note)
    {
        if (!fVoice.amp_env || !fVoice.filter_env) return;

        if (fVoice.note == note && fVoice.active) {
            fVoice.gate = false;
            synth_envelope_release(fVoice.amp_env);
            synth_envelope_release(fVoice.filter_env);
        }
    }

    float renderVoice(int sampleRate)
    {
        if (!fVoice.active) return 0.0f;

        // Handle portamento
        if (fVoice.sliding && fPortamento > 0.0f) {
            float slide_time = 0.001f + fPortamento * 0.5f;
            float slide_rate = (fVoice.target_freq - fVoice.current_freq) / (slide_time * sampleRate);

            fVoice.current_freq += slide_rate;

            if ((slide_rate > 0.0f && fVoice.current_freq >= fVoice.target_freq) ||
                (slide_rate < 0.0f && fVoice.current_freq <= fVoice.target_freq)) {
                fVoice.current_freq = fVoice.target_freq;
                fVoice.sliding = false;
            }
        }

        // Get LFO value
        float lfo_value = synth_lfo_process(fVoice.lfo, sampleRate);

        // LFO to pitch
        float pitch_mod = 1.0f;
        if (fLFOPitchDepth > 0.0f) {
            pitch_mod = 1.0f + (lfo_value * fLFOPitchDepth * 0.05f);
        }

        // Calculate VCO frequencies with octave and tune
        // Octave: 0=16' (-1oct), 0.5=8' (0oct), 1=4' (+1oct)
        float vco1_octave = (fVCO1Octave - 0.5f) * 2.0f; // -1 to +1 octaves
        float vco1_tune = (fVCO1Tune - 0.5f) * 2.0f;  // -1 to +1 semitones
        float vco1_freq = fVoice.current_freq * powf(2.0f, vco1_octave + vco1_tune / 12.0f) * pitch_mod;

        float vco2_octave = (fVCO2Octave - 0.5f) * 2.0f;
        float vco2_tune = (fVCO2Tune - 0.5f) * 2.0f;
        float vco2_freq = fVoice.current_freq * powf(2.0f, vco2_octave + vco2_tune / 12.0f) * pitch_mod;

        synth_oscillator_set_frequency(fVoice.vco1, vco1_freq);
        synth_oscillator_set_frequency(fVoice.vco2, vco2_freq);

        // VCO2 PWM (LFO modulates pulse width)
        float pulse_width = fVCO2PulseWidth + lfo_value * fVCO2PWMDepth * 0.4f;
        if (pulse_width < 0.05f) pulse_width = 0.05f;
        if (pulse_width > 0.95f) pulse_width = 0.95f;
        synth_oscillator_set_pulse_width(fVoice.vco2, pulse_width);

        // Generate oscillator outputs
        float vco1_sample = synth_oscillator_process(fVoice.vco1, sampleRate) * fVCO1Level;
        float vco2_sample = synth_oscillator_process(fVoice.vco2, sampleRate) * fVCO2Level;

        // VCO Sync - hard sync VCO2 to VCO1 (mixed when sync enabled)
        if (fVCOSync > 0.5f) {
            // Simple sync: reset VCO2 when VCO1 crosses zero
            // (Note: full hard sync requires phase tracking in oscillator)
            vco2_sample = vco2_sample * (1.0f - fVCOSync) + vco1_sample * vco2_sample * fVCOSync * 2.0f;
        }

        // MS-20 Normalled Path: VCO1 + VCO2 mix
        float sample = vco1_sample + vco2_sample;

        // External inputs (patch panel simulation)
        // Ring mod and noise can be patched into the signal path via mix controls
        if (fRingModLevel > 0.0f) {
            float ring_mod_sample = vco1_sample * vco2_sample * fRingModLevel * 2.0f;
            sample += ring_mod_sample;
        }

        if (fNoiseLevel > 0.0f) {
            float noise_sample = synth_noise_process(fVoice.noise) * fNoiseLevel;
            sample += noise_sample;
        }

        // Get envelope values
        float amp_env_value = synth_envelope_process(fVoice.amp_env, sampleRate);
        float filter_env_value = synth_envelope_process(fVoice.filter_env, sampleRate);

        // Check if voice should stop
        if (amp_env_value <= 0.0f && !fVoice.gate) {
            fVoice.active = false;
            return 0.0f;
        }

        // Apply MS-20 dual filter
        float lpf_cutoff = fLPFCutoff;
        float hpf_cutoff = fHPFCutoff;

        // Keyboard tracking - higher notes open filter
        if (fKeyboardTracking > 0.0f) {
            float key_track = (fVoice.note - 60) / 48.0f;  // C4 = 0, range ±4 octaves
            lpf_cutoff += key_track * fKeyboardTracking * 0.5f;
        }

        // Filter envelope modulation
        lpf_cutoff += fFilterEnvAmount * filter_env_value * 0.5f;
        hpf_cutoff += fFilterEnvAmount * filter_env_value * 0.3f;

        // LFO to filter
        if (fLFOFilterDepth > 0.0f) {
            lpf_cutoff += lfo_value * fLFOFilterDepth * 0.3f;
            hpf_cutoff += lfo_value * fLFOFilterDepth * 0.2f;
        }

        // Clamp
        if (lpf_cutoff > 1.0f) lpf_cutoff = 1.0f;
        if (lpf_cutoff < 0.0f) lpf_cutoff = 0.0f;
        if (hpf_cutoff > 1.0f) hpf_cutoff = 1.0f;
        if (hpf_cutoff < 0.0f) hpf_cutoff = 0.0f;

        synth_filter_ms20_set_hpf_cutoff(fVoice.filter, hpf_cutoff);
        synth_filter_ms20_set_hpf_peak(fVoice.filter, fHPFPeak);
        synth_filter_ms20_set_lpf_cutoff(fVoice.filter, lpf_cutoff);
        synth_filter_ms20_set_lpf_peak(fVoice.filter, fLPFPeak);

        sample = synth_filter_ms20_process(fVoice.filter, sample, sampleRate);

        // Apply amplitude envelope
        sample *= amp_env_value;

        // Apply velocity sensitivity
        float vel_scale = 1.0f - fVelocitySensitivity + (fVelocitySensitivity * (fVoice.velocity / 127.0f));
        sample *= vel_scale;

        // Apply master volume
        sample *= fVolume;

        // Soft clipping
        if (sample > 1.0f) sample = 1.0f;
        if (sample < -1.0f) sample = -1.0f;

        return sample;
    }

    // Voice
    RG20Voice fVoice;

    // Parameters
    float fVCO1Waveform;
    float fVCO1Octave;
    float fVCO1Tune;
    float fVCO1Level;

    float fVCO2Waveform;
    float fVCO2Octave;
    float fVCO2Tune;
    float fVCO2Level;
    float fVCO2PulseWidth;
    float fVCO2PWMDepth;
    float fVCOSync;

    float fNoiseLevel;
    float fRingModLevel;

    float fHPFCutoff;
    float fHPFPeak;
    float fLPFCutoff;
    float fLPFPeak;
    float fKeyboardTracking;

    float fFilterAttack;
    float fFilterDecay;
    float fFilterRelease;
    float fFilterEnvAmount;

    float fAmpAttack;
    float fAmpDecay;
    float fAmpRelease;

    float fLFOWaveform;
    float fLFORate;
    float fLFOPitchDepth;
    float fLFOFilterDepth;

    float fVelocitySensitivity;
    float fPortamento;
    float fVolume;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RG20_SynthPlugin)
};

Plugin* createPlugin()
{
    return new RG20_SynthPlugin();
}

END_NAMESPACE_DISTRHO
