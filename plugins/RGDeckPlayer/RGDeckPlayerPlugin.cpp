#include "DistrhoPlugin.hpp"

extern "C" {
#include "../../players/deck_player.h"
}

#include <cstring>
#include <cmath>
#include <cstdio>

START_NAMESPACE_DISTRHO

class RGDeckPlayerPlugin : public Plugin
{
public:
    RGDeckPlayerPlugin()
        : Plugin(kParameterCount, 0, 1)  // params, programs, states
        , fPlaying(0.0f)
        , fLoopPattern(0.0f)
        , fPrevPattern(0.0f)
        , fNextPattern(0.0f)
        , fLoopStart(0.0f)
        , fLoopEnd(0.0f)
        , fBPM(1.0f)  // 100% tempo
        , fOutputMode(0.0f)  // 0 = Stereo, 1 = Multi-channel
        , fMasterMute(0.0f)  // Master mute for priming
        , fNativeBPM(125)
        , fCurrentOrder(0)
        , fCurrentRow(0)
    {
        // Create Deck player (supports MOD/MED/AHX/SID)
        fDeckPlayer = deck_player_create();

        // Initialize channel parameters (16 channels)
        for (int i = 0; i < 16; i++) {
            fChannelMute[i] = 0.0f;
            fChannelVolume[i] = 1.0f;
            fChannelPan[i] = 0.0f;

            // Allocate channel buffers
            fChannelBuffers[i] = new float[2048];
            std::memset(fChannelBuffers[i], 0, 2048 * sizeof(float));
        }

        // Set default Amiga panning for first 4 channels
        fChannelPan[0] = -0.5f;
        fChannelPan[1] = 0.5f;
        fChannelPan[2] = 0.5f;
        fChannelPan[3] = -0.5f;

        // Set position callback
        deck_player_set_position_callback(fDeckPlayer, positionCallback, this);

        updateChannelControls();
    }

    ~RGDeckPlayerPlugin() override
    {
        if (fDeckPlayer) {
            deck_player_destroy(fDeckPlayer);
        }

        for (int i = 0; i < 16; i++) {
            delete[] fChannelBuffers[i];
        }
    }

protected:
    const char* getLabel() const override { return RGDECKPLAYER_DISPLAY_NAME; }
    const char* getDescription() const override { return RGDECKPLAYER_DESCRIPTION; }
    const char* getMaker() const override { return "Regroove"; }
    const char* getHomePage() const override { return "https://music.gbraad.nl/regrooved/"; }
    const char* getLicense() const override { return "GPL-3.0"; }
    uint32_t getVersion() const override { return d_version(1, 0, 0); }
    int64_t getUniqueId() const override { return d_cconst('R', 'G', 'D', 'K'); }

    void initAudioPort(bool input, uint32_t index, AudioPort& port) override
    {
        // We have no inputs, only outputs
        if (input) return;

        // Group outputs into 16 stereo pairs
        const uint32_t channelNum = (index / 2) + 1;
        const bool isLeft = (index % 2) == 0;

        port.groupId = index / 2;

        char nameBuf[32];
        char symbolBuf[16];

        std::sprintf(nameBuf, "Channel %d %s", channelNum, isLeft ? "Left" : "Right");
        std::sprintf(symbolBuf, "ch%d_%c", channelNum, isLeft ? 'l' : 'r');

        port.name = nameBuf;
        port.symbol = symbolBuf;
    }

    void initPortGroup(uint32_t groupId, PortGroup& portGroup) override
    {
        char nameBuf[32];
        char symbolBuf[16];

        std::sprintf(nameBuf, "Channel %d", groupId + 1);
        std::sprintf(symbolBuf, "ch%d", groupId + 1);

        portGroup.name = nameBuf;
        portGroup.symbol = symbolBuf;
    }

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

        case kParameterLoopPattern:
            param.name = "Loop Pattern";
            param.symbol = "loop_pattern";
            param.hints |= kParameterIsBoolean;
            param.ranges.def = 0.0f;
            break;

        case kParameterPrevPattern:
            param.name = "Previous Pattern";
            param.symbol = "prev_pattern";
            param.hints |= kParameterIsBoolean | kParameterIsTrigger;
            param.ranges.def = 0.0f;
            break;

