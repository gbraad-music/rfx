#include "DistrhoPlugin.hpp"
#include "../../synth/synth_oscillator.h"
#include "../../synth/synth_filter_ladder.h"
#include "../../synth/synth_envelope.h"
#include "../../synth/synth_lfo.h"
#include "../../synth/synth_noise.h"
#include <cstring>
#include <cmath>

START_NAMESPACE_DISTRHO

struct RG101Voice {
    SynthOscillator* osc;          // Main oscillator (saw + square)
    SynthOscillator* sub_osc;      // Sub-oscillator (-1 octave square)
    SynthFilterLadder* filter;     // Moog ladder filter
    SynthEnvelope* amp_env;        // Amplitude envelope
    SynthEnvelope* filter_env;     // Filter envelope
    SynthLFO* lfo;                 // LFO for modulation
    SynthNoise* noise;             // White noise

    int note;
    int velocity;
    bool active;
    bool gate;

    // Portamento state
    float current_freq;
    float target_freq;
    bool sliding;
};

class RG101_SynthPlugin : public Plugin
{
public:
    RG101_SynthPlugin()
        : Plugin(kParameterCount, 0, 0)
        , fSawLevel(0.8f)
        , fSquareLevel(0.0f)
        , fSubLevel(0.3f)
        , fNoiseLevel(0.0f)
        , fPulseWidth(0.5f)
        , fPWMDepth(0.0f)
        , fCutoff(0.5f)
        , fResonance(0.3f)
        , fEnvMod(0.5f)
        , fKeyboardTracking(0.5f)
        , fFilterAttack(0.003f)
        , fFilterDecay(0.3f)
        , fFilterSustain(0.0f)
        , fFilterRelease(0.1f)
        , fAmpAttack(0.003f)
        , fAmpDecay(0.3f)
        , fAmpSustain(0.7f)
        , fAmpRelease(0.1f)
        , fLFOWaveform(0.0f)
        , fLFORate(5.0f)
        , fLFOPitchDepth(0.0f)
        , fLFOFilterDepth(0.0f)
        , fLFOAmpDepth(0.0f)
        , fVelocitySensitivity(0.5f)
        , fPortamento(0.0f)
        , fVolume(0.7f)
    {
        // Initialize voice
        fVoice.osc = synth_oscillator_create();
        fVoice.sub_osc = synth_oscillator_create();
        fVoice.filter = synth_filter_ladder_create();
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
        if (!fVoice.osc || !fVoice.sub_osc || !fVoice.filter ||
            !fVoice.amp_env || !fVoice.filter_env || !fVoice.lfo || !fVoice.noise) {
            // Cleanup
            if (fVoice.osc) synth_oscillator_destroy(fVoice.osc);
            if (fVoice.sub_osc) synth_oscillator_destroy(fVoice.sub_osc);
            if (fVoice.filter) synth_filter_ladder_destroy(fVoice.filter);
            if (fVoice.amp_env) synth_envelope_destroy(fVoice.amp_env);
            if (fVoice.filter_env) synth_envelope_destroy(fVoice.filter_env);
            if (fVoice.lfo) synth_lfo_destroy(fVoice.lfo);
            if (fVoice.noise) synth_noise_destroy(fVoice.noise);
            return;
        }

        // Setup components
        synth_oscillator_set_waveform(fVoice.osc, SYNTH_OSC_SAW);
        synth_oscillator_set_waveform(fVoice.sub_osc, SYNTH_OSC_SQUARE);

        // Setup envelopes
        updateEnvelopes();

        // Setup LFO (sine wave, 5Hz default)
        synth_lfo_set_waveform(fVoice.lfo, SYNTH_LFO_SINE);
        synth_lfo_set_frequency(fVoice.lfo, fLFORate);

        // Setup filter
        synth_filter_ladder_set_cutoff(fVoice.filter, fCutoff);
        synth_filter_ladder_set_resonance(fVoice.filter, fResonance);
    }

    ~RG101_SynthPlugin() override
    {
        if (fVoice.osc) synth_oscillator_destroy(fVoice.osc);
        if (fVoice.sub_osc) synth_oscillator_destroy(fVoice.sub_osc);
        if (fVoice.filter) synth_filter_ladder_destroy(fVoice.filter);
        if (fVoice.amp_env) synth_envelope_destroy(fVoice.amp_env);
        if (fVoice.filter_env) synth_envelope_destroy(fVoice.filter_env);
        if (fVoice.lfo) synth_lfo_destroy(fVoice.lfo);
        if (fVoice.noise) synth_noise_destroy(fVoice.noise);
    }

protected:
    const char* getLabel() const override
    {
        return RG101_DISPLAY_NAME;
    }

