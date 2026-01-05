#include "DistrhoPlugin.hpp"

extern "C" {
#include "../../synth/mod_player.h"
}

#include <cstring>
#include <cmath>
#include <cstdio>

START_NAMESPACE_DISTRHO

class RGModPlayerPlugin : public Plugin
{
public:
    RGModPlayerPlugin()
        : Plugin(kParameterCount, 0, 1)  // params, programs, states
        , fPlaying(0.0f)
        , fLoopStart(0.0f)
        , fLoopEnd(0.0f)
        , fSpeed(6.0f)
        , fBPM(125.0f)
    {
        // Create MOD player
        fModPlayer = mod_player_create();

        // Initialize channel parameters
        for (int i = 0; i < 4; i++) {
            fChannelMute[i] = 0.0f;
            fChannelVolume[i] = 1.0f;
            fChannelPan[i] = 0.0f;
        }

        // Set default Amiga panning
        fChannelPan[0] = -0.5f;
        fChannelPan[1] = 0.5f;
        fChannelPan[2] = 0.5f;
        fChannelPan[3] = -0.5f;

        updateChannelControls();
    }

    ~RGModPlayerPlugin() override
    {
        if (fModPlayer) {
            mod_player_destroy(fModPlayer);
        }
    }

protected:
    const char* getLabel() const override { return RGMODPLAYER_DISPLAY_NAME; }
    const char* getDescription() const override { return RGMODPLAYER_DESCRIPTION; }
    const char* getMaker() const override { return "Regroove"; }
    const char* getHomePage() const override { return "https://music.gbraad.nl/regrooved/"; }
    const char* getLicense() const override { return "GPL-3.0"; }
    uint32_t getVersion() const override { return d_version(1, 0, 0); }
    int64_t getUniqueId() const override { return d_cconst('R', 'G', 'M', 'D'); }

    void initParameter(uint32_t index, Parameter& param) override
    {
        param.hints = kParameterIsAutomatable;
        param.ranges.min = 0.0f;
        param.ranges.max = 1.0f;
        param.ranges.def = 0.0f;

        switch (index) {
        case kParameterPlay:
            param.name = "Play";
            param.symbol = "play";
            param.hints |= kParameterIsBoolean;
            param.ranges.def = 0.0f;
            break;

        case kParameterLoopStart:
            param.name = "Loop Start";
            param.symbol = "loop_start";
            param.ranges.max = 127.0f;
            param.ranges.def = 0.0f;
            break;

        case kParameterLoopEnd:
            param.name = "Loop End";
            param.symbol = "loop_end";
            param.ranges.max = 127.0f;
            param.ranges.def = 0.0f;
            break;

        case kParameterSpeed:
            param.name = "Speed";
            param.symbol = "speed";
            param.ranges.min = 1.0f;
            param.ranges.max = 31.0f;
            param.ranges.def = 6.0f;
            break;

        case kParameterBPM:
            param.name = "BPM";
            param.symbol = "bpm";
            param.ranges.min = 32.0f;
            param.ranges.max = 255.0f;
            param.ranges.def = 125.0f;
            break;

        // Channel 1
        case kParameterCh1Mute:
            param.name = "Ch1 Mute";
            param.symbol = "ch1_mute";
            param.hints |= kParameterIsBoolean;
            break;
        case kParameterCh1Volume:
            param.name = "Ch1 Volume";
            param.symbol = "ch1_volume";
            param.ranges.def = 1.0f;
            break;
        case kParameterCh1Pan:
            param.name = "Ch1 Pan";
            param.symbol = "ch1_pan";
            param.ranges.min = -1.0f;
            param.ranges.max = 1.0f;
            param.ranges.def = -0.5f;
            break;

        // Channel 2
        case kParameterCh2Mute:
            param.name = "Ch2 Mute";
            param.symbol = "ch2_mute";
            param.hints |= kParameterIsBoolean;
            break;
        case kParameterCh2Volume:
            param.name = "Ch2 Volume";
            param.symbol = "ch2_volume";
            param.ranges.def = 1.0f;
            break;
        case kParameterCh2Pan:
            param.name = "Ch2 Pan";
            param.symbol = "ch2_pan";
            param.ranges.min = -1.0f;
            param.ranges.max = 1.0f;
            param.ranges.def = 0.5f;
            break;

        // Channel 3
        case kParameterCh3Mute:
            param.name = "Ch3 Mute";
            param.symbol = "ch3_mute";
            param.hints |= kParameterIsBoolean;
            break;
        case kParameterCh3Volume:
            param.name = "Ch3 Volume";
            param.symbol = "ch3_volume";
            param.ranges.def = 1.0f;
            break;
        case kParameterCh3Pan:
            param.name = "Ch3 Pan";
            param.symbol = "ch3_pan";
            param.ranges.min = -1.0f;
            param.ranges.max = 1.0f;
            param.ranges.def = 0.5f;
            break;

        // Channel 4
        case kParameterCh4Mute:
            param.name = "Ch4 Mute";
            param.symbol = "ch4_mute";
            param.hints |= kParameterIsBoolean;
            break;
        case kParameterCh4Volume:
            param.name = "Ch4 Volume";
            param.symbol = "ch4_volume";
            param.ranges.def = 1.0f;
            break;
        case kParameterCh4Pan:
            param.name = "Ch4 Pan";
            param.symbol = "ch4_pan";
            param.ranges.min = -1.0f;
            param.ranges.max = 1.0f;
            param.ranges.def = -0.5f;
            break;
        }
    }

