#include "DistrhoPlugin.hpp"
#include "../../synth/synth_envelope.h"
#include "../../synth/synth_voice_manager.h"
#include <cstring>
#include <cmath>

START_NAMESPACE_DISTRHO

#define MAX_VOICES 8

struct SonixOscillator {
    SynthEnvelope* amp_env;
    SynthEnvelope* pitch_env;
    float phase;
    int waveform_index;
    float level;
    float detune;
    float phase_offset;
};

struct SonixVoice {
    SonixOscillator oscillators[NUM_OSCILLATORS];
    int note;
    int velocity;
    bool active;
};

class RGSonix_SynthPlugin : public Plugin
{
public:
    RGSonix_SynthPlugin()
        : Plugin(kParameterCount, 0, 1)  // 1 state (waveforms)
    {
        // Initialize waveforms with sine wave
        for (int w = 0; w < NUM_WAVEFORMS; w++) {
            for (int i = 0; i < WAVETABLE_SIZE; i++) {
                float phase = (float)i / WAVETABLE_SIZE;
                fWaveforms[w][i] = sinf(phase * 2.0f * M_PI);
            }
        }

        // Initialize parameters with defaults
        initDefaults();

        // Create voice manager
        fVoiceManager = synth_voice_manager_create(MAX_VOICES);

        // Initialize voices
        for (int v = 0; v < MAX_VOICES; v++) {
            for (int o = 0; o < NUM_OSCILLATORS; o++) {
                fVoices[v].oscillators[o].amp_env = synth_envelope_create();
                fVoices[v].oscillators[o].pitch_env = synth_envelope_create();
                fVoices[v].oscillators[o].phase = 0.0f;
                fVoices[v].oscillators[o].waveform_index = 0;
                fVoices[v].oscillators[o].level = 0.0f;
                fVoices[v].oscillators[o].detune = 0.0f;
                fVoices[v].oscillators[o].phase_offset = 0.0f;
            }
            fVoices[v].note = -1;
            fVoices[v].velocity = 0;
            fVoices[v].active = false;
        }

        updateEnvelopes();
    }

    ~RGSonix_SynthPlugin() override
    {
        for (int v = 0; v < MAX_VOICES; v++) {
            for (int o = 0; o < NUM_OSCILLATORS; o++) {
                if (fVoices[v].oscillators[o].amp_env)
                    synth_envelope_destroy(fVoices[v].oscillators[o].amp_env);
                if (fVoices[v].oscillators[o].pitch_env)
                    synth_envelope_destroy(fVoices[v].oscillators[o].pitch_env);
            }
        }
        if (fVoiceManager) synth_voice_manager_destroy(fVoiceManager);
    }

protected:
    const char* getLabel() const override { return RGSONIX_DISPLAY_NAME; }
    const char* getDescription() const override { return RGSONIX_DESCRIPTION; }
    const char* getMaker() const override { return "Regroove"; }
    const char* getHomePage() const override { return "https://music.gbraad.nl/regrooved/"; }
    const char* getLicense() const override { return "GPL-3.0"; }
    uint32_t getVersion() const override { return d_version(1, 0, 0); }
    int64_t getUniqueId() const override { return d_cconst('R', 'G', 'S', 'X'); }

