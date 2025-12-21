#include "DistrhoPlugin.hpp"
#include "../../synth/synth_oscillator.h"
#include "../../synth/synth_envelope.h"
#include "../../synth/synth_noise.h"
#include <cstring>
#include <cmath>

START_NAMESPACE_DISTRHO

#define MAX_VOICES 5

enum DrumType {
    DRUM_BD = 0,
    DRUM_SD,
    DRUM_TOM1,
    DRUM_TOM2,
    DRUM_TOM3
};

struct SimmonsVoice {
    SynthOscillator* osc;
    SynthEnvelope* env;
    SynthEnvelope* pitch_env;
    SynthNoise* noise;
    DrumType type;
    bool active;
    float phase;

    // Pitch envelope state
    float start_pitch;
    float end_pitch;
};

class RGDS3_DrumPlugin : public Plugin
{
public:
    RGDS3_DrumPlugin()
        : Plugin(kParameterCount, 0, 0)
        // Bass Drum
        , fBDTone(0.3f)
        , fBDBend(0.7f)
        , fBDDecay(0.5f)
        , fBDNoise(0.1f)
        , fBDLevel(0.8f)
        // Snare Drum
        , fSDTone(0.5f)
        , fSDBend(0.5f)
        , fSDDecay(0.3f)
        , fSDNoise(0.6f)
        , fSDLevel(0.7f)
        // Tom 1
        , fT1Tone(0.6f)
        , fT1Bend(0.6f)
        , fT1Decay(0.4f)
        , fT1Noise(0.2f)
        , fT1Level(0.7f)
        // Tom 2
        , fT2Tone(0.5f)
        , fT2Bend(0.6f)
        , fT2Decay(0.4f)
        , fT2Noise(0.2f)
        , fT2Level(0.7f)
        // Tom 3
        , fT3Tone(0.4f)
        , fT3Bend(0.6f)
        , fT3Decay(0.4f)
        , fT3Noise(0.2f)
        , fT3Level(0.7f)
        // Master
        , fVolume(0.7f)
    {
        // Initialize voices
        for (int i = 0; i < MAX_VOICES; i++) {
            fVoices[i].osc = synth_oscillator_create();
            fVoices[i].env = synth_envelope_create();
            fVoices[i].pitch_env = synth_envelope_create();
            fVoices[i].noise = synth_noise_create();
            fVoices[i].type = (DrumType)i;
            fVoices[i].active = false;
            fVoices[i].phase = 0.0f;
            fVoices[i].start_pitch = 1.0f;
            fVoices[i].end_pitch = 1.0f;

            if (fVoices[i].osc) {
                synth_oscillator_set_waveform(fVoices[i].osc, SYNTH_OSC_SINE);
            }
        }
    }

    ~RGDS3_DrumPlugin() override
    {
        for (int i = 0; i < MAX_VOICES; i++) {
            if (fVoices[i].osc) synth_oscillator_destroy(fVoices[i].osc);
            if (fVoices[i].env) synth_envelope_destroy(fVoices[i].env);
            if (fVoices[i].pitch_env) synth_envelope_destroy(fVoices[i].pitch_env);
            if (fVoices[i].noise) synth_noise_destroy(fVoices[i].noise);
        }
    }

protected:
    const char* getLabel() const override
    {
        return RGDS3_DISPLAY_NAME;
    }

    const char* getDescription() const override
    {
        return RGDS3_DESCRIPTION;
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
        return d_cconst('R', 'D', 'S', '3');
    }