        case kParameterNextPattern:
            param.name = "Next Pattern";
            param.symbol = "next_pattern";
            param.hints |= kParameterIsBoolean | kParameterIsTrigger;
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
            param.ranges.def = 127.0f;
            break;

        case kParameterBPM:
            param.name = "Tempo";
            param.symbol = "tempo";
            param.ranges.min = 0.9f;   // -10%
            param.ranges.max = 1.1f;   // +10%
            param.ranges.def = 1.0f;   // 0%
            break;

        case kParameterOutputMode:
            param.name = "Output Mode";
            param.symbol = "output_mode";
            param.hints |= kParameterIsBoolean | kParameterIsInteger;
            param.ranges.min = 0.0f;
            param.ranges.max = 1.0f;
            param.ranges.def = 0.0f;  // Default to Stereo
            param.enumValues.count = 2;
            param.enumValues.restrictedMode = true;
            {
                ParameterEnumerationValue* const values = new ParameterEnumerationValue[2];
                values[0].label = "Stereo Mix";
                values[0].value = 0.0f;
                values[1].label = "Multi-channel";
                values[1].value = 1.0f;
                param.enumValues.values = values;
            }
            break;

        case kParameterMasterMute:
            param.name = "Master Mute";
            param.symbol = "master_mute";
            param.hints |= kParameterIsBoolean;
            param.ranges.def = 0.0f;
            break;

        case kParameterCurrentOrder:
            param.name = "Current Order";
            param.symbol = "current_order";
            param.hints = kParameterIsOutput;  // Read-only output
            param.ranges.max = 127.0f;
            param.ranges.def = 0.0f;
            break;

        case kParameterCurrentRow:
            param.name = "Current Row";
            param.symbol = "current_row";
            param.hints = kParameterIsOutput;  // Read-only output
            param.ranges.max = 255.0f;
            param.ranges.def = 0.0f;
            break;

        // Channel 1
        case kParameterCh1Mute:
            param.name = "Channel 1 Mute";
            param.symbol = "ch1_mute";
            param.hints |= kParameterIsBoolean;
            break;
        case kParameterCh1Volume:
            param.name = "Channel 1 Volume";
            param.symbol = "ch1_volume";
            param.ranges.def = 1.0f;
            break;
        case kParameterCh1Pan:
            param.name = "Channel 1 Pan";
            param.symbol = "ch1_pan";
            param.ranges.min = -1.0f;
            param.ranges.max = 1.0f;
            param.ranges.def = -0.5f;
            break;

        // Channel 2
        case kParameterCh2Mute:
            param.name = "Channel 2 Mute";
            param.symbol = "ch2_mute";
            param.hints |= kParameterIsBoolean;
            break;
        case kParameterCh2Volume:
            param.name = "Channel 2 Volume";
            param.symbol = "ch2_volume";
            param.ranges.def = 1.0f;
            break;
        case kParameterCh2Pan:
            param.name = "Channel 2 Pan";
            param.symbol = "ch2_pan";
            param.ranges.min = -1.0f;
            param.ranges.max = 1.0f;
            param.ranges.def = 0.5f;
            break;

        // Channel 3
        case kParameterCh3Mute:
            param.name = "Channel 3 Mute";
            param.symbol = "ch3_mute";
            param.hints |= kParameterIsBoolean;
            break;
        case kParameterCh3Volume:
            param.name = "Channel 3 Volume";
            param.symbol = "ch3_volume";
            param.ranges.def = 1.0f;
            break;
        case kParameterCh3Pan:
            param.name = "Channel 3 Pan";
            param.symbol = "ch3_pan";
            param.ranges.min = -1.0f;
            param.ranges.max = 1.0f;
            param.ranges.def = 0.5f;
            break;