    float getParameterValue(uint32_t index) const override
    {
        switch (index) {
        case kParameterPlay: return fPlaying;
        case kParameterLoopStart: return fLoopStart;
        case kParameterLoopEnd: return fLoopEnd;
        case kParameterSpeed: return fSpeed;
        case kParameterBPM: return fBPM;

        case kParameterCh1Mute: return fChannelMute[0];
        case kParameterCh1Volume: return fChannelVolume[0];
        case kParameterCh1Pan: return fChannelPan[0];

        case kParameterCh2Mute: return fChannelMute[1];
        case kParameterCh2Volume: return fChannelVolume[1];
        case kParameterCh2Pan: return fChannelPan[1];

        case kParameterCh3Mute: return fChannelMute[2];
        case kParameterCh3Volume: return fChannelVolume[2];
        case kParameterCh3Pan: return fChannelPan[2];

        case kParameterCh4Mute: return fChannelMute[3];
        case kParameterCh4Volume: return fChannelVolume[3];
        case kParameterCh4Pan: return fChannelPan[3];
        }

        return 0.0f;
    }

    void setParameterValue(uint32_t index, float value) override
    {
        if (!fModPlayer) return;

        switch (index) {
        case kParameterPlay:
            fPlaying = value;
            if (value > 0.5f) {
                mod_player_start(fModPlayer);
            } else {
                mod_player_stop(fModPlayer);
            }
            break;

        case kParameterLoopStart:
            fLoopStart = value;
            mod_player_set_loop_range(fModPlayer, (uint8_t)fLoopStart, (uint8_t)fLoopEnd);
            break;

        case kParameterLoopEnd:
            fLoopEnd = value;
            mod_player_set_loop_range(fModPlayer, (uint8_t)fLoopStart, (uint8_t)fLoopEnd);
            break;

        case kParameterSpeed:
            fSpeed = value;
            mod_player_set_speed(fModPlayer, (uint8_t)value);
            break;

        case kParameterBPM:
            fBPM = value;
            mod_player_set_bpm(fModPlayer, (uint8_t)value);
            break;

        case kParameterCh1Mute:
            fChannelMute[0] = value;
            mod_player_set_channel_mute(fModPlayer, 0, value > 0.5f);
            break;
        case kParameterCh1Volume:
            fChannelVolume[0] = value;
            mod_player_set_channel_volume(fModPlayer, 0, value);
            break;
        case kParameterCh1Pan:
            fChannelPan[0] = value;
            mod_player_set_channel_panning(fModPlayer, 0, value);
            break;

        case kParameterCh2Mute:
            fChannelMute[1] = value;
            mod_player_set_channel_mute(fModPlayer, 1, value > 0.5f);
            break;
        case kParameterCh2Volume:
            fChannelVolume[1] = value;
            mod_player_set_channel_volume(fModPlayer, 1, value);
            break;
        case kParameterCh2Pan:
            fChannelPan[1] = value;
            mod_player_set_channel_panning(fModPlayer, 1, value);
            break;

        case kParameterCh3Mute:
            fChannelMute[2] = value;
            mod_player_set_channel_mute(fModPlayer, 2, value > 0.5f);
            break;
        case kParameterCh3Volume:
            fChannelVolume[2] = value;
            mod_player_set_channel_volume(fModPlayer, 2, value);
            break;
        case kParameterCh3Pan:
            fChannelPan[2] = value;
            mod_player_set_channel_panning(fModPlayer, 2, value);
            break;

        case kParameterCh4Mute:
            fChannelMute[3] = value;
            mod_player_set_channel_mute(fModPlayer, 3, value > 0.5f);
            break;
        case kParameterCh4Volume:
            fChannelVolume[3] = value;
            mod_player_set_channel_volume(fModPlayer, 3, value);
            break;
        case kParameterCh4Pan:
            fChannelPan[3] = value;
            mod_player_set_channel_panning(fModPlayer, 3, value);
            break;
        }
    }

