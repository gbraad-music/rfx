#include "DistrhoPlugin.hpp"
#include "../../synth/synth_envelope.h"
#include "../../synth/synth_voice_manager.h"
#include <cstring>
#include <cmath>

START_NAMESPACE_DISTRHO

#define MAX_VOICES 8

struct SonixVoice {
    SynthEnvelope* envelope;
    int note;
    int velocity;
    bool active;
    float phase;  // Playback position in wavetable (0.0 to WAVETABLE_SIZE)
};

class RGSonix_SynthPlugin : public Plugin
{
public:
    RGSonix_SynthPlugin()
        : Plugin(kParameterCount, 0, 1)  // 1 state (waveform)
        , fAttack(0.01f)
        , fDecay(0.3f)
        , fSustain(0.7f)
        , fRelease(0.5f)
        , fPitchBend(0.0f)
        , fFineTune(0.0f)
        , fCoarseTune(0.0f)
        , fVolume(0.7f)
    {
        // Initialize with sine wave
        for (int i = 0; i < WAVETABLE_SIZE; i++) {
            float phase = (float)i / WAVETABLE_SIZE;
            fWavetable[i] = sinf(phase * 2.0f * M_PI);
        }

        // Create voice manager
        fVoiceManager = synth_voice_manager_create(MAX_VOICES);

        // Initialize voices
        for (int i = 0; i < MAX_VOICES; i++) {
            fVoices[i].envelope = synth_envelope_create();
            fVoices[i].note = -1;
            fVoices[i].velocity = 0;
            fVoices[i].active = false;
            fVoices[i].phase = 0.0f;
        }

        updateEnvelope();
    }

    ~RGSonix_SynthPlugin() override
    {
        for (int i = 0; i < MAX_VOICES; i++) {
            if (fVoices[i].envelope) synth_envelope_destroy(fVoices[i].envelope);
        }
        if (fVoiceManager) synth_voice_manager_destroy(fVoiceManager);
    }

protected:
    const char* getLabel() const override
    {
        return RGSONIX_DISPLAY_NAME;
    }

    const char* getDescription() const override
    {
        return RGSONIX_DESCRIPTION;
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
        return d_cconst('R', 'G', 'S', 'X');
    }

    void initParameter(uint32_t index, Parameter& param) override
    {
        param.hints = kParameterIsAutomatable;
        param.ranges.min = 0.0f;
        param.ranges.max = 1.0f;
        param.ranges.def = 0.5f;

        switch (index) {
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
        case kParameterPitchBend:
            param.name = "Pitch Bend";
            param.symbol = "pitch_bend";
            param.ranges.min = -1.0f;
            param.ranges.max = 1.0f;
            param.ranges.def = 0.0f;
            break;
        case kParameterFineTune:
            param.name = "Fine Tune";
            param.symbol = "fine_tune";
            param.ranges.min = -1.0f;
            param.ranges.max = 1.0f;
            param.ranges.def = 0.0f;
            break;
        case kParameterCoarseTune:
            param.name = "Coarse Tune";
            param.symbol = "coarse_tune";
            param.ranges.min = -12.0f;
            param.ranges.max = 12.0f;
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
        case kParameterAttack: return fAttack;
        case kParameterDecay: return fDecay;
        case kParameterSustain: return fSustain;
        case kParameterRelease: return fRelease;
        case kParameterPitchBend: return fPitchBend;
        case kParameterFineTune: return fFineTune;
        case kParameterCoarseTune: return fCoarseTune;
        case kParameterVolume: return fVolume;
        default: return 0.0f;
        }
    }

    void setParameterValue(uint32_t index, float value) override
    {
        switch (index) {
        case kParameterAttack: fAttack = value; updateEnvelope(); break;
        case kParameterDecay: fDecay = value; updateEnvelope(); break;
        case kParameterSustain: fSustain = value; updateEnvelope(); break;
        case kParameterRelease: fRelease = value; updateEnvelope(); break;
        case kParameterPitchBend: fPitchBend = value; break;
        case kParameterFineTune: fFineTune = value; break;
        case kParameterCoarseTune: fCoarseTune = value; break;
        case kParameterVolume: fVolume = value; break;
        }
    }

    void initState(uint32_t index, State& state) override
    {
        if (index == 0) {
            state.key = "waveform";
            state.defaultValue = "";  // Will be set by UI
            state.label = "Waveform Data";
            state.hints = kStateIsOnlyForDSP;
        }
    }

