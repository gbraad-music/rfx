#include "DistrhoPlugin.hpp"
#include "../../synth/synth_oscillator.h"
#include "../../synth/synth_filter.h"
#include "../../synth/synth_envelope.h"
#include <cstring>
#include <cmath>

START_NAMESPACE_DISTRHO

#define MAX_VOICES 1  // Monophonic, like the original TB303

struct RG303Voice {
    SynthOscillator* osc;
    SynthFilter* filter;
    SynthEnvelope* amp_env;
    SynthEnvelope* filter_env;

    int note;
    int velocity;
    bool active;      // Voice is playing (including release phase)
    bool gate;        // Note is being held (before note_off)

    // Portamento/slide state
    float current_freq;
    float target_freq;
    bool sliding;
};

class RG303_SynthPlugin : public Plugin
{
public:
    RG303_SynthPlugin()
        : Plugin(kParameterCount, 0, 8)  // 8 state values
        , fWaveform(0.0f)
        , fCutoff(0.5f)
        , fResonance(0.5f)
        , fEnvMod(0.5f)
        , fDecay(0.3f)
        , fAccent(0.0f)
        , fSlideTime(0.1f)
        , fVolume(0.7f)
    {
        // Initialize voice structure
        fVoice.osc = nullptr;
        fVoice.filter = nullptr;
        fVoice.amp_env = nullptr;
        fVoice.filter_env = nullptr;
        fVoice.note = -1;
        fVoice.velocity = 0;
        fVoice.active = false;
        fVoice.gate = false;
        fVoice.current_freq = 440.0f;
        fVoice.target_freq = 440.0f;
        fVoice.sliding = false;

        // Create synth components
        fVoice.osc = synth_oscillator_create();
        fVoice.filter = synth_filter_create();
        fVoice.amp_env = synth_envelope_create();
        fVoice.filter_env = synth_envelope_create();

        // Check if all components were created successfully
        if (!fVoice.osc || !fVoice.filter || !fVoice.amp_env || !fVoice.filter_env) {
            // Cleanup any that were created
            if (fVoice.osc) synth_oscillator_destroy(fVoice.osc);
            if (fVoice.filter) synth_filter_destroy(fVoice.filter);
            if (fVoice.amp_env) synth_envelope_destroy(fVoice.amp_env);
            if (fVoice.filter_env) synth_envelope_destroy(fVoice.filter_env);

            fVoice.osc = nullptr;
            fVoice.filter = nullptr;
            fVoice.amp_env = nullptr;
            fVoice.filter_env = nullptr;
            return;
        }

        // Set up TB303-style envelopes
        // Amplitude: quick attack, medium decay
        synth_envelope_set_attack(fVoice.amp_env, 0.003f);
        synth_envelope_set_decay(fVoice.amp_env, 0.2f);
        synth_envelope_set_sustain(fVoice.amp_env, 0.0f);  // No sustain, decay to 0
        synth_envelope_set_release(fVoice.amp_env, 0.01f);

        // Filter envelope: quick attack, adjustable decay
        synth_envelope_set_attack(fVoice.filter_env, 0.003f);
        synth_envelope_set_decay(fVoice.filter_env, 0.01f + fDecay * 2.0f);
        synth_envelope_set_sustain(fVoice.filter_env, 0.0f);
        synth_envelope_set_release(fVoice.filter_env, 0.01f);

        // Set default filter parameters
        synth_filter_set_type(fVoice.filter, SYNTH_FILTER_LPF);
        synth_filter_set_cutoff(fVoice.filter, fCutoff);
        synth_filter_set_resonance(fVoice.filter, fResonance);

        // Set default oscillator waveform
        synth_oscillator_set_waveform(fVoice.osc, SYNTH_OSC_SAW);
    }

    ~RG303_SynthPlugin() override
    {
        if (fVoice.osc) synth_oscillator_destroy(fVoice.osc);
        if (fVoice.filter) synth_filter_destroy(fVoice.filter);
        if (fVoice.amp_env) synth_envelope_destroy(fVoice.amp_env);
        if (fVoice.filter_env) synth_envelope_destroy(fVoice.filter_env);
    }

protected:
    const char* getLabel() const override { return "RG303_Synth"; }
    const char* getDescription() const override { return "RG303 bass synthesizer"; }
    const char* getMaker() const override { return "Regroove"; }
    const char* getHomePage() const override { return "https://github.com/gbraad/rfx"; }
    const char* getLicense() const override { return "ISC"; }
    uint32_t getVersion() const override { return d_version(1, 0, 0); }
    int64_t getUniqueId() const override { return d_cconst('T', 'B', '3', '3'); }

