#include "DistrhoPlugin.hpp"
#include "../../synth/synth_oscillator.h"
#include "../../synth/synth_filter.h"
#include "../../synth/synth_envelope.h"
#include "../../synth/synth_noise.h"
#include <cstring>
#include <cmath>

START_NAMESPACE_DISTRHO

#define MAX_DRUM_VOICES 16  // Polyphonic drums

enum DrumType {
    DRUM_TYPE_KICK,
    DRUM_TYPE_SNARE,
    DRUM_TYPE_CLOSED_HAT,
    DRUM_TYPE_OPEN_HAT,
    DRUM_TYPE_CLAP,
    DRUM_TYPE_TOM_LOW,
    DRUM_TYPE_TOM_MID,
    DRUM_TYPE_TOM_HIGH,
    DRUM_TYPE_COWBELL,
    DRUM_TYPE_RIMSHOT
};

struct DrumVoice {
    DrumType type;
    bool active;

    // Synthesis components
    SynthOscillator* osc1;      // Primary oscillator
    SynthOscillator* osc2;      // Secondary oscillator (for snare body)
    SynthNoise* noise;          // Noise generator
    SynthFilter* filter;        // Filter for shaping
    SynthEnvelope* amp_env;     // Amplitude envelope

    // Pitch envelope for kick/tom
    float pitch_start;
    float pitch_end;
    float pitch_time;
    float pitch_elapsed;

    // Sample counter for envelopes
    int sample_count;
    int total_samples;

    // Velocity
    float velocity;
};

class RG808_DrumPlugin : public Plugin
{
public:
    RG808_DrumPlugin()
        : Plugin(kParameterCount, 0, kParameterCount)  // parameters, programs, states
    {
        // Initialize parameters with defaults (balanced for mix levels)
        fKickLevel = 0.8f;
        fKickTune = 0.5f;
        fKickDecay = 0.5f;

        fSnareLevel = 0.7f;
        fSnareTune = 0.5f;
        fSnareSnappy = 0.5f;

        fHiHatLevel = 0.6f;
        fHiHatDecay = 0.3f;

        fClapLevel = 0.7f;

        fTomLevel = 0.7f;
        fTomTune = 0.5f;

        fMasterLevel = 0.7f;

        // Initialize voice pool
        for (int i = 0; i < MAX_DRUM_VOICES; i++) {
            fVoices[i].active = false;
            fVoices[i].osc1 = synth_oscillator_create();
            fVoices[i].osc2 = synth_oscillator_create();
            fVoices[i].noise = synth_noise_create();
            fVoices[i].filter = synth_filter_create();
            fVoices[i].amp_env = synth_envelope_create();
        }
    }

    ~RG808_DrumPlugin() override
    {
        for (int i = 0; i < MAX_DRUM_VOICES; i++) {
            if (fVoices[i].osc1) synth_oscillator_destroy(fVoices[i].osc1);
            if (fVoices[i].osc2) synth_oscillator_destroy(fVoices[i].osc2);
            if (fVoices[i].noise) synth_noise_destroy(fVoices[i].noise);
            if (fVoices[i].filter) synth_filter_destroy(fVoices[i].filter);
            if (fVoices[i].amp_env) synth_envelope_destroy(fVoices[i].amp_env);
        }
    }

protected:
    const char* getLabel() const override { return "RG808_Drum"; }
    const char* getDescription() const override { return "RG808 drum machine"; }
    const char* getMaker() const override { return "Regroove"; }
    const char* getHomePage() const override { return "https://github.com/gbraad/rfx"; }
    const char* getLicense() const override { return "ISC"; }
    uint32_t getVersion() const override { return d_version(1, 0, 0); }
    int64_t getUniqueId() const override { return d_cconst('R', 'G', '8', '8'); }