    void initState(uint32_t index, State& state) override
    {
        if (index == 0) {
            state.key = "file";
            state.label = "MOD File";
            state.hints = kStateIsFilenamePath;
            state.defaultValue = "";
        }
    }

    String getState(const char* key) const override
    {
        if (std::strcmp(key, "file") == 0) {
            return fFilePath;
        }
        return String();
    }

    void setState(const char* key, const char* value) override
    {
        if (std::strcmp(key, "file") == 0 && value != nullptr) {
            loadModFile(value);
        }
    }

    void run(const float**, float** outputs, uint32_t frames,
             const MidiEvent* midiEvents, uint32_t midiEventCount) override
    {
        (void)midiEvents;
        (void)midiEventCount;

        float* outL = outputs[0];
        float* outR = outputs[1];

        if (fModPlayer) {
            mod_player_process(fModPlayer, outL, outR, frames, (uint32_t)getSampleRate());
        } else {
            std::memset(outL, 0, frames * sizeof(float));
            std::memset(outR, 0, frames * sizeof(float));
        }
    }

private:
    ModPlayer* fModPlayer;
    String fFilePath;

    float fPlaying;
    float fLoopStart;
    float fLoopEnd;
    float fSpeed;
    float fBPM;

    float fChannelMute[4];
    float fChannelVolume[4];
    float fChannelPan[4];

    void loadModFile(const char* filepath)
    {
        if (!filepath || !fModPlayer) return;

        FILE* f = std::fopen(filepath, "rb");
        if (!f) return;

        // Get file size
        std::fseek(f, 0, SEEK_END);
        long size = std::ftell(f);
        std::fseek(f, 0, SEEK_SET);

        if (size <= 0) {
            std::fclose(f);
            return;
        }

        // Read file
        uint8_t* data = new uint8_t[size];
        size_t read = std::fread(data, 1, size, f);
        std::fclose(f);

        if (read == (size_t)size) {
            // Load MOD
            if (mod_player_load(fModPlayer, data, (uint32_t)size)) {
                fFilePath = filepath;

                // Update loop end to song length
                uint8_t song_length = mod_player_get_song_length(fModPlayer);
                if (song_length > 0) {
                    fLoopEnd = song_length - 1;
                    mod_player_set_loop_range(fModPlayer, (uint8_t)fLoopStart, (uint8_t)fLoopEnd);
                }

                updateChannelControls();
            }
        }

        delete[] data;
    }

    void updateChannelControls()
    {
        if (!fModPlayer) return;

        for (int i = 0; i < 4; i++) {
            mod_player_set_channel_mute(fModPlayer, i, fChannelMute[i] > 0.5f);
            mod_player_set_channel_volume(fModPlayer, i, fChannelVolume[i]);
            mod_player_set_channel_panning(fModPlayer, i, fChannelPan[i]);
        }

        mod_player_set_speed(fModPlayer, (uint8_t)fSpeed);
        mod_player_set_bpm(fModPlayer, (uint8_t)fBPM);
    }
};

Plugin* createPlugin()
{
    return new RGModPlayerPlugin();
}

END_NAMESPACE_DISTRHO