    void initParameter(uint32_t index, Parameter& param) override
    {
        param.hints = kParameterIsAutomatable;
        param.ranges.min = 0.0f;
        param.ranges.max = 1.0f;
        param.ranges.def = 0.5f;

        switch (index) {
        case kParameterWaveform:
            param.name = "Waveform";
            param.symbol = "waveform";
            param.ranges.def = 0.0f;
            param.hints |= kParameterIsInteger;
            param.ranges.max = 1.0f;  // 0=Saw, 1=Square
            break;
        case kParameterCutoff:
            param.name = "Cutoff";
            param.symbol = "cutoff";
            param.ranges.def = 0.5f;
            break;
        case kParameterResonance:
            param.name = "Resonance";
            param.symbol = "resonance";
            param.ranges.def = 0.5f;
            break;
        case kParameterEnvMod:
            param.name = "Env Mod";
            param.symbol = "envmod";
            param.ranges.def = 0.5f;
            break;
        case kParameterDecay:
            param.name = "Decay";
            param.symbol = "decay";
            param.ranges.def = 0.3f;
            break;
        case kParameterAccent:
            param.name = "Accent";
            param.symbol = "accent";
            param.ranges.def = 0.0f;
            break;
        case kParameterSlideTime:
            param.name = "Slide Time";
            param.symbol = "slide";
            param.ranges.def = 0.1f;
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
        case kParameterWaveform: return fWaveform;
        case kParameterCutoff: return fCutoff;
        case kParameterResonance: return fResonance;
        case kParameterEnvMod: return fEnvMod;
        case kParameterDecay: return fDecay;
        case kParameterAccent: return fAccent;
        case kParameterSlideTime: return fSlideTime;
        case kParameterVolume: return fVolume;
        default: return 0.0f;
        }
    }

    void setParameterValue(uint32_t index, float value) override
    {
        switch (index) {
        case kParameterWaveform:
            fWaveform = value;
            break;
        case kParameterCutoff:
            fCutoff = value;
            if (fVoice.filter) synth_filter_set_cutoff(fVoice.filter, fCutoff);
            break;
        case kParameterResonance:
            fResonance = value;
            if (fVoice.filter) synth_filter_set_resonance(fVoice.filter, fResonance);
            break;
        case kParameterEnvMod:
            fEnvMod = value;
            break;
        case kParameterDecay:
            fDecay = value;
            if (fVoice.filter_env) {
                synth_envelope_set_decay(fVoice.filter_env, 0.01f + fDecay * 2.0f);
            }
            if (fVoice.amp_env) {
                synth_envelope_set_decay(fVoice.amp_env, 0.01f + fDecay * 2.0f);
            }
            break;
        case kParameterAccent:
            fAccent = value;
            break;
        case kParameterSlideTime:
            fSlideTime = value;
            break;
        case kParameterVolume:
            fVolume = value;
            break;
        }
    }

    void initState(uint32_t index, State& state) override
    {
        switch (index) {
        case 0:
            state.key = "waveform";
            state.defaultValue = "0.0";
            break;
        case 1:
            state.key = "cutoff";
            state.defaultValue = "0.5";
            break;
        case 2:
            state.key = "resonance";
            state.defaultValue = "0.5";
            break;
        case 3:
            state.key = "envmod";
            state.defaultValue = "0.5";
            break;
        case 4:
            state.key = "decay";
            state.defaultValue = "0.3";
            break;
        case 5:
            state.key = "accent";
            state.defaultValue = "0.0";
            break;
        case 6:
            state.key = "slide";
            state.defaultValue = "0.1";
            break;
        case 7:
            state.key = "volume";
            state.defaultValue = "0.7";
            break;
        }
        state.hints = kStateIsOnlyForDSP;
    }