    const char* getDescription() const override
    {
        return RG101_DESCRIPTION;
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
        return d_cconst('R', 'G', '1', '1');
    }

    void initParameter(uint32_t index, Parameter& param) override
    {
        param.hints = kParameterIsAutomatable;
        param.ranges.min = 0.0f;
        param.ranges.max = 1.0f;
        param.ranges.def = 0.5f;

        switch (index) {
        // Oscillator Section
        case kParameterSawLevel:
            param.name = "Saw Level";
            param.symbol = "saw_level";
            param.ranges.def = 0.8f;
            break;
        case kParameterSquareLevel:
            param.name = "Square Level";
            param.symbol = "square_level";
            param.ranges.def = 0.0f;
            break;
        case kParameterSubLevel:
            param.name = "Sub Level";
            param.symbol = "sub_level";
            param.ranges.def = 0.3f;
            break;
        case kParameterNoiseLevel:
            param.name = "Noise Level";
            param.symbol = "noise_level";
            param.ranges.def = 0.0f;
            break;
        case kParameterPulseWidth:
            param.name = "Pulse Width";
            param.symbol = "pulse_width";
            param.ranges.def = 0.5f;
            break;
        case kParameterPWMDepth:
            param.name = "PWM Depth";
            param.symbol = "pwm_depth";
            param.ranges.def = 0.0f;
            break;

        // Filter Section
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
        case kParameterKeyboardTracking:
            param.name = "Kbd Track";
            param.symbol = "kbd_track";
            param.ranges.def = 0.5f;
            break;

        // Filter Envelope
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
        case kParameterFilterSustain:
            param.name = "Filt Sustain";
            param.symbol = "filt_sustain";
            param.ranges.def = 0.0f;
            break;
        case kParameterFilterRelease:
            param.name = "Filt Release";
            param.symbol = "filt_release";
            param.ranges.def = 0.1f;
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
        case kParameterLFOAmpDepth:
            param.name = "LFO Amp";
            param.symbol = "lfo_amp";
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
            param.ranges.def = 0.7f;
            break;
        }
    }

    float getParameterValue(uint32_t index) const override
    {
        switch (index) {
        case kParameterSawLevel: return fSawLevel;
        case kParameterSquareLevel: return fSquareLevel;
        case kParameterSubLevel: return fSubLevel;
        case kParameterNoiseLevel: return fNoiseLevel;
        case kParameterPulseWidth: return fPulseWidth;
        case kParameterPWMDepth: return fPWMDepth;
        case kParameterCutoff: return fCutoff;
        case kParameterResonance: return fResonance;
        case kParameterEnvMod: return fEnvMod;
        case kParameterKeyboardTracking: return fKeyboardTracking;
        case kParameterFilterAttack: return fFilterAttack;
        case kParameterFilterDecay: return fFilterDecay;
        case kParameterFilterSustain: return fFilterSustain;
        case kParameterFilterRelease: return fFilterRelease;
        case kParameterAmpAttack: return fAmpAttack;
        case kParameterAmpDecay: return fAmpDecay;
        case kParameterAmpSustain: return fAmpSustain;
        case kParameterAmpRelease: return fAmpRelease;
        case kParameterLFOWaveform: return fLFOWaveform;
        case kParameterLFORate: return fLFORate;
        case kParameterLFOPitchDepth: return fLFOPitchDepth;
        case kParameterLFOFilterDepth: return fLFOFilterDepth;
        case kParameterLFOAmpDepth: return fLFOAmpDepth;
        case kParameterVelocitySensitivity: return fVelocitySensitivity;
        case kParameterPortamento: return fPortamento;
        case kParameterVolume: return fVolume;
        default: return 0.0f;
        }
    }