    void initParameter(uint32_t index, Parameter& param) override
    {
        param.hints = kParameterIsAutomatable;
        param.ranges.min = 0.0f;
        param.ranges.max = 1.0f;
        param.ranges.def = 0.5f;

        switch (index) {
        // Bass Drum
        case kParameterBDTone:
            param.name = "BD Tone";
            param.symbol = "bd_tone";
            param.ranges.def = 0.3f;
            break;
        case kParameterBDBend:
            param.name = "BD Bend";
            param.symbol = "bd_bend";
            param.ranges.def = 0.7f;
            break;
        case kParameterBDDecay:
            param.name = "BD Decay";
            param.symbol = "bd_decay";
            param.ranges.def = 0.5f;
            break;
        case kParameterBDNoise:
            param.name = "BD Noise";
            param.symbol = "bd_noise";
            param.ranges.def = 0.1f;
            break;
        case kParameterBDLevel:
            param.name = "BD Level";
            param.symbol = "bd_level";
            param.ranges.def = 0.8f;
            break;

        // Snare Drum
        case kParameterSDTone:
            param.name = "SD Tone";
            param.symbol = "sd_tone";
            param.ranges.def = 0.5f;
            break;
        case kParameterSDBend:
            param.name = "SD Bend";
            param.symbol = "sd_bend";
            param.ranges.def = 0.5f;
            break;
        case kParameterSDDecay:
            param.name = "SD Decay";
            param.symbol = "sd_decay";
            param.ranges.def = 0.3f;
            break;
        case kParameterSDNoise:
            param.name = "SD Noise";
            param.symbol = "sd_noise";
            param.ranges.def = 0.6f;
            break;
        case kParameterSDLevel:
            param.name = "SD Level";
            param.symbol = "sd_level";
            param.ranges.def = 0.7f;
            break;

        // Tom 1
        case kParameterT1Tone:
            param.name = "T1 Tone";
            param.symbol = "t1_tone";
            param.ranges.def = 0.6f;
            break;
        case kParameterT1Bend:
            param.name = "T1 Bend";
            param.symbol = "t1_bend";
            param.ranges.def = 0.6f;
            break;
        case kParameterT1Decay:
            param.name = "T1 Decay";
            param.symbol = "t1_decay";
            param.ranges.def = 0.4f;
            break;
        case kParameterT1Noise:
            param.name = "T1 Noise";
            param.symbol = "t1_noise";
            param.ranges.def = 0.2f;
            break;
        case kParameterT1Level:
            param.name = "T1 Level";
            param.symbol = "t1_level";
            param.ranges.def = 0.7f;
            break;

        // Tom 2
        case kParameterT2Tone:
            param.name = "T2 Tone";
            param.symbol = "t2_tone";
            param.ranges.def = 0.5f;
            break;
        case kParameterT2Bend:
            param.name = "T2 Bend";
            param.symbol = "t2_bend";
            param.ranges.def = 0.6f;
            break;
        case kParameterT2Decay:
            param.name = "T2 Decay";
            param.symbol = "t2_decay";
            param.ranges.def = 0.4f;
            break;
        case kParameterT2Noise:
            param.name = "T2 Noise";
            param.symbol = "t2_noise";
            param.ranges.def = 0.2f;
            break;
        case kParameterT2Level:
            param.name = "T2 Level";
            param.symbol = "t2_level";
            param.ranges.def = 0.7f;
            break;

        // Tom 3
        case kParameterT3Tone:
            param.name = "T3 Tone";
            param.symbol = "t3_tone";
            param.ranges.def = 0.4f;
            break;
        case kParameterT3Bend:
            param.name = "T3 Bend";
            param.symbol = "t3_bend";
            param.ranges.def = 0.6f;
            break;
        case kParameterT3Decay:
            param.name = "T3 Decay";
            param.symbol = "t3_decay";
            param.ranges.def = 0.4f;
            break;
        case kParameterT3Noise:
            param.name = "T3 Noise";
            param.symbol = "t3_noise";
            param.ranges.def = 0.2f;
            break;
        case kParameterT3Level:
            param.name = "T3 Level";
            param.symbol = "t3_level";
            param.ranges.def = 0.7f;
            break;

        // Master
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
        case kParameterBDTone: return fBDTone;
        case kParameterBDBend: return fBDBend;
        case kParameterBDDecay: return fBDDecay;
        case kParameterBDNoise: return fBDNoise;
        case kParameterBDLevel: return fBDLevel;
        case kParameterSDTone: return fSDTone;
        case kParameterSDBend: return fSDBend;
        case kParameterSDDecay: return fSDDecay;
        case kParameterSDNoise: return fSDNoise;
        case kParameterSDLevel: return fSDLevel;
        case kParameterT1Tone: return fT1Tone;
        case kParameterT1Bend: return fT1Bend;
        case kParameterT1Decay: return fT1Decay;
        case kParameterT1Noise: return fT1Noise;
        case kParameterT1Level: return fT1Level;
        case kParameterT2Tone: return fT2Tone;
        case kParameterT2Bend: return fT2Bend;
        case kParameterT2Decay: return fT2Decay;
        case kParameterT2Noise: return fT2Noise;
        case kParameterT2Level: return fT2Level;
        case kParameterT3Tone: return fT3Tone;
        case kParameterT3Bend: return fT3Bend;
        case kParameterT3Decay: return fT3Decay;
        case kParameterT3Noise: return fT3Noise;
        case kParameterT3Level: return fT3Level;
        case kParameterVolume: return fVolume;
        default: return 0.0f;
        }
    }