    void setState(const char* key, const char* value) override
    {
        float fValue = std::atof(value);

        if (std::strcmp(key, "waveform") == 0) {
            fWaveform = fValue;
        }
        else if (std::strcmp(key, "cutoff") == 0) {
            fCutoff = fValue;
            if (fVoice.filter) synth_filter_set_cutoff(fVoice.filter, fCutoff);
        }
        else if (std::strcmp(key, "resonance") == 0) {
            fResonance = fValue;
            if (fVoice.filter) synth_filter_set_resonance(fVoice.filter, fResonance);
        }
        else if (std::strcmp(key, "envmod") == 0) {
            fEnvMod = fValue;
        }
        else if (std::strcmp(key, "decay") == 0) {
            fDecay = fValue;
            if (fVoice.filter_env) {
                synth_envelope_set_decay(fVoice.filter_env, 0.01f + fDecay * 2.0f);
            }
            if (fVoice.amp_env) {
                synth_envelope_set_decay(fVoice.amp_env, 0.01f + fDecay * 2.0f);
            }
        }
        else if (std::strcmp(key, "accent") == 0) {
            fAccent = fValue;
        }
        else if (std::strcmp(key, "slide") == 0) {
            fSlideTime = fValue;
        }
        else if (std::strcmp(key, "volume") == 0) {
            fVolume = fValue;
        }
    }

    String getState(const char* key) const override
    {
        char buf[32];

        if (std::strcmp(key, "waveform") == 0) {
            std::snprintf(buf, sizeof(buf), "%.1f", fWaveform);
        }
        else if (std::strcmp(key, "cutoff") == 0) {
            std::snprintf(buf, sizeof(buf), "%.6f", fCutoff);
        }
        else if (std::strcmp(key, "resonance") == 0) {
            std::snprintf(buf, sizeof(buf), "%.6f", fResonance);
        }
        else if (std::strcmp(key, "envmod") == 0) {
            std::snprintf(buf, sizeof(buf), "%.6f", fEnvMod);
        }
        else if (std::strcmp(key, "decay") == 0) {
            std::snprintf(buf, sizeof(buf), "%.6f", fDecay);
        }
        else if (std::strcmp(key, "accent") == 0) {
            std::snprintf(buf, sizeof(buf), "%.6f", fAccent);
        }
        else if (std::strcmp(key, "volume") == 0) {
            std::snprintf(buf, sizeof(buf), "%.6f", fVolume);
        }
        else {
            std::snprintf(buf, sizeof(buf), "0.0");
        }

        return String(buf);
    }

    void activate() override
    {
        // Safety check - only reset if components exist
        if (fVoice.osc) synth_oscillator_reset(fVoice.osc);
        if (fVoice.filter) {
            synth_filter_reset(fVoice.filter);
            // Restore filter parameters after reset
            synth_filter_set_cutoff(fVoice.filter, fCutoff);
            synth_filter_set_resonance(fVoice.filter, fResonance);
        }
        if (fVoice.amp_env) synth_envelope_reset(fVoice.amp_env);
        if (fVoice.filter_env) synth_envelope_reset(fVoice.filter_env);
        fVoice.active = false;
    }

