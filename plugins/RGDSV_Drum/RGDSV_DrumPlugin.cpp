#include "DistrhoPlugin.hpp"
#include "../../synth/synth_oscillator.h"
#include "../../synth/synth_envelope.h"
#include "../../synth/synth_noise.h"
#include "../../synth/synth_filter.h"
#include <cstring>
#include <cmath>

START_NAMESPACE_DISTRHO

#define MAX_VOICES 6

enum DrumType {
    DRUM_BD = 0,
    DRUM_SD,
    DRUM_TOM1,
    DRUM_TOM2,
    DRUM_TOM3,
    DRUM_TOM4
};

struct SimmonsVVoice {
    SynthOscillator* osc;
    SynthEnvelope* env;
    SynthEnvelope* pitch_env;
    SynthNoise* noise;
    SynthFilter* filter;
    DrumType type;
    bool active;

    // Pitch envelope state
    float start_pitch;
    float end_pitch;

    // Click transient
    float click_phase;
};

class RGDSV_DrumPlugin : public Plugin
{
public:
    RGDSV_DrumPlugin()
        : Plugin(kParameterCount, 0, 0)
    {
        // Initialize default parameters
        initDrumDefaults();

        // Initialize voices
        for (int i = 0; i < MAX_VOICES; i++) {
            fVoices[i].osc = synth_oscillator_create();
            fVoices[i].env = synth_envelope_create();
            fVoices[i].pitch_env = synth_envelope_create();
            fVoices[i].noise = synth_noise_create();
            fVoices[i].filter = synth_filter_create();
            fVoices[i].type = (DrumType)i;
            fVoices[i].active = false;
            fVoices[i].start_pitch = 1.0f;
            fVoices[i].end_pitch = 1.0f;
            fVoices[i].click_phase = 0.0f;

            if (fVoices[i].osc) {
                synth_oscillator_set_waveform(fVoices[i].osc, SYNTH_OSC_SINE);
            }
        }
    }

    ~RGDSV_DrumPlugin() override
    {
        for (int i = 0; i < MAX_VOICES; i++) {
            if (fVoices[i].osc) synth_oscillator_destroy(fVoices[i].osc);
            if (fVoices[i].env) synth_envelope_destroy(fVoices[i].env);
            if (fVoices[i].pitch_env) synth_envelope_destroy(fVoices[i].pitch_env);
            if (fVoices[i].noise) synth_noise_destroy(fVoices[i].noise);
            if (fVoices[i].filter) synth_filter_destroy(fVoices[i].filter);
        }
    }

protected:
    const char* getLabel() const override
    {
        return RGDSV_DISPLAY_NAME;
    }

    const char* getDescription() const override
    {
        return RGDSV_DESCRIPTION;
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
        return d_cconst('R', 'D', 'S', 'V');
    }