    void initParameter(uint32_t index, Parameter& param) override
    {
        param.hints = kParameterIsAutomatable;
        param.ranges.min = 0.0f;
        param.ranges.max = 1.0f;
        param.ranges.def = 0.5f;

        switch (index) {
        case kParameterKickLevel:
            param.name = "Kick Level";
            param.symbol = "kick_level";
            param.ranges.def = 0.8f;
            break;
        case kParameterKickTune:
            param.name = "Kick Tune";
            param.symbol = "kick_tune";
            break;
        case kParameterKickDecay:
            param.name = "Kick Decay";
            param.symbol = "kick_decay";
            break;
        case kParameterSnareLevel:
            param.name = "Snare Level";
            param.symbol = "snare_level";
            param.ranges.def = 0.7f;
            break;
        case kParameterSnareTune:
            param.name = "Snare Tune";
            param.symbol = "snare_tune";
            break;
        case kParameterSnareSnappy:
            param.name = "Snare Snappy";
            param.symbol = "snare_snappy";
            break;
        case kParameterHiHatLevel:
            param.name = "Hi-Hat Level";
            param.symbol = "hihat_level";
            param.ranges.def = 0.6f;
            break;
        case kParameterHiHatDecay:
            param.name = "Hi-Hat Decay";
            param.symbol = "hihat_decay";
            param.ranges.def = 0.3f;
            break;
        case kParameterClapLevel:
            param.name = "Clap Level";
            param.symbol = "clap_level";
            param.ranges.def = 0.7f;
            break;
        case kParameterTomLevel:
            param.name = "Tom Level";
            param.symbol = "tom_level";
            param.ranges.def = 0.7f;
            break;
        case kParameterTomTune:
            param.name = "Tom Tune";
            param.symbol = "tom_tune";
            break;
        case kParameterMasterLevel:
            param.name = "Master Level";
            param.symbol = "master_level";
            param.ranges.def = 0.7f;
            break;
        }
    }

    float getParameterValue(uint32_t index) const override
    {
        switch (index) {
        case kParameterKickLevel: return fKickLevel;
        case kParameterKickTune: return fKickTune;
        case kParameterKickDecay: return fKickDecay;
        case kParameterSnareLevel: return fSnareLevel;
        case kParameterSnareTune: return fSnareTune;
        case kParameterSnareSnappy: return fSnareSnappy;
        case kParameterHiHatLevel: return fHiHatLevel;
        case kParameterHiHatDecay: return fHiHatDecay;
        case kParameterClapLevel: return fClapLevel;
        case kParameterTomLevel: return fTomLevel;
        case kParameterTomTune: return fTomTune;
        case kParameterMasterLevel: return fMasterLevel;
        default: return 0.0f;
        }
    }

    void setParameterValue(uint32_t index, float value) override
    {
        switch (index) {
        case kParameterKickLevel: fKickLevel = value; break;
        case kParameterKickTune: fKickTune = value; break;
        case kParameterKickDecay: fKickDecay = value; break;
        case kParameterSnareLevel: fSnareLevel = value; break;
        case kParameterSnareTune: fSnareTune = value; break;
        case kParameterSnareSnappy: fSnareSnappy = value; break;
        case kParameterHiHatLevel: fHiHatLevel = value; break;
        case kParameterHiHatDecay: fHiHatDecay = value; break;
        case kParameterClapLevel: fClapLevel = value; break;
        case kParameterTomLevel: fTomLevel = value; break;
        case kParameterTomTune: fTomTune = value; break;
        case kParameterMasterLevel: fMasterLevel = value; break;
        }
    }

    void initState(uint32_t index, State& state) override
    {
        setParameterValue(index, getParameterValue(index));
        char buf[32];
        std::snprintf(buf, sizeof(buf), "%.6f", getParameterValue(index));
        state.defaultValue = buf;

        // Use parameter symbol as state key
        Parameter param;
        initParameter(index, param);
        state.key = param.symbol;
        state.hints = kStateIsOnlyForDSP;
    }