    void setState(const char* key, const char* value) override
    {
        if (std::strcmp(key, "waveform") == 0) {
            // Parse waveform data (comma-separated floats)
            const char* ptr = value;
            for (int i = 0; i < WAVETABLE_SIZE && *ptr; i++) {
                fWavetable[i] = std::atof(ptr);
                // Skip to next comma
                while (*ptr && *ptr != ',') ptr++;
                if (*ptr == ',') ptr++;
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
        for (int i = 0; i < MAX_VOICES; i++) {
            if (fVoices[i].envelope) {
                synth_envelope_set_attack(fVoices[i].envelope, 0.001f + fAttack * 2.0f);
                synth_envelope_set_decay(fVoices[i].envelope, 0.01f + fDecay * 3.0f);
                synth_envelope_set_sustain(fVoices[i].envelope, fSustain);
                synth_envelope_set_release(fVoices[i].envelope, 0.01f + fRelease * 5.0f);
            }
        }
    }

    void handleNoteOn(uint8_t note, uint8_t velocity, int sampleRate)
    {
        if (!fVoiceManager) return;

        int voice_idx = synth_voice_manager_allocate(fVoiceManager, note, velocity);
        if (voice_idx < 0 || voice_idx >= MAX_VOICES) return;

        SonixVoice* voice = &fVoices[voice_idx];
        if (!voice->envelope) return;

        voice->note = note;
        voice->velocity = velocity;
        voice->phase = 0.0f;
        voice->active = true;

        synth_envelope_trigger(voice->envelope);
    }

    void handleNoteOff(uint8_t note)
    {
        if (!fVoiceManager) return;

        int voice_idx = synth_voice_manager_release(fVoiceManager, note);
        if (voice_idx < 0 || voice_idx >= MAX_VOICES) return;

        SonixVoice* voice = &fVoices[voice_idx];
        if (voice->envelope) {
            synth_envelope_release(voice->envelope);
        }
    }

    void renderFrame(float* outL, float* outR, uint32_t framePos, int sampleRate)
    {
        float mixL = 0.0f;
        float mixR = 0.0f;

        for (int i = 0; i < MAX_VOICES; i++) {
            const VoiceMeta* meta = synth_voice_manager_get_voice(fVoiceManager, i);
            if (!meta || meta->state == VOICE_INACTIVE) {
                fVoices[i].active = false;
                continue;
            }

            if (!fVoices[i].active) continue;

            SonixVoice* voice = &fVoices[i];

            float env_value = synth_envelope_process(voice->envelope, sampleRate);

            // Check if voice finished
            if (env_value <= 0.0f && meta->state == VOICE_RELEASING) {
                synth_voice_manager_stop_voice(fVoiceManager, i);
                voice->active = false;
                continue;
            }

            // Update amplitude for voice stealing
            synth_voice_manager_update_amplitude(fVoiceManager, i, env_value);

            // Calculate frequency with pitch modulation
            float note_freq = 440.0f * powf(2.0f, (voice->note - 69) / 12.0f);
            note_freq *= powf(2.0f, fCoarseTune / 12.0f);  // Coarse tune (semitones)
            note_freq *= powf(2.0f, fFineTune / 12.0f);    // Fine tune (cents as fraction)
            note_freq *= powf(2.0f, fPitchBend * 2.0f / 12.0f);  // Pitch bend (±2 semitones)

            // Calculate phase increment
            float phase_inc = (note_freq / sampleRate) * WAVETABLE_SIZE;

            // Read from wavetable with linear interpolation
            float int_part;
            float frac = modff(voice->phase, &int_part);
            int idx1 = (int)int_part % WAVETABLE_SIZE;
            int idx2 = (idx1 + 1) % WAVETABLE_SIZE;

            float sample = fWavetable[idx1] * (1.0f - frac) + fWavetable[idx2] * frac;

            // Advance phase
            voice->phase += phase_inc;
            while (voice->phase >= WAVETABLE_SIZE) {
                voice->phase -= WAVETABLE_SIZE;
            }

            // Apply envelope and velocity
            float vel_scale = voice->velocity / 127.0f;
            sample *= env_value * vel_scale;

            mixL += sample;
            mixR += sample;
        }

        // Reduce per-voice level for polyphony
        mixL *= 0.3f;
        mixR *= 0.3f;

        // Apply master volume
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
    SonixVoice fVoices[MAX_VOICES];

    // Wavetable
    float fWavetable[WAVETABLE_SIZE];

    // Parameters
    float fAttack, fDecay, fSustain, fRelease;
    float fPitchBend, fFineTune, fCoarseTune;
    float fVolume;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RGSonix_SynthPlugin)
};

Plugin* createPlugin()
{
    return new RGSonix_SynthPlugin();
}

END_NAMESPACE_DISTRHO