    void initParameter(uint32_t index, Parameter& param) override
    {
        param.hints = kParameterIsAutomatable;
        param.ranges.min = 0.0f;
        param.ranges.max = 1.0f;
        param.ranges.def = 0.5f;

        // Helper macro for drum parameters
        #define DRUM_PARAMS(PREFIX, NAME, WAVE_DEF, TONE_DEF, BEND_DEF, DECAY_DEF, CLICK_DEF, NOISE_DEF, FILTER_DEF, LEVEL_DEF) \
            case kParameter##PREFIX##Wave: \
                param.name = NAME " Wave"; \
                param.symbol = #PREFIX "_wave"; \
                param.ranges.def = WAVE_DEF; \
                break; \
            case kParameter##PREFIX##Tone: \
                param.name = NAME " Tone"; \
                param.symbol = #PREFIX "_tone"; \
                param.ranges.def = TONE_DEF; \
                break; \
            case kParameter##PREFIX##Bend: \
                param.name = NAME " Bend"; \
                param.symbol = #PREFIX "_bend"; \
                param.ranges.def = BEND_DEF; \
                break; \
            case kParameter##PREFIX##Decay: \
                param.name = NAME " Decay"; \
                param.symbol = #PREFIX "_decay"; \
                param.ranges.def = DECAY_DEF; \
                break; \
            case kParameter##PREFIX##Click: \
                param.name = NAME " Click"; \
                param.symbol = #PREFIX "_click"; \
                param.ranges.def = CLICK_DEF; \
                break; \
            case kParameter##PREFIX##Noise: \
                param.name = NAME " Noise"; \
                param.symbol = #PREFIX "_noise"; \
                param.ranges.def = NOISE_DEF; \
                break; \
            case kParameter##PREFIX##Filter: \
                param.name = NAME " Filter"; \
                param.symbol = #PREFIX "_filter"; \
                param.ranges.def = FILTER_DEF; \
                break; \
            case kParameter##PREFIX##Level: \
                param.name = NAME " Level"; \
                param.symbol = #PREFIX "_level"; \
                param.ranges.def = LEVEL_DEF; \
                break;

        switch (index) {
        DRUM_PARAMS(BD, "BD", 0.0f, 0.3f, 0.7f, 0.5f, 0.3f, 0.1f, 0.8f, 0.8f)
        DRUM_PARAMS(SD, "SD", 0.0f, 0.5f, 0.5f, 0.3f, 0.5f, 0.6f, 0.6f, 0.7f)
        DRUM_PARAMS(T1, "T1", 0.0f, 0.6f, 0.6f, 0.4f, 0.3f, 0.2f, 0.7f, 0.7f)
        DRUM_PARAMS(T2, "T2", 0.0f, 0.5f, 0.6f, 0.4f, 0.3f, 0.2f, 0.7f, 0.7f)
        DRUM_PARAMS(T3, "T3", 0.0f, 0.4f, 0.6f, 0.4f, 0.3f, 0.2f, 0.7f, 0.7f)
        DRUM_PARAMS(T4, "T4", 0.0f, 0.35f, 0.6f, 0.4f, 0.3f, 0.2f, 0.7f, 0.7f)

        case kParameterVolume:
            param.name = "Volume";
            param.symbol = "volume";
            param.ranges.def = 0.7f;
            break;
        }

        #undef DRUM_PARAMS
    }

    float getParameterValue(uint32_t index) const override
    {
        if (index < kParameterCount) {
            return fParameters[index];
        }
        return 0.0f;
    }

    void setParameterValue(uint32_t index, float value) override
    {
        if (index < kParameterCount) {
            fParameters[index] = value;
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
                triggerDrum(note, velocity, sampleRate);
            }
        }

        // Render remaining frames
        while (framePos < frames) {
            renderFrame(outL, outR, framePos, sampleRate);
            framePos++;
        }
    }