    void setParameterValue(uint32_t index, float value) override
    {
        switch (index) {
        case kParameterSawLevel: fSawLevel = value; break;
        case kParameterSquareLevel: fSquareLevel = value; break;
        case kParameterSubLevel: fSubLevel = value; break;
        case kParameterNoiseLevel: fNoiseLevel = value; break;
        case kParameterPulseWidth: fPulseWidth = value; break;
        case kParameterPWMDepth: fPWMDepth = value; break;
        case kParameterCutoff: fCutoff = value; break;
        case kParameterResonance: fResonance = value; break;
        case kParameterEnvMod: fEnvMod = value; break;
        case kParameterKeyboardTracking: fKeyboardTracking = value; break;
        case kParameterFilterAttack: fFilterAttack = value; updateEnvelopes(); break;
        case kParameterFilterDecay: fFilterDecay = value; updateEnvelopes(); break;
        case kParameterFilterSustain: fFilterSustain = value; updateEnvelopes(); break;
        case kParameterFilterRelease: fFilterRelease = value; updateEnvelopes(); break;
        case kParameterAmpAttack: fAmpAttack = value; updateEnvelopes(); break;
        case kParameterAmpDecay: fAmpDecay = value; updateEnvelopes(); break;
        case kParameterAmpSustain: fAmpSustain = value; updateEnvelopes(); break;
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
        case kParameterLFOAmpDepth: fLFOAmpDepth = value; break;
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
                // Note On
                handleNoteOn(note, velocity, sampleRate);
            } else if (status == 0x80 || (status == 0x90 && velocity == 0)) {
                // Note Off
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
    void updateEnvelopes()
    {
        if (!fVoice.amp_env || !fVoice.filter_env) return;

        // Amp envelope
        synth_envelope_set_attack(fVoice.amp_env, 0.001f + fAmpAttack * 2.0f);
        synth_envelope_set_decay(fVoice.amp_env, 0.01f + fAmpDecay * 3.0f);
        synth_envelope_set_sustain(fVoice.amp_env, fAmpSustain);
        synth_envelope_set_release(fVoice.amp_env, 0.01f + fAmpRelease * 3.0f);

        // Filter envelope
        synth_envelope_set_attack(fVoice.filter_env, 0.001f + fFilterAttack * 2.0f);
        synth_envelope_set_decay(fVoice.filter_env, 0.01f + fFilterDecay * 3.0f);
        synth_envelope_set_sustain(fVoice.filter_env, fFilterSustain);
        synth_envelope_set_release(fVoice.filter_env, 0.01f + fFilterRelease * 3.0f);
    }

    void handleNoteOn(uint8_t note, uint8_t velocity, int sampleRate)
    {
        if (!fVoice.osc || !fVoice.amp_env || !fVoice.filter_env) return;

        // Calculate frequency
        float new_freq = 440.0f * powf(2.0f, (note - 69) / 12.0f);

        // SH-101 style portamento (slide) - only if previous note still held
        bool shouldSlide = fVoice.gate && fVoice.active && fPortamento > 0.0f;

        fVoice.note = note;
        fVoice.velocity = velocity;
        fVoice.active = true;
        fVoice.gate = true;

        if (shouldSlide) {
            // Slide to new note
            fVoice.target_freq = new_freq;
            fVoice.sliding = true;
        } else {
            // Fresh note
            fVoice.current_freq = new_freq;
            fVoice.target_freq = new_freq;
            fVoice.sliding = false;

            synth_oscillator_set_frequency(fVoice.osc, new_freq);
            synth_oscillator_set_frequency(fVoice.sub_osc, new_freq * 0.5f); // -1 octave

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
            float slide_time = 0.001f + fPortamento * 0.5f; // 1ms to 500ms
            float slide_rate = (fVoice.target_freq - fVoice.current_freq) / (slide_time * sampleRate);

            fVoice.current_freq += slide_rate;

            if ((slide_rate > 0.0f && fVoice.current_freq >= fVoice.target_freq) ||
                (slide_rate < 0.0f && fVoice.current_freq <= fVoice.target_freq)) {
                fVoice.current_freq = fVoice.target_freq;
                fVoice.sliding = false;
            }

            synth_oscillator_set_frequency(fVoice.osc, fVoice.current_freq);
            synth_oscillator_set_frequency(fVoice.sub_osc, fVoice.current_freq * 0.5f);
        }

        // Get LFO value
        float lfo_value = synth_lfo_process(fVoice.lfo, sampleRate);

        // LFO to pitch (vibrato)
        if (fLFOPitchDepth > 0.0f) {
            float pitch_mod = 1.0f + (lfo_value * fLFOPitchDepth * 0.05f); // ±5% max
            synth_oscillator_set_frequency(fVoice.osc, fVoice.current_freq * pitch_mod);
            synth_oscillator_set_frequency(fVoice.sub_osc, fVoice.current_freq * 0.5f * pitch_mod);
        }

        // PWM (pulse width modulation)
        float pw = fPulseWidth;
        if (fPWMDepth > 0.0f) {
            pw += lfo_value * fPWMDepth * 0.4f; // ±40% modulation
            if (pw < 0.1f) pw = 0.1f;
            if (pw > 0.9f) pw = 0.9f;
        }
        synth_oscillator_set_pulse_width(fVoice.osc, pw);

        // Generate oscillator outputs
        float saw_sample = (fSawLevel > 0.0f) ? synth_oscillator_process(fVoice.osc, sampleRate) * fSawLevel : 0.0f;

        // For square, temporarily change waveform
        float square_sample = 0.0f;
        if (fSquareLevel > 0.0f) {
            synth_oscillator_set_waveform(fVoice.osc, SYNTH_OSC_SQUARE);
            square_sample = synth_oscillator_process(fVoice.osc, sampleRate) * fSquareLevel;
            synth_oscillator_set_waveform(fVoice.osc, SYNTH_OSC_SAW);
        }

        float sub_sample = (fSubLevel > 0.0f) ? synth_oscillator_process(fVoice.sub_osc, sampleRate) * fSubLevel : 0.0f;
        float noise_sample = (fNoiseLevel > 0.0f) ? synth_noise_process(fVoice.noise) * fNoiseLevel : 0.0f;

        // Mix oscillators
        float sample = saw_sample + square_sample + sub_sample + noise_sample;

        // Reduce level to prevent clipping (multiple oscillators)
        sample *= 0.2f;

        // Get envelope values
        float amp_env_value = synth_envelope_process(fVoice.amp_env, sampleRate);
        float filter_env_value = synth_envelope_process(fVoice.filter_env, sampleRate);

        // Check if voice should stop
        if (amp_env_value <= 0.0f && !fVoice.gate) {
            fVoice.active = false;
            return 0.0f;
        }

        // Calculate filter cutoff with modulation
        float cutoff = fCutoff;

        // Filter envelope modulation
        cutoff += fEnvMod * filter_env_value;

        // Keyboard tracking (higher notes = higher cutoff)
        if (fKeyboardTracking > 0.0f) {
            float note_offset = (fVoice.note - 60) / 60.0f; // C4 = 0
            cutoff += note_offset * fKeyboardTracking * 0.5f;
        }

        // LFO to filter
        cutoff += lfo_value * fLFOFilterDepth * 0.3f;

        // Clamp cutoff
        if (cutoff > 1.0f) cutoff = 1.0f;
        if (cutoff < 0.0f) cutoff = 0.0f;

        // Apply filter
        synth_filter_ladder_set_cutoff(fVoice.filter, cutoff);
        synth_filter_ladder_set_resonance(fVoice.filter, fResonance);
        sample = synth_filter_ladder_process(fVoice.filter, sample, sampleRate);

        // Apply amplitude envelope
        sample *= amp_env_value;

        // LFO to amplitude (tremolo)
        if (fLFOAmpDepth > 0.0f) {
            sample *= 1.0f + lfo_value * fLFOAmpDepth * 0.5f;
        }

        // Velocity sensitivity
        if (fVelocitySensitivity > 0.0f) {
            float vel_scale = 1.0f - fVelocitySensitivity + (fVelocitySensitivity * (fVoice.velocity / 127.0f));
            sample *= vel_scale;
        }

        // Apply master volume
        sample *= fVolume;

        // Soft clipping
        if (sample > 1.0f) sample = 1.0f;
        if (sample < -1.0f) sample = -1.0f;

        return sample;
    }

    // Voice
    RG101Voice fVoice;

    // Parameters
    float fSawLevel;
    float fSquareLevel;
    float fSubLevel;
    float fNoiseLevel;
    float fPulseWidth;
    float fPWMDepth;

    float fCutoff;
    float fResonance;
    float fEnvMod;
    float fKeyboardTracking;

    float fFilterAttack;
    float fFilterDecay;
    float fFilterSustain;
    float fFilterRelease;

    float fAmpAttack;
    float fAmpDecay;
    float fAmpSustain;
    float fAmpRelease;

    float fLFOWaveform;
    float fLFORate;
    float fLFOPitchDepth;
    float fLFOFilterDepth;
    float fLFOAmpDepth;

    float fVelocitySensitivity;
    float fPortamento;
    float fVolume;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RG101_SynthPlugin)
};

Plugin* createPlugin()
{
    return new RG101_SynthPlugin();
}

END_NAMESPACE_DISTRHO