    void run(const float**, float** outputs, uint32_t frames,
             const MidiEvent* midiEvents, uint32_t midiEventCount) override
    {
        float* outL = outputs[0];
        float* outR = outputs[1];

        // Clear output buffers
        std::memset(outL, 0, sizeof(float) * frames);
        std::memset(outR, 0, sizeof(float) * frames);

        // Safety check - if components don't exist, return silence
        if (!fVoice.osc || !fVoice.filter || !fVoice.amp_env || !fVoice.filter_env) {
            return;
        }

        int sampleRate = (int)getSampleRate();
        if (sampleRate <= 0) sampleRate = 44100; // Fallback

        uint32_t framePos = 0;
        uint32_t midiEventIndex = 0;

        while (framePos < frames) {
            // Process MIDI events at their frame position
            while (midiEventIndex < midiEventCount &&
                   midiEvents[midiEventIndex].frame == framePos) {
                const MidiEvent& event = midiEvents[midiEventIndex++];

                if (event.size < 1) continue;

                const uint8_t status = event.data[0] & 0xF0;

                if (status == 0x90 && event.size >= 3) {
                    // Note On
                    const uint8_t note = event.data[1];
                    const uint8_t velocity = event.data[2];

                    if (velocity > 0) {
                        handleNoteOn(note, velocity);
                    } else {
                        handleNoteOff(note);
                    }
                }
                else if (status == 0x80 && event.size >= 3) {
                    // Note Off
                    const uint8_t note = event.data[1];
                    handleNoteOff(note);
                }
            }

            // Render audio for this sample
            if (fVoice.active) {
                // Handle pitch slide/portamento
                if (fVoice.sliding && fSlideTime > 0.0f) {
                    // Calculate slide speed (Hz per sample)
                    float slide_rate = (fVoice.target_freq - fVoice.current_freq) / (fSlideTime * sampleRate);

                    // Update current frequency
                    fVoice.current_freq += slide_rate;

                    // Check if we've reached the target
                    if ((slide_rate > 0.0f && fVoice.current_freq >= fVoice.target_freq) ||
                        (slide_rate < 0.0f && fVoice.current_freq <= fVoice.target_freq)) {
                        fVoice.current_freq = fVoice.target_freq;
                        fVoice.sliding = false;
                    }

                    // Update oscillator frequency
                    synth_oscillator_set_frequency(fVoice.osc, fVoice.current_freq);
                }

                // Generate oscillator output
                float sample = synth_oscillator_process(fVoice.osc, sampleRate);

                // Reduce oscillator level to prevent clipping (sawtooth is hot!)
                // Heavy reduction for proper mix levels
                sample *= 0.25f;

                // Apply amplitude envelope
                float amp_env_value = synth_envelope_process(fVoice.amp_env, sampleRate);

                // Apply accent (increases amplitude and filter cutoff)
                float accent_amount = fAccent * (fVoice.velocity / 127.0f);
                float amp = amp_env_value * (1.0f + accent_amount);

                sample *= amp;

                // Calculate filter cutoff with envelope modulation
                float filter_env_value = synth_envelope_process(fVoice.filter_env, sampleRate);
                float cutoff = fCutoff + (fEnvMod * filter_env_value * (1.0f + accent_amount));
                if (cutoff > 1.0f) cutoff = 1.0f;
                if (cutoff < 0.0f) cutoff = 0.0f;

                synth_filter_set_cutoff(fVoice.filter, cutoff);

                // Apply filter
                sample = synth_filter_process(fVoice.filter, sample, sampleRate);

                // Apply master volume
                sample *= fVolume;

                // Soft clipping to prevent hard clipping
                if (sample > 1.0f) sample = 1.0f;
                if (sample < -1.0f) sample = -1.0f;

                // Output to both channels (mono synth)
                outL[framePos] = sample;
                outR[framePos] = sample;

                // Check if voice should be deactivated
                if (!synth_envelope_is_active(fVoice.amp_env)) {
                    fVoice.active = false;
                }
            }

            framePos++;
        }
    }

private:
    void handleNoteOn(uint8_t note, uint8_t velocity)
    {
        // Safety check
        if (!fVoice.osc || !fVoice.amp_env || !fVoice.filter_env) return;

        // Calculate new frequency
        float new_freq = 440.0f * powf(2.0f, (note - 69) / 12.0f);

        // TB303-style slide: Only slide if previous note is still HELD (gate is on)
        // If gate is off (note_off was called), this is a new note, not a slide
        bool shouldSlide = fVoice.gate && fVoice.active;

        fVoice.note = note;
        fVoice.velocity = velocity;
        fVoice.active = true;
        fVoice.gate = true;  // New note is now being held

        if (shouldSlide) {
            // Previous note was still being held - SLIDE to new note
            // Don't retrigger envelopes, just change pitch
            fVoice.target_freq = new_freq;
            fVoice.sliding = true;
            // current_freq stays at current value, will glide to target_freq
        } else {
            // Previous note was released or no previous note - START fresh note (no slide)
            fVoice.current_freq = new_freq;
            fVoice.target_freq = new_freq;
            fVoice.sliding = false;

            synth_oscillator_set_frequency(fVoice.osc, new_freq);

            // Set waveform (0=Saw, 1=Square)
            SynthOscWaveform waveform = (fWaveform < 0.5f) ? SYNTH_OSC_SAW : SYNTH_OSC_SQUARE;
            synth_oscillator_set_waveform(fVoice.osc, waveform);

            // Trigger envelopes
            synth_envelope_trigger(fVoice.amp_env);
            synth_envelope_trigger(fVoice.filter_env);
        }
    }

    void handleNoteOff(uint8_t note)
    {
        // Safety check
        if (!fVoice.amp_env || !fVoice.filter_env) return;

        if (fVoice.note == note && fVoice.active) {
            fVoice.gate = false;  // Note is no longer being held
            synth_envelope_release(fVoice.amp_env);
            synth_envelope_release(fVoice.filter_env);
        }
    }

    RG303Voice fVoice;

    // Parameters
    float fWaveform;
    float fCutoff;
    float fResonance;
    float fEnvMod;
    float fDecay;
    float fAccent;
    float fSlideTime;
    float fVolume;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RG303_SynthPlugin)
};

Plugin* createPlugin()
{
    return new RG303_SynthPlugin();
}

END_NAMESPACE_DISTRHO