        // Channel 4
        case kParameterCh4Mute:
            param.name = "Channel 4 Mute";
            param.symbol = "ch4_mute";
            param.hints |= kParameterIsBoolean;
            break;
        case kParameterCh4Volume:
            param.name = "Channel 4 Volume";
            param.symbol = "ch4_volume";
            param.ranges.def = 1.0f;
            break;
        case kParameterCh4Pan:
            param.name = "Channel 4 Pan";
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
        case kParameterLoopPattern: return fLoopPattern;
        case kParameterPrevPattern: return fPrevPattern;
        case kParameterNextPattern: return fNextPattern;
        case kParameterLoopStart: return fLoopStart;
        case kParameterLoopEnd: return fLoopEnd;
        case kParameterBPM: return fBPM;
        case kParameterOutputMode: return fOutputMode;
        case kParameterMasterMute: return fMasterMute;
        case kParameterCurrentOrder: return (float)fCurrentOrder;
        case kParameterCurrentRow: return (float)fCurrentRow;

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

        default: return 0.0f;
        }
    }

    void setParameterValue(uint32_t index, float value) override
    {
        switch (index) {
        case kParameterPlay:
            fPlaying = value;  // ALWAYS update state first

            if (value > 0.5f) {
                // Start playback
                if (fDeckPlayer) {
                    deck_player_start(fDeckPlayer);
                }
            } else {
                // Stop playback
                if (fDeckPlayer) {
                    deck_player_stop(fDeckPlayer);
                }
            }
            break;

        case kParameterLoopPattern:
            fLoopPattern = value;
            if (fDeckPlayer) {
                if (value > 0.5f) {
                    // Loop current pattern
                    deck_player_set_loop_range(fDeckPlayer, fCurrentOrder, fCurrentOrder);
                } else {
                    // Loop full song
                    uint8_t len = deck_player_get_song_length(fDeckPlayer);
                    if (len > 0) {
                        deck_player_set_loop_range(fDeckPlayer, 0, len - 1);
                    }
                }
            }
            break;

        case kParameterPrevPattern:
            if (value > 0.5f && fPrevPattern <= 0.5f) {
                // Trigger: go to previous pattern
                if (fDeckPlayer && fCurrentOrder > 0) {
                    deck_player_set_position(fDeckPlayer, fCurrentOrder - 1, 0);
                }
            }
            fPrevPattern = value;
            break;

        case kParameterNextPattern:
            if (value > 0.5f && fNextPattern <= 0.5f) {
                // Trigger: go to next pattern
                if (fDeckPlayer) {
                    uint8_t len = deck_player_get_song_length(fDeckPlayer);
                    if (fCurrentOrder < len - 1) {
                        deck_player_set_position(fDeckPlayer, fCurrentOrder + 1, 0);
                    }
                }
            }
            fNextPattern = value;
            break;

        case kParameterLoopStart:
            fLoopStart = value;
            if (fDeckPlayer) {
                deck_player_set_loop_range(fDeckPlayer, (uint16_t)fLoopStart, (uint16_t)fLoopEnd);
            }
            break;

        case kParameterLoopEnd:
            fLoopEnd = value;
            if (fDeckPlayer) {
                deck_player_set_loop_range(fDeckPlayer, (uint16_t)fLoopStart, (uint16_t)fLoopEnd);
            }
            break;

        case kParameterBPM:
            fBPM = value;  // Store tempo multiplier (0.9 - 1.1)
            if (fDeckPlayer) {
                // Apply multiplier to native BPM
                uint16_t actual_bpm = (uint16_t)(fNativeBPM * fBPM);
                deck_player_set_bpm(fDeckPlayer, actual_bpm);
            }
            break;

        case kParameterOutputMode:
            fOutputMode = value;
            break;

        case kParameterMasterMute:
            fMasterMute = value;
            break;

        case kParameterCh1Mute: fChannelMute[0] = value; updateChannelControls(); break;
        case kParameterCh1Volume: fChannelVolume[0] = value; break;
        case kParameterCh1Pan: fChannelPan[0] = value; break;

        case kParameterCh2Mute: fChannelMute[1] = value; updateChannelControls(); break;
        case kParameterCh2Volume: fChannelVolume[1] = value; break;
        case kParameterCh2Pan: fChannelPan[1] = value; break;

        case kParameterCh3Mute: fChannelMute[2] = value; updateChannelControls(); break;
        case kParameterCh3Volume: fChannelVolume[2] = value; break;
        case kParameterCh3Pan: fChannelPan[2] = value; break;

        case kParameterCh4Mute: fChannelMute[3] = value; updateChannelControls(); break;
        case kParameterCh4Volume: fChannelVolume[3] = value; break;
        case kParameterCh4Pan: fChannelPan[3] = value; break;
        }
    }

    void initState(uint32_t index, State& state) override
    {
        if (index == 0) {
            state.key = "file";
            state.label = "File";
            state.hints = kStateIsFilenamePath;
            state.defaultValue = "";
        }
    }

    void setState(const char* key, const char* value) override
    {
        if (std::strcmp(key, "file") == 0 && value && value[0] != '\0') {
            loadFile(value);
        }
    }

    void run(const float** inputs, float** outputs, uint32_t frames,
             const MidiEvent* midiEvents, uint32_t midiEventCount) override
    {
        (void)inputs;  // No audio inputs
        (void)midiEvents;  // MIDI not yet implemented
        (void)midiEventCount;

        if (!fDeckPlayer) {
            // Clear all outputs
            for (uint32_t i = 0; i < 32; i++) {
                std::memset(outputs[i], 0, frames * sizeof(float));
            }
            return;
        }

        // Get number of channels
        uint8_t numChannels = deck_player_get_num_channels(fDeckPlayer);
        if (numChannels == 0) numChannels = 4;  // Default to 4
        if (numChannels > 16) numChannels = 16;  // Cap at 16

        // Allocate temporary buffers for mixing
        float* mixLeft = new float[frames];
        float* mixRight = new float[frames];
        std::memset(mixLeft, 0, frames * sizeof(float));
        std::memset(mixRight, 0, frames * sizeof(float));

        // Prepare channel output pointers (up to 16 channels)
        float* channelOutputs[16] = {nullptr};
        for (int i = 0; i < numChannels && i < 16; i++) {
            if (frames <= 2048) {
                channelOutputs[i] = fChannelBuffers[i];
                std::memset(channelOutputs[i], 0, frames * sizeof(float));
            }
        }

        // Only process audio when playing
        if (fPlaying > 0.5f) {
            // Render audio with per-channel outputs
            deck_player_process_channels(fDeckPlayer, mixLeft, mixRight, channelOutputs, frames, (int)getSampleRate());
        }

        // Route outputs based on Output Mode parameter
        bool multiChannelMode = fOutputMode > 0.5f;

        if (multiChannelMode) {
            // MULTI-CHANNEL MODE:
            // Output all 16 channels to their respective stereo pairs

            for (uint32_t ch = 0; ch < numChannels && ch < 16; ch++) {
                if (channelOutputs[ch]) {
                    float volume = fChannelVolume[ch];
                    float pan = fChannelPan[ch];  // -1.0 (left) to 1.0 (right)
                    float panLeft = std::max(0.0f, 1.0f - (pan + 1.0f) * 0.5f);
                    float panRight = std::max(0.0f, (pan + 1.0f) * 0.5f);

                    for (uint32_t i = 0; i < frames; i++) {
                        float sample = channelOutputs[ch][i] * volume;
                        outputs[ch * 2][i] = sample * panLeft;      // Left
                        outputs[ch * 2 + 1][i] = sample * panRight; // Right
                    }
                } else {
                    std::memset(outputs[ch * 2], 0, frames * sizeof(float));
                    std::memset(outputs[ch * 2 + 1], 0, frames * sizeof(float));
                }
            }

            // Clear unused channel outputs
            for (uint32_t ch = numChannels; ch < 16; ch++) {
                std::memset(outputs[ch * 2], 0, frames * sizeof(float));
                std::memset(outputs[ch * 2 + 1], 0, frames * sizeof(float));
            }

        } else {
            // STEREO MODE (DEFAULT):
            // Output 0-1: Channel 1 - Stereo mix (L/R)
            // Outputs 2-31: Silent

            std::memcpy(outputs[0], mixLeft, frames * sizeof(float));
            std::memcpy(outputs[1], mixRight, frames * sizeof(float));

            // Clear all other outputs
            for (uint32_t i = 2; i < 32; i++) {
                std::memset(outputs[i], 0, frames * sizeof(float));
            }
        }

        // Apply master mute (for priming - mutes final output without changing channel states)
        if (fMasterMute > 0.5f) {
            for (uint32_t i = 0; i < 32; i++) {
                std::memset(outputs[i], 0, frames * sizeof(float));
            }
        }

        delete[] mixLeft;
        delete[] mixRight;
    }