    void setState(const char* key, const char* value) override
    {
        float fValue = std::atof(value);

        // Direct symbol matching
        if (std::strcmp(key, "kick_level") == 0) fKickLevel = fValue;
        else if (std::strcmp(key, "kick_tune") == 0) fKickTune = fValue;
        else if (std::strcmp(key, "kick_decay") == 0) fKickDecay = fValue;
        else if (std::strcmp(key, "snare_level") == 0) fSnareLevel = fValue;
        else if (std::strcmp(key, "snare_tune") == 0) fSnareTune = fValue;
        else if (std::strcmp(key, "snare_snappy") == 0) fSnareSnappy = fValue;
        else if (std::strcmp(key, "hihat_level") == 0) fHiHatLevel = fValue;
        else if (std::strcmp(key, "hihat_decay") == 0) fHiHatDecay = fValue;
        else if (std::strcmp(key, "clap_level") == 0) fClapLevel = fValue;
        else if (std::strcmp(key, "tom_level") == 0) fTomLevel = fValue;
        else if (std::strcmp(key, "tom_tune") == 0) fTomTune = fValue;
        else if (std::strcmp(key, "master_level") == 0) fMasterLevel = fValue;
    }

    String getState(const char* key) const override
    {
        char buf[32];

        // Direct symbol matching without calling initParameter
        if (std::strcmp(key, "kick_level") == 0) {
            std::snprintf(buf, sizeof(buf), "%.6f", fKickLevel);
        }
        else if (std::strcmp(key, "kick_tune") == 0) {
            std::snprintf(buf, sizeof(buf), "%.6f", fKickTune);
        }
        else if (std::strcmp(key, "kick_decay") == 0) {
            std::snprintf(buf, sizeof(buf), "%.6f", fKickDecay);
        }
        else if (std::strcmp(key, "snare_level") == 0) {
            std::snprintf(buf, sizeof(buf), "%.6f", fSnareLevel);
        }
        else if (std::strcmp(key, "snare_tune") == 0) {
            std::snprintf(buf, sizeof(buf), "%.6f", fSnareTune);
        }
        else if (std::strcmp(key, "snare_snappy") == 0) {
            std::snprintf(buf, sizeof(buf), "%.6f", fSnareSnappy);
        }
        else if (std::strcmp(key, "hihat_level") == 0) {
            std::snprintf(buf, sizeof(buf), "%.6f", fHiHatLevel);
        }
        else if (std::strcmp(key, "hihat_decay") == 0) {
            std::snprintf(buf, sizeof(buf), "%.6f", fHiHatDecay);
        }
        else if (std::strcmp(key, "clap_level") == 0) {
            std::snprintf(buf, sizeof(buf), "%.6f", fClapLevel);
        }
        else if (std::strcmp(key, "tom_level") == 0) {
            std::snprintf(buf, sizeof(buf), "%.6f", fTomLevel);
        }
        else if (std::strcmp(key, "tom_tune") == 0) {
            std::snprintf(buf, sizeof(buf), "%.6f", fTomTune);
        }
        else if (std::strcmp(key, "master_level") == 0) {
            std::snprintf(buf, sizeof(buf), "%.6f", fMasterLevel);
        }
        else {
            std::snprintf(buf, sizeof(buf), "0.0");
        }

        return String(buf);
    }

    void activate() override
    {
        // Reset all voices
        for (int i = 0; i < MAX_DRUM_VOICES; i++) {
            fVoices[i].active = false;
            if (fVoices[i].osc1) synth_oscillator_reset(fVoices[i].osc1);
            if (fVoices[i].osc2) synth_oscillator_reset(fVoices[i].osc2);
            if (fVoices[i].noise) synth_noise_reset(fVoices[i].noise);
            if (fVoices[i].filter) synth_filter_reset(fVoices[i].filter);
            if (fVoices[i].amp_env) synth_envelope_reset(fVoices[i].amp_env);
        }
    }