    void initParameter(uint32_t index, Parameter& param) override
    {
        param.hints = kParameterIsAutomatable;
        param.ranges.min = 0.0f;
        param.ranges.max = 1.0f;
        param.ranges.def = 0.5f;

        // Helper macro for oscillator parameters
        #define OSC_PARAMS(n, base) \
            case kParameterOsc##n##Wave: \
                param.name = "Osc" #n " Wave"; \
                param.symbol = "osc" #n "_wave"; \
                param.ranges.max = NUM_WAVEFORMS - 1; \
                param.ranges.def = 0.0f; \
                break; \
            case kParameterOsc##n##Level: \
                param.name = "Osc" #n " Level"; \
                param.symbol = "osc" #n "_level"; \
                param.ranges.def = (n == 1) ? 0.7f : 0.0f; \
                break; \
            case kParameterOsc##n##Detune: \
                param.name = "Osc" #n " Detune"; \
                param.symbol = "osc" #n "_detune"; \
                param.ranges.min = -1.0f; \
                param.ranges.max = 1.0f; \
                param.ranges.def = 0.0f; \
                break; \
            case kParameterOsc##n##Phase: \
                param.name = "Osc" #n " Phase"; \
                param.symbol = "osc" #n "_phase"; \
                param.ranges.def = 0.0f; \
                break; \
            case kParameterOsc##n##AmpAttack: \
                param.name = "Osc" #n " Amp Attack"; \
                param.symbol = "osc" #n "_amp_attack"; \
                param.ranges.def = 0.01f; \
                break; \
            case kParameterOsc##n##AmpDecay: \
                param.name = "Osc" #n " Amp Decay"; \
                param.symbol = "osc" #n "_amp_decay"; \
                param.ranges.def = 0.3f; \
                break; \
            case kParameterOsc##n##AmpSustain: \
                param.name = "Osc" #n " Amp Sustain"; \
                param.symbol = "osc" #n "_amp_sustain"; \
                param.ranges.def = 0.7f; \
                break; \
            case kParameterOsc##n##AmpRelease: \
                param.name = "Osc" #n " Amp Release"; \
                param.symbol = "osc" #n "_amp_release"; \
                param.ranges.def = 0.5f; \
                break; \
            case kParameterOsc##n##PitchAttack: \
                param.name = "Osc" #n " Pitch Attack"; \
                param.symbol = "osc" #n "_pitch_attack"; \
                param.ranges.def = 0.01f; \
                break; \
            case kParameterOsc##n##PitchDecay: \
                param.name = "Osc" #n " Pitch Decay"; \
                param.symbol = "osc" #n "_pitch_decay"; \
                param.ranges.def = 0.3f; \
                break; \
            case kParameterOsc##n##PitchDepth: \
                param.name = "Osc" #n " Pitch Depth"; \
                param.symbol = "osc" #n "_pitch_depth"; \
                param.ranges.min = -12.0f; \
                param.ranges.max = 12.0f; \
                param.ranges.def = 0.0f; \
                break

        switch (index) {
            OSC_PARAMS(1, kParameterOsc1Wave);
            OSC_PARAMS(2, kParameterOsc2Wave);
            OSC_PARAMS(3, kParameterOsc3Wave);
            OSC_PARAMS(4, kParameterOsc4Wave);

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

        case kParameterVelToAmp:
            param.name = "Velocity to Amplitude";
            param.symbol = "vel_to_amp";
            param.ranges.def = 0.5f;
            break;
        case kParameterVelToPitch:
            param.name = "Velocity to Pitch";
            param.symbol = "vel_to_pitch";
            param.ranges.def = 0.0f;
            break;
        case kParameterVelToAttack:
            param.name = "Velocity to Attack Time";
            param.symbol = "vel_to_attack";
            param.ranges.def = 0.0f;
            break;
        case kParameterVelToWave:
            param.name = "Velocity to Waveform";
            param.symbol = "vel_to_wave";
            param.ranges.def = 0.0f;
            break;

        case kParameterVolume:
            param.name = "Volume";
            param.symbol = "volume";
            param.ranges.def = 0.7f;
            break;
        }

        #undef OSC_PARAMS
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

            // Update envelopes when amp/pitch envelope params change
            if ((index >= kParameterOsc1AmpAttack && index <= kParameterOsc4PitchDepth)) {
                updateEnvelopes();
            }
        }
    }

    void initState(uint32_t index, State& state) override
    {
        if (index == 0) {
            state.key = "waveforms";
            state.defaultValue = "";
            state.label = "Waveform Data";
            state.hints = kStateIsOnlyForDSP;
        }
    }

    void setState(const char* key, const char* value) override
    {
        if (std::strcmp(key, "waveforms") == 0) {
            // Parse waveform data (format: "wave0_sample0,wave0_sample1,...;wave1_sample0,...")
            const char* ptr = value;
            for (int w = 0; w < NUM_WAVEFORMS && *ptr; w++) {
                for (int i = 0; i < WAVETABLE_SIZE && *ptr; i++) {
                    fWaveforms[w][i] = std::atof(ptr);
                    while (*ptr && *ptr != ',' && *ptr != ';') ptr++;
                    if (*ptr == ',') ptr++;
                }
                if (*ptr == ';') ptr++;
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
    void initDefaults()
    {
        for (int i = 0; i < kParameterCount; i++) {
            fParameters[i] = 0.5f;
        }

        // Oscillator 1 defaults (active by default)
        fParameters[kParameterOsc1Wave] = 0.0f;
        fParameters[kParameterOsc1Level] = 0.7f;
        fParameters[kParameterOsc1Detune] = 0.0f;
        fParameters[kParameterOsc1Phase] = 0.0f;
        fParameters[kParameterOsc1AmpAttack] = 0.01f;
        fParameters[kParameterOsc1AmpDecay] = 0.3f;
        fParameters[kParameterOsc1AmpSustain] = 0.7f;
        fParameters[kParameterOsc1AmpRelease] = 0.5f;
        fParameters[kParameterOsc1PitchAttack] = 0.01f;
        fParameters[kParameterOsc1PitchDecay] = 0.3f;
        fParameters[kParameterOsc1PitchDepth] = 0.0f;

        // Oscillators 2-4 defaults (inactive by default)
        for (int osc = 2; osc <= 4; osc++) {
            int base = (osc - 1) * 11;
            fParameters[base + 0] = 0.0f;  // Wave
            fParameters[base + 1] = 0.0f;  // Level (off)
            fParameters[base + 2] = 0.0f;  // Detune
            fParameters[base + 3] = 0.0f;  // Phase
            fParameters[base + 4] = 0.01f; // Amp Attack
            fParameters[base + 5] = 0.3f;  // Amp Decay
            fParameters[base + 6] = 0.7f;  // Amp Sustain
            fParameters[base + 7] = 0.5f;  // Amp Release
            fParameters[base + 8] = 0.01f; // Pitch Attack
            fParameters[base + 9] = 0.3f;  // Pitch Decay
            fParameters[base + 10] = 0.0f; // Pitch Depth
        }

        fParameters[kParameterFineTune] = 0.0f;
        fParameters[kParameterCoarseTune] = 0.0f;
        fParameters[kParameterVelToAmp] = 0.5f;
        fParameters[kParameterVelToPitch] = 0.0f;
        fParameters[kParameterVelToAttack] = 0.0f;
        fParameters[kParameterVelToWave] = 0.0f;
        fParameters[kParameterVolume] = 0.7f;
    }

    void updateEnvelopes()
    {
        for (int v = 0; v < MAX_VOICES; v++) {
            for (int o = 0; o < NUM_OSCILLATORS; o++) {
                int base = o * 11;

                // Amp envelope
                if (fVoices[v].oscillators[o].amp_env) {
                    synth_envelope_set_attack(fVoices[v].oscillators[o].amp_env,
                        0.001f + fParameters[base + 4] * 2.0f);
                    synth_envelope_set_decay(fVoices[v].oscillators[o].amp_env,
                        0.01f + fParameters[base + 5] * 3.0f);
                    synth_envelope_set_sustain(fVoices[v].oscillators[o].amp_env,
                        fParameters[base + 6]);
                    synth_envelope_set_release(fVoices[v].oscillators[o].amp_env,
                        0.01f + fParameters[base + 7] * 5.0f);
                }

                // Pitch envelope (AD only, sustain at 0, release at 0.01)
                if (fVoices[v].oscillators[o].pitch_env) {
                    synth_envelope_set_attack(fVoices[v].oscillators[o].pitch_env,
                        0.001f + fParameters[base + 8] * 2.0f);
                    synth_envelope_set_decay(fVoices[v].oscillators[o].pitch_env,
                        0.01f + fParameters[base + 9] * 3.0f);
                    synth_envelope_set_sustain(fVoices[v].oscillators[o].pitch_env, 0.0f);
                    synth_envelope_set_release(fVoices[v].oscillators[o].pitch_env, 0.01f);
                }
            }
        }
    }

    void handleNoteOn(uint8_t note, uint8_t velocity, int)
    {
        if (!fVoiceManager) return;

        int voice_idx = synth_voice_manager_allocate(fVoiceManager, note, velocity);
        if (voice_idx < 0 || voice_idx >= MAX_VOICES) return;

        SonixVoice* voice = &fVoices[voice_idx];
        voice->note = note;
        voice->velocity = velocity;
        voice->active = true;

        // Velocity modulation for attack time
        float vel_attack_mod = 1.0f - (fParameters[kParameterVelToAttack] * (velocity / 127.0f));
        if (vel_attack_mod < 0.1f) vel_attack_mod = 0.1f;

        for (int o = 0; o < NUM_OSCILLATORS; o++) {
            int base = o * 11;

            voice->oscillators[o].waveform_index = (int)fParameters[base + 0];
            voice->oscillators[o].level = fParameters[base + 1];
            voice->oscillators[o].detune = fParameters[base + 2];
            voice->oscillators[o].phase_offset = fParameters[base + 3];
            voice->oscillators[o].phase = voice->oscillators[o].phase_offset * WAVETABLE_SIZE;

            // Trigger amp envelope with velocity-modulated attack
            if (voice->oscillators[o].amp_env) {
                float base_attack = 0.001f + fParameters[base + 4] * 2.0f;
                synth_envelope_set_attack(voice->oscillators[o].amp_env, base_attack * vel_attack_mod);
                synth_envelope_trigger(voice->oscillators[o].amp_env);
            }

            // Trigger pitch envelope
            if (voice->oscillators[o].pitch_env) {
                synth_envelope_trigger(voice->oscillators[o].pitch_env);
            }
        }
    }

    void handleNoteOff(uint8_t note)
    {
        if (!fVoiceManager) return;

        int voice_idx = synth_voice_manager_release(fVoiceManager, note);
        if (voice_idx < 0 || voice_idx >= MAX_VOICES) return;

        SonixVoice* voice = &fVoices[voice_idx];
        for (int o = 0; o < NUM_OSCILLATORS; o++) {
            if (voice->oscillators[o].amp_env) {
                synth_envelope_release(voice->oscillators[o].amp_env);
            }
            if (voice->oscillators[o].pitch_env) {
                synth_envelope_release(voice->oscillators[o].pitch_env);
            }
        }
    }

    void renderFrame(float* outL, float* outR, uint32_t framePos, int sampleRate)
    {
        float mixL = 0.0f;
        float mixR = 0.0f;

        for (int v = 0; v < MAX_VOICES; v++) {
            const VoiceMeta* meta = synth_voice_manager_get_voice(fVoiceManager, v);
            if (!meta || meta->state == VOICE_INACTIVE) {
                fVoices[v].active = false;
                continue;
            }

            if (!fVoices[v].active) continue;

            SonixVoice* voice = &fVoices[v];
            float voice_sample = 0.0f;
            bool any_active = false;

            // Calculate base frequency with global pitch modulation
            float base_freq = 440.0f * powf(2.0f, (voice->note - 69) / 12.0f);
            base_freq *= powf(2.0f, fParameters[kParameterCoarseTune] / 12.0f);
            base_freq *= powf(2.0f, fParameters[kParameterFineTune] / 12.0f);

            // Velocity to pitch modulation
            float vel_pitch_mod = fParameters[kParameterVelToPitch] * (voice->velocity / 127.0f) * 2.0f;
            base_freq *= powf(2.0f, vel_pitch_mod / 12.0f);

            for (int o = 0; o < NUM_OSCILLATORS; o++) {
                if (voice->oscillators[o].level <= 0.0f) continue;

                float amp_env = synth_envelope_process(voice->oscillators[o].amp_env, sampleRate);
                if (amp_env > 0.0f) any_active = true;

                float pitch_env = synth_envelope_process(voice->oscillators[o].pitch_env, sampleRate);

                // Calculate oscillator frequency
                int base_param = o * 11;
                float osc_freq = base_freq;
                osc_freq *= powf(2.0f, voice->oscillators[o].detune / 12.0f);
                osc_freq *= powf(2.0f, (pitch_env * fParameters[base_param + 10]) / 12.0f);

                // Calculate phase increment
                float phase_inc = (osc_freq / sampleRate) * WAVETABLE_SIZE;

                // Waveform selection with velocity modulation
                int wave_idx = voice->oscillators[o].waveform_index;
                float vel_wave_mod = fParameters[kParameterVelToWave] * (voice->velocity / 127.0f);
                wave_idx = (int)(wave_idx + vel_wave_mod * (NUM_WAVEFORMS - 1));
                if (wave_idx < 0) wave_idx = 0;
                if (wave_idx >= NUM_WAVEFORMS) wave_idx = NUM_WAVEFORMS - 1;

                // Read from wavetable with linear interpolation
                float int_part;
                float frac = modff(voice->oscillators[o].phase, &int_part);
                int idx1 = (int)int_part % WAVETABLE_SIZE;
                int idx2 = (idx1 + 1) % WAVETABLE_SIZE;

                float sample = fWaveforms[wave_idx][idx1] * (1.0f - frac) +
                              fWaveforms[wave_idx][idx2] * frac;

                // Advance phase
                voice->oscillators[o].phase += phase_inc;
                while (voice->oscillators[o].phase >= WAVETABLE_SIZE) {
                    voice->oscillators[o].phase -= WAVETABLE_SIZE;
                }

                // Apply amplitude envelope and level
                sample *= amp_env * voice->oscillators[o].level;

                voice_sample += sample;
            }

            // Check if voice finished
            if (!any_active && meta->state == VOICE_RELEASING) {
                synth_voice_manager_stop_voice(fVoiceManager, v);
                voice->active = false;
                continue;
            }

            // Velocity sensitivity to amplitude
            float vel_scale = 1.0f - fParameters[kParameterVelToAmp] +
                             (fParameters[kParameterVelToAmp] * (voice->velocity / 127.0f));
            voice_sample *= vel_scale;

            mixL += voice_sample;
            mixR += voice_sample;
        }

        // Reduce per-voice level for polyphony
        mixL *= 0.3f;
        mixR *= 0.3f;

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

    // Voice management
    SynthVoiceManager* fVoiceManager;
    SonixVoice fVoices[MAX_VOICES];

    // Waveforms (8 user-defined waveforms)
    float fWaveforms[NUM_WAVEFORMS][WAVETABLE_SIZE];

    // Parameters
    float fParameters[kParameterCount];

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RGSonix_SynthPlugin)
};

Plugin* createPlugin()
{
    return new RGSonix_SynthPlugin();
}

END_NAMESPACE_DISTRHO