private:
    DeckPlayer* fDeckPlayer;
    float fPlaying;
    float fLoopPattern;
    float fPrevPattern;
    float fNextPattern;
    float fLoopStart;
    float fLoopEnd;
    float fBPM;  // Tempo multiplier (0.9 - 1.1)
    float fOutputMode;  // 0 = Stereo Mix, 1 = Multi-channel
    float fMasterMute;  // Master output mute (priming)
    uint16_t fNativeBPM;  // Original BPM from loaded file
    float fChannelMute[16];
    float fChannelVolume[16];
    float fChannelPan[16];
    float* fChannelBuffers[16];

    uint8_t fCurrentOrder;
    uint16_t fCurrentRow;
    char fFilename[256];

    bool loadFile(const char* filename)
    {
        if (!fDeckPlayer || !filename) return false;

        // Read file
        FILE* f = fopen(filename, "rb");
        if (!f) return false;

        fseek(f, 0, SEEK_END);
        long size = ftell(f);
        fseek(f, 0, SEEK_SET);

        if (size <= 0 || size > 10 * 1024 * 1024) {
            fclose(f);
            return false;
        }

        uint8_t* data = new uint8_t[size];
        size_t read = fread(data, 1, size, f);
        fclose(f);

        if (read != (size_t)size) {
            delete[] data;
            return false;
        }

        // Try to load the file
        bool loadSuccess = deck_player_load(fDeckPlayer, data, size);
        delete[] data;

        if (!loadSuccess) {
            return false;
        }

        // SUCCESS - file loaded! Save filename
        std::strncpy(fFilename, filename, sizeof(fFilename) - 1);
        fFilename[sizeof(fFilename) - 1] = '\0';

        // Get native BPM from loaded file
        fNativeBPM = deck_player_get_bpm(fDeckPlayer);

        // Apply current tempo multiplier to new file
        uint16_t actual_bpm = (uint16_t)(fNativeBPM * fBPM);
        deck_player_set_bpm(fDeckPlayer, actual_bpm);

        // Reset state after loading new file
        deck_player_set_position(fDeckPlayer, 0, 0);  // Reset to start

        // DON'T reset fPlaying - let user control playback via parameter
        // If fPlaying is 1.0, file will start playing automatically
        // If fPlaying is 0.0, file will stay stopped

        fLoopPattern = 0.0f;  // Disable pattern loop
        fCurrentOrder = 0;
        fCurrentRow = 0;

        // Unmute all channels (reset from previous file)
        for (int i = 0; i < 16; i++) {
            fChannelMute[i] = 0.0f;
            deck_player_set_channel_mute(fDeckPlayer, i, false);
        }

        // Set default loop range to full song
        uint8_t len = deck_player_get_song_length(fDeckPlayer);
        if (len > 0) {
            deck_player_set_loop_range(fDeckPlayer, 0, len - 1);
        }

        return true;
    }

    void updateChannelControls()
    {
        if (!fDeckPlayer) return;

        for (int i = 0; i < 16; i++) {
            deck_player_set_channel_mute(fDeckPlayer, i, fChannelMute[i] > 0.5f);
        }
    }

    static void positionCallback(uint8_t order, uint16_t pattern, uint16_t row, void* userData)
    {
        (void)pattern;  // Unused parameter
        RGDeckPlayerPlugin* plugin = static_cast<RGDeckPlayerPlugin*>(userData);

        // Update internal state
        plugin->fCurrentOrder = order;
        plugin->fCurrentRow = row;

        // Notify UI of position change (output parameters)
        const_cast<RGDeckPlayerPlugin*>(plugin)->setParameterValue(kParameterCurrentOrder, (float)order);
        const_cast<RGDeckPlayerPlugin*>(plugin)->setParameterValue(kParameterCurrentRow, (float)row);
    }
};

Plugin* createPlugin()
{
    return new RGDeckPlayerPlugin();
}

END_NAMESPACE_DISTRHO
