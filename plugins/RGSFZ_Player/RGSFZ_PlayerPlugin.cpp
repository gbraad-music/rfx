#include "DistrhoPlugin.hpp"

extern "C" {
#include "../../synth/sfz_parser.h"
#include "../../synth/synth_sample_player.h"
#include "../../synth/synth_midi.h"
}

#include <cstring>

START_NAMESPACE_DISTRHO

#define MAX_VOICES 16

struct SFZVoice {
    SynthSamplePlayer* player;
    const SFZRegion* region;
    SampleData sample_data;  // Temporary sample data struct
};

class RGSFZ_PlayerPlugin : public Plugin
{
public:
    RGSFZ_PlayerPlugin()
        : Plugin(kParameterCount, 0, 1)  // 1 state (SFZ file path)
        , fVolume(0.8f)
        , fPan(0.0f)
        , fAttack(0.001f)
        , fDecay(0.5f)
        , fSFZ(nullptr)
        , fMidi(nullptr)
    {
        // Create MIDI handler with polyphonic voice allocation
        fMidi = synth_midi_create(MAX_VOICES, VOICE_ALLOC_POLYPHONIC);

        // Create voices
        for (int i = 0; i < MAX_VOICES; i++) {
            fVoices[i].player = synth_sample_player_create();
            fVoices[i].region = nullptr;
        }
    }

    ~RGSFZ_PlayerPlugin() override
    {
        // Destroy MIDI handler
        if (fMidi) {
            synth_midi_destroy(fMidi);
        }

        // Destroy voices
        for (int i = 0; i < MAX_VOICES; i++) {
            if (fVoices[i].player) {
                synth_sample_player_destroy(fVoices[i].player);
            }
        }

        if (fSFZ) {
            sfz_free(fSFZ);
        }
    }

protected:
    const char* getLabel() const override { return RGSFZ_DISPLAY_NAME; }
    const char* getDescription() const override { return RGSFZ_DESCRIPTION; }
    const char* getMaker() const override { return "Regroove"; }
    const char* getHomePage() const override { return "https://music.gbraad.nl/regrooved/"; }
    const char* getLicense() const override { return "GPL-3.0"; }
    uint32_t getVersion() const override { return d_version(1, 0, 0); }
    int64_t getUniqueId() const override { return d_cconst('R', 'G', 'S', 'F'); }

    void initParameter(uint32_t index, Parameter& param) override
    {
        param.hints = kParameterIsAutomatable;
        param.ranges.min = 0.0f;
        param.ranges.max = 1.0f;

        switch (index) {
        case kParameterVolume:
            param.name = "Volume";
            param.symbol = "volume";
            param.ranges.def = 0.8f;
            break;
        case kParameterPan:
            param.name = "Pan";
            param.symbol = "pan";
            param.ranges.min = -1.0f;
            param.ranges.max = 1.0f;
            param.ranges.def = 0.0f;
            break;
        case kParameterAttack:
            param.name = "Attack";
            param.symbol = "attack";
            param.ranges.def = 0.001f;
            break;
        case kParameterDecay:
            param.name = "Decay";
            param.symbol = "decay";
            param.ranges.def = 0.5f;
            break;
        }
    }

    float getParameterValue(uint32_t index) const override
    {
        switch (index) {
        case kParameterVolume: return fVolume;
        case kParameterPan: return fPan;
        case kParameterAttack: return fAttack;
        case kParameterDecay: return fDecay;
        default: return 0.0f;
        }
    }

    void setParameterValue(uint32_t index, float value) override
    {
        switch (index) {
        case kParameterVolume: fVolume = value; break;
        case kParameterPan: fPan = value; break;
        case kParameterAttack: fAttack = value; break;
        case kParameterDecay:
            fDecay = value;
            // Update all voice decay times
            for (int i = 0; i < MAX_VOICES; i++) {
                float decay_time = 0.5f + fDecay * 7.5f;  // 0.5s to 8s
                synth_sample_player_set_loop_decay(fVoices[i].player, decay_time);
            }
            break;
        }
    }

    void initState(uint32_t index, State& state) override
    {
        if (index == 0) {
            state.key = "sfz_path";
            state.defaultValue = "";
            state.label = "SFZ File Path";
            state.hints = kStateIsFilenamePath;
        }
    }

    void setState(const char* key, const char* value) override
    {
        if (strcmp(key, "sfz_path") == 0) {
            loadSFZ(value);
        }
    }