    void run(const float**, float** outputs, uint32_t frames,
             const MidiEvent* midiEvents, uint32_t midiEventCount) override
    {
        float* outL = outputs[0];
        float* outR = outputs[1];

        // Clear output buffers
        std::memset(outL, 0, sizeof(float) * frames);
        std::memset(outR, 0, sizeof(float) * frames);

        int sampleRate = (int)getSampleRate();
        if (sampleRate <= 0) sampleRate = 44100;

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
                        triggerDrum(note, velocity, sampleRate);
                    }
                }
            }

            // Render active drum voices
            float mixL = 0.0f;
            float mixR = 0.0f;

            for (int i = 0; i < MAX_DRUM_VOICES; i++) {
                if (!fVoices[i].active) continue;

                float sample = processDrumVoice(&fVoices[i], sampleRate);

                mixL += sample;
                mixR += sample;
            }

            // Apply master level
            mixL *= fMasterLevel;
            mixR *= fMasterLevel;

            // Simple clamping to prevent hard clipping
            if (mixL > 1.0f) mixL = 1.0f;
            if (mixL < -1.0f) mixL = -1.0f;
            if (mixR > 1.0f) mixR = 1.0f;
            if (mixR < -1.0f) mixR = -1.0f;

            outL[framePos] = mixL;
            outR[framePos] = mixR;

            framePos++;
        }
    }