private:
    void initDrumDefaults()
    {
        // Initialize all parameters to 0.5f
        for (int i = 0; i < kParameterCount; i++) {
            fParameters[i] = 0.5f;
        }

        // Set specific defaults (matching initParameter)
        fParameters[kParameterBDWave] = 0.0f;
        fParameters[kParameterBDTone] = 0.3f;
        fParameters[kParameterBDBend] = 0.7f;
        fParameters[kParameterBDDecay] = 0.5f;
        fParameters[kParameterBDClick] = 0.3f;
        fParameters[kParameterBDNoise] = 0.1f;
        fParameters[kParameterBDFilter] = 0.8f;
        fParameters[kParameterBDLevel] = 0.8f;

        fParameters[kParameterSDWave] = 0.0f;
        fParameters[kParameterSDTone] = 0.5f;
        fParameters[kParameterSDBend] = 0.5f;
        fParameters[kParameterSDDecay] = 0.3f;
        fParameters[kParameterSDClick] = 0.5f;
        fParameters[kParameterSDNoise] = 0.6f;
        fParameters[kParameterSDFilter] = 0.6f;
        fParameters[kParameterSDLevel] = 0.7f;

        fParameters[kParameterT1Wave] = 0.0f;
        fParameters[kParameterT1Tone] = 0.6f;
        fParameters[kParameterT1Bend] = 0.6f;
        fParameters[kParameterT1Decay] = 0.4f;
        fParameters[kParameterT1Click] = 0.3f;
        fParameters[kParameterT1Noise] = 0.2f;
        fParameters[kParameterT1Filter] = 0.7f;
        fParameters[kParameterT1Level] = 0.7f;

        fParameters[kParameterT2Wave] = 0.0f;
        fParameters[kParameterT2Tone] = 0.5f;
        fParameters[kParameterT2Bend] = 0.6f;
        fParameters[kParameterT2Decay] = 0.4f;
        fParameters[kParameterT2Click] = 0.3f;
        fParameters[kParameterT2Noise] = 0.2f;
        fParameters[kParameterT2Filter] = 0.7f;
        fParameters[kParameterT2Level] = 0.7f;

        fParameters[kParameterT3Wave] = 0.0f;
        fParameters[kParameterT3Tone] = 0.4f;
        fParameters[kParameterT3Bend] = 0.6f;
        fParameters[kParameterT3Decay] = 0.4f;
        fParameters[kParameterT3Click] = 0.3f;
        fParameters[kParameterT3Noise] = 0.2f;
        fParameters[kParameterT3Filter] = 0.7f;
        fParameters[kParameterT3Level] = 0.7f;

        fParameters[kParameterT4Wave] = 0.0f;
        fParameters[kParameterT4Tone] = 0.35f;
        fParameters[kParameterT4Bend] = 0.6f;
        fParameters[kParameterT4Decay] = 0.4f;
        fParameters[kParameterT4Click] = 0.3f;
        fParameters[kParameterT4Noise] = 0.2f;
        fParameters[kParameterT4Filter] = 0.7f;
        fParameters[kParameterT4Level] = 0.7f;

        fParameters[kParameterVolume] = 0.7f;
    }

    void triggerDrum(uint8_t note, uint8_t velocity, int sampleRate)
    {
        DrumType type;
        int param_base;

        // Map MIDI note to drum type
        switch (note) {
        case MIDI_NOTE_BD:
            type = DRUM_BD;
            param_base = kParameterBDWave;
            break;
        case MIDI_NOTE_SD:
            type = DRUM_SD;
            param_base = kParameterSDWave;
            break;
        case MIDI_NOTE_TOM1:
            type = DRUM_TOM1;
            param_base = kParameterT1Wave;
            break;
        case MIDI_NOTE_TOM2:
            type = DRUM_TOM2;
            param_base = kParameterT2Wave;
            break;
        case MIDI_NOTE_TOM3:
            type = DRUM_TOM3;
            param_base = kParameterT3Wave;
            break;
        case MIDI_NOTE_TOM4:
            type = DRUM_TOM4;
            param_base = kParameterT4Wave;
            break;
        default:
            return; // Unknown note
        }

        // Get parameters for this drum
        float wave = fParameters[param_base + 0];
        float tone = fParameters[param_base + 1];
        float bend = fParameters[param_base + 2];
        float decay = fParameters[param_base + 3];
        // click, noise, filter, level retrieved in renderFrame

        SimmonsVVoice* voice = &fVoices[type];
        if (!voice->env || !voice->pitch_env) return;

        // Set waveform based on wave parameter
        if (voice->osc) {
            SynthOscWaveform waveform = SYNTH_OSC_SINE;
            if (wave < 0.33f) waveform = SYNTH_OSC_SINE;
            else if (wave < 0.66f) waveform = SYNTH_OSC_SAW;
            else waveform = SYNTH_OSC_SQUARE;
            synth_oscillator_set_waveform(voice->osc, waveform);
        }

        // Setup amplitude envelope
        synth_envelope_set_attack(voice->env, 0.001f);
        synth_envelope_set_decay(voice->env, 0.01f + decay * 2.0f);
        synth_envelope_set_sustain(voice->env, 0.0f);
        synth_envelope_set_release(voice->env, 0.01f);

        // Setup pitch envelope
        float base_freq = 50.0f + tone * 300.0f;
        float bend_ratio = 1.5f + bend * 6.0f;

        voice->start_pitch = base_freq * bend_ratio;
        voice->end_pitch = base_freq;

        synth_envelope_set_attack(voice->pitch_env, 0.001f);
        synth_envelope_set_decay(voice->pitch_env, 0.005f + bend * 0.1f);
        synth_envelope_set_sustain(voice->pitch_env, 0.0f);
        synth_envelope_set_release(voice->pitch_env, 0.01f);

        // Trigger envelopes
        synth_envelope_trigger(voice->env);
        synth_envelope_trigger(voice->pitch_env);

        voice->active = true;
        voice->click_phase = 0.0f;
    }

    void renderFrame(float* outL, float* outR, uint32_t framePos, int sampleRate)
    {
        float mix = 0.0f;

        for (int i = 0; i < MAX_VOICES; i++) {
            SimmonsVVoice* voice = &fVoices[i];
            if (!voice->active) continue;

            float env_value = synth_envelope_process(voice->env, sampleRate);
            float pitch_env_value = synth_envelope_process(voice->pitch_env, sampleRate);

            // Check if voice finished
            if (env_value <= 0.0f) {
                voice->active = false;
                continue;
            }

            // Get parameters for this drum
            int param_base = voice->type * 8;  // 8 params per drum
            float click_level = fParameters[param_base + 4];
            float noise_level = fParameters[param_base + 5];
            float filter_cutoff = fParameters[param_base + 6];
            float level = fParameters[param_base + 7];

            // Calculate current pitch
            float current_pitch = voice->end_pitch + (voice->start_pitch - voice->end_pitch) * pitch_env_value;

            // Generate tone oscillator
            if (voice->osc) {
                synth_oscillator_set_frequency(voice->osc, current_pitch);
            }
            float tone_sample = voice->osc ? synth_oscillator_process(voice->osc, sampleRate) : 0.0f;

            // Generate click transient (short high-frequency burst)
            float click_sample = 0.0f;
            if (click_level > 0.0f && voice->click_phase < 0.005f) {
                // High frequency sine for click
                click_sample = sinf(voice->click_phase * 2.0f * M_PI * 8000.0f) * (1.0f - voice->click_phase / 0.005f);
                voice->click_phase += 1.0f / sampleRate;
            }

            // Generate noise
            float noise_sample = voice->noise ? synth_noise_process(voice->noise) : 0.0f;

            // Mix tone, click, and noise
            float sample = tone_sample * (1.0f - noise_level) + noise_sample * noise_level;
            sample += click_sample * click_level;

            // Apply filter
            if (voice->filter) {
                synth_filter_set_cutoff(voice->filter, filter_cutoff);
                synth_filter_set_resonance(voice->filter, 0.3f);
                sample = synth_filter_process(voice->filter, sample, sampleRate);
            }

            // Apply envelope and level
            sample *= env_value * level;

            mix += sample;
        }

        // Apply master volume
        mix *= fParameters[kParameterVolume] * 0.4f;

        // Soft clipping
        if (mix > 1.0f) mix = 1.0f;
        if (mix < -1.0f) mix = -1.0f;

        outL[framePos] += mix;
        outR[framePos] += mix;
    }

    SimmonsVVoice fVoices[MAX_VOICES];
    float fParameters[kParameterCount];

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RGDSV_DrumPlugin)
};

Plugin* createPlugin()
{
    return new RGDSV_DrumPlugin();
}

END_NAMESPACE_DISTRHO