    void run(const float**, float** outputs, uint32_t frames,
             const MidiEvent* midiEvents, uint32_t midiEventCount) override
    {
        float* outL = outputs[0];
        float* outR = outputs[1];
        uint32_t framePos = 0;
        const int sampleRate = (int)getSampleRate();

        if (!fMidi) return;

        // Process MIDI events
        for (uint32_t i = 0; i < midiEventCount; ++i) {
            const MidiEvent& event = midiEvents[i];

            // Render frames up to this event
            while (framePos < event.frame) {
                renderFrame(outL, outR, framePos, sampleRate);
                framePos++;
            }

            // Parse MIDI message using tracker_midi
            SynthMidiMessage msg;
            if (!synth_midi_parse(event.data, event.size, &msg)) {
                continue;
            }

            switch (msg.type) {
                case MIDI_NOTE_ON:
                    if (msg.velocity > 0) {
                        handleNoteOn(msg.channel, msg.note, msg.velocity);
                    } else {
                        // Velocity 0 = note off
                        handleNoteOff(msg.channel, msg.note);
                    }
                    break;

                case MIDI_NOTE_OFF:
                    handleNoteOff(msg.channel, msg.note);
                    break;

                case MIDI_CC:
                    // Handle All Notes Off
                    if (msg.cc_number == MIDI_CC_ALL_NOTES_OFF ||
                        msg.cc_number == MIDI_CC_ALL_SOUND_OFF) {
                        synth_midi_all_notes_off(fMidi);
                        for (int j = 0; j < MAX_VOICES; j++) {
                            if (fVoices[j].player) {
                                synth_sample_player_release(fVoices[j].player);
                            }
                        }
                    }
                    break;

                default:
                    break;
            }
        }

        // Render remaining frames
        while (framePos < frames) {
            renderFrame(outL, outR, framePos, sampleRate);
            framePos++;
        }
    }

private:
    void loadSFZ(const char* filepath)
    {
        if (fSFZ) {
            sfz_free(fSFZ);
            fSFZ = nullptr;
        }

        fSFZ = sfz_parse(filepath);
        if (fSFZ) {
            sfz_load_samples(fSFZ);
        }
    }

    void handleNoteOn(uint8_t channel, uint8_t note, uint8_t velocity)
    {
        if (!fSFZ || !fMidi) return;

        // Find matching region
        const SFZRegion* region = sfz_find_region(fSFZ, note, velocity);
        if (!region || !region->sample_data) return;

        // Allocate voice using tracker_midi
        int voice_idx = synth_midi_allocate_voice(fMidi, channel, note, velocity);
        if (voice_idx < 0 || voice_idx >= MAX_VOICES) return;

        SFZVoice* v = &fVoices[voice_idx];

        // Setup sample data for synth_sample_player
        // For SFZ, we treat the full sample as attack (one-shot)
        // and use the region's offset/end for slicing
        v->sample_data.attack_data = region->sample_data;
        v->sample_data.attack_length = region->end > 0 ? region->end - region->offset : region->sample_length - region->offset;
        v->sample_data.loop_data = nullptr;  // No loop for now
        v->sample_data.loop_length = 0;
        v->sample_data.sample_rate = region->sample_rate;
        v->sample_data.root_note = region->pitch_keycenter;

        // Load sample into player
        synth_sample_player_load_sample(v->player, &v->sample_data);

        // Set decay time
        float decay_time = 0.5f + fDecay * 7.5f;
        synth_sample_player_set_loop_decay(v->player, decay_time);

        // Trigger playback
        synth_sample_player_trigger(v->player, note, velocity);

        v->region = region;
    }

    void handleNoteOff(uint8_t channel, uint8_t note)
    {
        if (!fMidi) return;

        // Find voices playing this note
        int released_voices[MAX_VOICES];
        int count = synth_midi_find_voices_for_note(fMidi, channel, note, released_voices);

        // Release found voices
        for (int i = 0; i < count; i++) {
            int voice_idx = released_voices[i];
            if (voice_idx >= 0 && voice_idx < MAX_VOICES) {
                synth_sample_player_release(fVoices[voice_idx].player);
                synth_midi_release_voice(fMidi, voice_idx);
            }
        }
    }

    void renderFrame(float* outL, float* outR, uint32_t framePos, int sampleRate)
    {
        float mixL = 0.0f, mixR = 0.0f;

        for (int i = 0; i < MAX_VOICES; i++) {
            // Check if voice is active via MIDI handler
            if (!fMidi || !fMidi->voices[i].active) {
                continue;
            }

            SFZVoice* v = &fVoices[i];
            float sample = synth_sample_player_process(v->player, sampleRate);

            // Release voice if sample player is no longer active
            if (!synth_sample_player_is_active(v->player)) {
                synth_midi_release_voice(fMidi, i);
                continue;
            }

            // Apply region panning + master pan
            float total_pan = fPan + (v->region ? v->region->pan / 100.0f : 0.0f);
            if (total_pan < -1.0f) total_pan = -1.0f;
            if (total_pan > 1.0f) total_pan = 1.0f;

            // Constant power panning
            float pan_angle = (total_pan + 1.0f) * 0.25f * 3.14159265f;
            float pan_left = cosf(pan_angle);
            float pan_right = sinf(pan_angle);

            mixL += sample * pan_left;
            mixR += sample * pan_right;
        }

        // Apply master volume
        mixL *= fVolume * 0.3f;  // Per-voice reduction
        mixR *= fVolume * 0.3f;

        outL[framePos] = mixL;
        outR[framePos] = mixR;
    }

    SFZVoice fVoices[MAX_VOICES];
    SFZData* fSFZ;
    SynthMidiHandler* fMidi;
    float fVolume, fPan, fAttack, fDecay;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RGSFZ_PlayerPlugin)
};

Plugin* createPlugin() { return new RGSFZ_PlayerPlugin(); }

END_NAMESPACE_DISTRHO