private:
    void triggerDrum(uint8_t note, uint8_t velocity, int sampleRate)
    {
        // Find free voice
        DrumVoice* voice = nullptr;
        for (int i = 0; i < MAX_DRUM_VOICES; i++) {
            if (!fVoices[i].active) {
                voice = &fVoices[i];
                break;
            }
        }

        if (!voice) return; // No free voices

        // Determine drum type from MIDI note
        DrumType type;
        switch (note) {
        case kDrumKick: type = DRUM_TYPE_KICK; break;
        case kDrumSnare: type = DRUM_TYPE_SNARE; break;
        case kDrumClosedHat: type = DRUM_TYPE_CLOSED_HAT; break;
        case kDrumOpenHat: type = DRUM_TYPE_OPEN_HAT; break;
        case kDrumClap: type = DRUM_TYPE_CLAP; break;
        case kDrumLowTom: type = DRUM_TYPE_TOM_LOW; break;
        case kDrumMidTom: type = DRUM_TYPE_TOM_MID; break;
        case kDrumHighTom: type = DRUM_TYPE_TOM_HIGH; break;
        case kDrumCowbell: type = DRUM_TYPE_COWBELL; break;
        case kDrumRimshot: type = DRUM_TYPE_RIMSHOT; break;
        default: return; // Unknown drum note
        }

        voice->type = type;
        voice->active = true;
        voice->velocity = velocity / 127.0f;
        voice->sample_count = 0;

        // Reset oscillators to prevent phase discontinuity clicks
        if (voice->osc1) synth_oscillator_reset(voice->osc1);
        if (voice->osc2) synth_oscillator_reset(voice->osc2);
        if (voice->noise) synth_noise_reset(voice->noise);
        if (voice->filter) synth_filter_reset(voice->filter);

        // Setup drum voice based on type
        setupDrumVoice(voice, type, sampleRate);
    }

    void setupDrumVoice(DrumVoice* voice, DrumType type, int sampleRate)
    {
        switch (type) {
        case DRUM_TYPE_KICK:
            // 808 Kick: Pitch-swept sine wave
            synth_oscillator_set_waveform(voice->osc1, SYNTH_OSC_SINE);
            voice->pitch_start = 150.0f + (fKickTune * 100.0f); // 150-250 Hz start
            voice->pitch_end = 40.0f + (fKickTune * 20.0f);     // 40-60 Hz end
            voice->pitch_time = 0.05f;
            voice->pitch_elapsed = 0.0f;
            // Extended decay range: 300ms to 2000ms (authentic 808 range)
            voice->total_samples = (int)(sampleRate * (0.3f + fKickDecay * 1.7f));
            synth_oscillator_set_frequency(voice->osc1, voice->pitch_start);
            break;

        case DRUM_TYPE_SNARE:
            // 808 Snare: Two oscillators (body) + noise (snare)
            synth_oscillator_set_waveform(voice->osc1, SYNTH_OSC_SINE);
            synth_oscillator_set_waveform(voice->osc2, SYNTH_OSC_TRIANGLE);
            synth_oscillator_set_frequency(voice->osc1, 180.0f + (fSnareTune * 100.0f));
            synth_oscillator_set_frequency(voice->osc2, 330.0f + (fSnareTune * 150.0f));
            voice->total_samples = (int)(sampleRate * 0.15f); // 150ms

            // Filter for noise (bandpass-ish)
            synth_filter_set_type(voice->filter, SYNTH_FILTER_BPF);
            synth_filter_set_cutoff(voice->filter, 0.7f);
            synth_filter_set_resonance(voice->filter, 0.3f);
            break;

        case DRUM_TYPE_CLOSED_HAT:
            // Closed hi-hat: Filtered noise, short decay
            voice->total_samples = (int)(sampleRate * (0.05f + fHiHatDecay * 0.15f)); // 50-200ms
            synth_filter_set_type(voice->filter, SYNTH_FILTER_HPF);
            synth_filter_set_cutoff(voice->filter, 0.85f);
            synth_filter_set_resonance(voice->filter, 0.2f);
            break;

        case DRUM_TYPE_OPEN_HAT:
            // Open hi-hat: Filtered noise, longer decay
            voice->total_samples = (int)(sampleRate * (0.3f + fHiHatDecay * 0.7f)); // 300ms-1s
            synth_filter_set_type(voice->filter, SYNTH_FILTER_HPF);
            synth_filter_set_cutoff(voice->filter, 0.8f);
            synth_filter_set_resonance(voice->filter, 0.3f);
            break;

        case DRUM_TYPE_CLAP:
            // Clap: Multiple short noise bursts
            voice->total_samples = (int)(sampleRate * 0.08f); // 80ms
            synth_filter_set_type(voice->filter, SYNTH_FILTER_BPF);
            synth_filter_set_cutoff(voice->filter, 0.5f);
            synth_filter_set_resonance(voice->filter, 0.2f);
            break;

        case DRUM_TYPE_TOM_LOW:
        case DRUM_TYPE_TOM_MID:
        case DRUM_TYPE_TOM_HIGH: {
            // Toms: Pitch-swept sine/triangle
            synth_oscillator_set_waveform(voice->osc1, SYNTH_OSC_SINE);
            float baseFreq = (type == DRUM_TYPE_TOM_LOW) ? 80.0f :
                           (type == DRUM_TYPE_TOM_MID) ? 120.0f : 180.0f;
            voice->pitch_start = baseFreq + (fTomTune * 80.0f);
            voice->pitch_end = voice->pitch_start * 0.5f;
            voice->pitch_time = 0.03f;
            voice->pitch_elapsed = 0.0f;
            voice->total_samples = (int)(sampleRate * 0.25f);
            synth_oscillator_set_frequency(voice->osc1, voice->pitch_start);
            break;
        }

        case DRUM_TYPE_COWBELL:
            // Cowbell: Two square waves
            synth_oscillator_set_waveform(voice->osc1, SYNTH_OSC_SQUARE);
            synth_oscillator_set_waveform(voice->osc2, SYNTH_OSC_SQUARE);
            synth_oscillator_set_frequency(voice->osc1, 540.0f);
            synth_oscillator_set_frequency(voice->osc2, 800.0f);
            voice->total_samples = (int)(sampleRate * 0.2f);
            break;

        case DRUM_TYPE_RIMSHOT:
            // Rimshot: Short oscillator burst
            synth_oscillator_set_waveform(voice->osc1, SYNTH_OSC_TRIANGLE);
            synth_oscillator_set_frequency(voice->osc1, 1800.0f);
            voice->total_samples = (int)(sampleRate * 0.02f); // 20ms
            break;
        }
    }

    float processDrumVoice(DrumVoice* voice, int sampleRate)
    {
        float sample = 0.0f;
        float envelope = 0.0f;

        // Calculate envelope
        if (voice->sample_count < voice->total_samples) {
            float t = (float)voice->sample_count / voice->total_samples;
            envelope = (1.0f - t) * voice->velocity; // Linear decay
            voice->sample_count++;
        } else {
            voice->active = false;
            return 0.0f;
        }

        // Generate sample based on drum type
        switch (voice->type) {
        case DRUM_TYPE_KICK: {
            // Update pitch envelope
            if (voice->pitch_elapsed < voice->pitch_time) {
                float t = voice->pitch_elapsed / voice->pitch_time;
                float freq = voice->pitch_start + (voice->pitch_end - voice->pitch_start) * t;
                synth_oscillator_set_frequency(voice->osc1, freq);
                voice->pitch_elapsed += 1.0f / sampleRate;
            }
            // Reduce kick level to prevent clipping (50% max output)
            sample = synth_oscillator_process(voice->osc1, sampleRate) * envelope * fKickLevel * 0.5f;
            break;
        }

        case DRUM_TYPE_SNARE: {
            // Mix oscillators and noise
            float osc = (synth_oscillator_process(voice->osc1, sampleRate) * 0.5f +
                        synth_oscillator_process(voice->osc2, sampleRate) * 0.3f);
            float noise = synth_noise_process(voice->noise);
            noise = synth_filter_process(voice->filter, noise, sampleRate);

            // Blend body and snare based on "snappy" parameter
            sample = (osc * (1.0f - fSnareSnappy) + noise * fSnareSnappy) * envelope * fSnareLevel;
            break;
        }

        case DRUM_TYPE_CLOSED_HAT:
        case DRUM_TYPE_OPEN_HAT: {
            float noise = synth_noise_process(voice->noise);
            sample = synth_filter_process(voice->filter, noise, sampleRate) * envelope * fHiHatLevel;
            break;
        }

        case DRUM_TYPE_CLAP: {
            float noise = synth_noise_process(voice->noise);
            sample = synth_filter_process(voice->filter, noise, sampleRate) * envelope * fClapLevel;
            break;
        }

        case DRUM_TYPE_TOM_LOW:
        case DRUM_TYPE_TOM_MID:
        case DRUM_TYPE_TOM_HIGH: {
            // Update pitch envelope
            if (voice->pitch_elapsed < voice->pitch_time) {
                float t = voice->pitch_elapsed / voice->pitch_time;
                float freq = voice->pitch_start + (voice->pitch_end - voice->pitch_start) * t;
                synth_oscillator_set_frequency(voice->osc1, freq);
                voice->pitch_elapsed += 1.0f / sampleRate;
            }
            sample = synth_oscillator_process(voice->osc1, sampleRate) * envelope * fTomLevel;
            break;
        }

        case DRUM_TYPE_COWBELL: {
            sample = (synth_oscillator_process(voice->osc1, sampleRate) * 0.6f +
                     synth_oscillator_process(voice->osc2, sampleRate) * 0.4f) * envelope * fMasterLevel;
            break;
        }

        case DRUM_TYPE_RIMSHOT: {
            sample = synth_oscillator_process(voice->osc1, sampleRate) * envelope * fMasterLevel;
            break;
        }
        }

        return sample;
    }

    DrumVoice fVoices[MAX_DRUM_VOICES];

    // Parameters
    float fKickLevel;
    float fKickTune;
    float fKickDecay;

    float fSnareLevel;
    float fSnareTune;
    float fSnareSnappy;

    float fHiHatLevel;
    float fHiHatDecay;

    float fClapLevel;

    float fTomLevel;
    float fTomTune;

    float fMasterLevel;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RG808_DrumPlugin)
};

Plugin* createPlugin()
{
    return new RG808_DrumPlugin();
}

END_NAMESPACE_DISTRHO