    void setParameterValue(uint32_t index, float value) override
    {
        switch (index) {
        case kParameterBDTone: fBDTone = value; break;
        case kParameterBDBend: fBDBend = value; break;
        case kParameterBDDecay: fBDDecay = value; break;
        case kParameterBDNoise: fBDNoise = value; break;
        case kParameterBDLevel: fBDLevel = value; break;
        case kParameterSDTone: fSDTone = value; break;
        case kParameterSDBend: fSDBend = value; break;
        case kParameterSDDecay: fSDDecay = value; break;
        case kParameterSDNoise: fSDNoise = value; break;
        case kParameterSDLevel: fSDLevel = value; break;
        case kParameterT1Tone: fT1Tone = value; break;
        case kParameterT1Bend: fT1Bend = value; break;
        case kParameterT1Decay: fT1Decay = value; break;
        case kParameterT1Noise: fT1Noise = value; break;
        case kParameterT1Level: fT1Level = value; break;
        case kParameterT2Tone: fT2Tone = value; break;
        case kParameterT2Bend: fT2Bend = value; break;
        case kParameterT2Decay: fT2Decay = value; break;
        case kParameterT2Noise: fT2Noise = value; break;
        case kParameterT2Level: fT2Level = value; break;
        case kParameterT3Tone: fT3Tone = value; break;
        case kParameterT3Bend: fT3Bend = value; break;
        case kParameterT3Decay: fT3Decay = value; break;
        case kParameterT3Noise: fT3Noise = value; break;
        case kParameterT3Level: fT3Level = value; break;
        case kParameterVolume: fVolume = value; break;
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
    void triggerDrum(uint8_t note, uint8_t velocity, int sampleRate)
    {
        DrumType type;
        float tone, bend, decay, noise_level, level;

        // Map MIDI note to drum type and get parameters
        switch (note) {
        case MIDI_NOTE_BD:
            type = DRUM_BD;
            tone = fBDTone;
            bend = fBDBend;
            decay = fBDDecay;
            noise_level = fBDNoise;
            level = fBDLevel;
            break;
        case MIDI_NOTE_SD:
            type = DRUM_SD;
            tone = fSDTone;
            bend = fSDBend;
            decay = fSDDecay;
            noise_level = fSDNoise;
            level = fSDLevel;
            break;
        case MIDI_NOTE_TOM1:
            type = DRUM_TOM1;
            tone = fT1Tone;
            bend = fT1Bend;
            decay = fT1Decay;
            noise_level = fT1Noise;
            level = fT1Level;
            break;
        case MIDI_NOTE_TOM2:
            type = DRUM_TOM2;
            tone = fT2Tone;
            bend = fT2Bend;
            decay = fT2Decay;
            noise_level = fT2Noise;
            level = fT2Level;
            break;
        case MIDI_NOTE_TOM3:
            type = DRUM_TOM3;
            tone = fT3Tone;
            bend = fT3Bend;
            decay = fT3Decay;
            noise_level = fT3Noise;
            level = fT3Level;
            break;
        default:
            return; // Unknown note
        }

        SimmonsVoice* voice = &fVoices[type];
        if (!voice->env || !voice->pitch_env) return;

        // Setup amplitude envelope (fast attack, exponential decay)
        synth_envelope_set_attack(voice->env, 0.001f);
        synth_envelope_set_decay(voice->env, 0.01f + decay * 2.0f);
        synth_envelope_set_sustain(voice->env, 0.0f);
        synth_envelope_set_release(voice->env, 0.01f);

        // Setup pitch envelope (Simmons characteristic pitch sweep)
        // Start pitch high, sweep down based on bend amount
        float base_freq = 50.0f + tone * 300.0f; // 50Hz to 350Hz
        float bend_ratio = 1.5f + bend * 6.0f; // 1.5x to 7.5x pitch bend

        voice->start_pitch = base_freq * bend_ratio;
        voice->end_pitch = base_freq;

        // Pitch envelope: fast attack to start pitch, decay to end pitch
        synth_envelope_set_attack(voice->pitch_env, 0.001f);
        synth_envelope_set_decay(voice->pitch_env, 0.005f + bend * 0.1f); // Bend time
        synth_envelope_set_sustain(voice->pitch_env, 0.0f);
        synth_envelope_set_release(voice->pitch_env, 0.01f);

        // Trigger envelopes
        synth_envelope_trigger(voice->env);
        synth_envelope_trigger(voice->pitch_env);

        voice->active = true;
        voice->phase = 0.0f;
    }

    void renderFrame(float* outL, float* outR, uint32_t framePos, int sampleRate)
    {
        float mix = 0.0f;

        for (int i = 0; i < MAX_VOICES; i++) {
            SimmonsVoice* voice = &fVoices[i];
            if (!voice->active) continue;

            float env_value = synth_envelope_process(voice->env, sampleRate);
            float pitch_env_value = synth_envelope_process(voice->pitch_env, sampleRate);

            // Check if voice finished
            if (env_value <= 0.0f) {
                voice->active = false;
                continue;
            }

            // Calculate current pitch using pitch envelope
            float current_pitch = voice->end_pitch + (voice->start_pitch - voice->end_pitch) * pitch_env_value;

            // Generate tone oscillator
            if (voice->osc) {
                synth_oscillator_set_frequency(voice->osc, current_pitch);
            }
            float tone_sample = voice->osc ? synth_oscillator_process(voice->osc, sampleRate) : 0.0f;

            // Generate noise
            float noise_sample = voice->noise ? synth_noise_process(voice->noise) : 0.0f;

            // Get noise level for this drum type
            float noise_level = 0.0f;
            float level = 1.0f;

            switch (voice->type) {
            case DRUM_BD:
                noise_level = fBDNoise;
                level = fBDLevel;
                break;
            case DRUM_SD:
                noise_level = fSDNoise;
                level = fSDLevel;
                break;
            case DRUM_TOM1:
                noise_level = fT1Noise;
                level = fT1Level;
                break;
            case DRUM_TOM2:
                noise_level = fT2Noise;
                level = fT2Level;
                break;
            case DRUM_TOM3:
                noise_level = fT3Noise;
                level = fT3Level;
                break;
            }

            // Mix tone and noise
            float sample = tone_sample * (1.0f - noise_level) + noise_sample * noise_level;

            // Apply envelope and level
            sample *= env_value * level;

            mix += sample;
        }

        // Apply master volume
        mix *= fVolume * 0.5f;

        // Soft clipping
        if (mix > 1.0f) mix = 1.0f;
        if (mix < -1.0f) mix = -1.0f;

        outL[framePos] += mix;
        outR[framePos] += mix;
    }

    SimmonsVoice fVoices[MAX_VOICES];

    // Parameters
    float fBDTone, fBDBend, fBDDecay, fBDNoise, fBDLevel;
    float fSDTone, fSDBend, fSDDecay, fSDNoise, fSDLevel;
    float fT1Tone, fT1Bend, fT1Decay, fT1Noise, fT1Level;
    float fT2Tone, fT2Bend, fT2Decay, fT2Noise, fT2Level;
    float fT3Tone, fT3Bend, fT3Decay, fT3Noise, fT3Level;
    float fVolume;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RGDS3_DrumPlugin)
};

Plugin* createPlugin()
{
    return new RGDS3_DrumPlugin();
}

END_NAMESPACE_DISTRHO
