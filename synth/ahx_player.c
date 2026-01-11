#include "ahx_player.h"
#include "tracker_modulator.h"
#include "tracker_sequence.h"
#include "tracker_voice.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

// HivelyTracker formula - returns 16.16 fixed-point delta value
#define AMIGA_PAULA_PAL_CLK 3546895
#define Period2Freq(period) ((AMIGA_PAULA_PAL_CLK * 65536.0f) / (period))
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define CLAMP(x, low, high) (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))

// Forward declarations
typedef struct AhxPListEntry AhxPListEntry;
typedef struct AhxPList AhxPList;
typedef struct AhxEnvelope AhxEnvelope;
typedef struct AhxInstrument AhxInstrument;
typedef struct AhxPosition AhxPosition;
typedef struct AhxStep AhxStep;
typedef struct AhxSong AhxSong;
typedef struct AhxVoice AhxVoice;
typedef struct AhxWaves AhxWaves;

// Structure definitions (matching C++ classes)
struct AhxPListEntry {
    int Note;
    int Fixed;
    int Waveform;
    int FX[2];
    int FXParam[2];
};

struct AhxPList {
    int Speed;
    int Length;
    AhxPListEntry* Entries;
};

struct AhxEnvelope {
    int aFrames, aVolume;
    int dFrames, dVolume;
    int sFrames;
    int rFrames, rVolume;
};

struct AhxInstrument {
    char* Name;
    int Volume;
    int WaveLength;
    AhxEnvelope Envelope;
    int FilterLowerLimit, FilterUpperLimit, FilterSpeed;
    int SquareLowerLimit, SquareUpperLimit, SquareSpeed;
    int VibratoDelay, VibratoDepth, VibratoSpeed;
    int HardCutRelease, HardCutReleaseFrames;
    AhxPList PList;
};

struct AhxPosition {
    int Track[4];
    int Transpose[4];
};

struct AhxStep {
    int Note;
    int Instrument;
    int FX;
    int FXParam;
};

struct AhxSong {
    char* Name;
    int Restart, PositionNr, TrackLength, TrackNr, InstrumentNr, SubsongNr;
    int Revision, SpeedMultiplier;
    AhxPosition* Positions;
    AhxStep** Tracks;
    AhxInstrument* Instruments;
    int* Subsongs;
};

struct AhxVoice {
    // Generic tracker components
    TrackerModulator filter_mod;
    TrackerModulator square_mod;
    TrackerSequence plist_seq;
    TrackerVoice voice_playback;

    // Public mixing variables
    int VoiceVolume, VoicePeriod;
    char VoiceBuffer[0x281];
    uint32_t Delta;  // HVL: Pre-calculated sample delta (16.16 fixed-point)

    // Internal state
    int Track, Transpose;
    int NextTrack, NextTranspose;
    int ADSRVolume;
    AhxEnvelope ADSR;
    AhxInstrument* Instrument;
    int InstrPeriod, TrackPeriod, VibratoPeriod;
    int NoteMaxVolume, PerfSubVolume, TrackMasterVolume;
    int NewWaveform, Waveform, PlantSquare, PlantPeriod, IgnoreSquare;
    int TrackOn, FixedNote;
    int VolumeSlideUp, VolumeSlideDown;
    int HardCut, HardCutRelease, HardCutReleaseF;
    int PeriodSlideSpeed, PeriodSlidePeriod, PeriodSlideLimit, PeriodSlideOn, PeriodSlideWithLimit;
    int PeriodPerfSlideSpeed, PeriodPerfSlidePeriod, PeriodPerfSlideOn;
    int VibratoDelay, VibratoCurrent, VibratoDepth, VibratoSpeed;
    int SquareOn, SquareInit, SquareWait, SquareLowerLimit, SquareUpperLimit, SquarePos, SquareSign, SquareSlidingIn, SquareReverse;
    int FilterOn, FilterInit, FilterWait, FilterLowerLimit, FilterUpperLimit, FilterPos, FilterSign, FilterSpeed, FilterSlidingIn, IgnoreFilter;
    int PerfCurrent, PerfSpeed, PerfWait;
    int WaveLength;
    AhxPList* PerfList;
    int NoteDelayWait, NoteDelayOn, NoteCutWait, NoteCutOn;
    char* AudioPointer;
    char* AudioSource;
    int AudioPeriod, AudioVolume;
    char SquareTempBuffer[0x80];

    // HVL-style panning
    int PanMultLeft, PanMultRight;

    // Per-voice white noise random state
    int WNRandom;
};

struct AhxWaves {
    char LowPasses[(0xfc+0xfc+0x80*0x1f+0x80+3*0x280)*31];
    char Triangle04[0x04], Triangle08[0x08], Triangle10[0x10], Triangle20[0x20], Triangle40[0x40], Triangle80[0x80];
    char Sawtooth04[0x04], Sawtooth08[0x08], Sawtooth10[0x10], Sawtooth20[0x20], Sawtooth40[0x40], Sawtooth80[0x80];
    char Squares[0x80*0x20];
    char WhiteNoiseBig[0x280*3];
    char HighPasses[(0xfc+0xfc+0x80*0x1f+0x80+3*0x280)*31];
};

struct AhxPlayer {
    AhxSong Song;
    AhxVoice Voices[4];
    AhxWaves* Waves;
    int OurWaves;

    int StepWaitFrames, GetNewPosition, SongEndReached, TimingValue;
    int PatternBreak;
    int MainVolume;
    int Playing, Tempo;
    int PosNr, PosJump;
    int NoteNr, PosJumpNote;
    int PlayingTime;
    char* WaveformTab[4];

    // Mixing state
    int VolumeTable[65][256];  // Kept for compatibility but not used in HVL mode
    float Boost;
    int Oversampling;
    int pos[4];  // Sample positions for each voice
    int frame_counter;  // Frame timing counter for 50Hz IRQ
    int mixgain;  // HVL mixing gain
    int panning_left[256];   // HVL panning table
    int panning_right[256];  // HVL panning table

    // Callback
    AhxPositionCallback position_callback;
    void* position_callback_userdata;
    uint16_t last_position;
    uint16_t last_row;

    // Channel muting
    bool channel_muted[4];

    // Looping control
    bool disable_looping;

    // Sample rate for delta calculation
    int current_sample_rate;
};

// Vibrato table (from AHX.cpp line 30)
static const int VibratoTable[64] = {
    0,24,49,74,97,120,141,161,180,197,212,224,235,244,250,253,255,
    253,250,244,235,224,212,197,180,161,141,120,97,74,49,24,
    0,-24,-49,-74,-97,-120,-141,-161,-180,-197,-212,-224,-235,-244,-250,-253,-255,
    -253,-250,-244,-235,-224,-212,-197,-180,-161,-141,-120,-97,-74,-49,-24
};

// Period table (from AHX.cpp line 37)
static const int PeriodTable[61] = {
    0x0000, 0x0D60, 0x0CA0, 0x0BE8, 0x0B40, 0x0A98, 0x0A00, 0x0970,
    0x08E8, 0x0868, 0x07F0, 0x0780, 0x0714, 0x06B0, 0x0650, 0x05F4,
    0x05A0, 0x054C, 0x0500, 0x04B8, 0x0474, 0x0434, 0x03F8, 0x03C0,
    0x038A, 0x0358, 0x0328, 0x02FA, 0x02D0, 0x02A6, 0x0280, 0x025C,
    0x023A, 0x021A, 0x01FC, 0x01E0, 0x01C5, 0x01AC, 0x0194, 0x017D,
    0x0168, 0x0153, 0x0140, 0x012E, 0x011D, 0x010D, 0x00FE, 0x00F0,
    0x00E2, 0x00D6, 0x00CA, 0x00BE, 0x00B4, 0x00AA, 0x00A0, 0x0097,
    0x008F, 0x0087, 0x007F, 0x0078, 0x0071
};

// White noise table (from AHX.cpp line 61)
static const unsigned char WhiteNoiseBig[] = {
    0x7f,0x7f,0xa8,0xe2,0x78,0x3e,0x2c,0x92,0x52,0xd5,0x80,0x80,0xab,0x80,0x7f,0x37,
    0x7f,0x7f,0x15,0x3b,0xbc,0x66,0xf3,0x7f,0x80,0x80,0x80,0x80,0x42,0xe5,0xf8,0x80,
    0x7f,0x7f,0x26,0x7f,0x80,0x97,0x80,0x5f,0xa7,0x7f,0x80,0x80,0x80,0x7f,0x7f,0x7f,
    0xce,0x79,0x8c,0x80,0x4a,0x7f,0x80,0x16,0x7f,0x7f,0x80,0x80,0x09,0xf1,0x80,0x95,
    0x78,0x78,0x7f,0xb8,0xe2,0x52,0x7f,0x08,0x93,0x7f,0x7f,0x80,0xfb,0xa8,0x44,0xe5,
    0xca,0x09,0x7f,0x80,0x7f,0x80,0xcb,0x80,0x7f,0xf7,0x80,0x80,0xb7,0x7f,0x5b,0x80,
    0x3b,0x14,0xcf,0x80,0x7f,0x80,0x16,0x1f,0x67,0xa1,0x62,0x71,0x71,0xa7,0x7f,0x44,
    0x41,0x80,0x7f,0xcd,0x41,0x43,0x4b,0xf3,0x80,0xc7,0xdf,0xdf,0xd5,0x27,0x1f,0x1f,
    0x9f,0x36,0x24,0x73,0x71,0x7f,0x80,0x7f,0x79,0x42,0x7f,0x7f,0x80,0x80,0x80,0x2e,
    0x22,0x7f,0xf2,0x46,0x80,0x80,0xb4,0xd2,0x35,0x2e,0x80,0x8f,0xb5,0xbc,0x80,0x38,
    0xf2,0x7f,0x10,0x2d,0x7f,0x7f,0x26,0x91,0x7f,0xf0,0x7f,0xdf,0x2b,0x7f,0x80,0x3e,
    0x7f,0x7f,0x80,0x80,0xab,0xae,0x7f,0xca,0x80,0x80,0xf3,0xba,0x34,0x80,0x80,0x7f,
    0x7f,0x80,0x3e,0x66,0x80,0x17,0x80,0xab,0x80,0x09,0xf3,0x7f,0x29,0x80,0xc4,0x7f,
    0x80,0xd3,0x7f,0xba,0x80,0x7f,0x80,0x9d,0x7f,0x80,0x38,0x80,0x7f,0x7f,0x7f,0x69,
    0x7f,0x7f,0x15,0x4f,0x80,0x7c,0x8c,0x1b,0x7f,0x7f,0x80,0x80,0x70,0x2b,0x80,0x7f,
    0x5a,0xc1,0x7f,0x80,0x7f,0x45,0xbb,0x80,0x7f,0xf7,0xce,0x80,0x80,0x80,0xda,0x9d,
    0x7f,0x80,0x7f,0xba,0xe2,0x02,0x80,0x95,0xba,0x80,0xfa,0xfe,0x80,0xb4,0x80,0x80,
    0x88,0x7f,0x7f,0x12,0x80,0x80,0x0e,0x9b,0x80,0x80,0x4f,0xc9,0x2b,0x80,0x77,0xb5,
    0x7f,0x51,0x7f,0x7f,0x7f,0x7f,0x80,0x7f,0xf1,0x80,0x31,0xe6,0x80,0x7f,0x80,0xa5,
    0x80,0x7f,0xca,0x7f,0x25,0x80,0x92,0xb4,0x7f,0x80,0x97,0x7f,0x7f,0x94,0x20,0x1b,
    0x3b,0x7f,0xee,0xca,0x80,0x80,0x42,0x80,0x80,0xa3,0x80,0xc5,0xf1,0x80,0x7f,0x7f,
    0x7f,0x51,0xaf,0x7f,0x35,0x42,0x80,0x7f,0xf1,0x80,0xc5,0x7f,0x7f,0x7f,0x80,0x28,
    0x7f,0xb3,0x2c,0x2c,0xea,0x7f,0x7f,0x80,0x7f,0x21,0xa9,0x7f,0x34,0x7f,0xae,0x1e,
    0xc5,0xbf,0xae,0x7f,0x8b,0x37,0x7f,0x0d,0x80,0x73,0x23,0xbb,0x80,0x80,0xc6,0x80,
    0xb6,0x80,0x7f,0x80,0x80,0x7f,0x7f,0x80,0x21,0x7f,0x20,0x45,0xa7,0xca,0x7f,0x80,
    0x80,0x80,0x3d,0x7f,0x15,0x45,0xf3,0xd8,0x8b,0x9b,0xce,0x55,0x80,0x80,0x7f,0xbd,
    0xce,0x7f,0x36,0x80,0x7f,0xbf,0x62,0x23,0x07,0x25,0xf1,0xca,0x59,0x7f,0xaa,0x7f,
    0x7f,0x47,0x93,0x80,0x1b,0x21,0x80,0x9b,0xca,0x80,0x2d,0x80,0x98,0x7f,0x7f,0x7f,
    0xee,0x80,0x80,0x80,0x7f,0x20,0x3b,0x80,0x3c,0x22,0xcf,0x7f,0x80,0x80,0x59,0x9d,
    0x7f,0x2a,0x7f,0x80,0x7c,0x80,0xd3,0x21,0x80,0xa7,0x7f,0x7f,0x80,0x09,0x3d,0x7f,
    0x7f,0xae,0x80,0xa7,0x80,0x7f,0x73,0x05,0x3d,0x80,0x7f,0x7f,0x7f,0x26,0x3b,0x7f,
    0xf6,0x80,0x7f,0x5e,0x47,0xdf,0x80,0x7c,0x36,0x36,0x7f,0xff,0xbc,0xbc,0xbc,0x7f,
    0x7f,0x7f,0x80,0x80,0x4d,0x21,0x7f,0x7f,0x7f,0x41,0x4d,0x80,0x7f,0x7f,0x80,0xc0,
    0xaf,0x2c,0x7f,0x17,0x35,0x80,0x80,0x7f,0xf0,0x3c,0x12,0x87,0x7f,0x80,0x80,0x13,
    0x73,0x2d,0x3e,0x80,0x7f,0x80,0xa6,0xd8,0x19,0x80,0x7f,0x27,0x80,0x7f,0x80,0x7f,
    0x80,0x7f,0x23,0x80,0x4d,0x80,0x7f,0x7f,0x89,0x7f,0x80,0xb5,0x4a,0x17,0xaf,0x88,
    0x95,0x80,0x70,0x77,0x97,0x7f,0x80,0x80,0x22,0x9b,0x02,0x2f,0x80,0x80,0x98,0x7f,
    0x7f,0x12,0x2d,0x28,0xce,0xaf,0x90,0x58,0xe9,0x1a,0x71,0x2f,0x5c,0x7f,0x80,0x7f,
    0x7f,0x80,0x7f,0x47,0xcd,0xaf,0x2c,0x06,0x80,0x2f,0x80,0xe8,0x80,0x2e,0x58,0x11,
    0xd7,0xad,0x58,0x43,0x17,0x9f,0x70,0xc3,0x80,0x70,0x19,0xc3,0x37,0x2e,0x42,0x80,
    0x2c,0xbc,0x80,0x7f,0x7f,0x7f,0x10,0x45,0x2d,0x3e,0x3e,0x90,0x80,0xa6,0xd8,0x5b,
    0x80,0x7f,0x27,0x80,0x7f,0x80,0x33,0x80,0x75,0x80,0x7f,0x7f,0x94,0x80,0x21,0xf1,
    0x7f,0xee,0x7f,0xae,0xf6,0xae,0x80,0x41,0x80,0xa5,0x7f,0x40,0x7f,0x8a,0x3d,0x12,
    0xdd,0x7f,0x9e,0x7f,0x92,0x36,0x66,0x19,0x80,0x80,0xa7,0xa0,0x90,0x80,0x5f,0x23,
    0x57,0x80,0x31,0x80,0x2d,0x36,0xa0,0xd2,0x8f,0xd9,0x3f,0x80,0x3e,0x80,0x29,0xd8,
    0xad,0x7f,0x7f,0x51,0xbb,0x70,0xcb,0xb5,0xdc,0x3d,0xc2,0xb7,0x7f,0xba,0x80,0x3e,
    0x80,0x7f,0x3b,0x44,0x80,0xa6,0x7f,0x80,0x80,0x7c,0x80,0x61,0x7f,0xca,0x7f,0x7f,
    0x80,0xff,0x34,0x7f,0x46,0x05,0x7f,0x24,0x7f,0x7f,0x7f,0x7f,0xbc,0x7f,0x7f,0x7f,
    0x80,0x7f,0x15,0x7f,0xce,0xe5,0x7f,0x80,0x7f,0xbd,0x58,0x85,0x33,0x7f,0x7e,0x80,
    0x80,0x80,0x7f,0x7f,0x80,0x7f,0xf7,0x32,0x94,0x40,0x73,0x7f,0x7f,0xee,0xdc,0x7f,
    0x24,0x7f,0x7f,0xba,0xc6,0x27,0x21,0x95,0x80,0x3d,0xa4,0x80,0x7f,0x7f,0x80,0x7f,
    0x7f,0x94,0x7f,0x7f,0x94,0x80,0x61,0x7f,0x80,0x7f,0x7f,0x79,0x80,0x42,0x7f,0xbe,
    0x80,0x80,0xc2,0x43,0xf7,0xac,0xac,0x80,0x7f,0x7f,0x7f,0x80,0x14,0x7f,0x15,0x7f,
    0xc2,0x1d,0x7f,0x80,0x7f,0xbb,0x80,0x80,0x80,0x80,0xb6,0x7f,0x7f,0x44,0x7f,0x09,
    0x07,0x80,0x7f,0x80,0x7f,0x7f,0x96,0x7f,0xce,0x80,0x80,0x61,0x65,0x80,0x2d,0x4a,
    0x7f,0x7f,0x80,0x7f,0x46,0x80,0x7f,0xaa,0x44,0x80,0xcb,0x89,0x7f,0x80,0x7f,0x80,
    0x7f,0x8e,0x9f,0x80,0xc3,0x43,0x71,0x99,0x80,0x7f,0x47,0x41,0xaf,0x80,0x3b,0xb6,
    0x7f,0x72,0x80,0xd1,0x80,0x7f,0x44,0x80,0x2f,0x7f,0x7f,0x42,0x80,0x7f,0xf0,0x7f,
    0x45,0x7f,0x80,0x7f,0x80,0xc0,0xaf,0x7f,0x9c,0x1e,0x35,0x7f,0xca,0x65,0xf1,0x3c,
    0x92,0xb4,0xa0,0x80,0x7f,0x7f,0x0f,0xd7,0x73,0x80,0x0e,0x80,0x7f,0x80,0x7c,0xca,
    0xc7,0xad,0x80,0x80,0x3d,0x9e,0xf0,0x82,0x8d,0xd9,0x19,0x7f,0x93,0x7f,0x80,0x80,
    0x80,0x98,0x80,0x80,0x7f,0x3b,0x28,0xce,0x09,0x7f,0x5e,0xe9,0x80,0x80,0x7f,0x45,
    0x80,0xfa,0x7f,0x7f,0x80,0x7f,0x80,0x7f,0x7f,0x11,0x80,0xb4,0x2c,0x80,0x13,0x7f,
    0x80,0x80,0xc5,0x7f,0x7f,0xee,0x82,0x80,0x80,0x41,0x80,0x11,0x7f,0x80,0xc1,0x7f,
    0xad,0x7f,0x7f,0x7f,0x81,0xf1,0x80,0x31,0xa0,0x80,0x7f,0x7f,0x25,0x57,0x7f,0xc4,
    0x80,0x2d,0x36,0x7f,0xbd,0x80,0xd9,0x7f,0xbb,0x7f,0x80,0x2f,0x7f,0x36,0x80,0x3e,
    0x58,0x80,0x80,0x41,0x5f,0x80,0x22,0x80,0x80,0xcc,0x7f,0x7f,0x24,0xc5,0x29,0xe6,
    0xc4,0x7f,0x80,0xd1,0x80,0x3a,0x0c,0xa1,0x80,0xb7,0x7f,0xbe,0x80,0x14,0x95,0x80,
    0xf3,0x7f,0x89,0x80,0xc1,0x7f,0x80,0x7f,0x7f,0xa8,0x1e,0xc3,0x43,0x21,0x80,0x80,
    0x7f,0x47,0xcd,0x7b,0x80,0x3b,0x80,0x7f,0x25,0x80,0xd1,0x27,0x89,0x7f,0x80,0x28,
    0xa4,0x90,0x7f,0x59,0x7f,0x24,0x7f,0xb1,0x5c,0x7f,0xbf,0x7f,0x7f,0x80,0x16,0x80,
    0xdb,0x80,0x7f,0x80,0x7f,0x7f,0xf5,0xb2,0x7f,0x7f,0x80,0x7f,0x0f,0x80,0x80,0x80,
    0x77,0x80,0x2e,0x80,0x3c,0xa0,0x7f,0x2b,0x7f,0x68,0x80,0xc0,0x7f,0x7f,0x7f,0x10,
    0xb5,0x7f,0xca,0x11,0x91,0x80,0x95,0x7f,0x7f,0x7f,0x7f,0x80,0x80,0xcb,0x80,0x7f,
    0x81,0x7f,0xac,0xaa,0x7f,0x7f,0x80,0x93,0x3a,0xc0,0x80,0x80,0x98,0x52,0x80,0x7f,
    0xe1,0xa8,0xdc,0x85,0xb3,0x76,0x7f,0xba,0x80,0x7f,0xa3,0x80,0xb4,0x80,0xc6,0x21,
    0x7f,0x0f,0x7f,0x7f,0x80,0x09,0x7f,0x7f,0x7f,0xa1,0xf8,0x7f,0xa3,0x7f,0x26,0x80,
    0xc3,0x80,0x41,0x2b,0x7f,0x7f,0x80,0xc1,0x55,0x7f,0x7f,0x7f,0xaf,0x80,0x80,0x80,
    0x31,0x80,0x7f,0x7f,0xbf,0x52,0x39,0x66,0x73,0xf7,0x5c,0xe9,0x80,0x7f,0x7f,0x42,
    0x55,0x80,0x80,0x92,0x7f,0x7f,0x80,0x97,0x7f,0x15,0x80,0x23,0x1b,0xbb,0x9a,0x80,
    0x80,0x80,0xb6,0x28,0xbe,0x80,0x7f,0x0f,0xeb,0xf0,0x80,0x5f,0xc9,0x21,0x6b,0x7f,
    0x4c,0x80,0x7f,0xad,0xc4,0xc1,0x7f,0x96,0x7f,0x7f,0xaf,0x7f,0xe1,0x9e,0x80,0x7f,
    0xb3,0xf6,0x80,0x80,0x80,0x80,0xab,0xf0,0x80,0x80,0xfa,0x3a,0x7f,0x80,0x80,0x89,
    0x7f,0x08,0x7f,0x80,0x7f,0x80,0xfa,0x44,0x8f,0x09,0x7f,0x80,0x7f,0x80,0x80,0x22,
    0x9b,0x7f,0xb8,0x80,0x7f,0x7f,0x80,0x7f,0x15,0x2d,0x7f,0x7f,0x7f,0x95,0x58,0x93,
    0x7f,0xf0,0xe2,0xdc,0x7f,0x15,0x7f,0x80,0x7f,0x81,0x7f,0xf2,0x94,0x80,0x80,0x7f,
    0x80,0x7f,0xce,0x80,0x80,0x80,0x80,0x80,0x9b,0x80,0x3f,0xa2,0x80,0x98,0x02,0x7f,
    0x20,0x29,0xa8,0x78,0x7f,0x44,0x69,0x11,0x7f,0xca,0x41,0x4d,0x17,0x7f,0x7f,0x80,
    0x80,0x70,0xf7,0x7f,0xfc,0x80,0x80,0x7f,0xce,0x7f,0x80,0x80,0x4a,0x1d,0x80,0x4d,
    0x7f,0x80,0x7f,0xf2,0x80,0xfe,0x80,0x80,0xec,0x62,0x7f,0x7f,0xff,0x80,0xcb,0x80,
    0x7f,0x80,0xc0,0x7f,0x80,0x4e,0x21,0x35,0x0c,0xaf,0xb2,0x7f,0x80,0x3e,0xf0,0x96,
    0xac,0x7f,0x2b,0xea,0x80,0x80,0x80,0x80,0xa0,0x7f,0x44,0x7f,0x7f,0x6d,0xc7,0x7f,
    0x24,0x80,0x2a,0x7f,0x80,0x3c,0x80,0xec,0x7f,0x80,0xe8,0x80,0xa4,0x2a,0x3e,0x56,
    0x80,0x80,0xd3,0xdb,0xb5,0xc0,0x80,0x7f,0xaf,0x14,0x35,0x80,0x38,0x7f,0x96,0x7f,
    0x7f,0x68,0x7f,0x7f,0x41,0x7f,0x44,0x7f,0x80,0xc7,0xc7,0x80,0x80,0x80,0x14,0x80,
    0x7f,0x7f,0xdc,0x1d,0x7f,0x7f,0x7f,0xbf,0x80,0x5c,0x80,0x77,0xf7,0xc0,0xc1,0x80,
    0x23,0x59,0x80,0x80,0x7f,0xad,0xdc,0x7f,0x8a,0x89,0x7f,0xba,0x7f,0x7f,0x80,0xa9,
    0x80,0x80,0x7f,0x4b,0x91,0x7f,0x4c,0x7f,0x44,0xaf,0x7f,0x7f,0x80,0x7f,0x7f,0xb8,
    0x80,0x3c,0x7f,0x3b,0x7f,0x80,0xe8,0x80,0x7f,0x7a,0x2c,0x56,0x80,0x7f,0x80,0xe8,
    0x7f,0x7f,0x17,0x3f,0x7f,0xd8,0x05,0x73,0xdf,0x2d,0xb4,0x80,0x7f,0x95,0x80,0x8c,
    0x7f,0x7f,0xe3,0x80,0x09,0x25,0x7f,0x7f,0x7f,0x7f,0xaa,0x7f,0x15,0xc3,0xaf,0xba,
    0x80,0x80,0x2c,0xf0,0xba,0x7f,0x7f,0x68,0x7f,0x7f,0x7f,0x17,0x4f,0x85,0x80,0x80,
    0x70,0x7f,0x9b,0x62,0x2d,0x80,0x80,0x9b,0x80,0x80,0x95,0x80,0x98,0x7f,0xf7,0x7f,
    0x36,0x80,0x80,0x80,0x7f,0x27,0x80,0x7f,0xca,0x27,0x80,0x0e,0x80,0x3a,0x80,0x80,
    0x31,0xf0,0x7f,0x94,0xb2,0x52,0x7f,0x80,0x80,0x88,0x5d,0x05,0xa3,0x14,0x91,0x80,
    0xcc,0x7f,0x80,0x7f,0x7f,0x80,0x80,0x7f,0x80,0x7f,0x7f,0x4c,0x7f,0xf6,0x7f,0x7f,
    0x80,0xa4,0x7f,0x7f,0x95,0x7f,0x24,0x7f,0xf7,0x62,0x7f,0x80,0x21,0x7f,0x44,0x7f,
    0x43,0x4d,0xcb,0x80,0x7f,0x80,0xc0,0x80,0x7f,0x7f,0x12,0x35,0x24,0x4b,0x93,0x90,
    0x80,0x80,0xc7,0x2b,0x80,0x3b,0x08,0x7f,0x5e,0x7f,0x51,0x80,0xa1,0xb2,0x80,0x7f,
    0xae,0x80,0x7f,0x5a,0x4b,0xf7,0x80,0x80,0xc2,0x7f,0x80,0x80,0x92,0x34,0x80,0x95,
    0xac,0x80,0xa7,0x7f,0x7f,0x11,0x3b,0x3c,0x7f,0x80,0x7f,0x80,0xe8,0x66,0x7f,0x7f,
    0x17,0xd7,0xa3,0x3a,0x80,0x70,0x80,0x80,0x7f,0x7f,0x80,0x80,0x80,0x5c,0x2d,0x80,
    0x17,0x7f,0x7f,0x80,0x38,0x80,0xab,0x7f,0x0f,0x80,0x7f,0x80,0x80,0xc8,0xf1,0xaa,
    0x7f,0x7f,0x80,0x7f,0x7f,0x80,0x4f,0xa7,0xc4,0x80,0x02,0x37,0x80,0x3d,0x80,0x7f,
    0x7f,0xb8,0x7f,0x80,0x2f,0x14,0x13,0x80,0x38,0x80,0x7f,0xf0,0x7f,0x68,0x7f,0x59,
    0xe9,0x2a,0xce,0x7b,0x5c,0x80,0xec,0x7f,0x7f,0x7f,0xf8,0x80,0x80,0x88,0x2d,0x7f,
    0x43,0x13,0x91,0xd8,0x80,0xc4,0x7f,0x3b,0x7f,0x80,0x80,0xcb,0x80,0x80,0x80,0x7f,
    0xac,0x7f,0x26,0x7f,0x80,0x80,0xd9,0x27,0x1b,0x7f,0x7a,0x34,0x7f,0x80,0x7f,0x7f,
    0x7f,0x0c,0x7f,0x7f,0x7f,0x80,0x7f,0x80,0x17,0x80,0x6e,0x80,0x76,0x80,0x80,0x5f,
    0xa1,0xa0,0x9e,0x7f,0x4d,0x55,0xd5,0x19,0x7f,0x7f,0x7f,0x80,0x13,0xe7,0x2c,0x2c
};

// Stereo panning positions (HVL)
static const int stereopan_left[]  = { 128,  96,  64,  32,   0 };
static const int stereopan_right[] = { 128, 160, 193, 225, 255 };

// Forward declarations of internal functions
static void voice_init(AhxVoice* voice);
static void voice_calc_adsr(AhxVoice* voice);
static void gen_panning_tables(AhxPlayer* player);
static void waves_generate(AhxWaves* waves);
static void waves_generate_triangle(char* buffer, int len);
static void waves_generate_square(char* buffer);
static void waves_generate_sawtooth(char* buffer, int len);
static void waves_generate_white_noise(char* buffer, int len);
static void waves_generate_filter_waveforms(char* buffer, char* low, char* high);
static void player_process_step(AhxPlayer* player, int v);
static void player_process_frame(AhxPlayer* player, int v);
static void player_set_audio(AhxPlayer* player, int v);
static void player_plist_command_parse(AhxPlayer* player, int v, int fx, int fx_param);
static void player_play_irq(AhxPlayer* player);
static void init_volume_table(AhxPlayer* player, float boost);

// Generate HVL panning tables (from HVL replay.c)
static void gen_panning_tables(AhxPlayer* player) {
    double aa = (3.14159265 * 2.0) / 4.0;  // Quarter of the way through the sinewave
    double ab = 0.0;                         // Start of the climb from zero

    for (int i = 0; i < 256; i++) {
        player->panning_left[i]  = (int)(sin(aa) * 255.0);
        player->panning_right[i] = (int)(sin(ab) * 255.0);

        aa += (3.14159265 * 2.0 / 4.0) / 256.0;
        ab += (3.14159265 * 2.0 / 4.0) / 256.0;
    }
    player->panning_left[255] = 0;
    player->panning_right[0] = 0;
}

// Voice functions
static void voice_init(AhxVoice* voice) {
    memset(voice, 0, sizeof(AhxVoice));
    memset(voice->VoiceBuffer, 0, 0x281);

    // Initialize generic tracker components
    tracker_modulator_init(&voice->filter_mod);
    tracker_modulator_init(&voice->square_mod);
    tracker_sequence_init(&voice->plist_seq);
    tracker_voice_init(&voice->voice_playback);

    voice->TrackOn = 1;
    voice->TrackMasterVolume = 0x40;
    voice->WNRandom = 0x280;
    voice->Delta = 1;
}

static void voice_calc_adsr(AhxVoice* voice) {
    voice->ADSR.aFrames = voice->Instrument->Envelope.aFrames;
    voice->ADSR.aVolume = voice->Instrument->Envelope.aVolume * 256 / voice->ADSR.aFrames;
    voice->ADSR.dFrames = voice->Instrument->Envelope.dFrames;
    voice->ADSR.dVolume = (voice->Instrument->Envelope.dVolume - voice->Instrument->Envelope.aVolume) * 256 / voice->ADSR.dFrames;
    voice->ADSR.sFrames = voice->Instrument->Envelope.sFrames;
    voice->ADSR.rFrames = voice->Instrument->Envelope.rFrames;
    voice->ADSR.rVolume = (voice->Instrument->Envelope.rVolume - voice->Instrument->Envelope.dVolume) * 256 / voice->ADSR.rFrames;
}

// Wave generation functions
static inline void clip_float(float* x) {
    if (*x > 127.f) { *x = 127.f; return; }
    if (*x < -128.f) { *x = -128.f; return; }
}

static void waves_generate_filter_waveforms(char* buffer, char* low, char* high) {
    int length_table[] = { 3, 7, 0xf, 0x1f, 0x3f, 0x7f, 3, 7, 0xf, 0x1f, 0x3f, 0x7f,
        0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,
        0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,
        (0x280*3)-1
    };

    for (int temp = 0, freq = 8; temp < 31; temp++, freq += 3) {
        char* a0 = buffer;
        for (int waves = 0; waves < 6+6+0x20+1; waves++) {
            float fre = (float)freq * 1.25f / 100.f;
            float high_val, mid = 0.f, low_val = 0.f;
            for (int i = 0; i <= length_table[waves]; i++) {
                high_val = a0[i] - mid - low_val;
                clip_float(&high_val);
                mid += high_val * fre;
                clip_float(&mid);
                low_val += mid * fre;
                clip_float(&low_val);
            }
            for (int i = 0; i <= length_table[waves]; i++) {
                high_val = a0[i] - mid - low_val;
                clip_float(&high_val);
                mid += high_val * fre;
                clip_float(&mid);
                low_val += mid * fre;
                clip_float(&low_val);
                *low++ = (char)low_val;
                *high++ = (char)high_val;
            }
            a0 += length_table[waves] + 1;
        }
    }
}

static void waves_generate_triangle(char* buffer, int len) {
    int d2 = len;
    int d5 = d2 >> 2;
    int d1 = 128 / d5;
    int d4 = -(d2 >> 1);
    char* edi = buffer;
    int eax = 0;

    for (int ecx = 0; ecx < d5; ecx++) {
        *edi++ = eax;
        eax += d1;
    }
    *edi++ = 0x7f;
    if (d5 != 1) {
        eax = 128;
        for (int ecx = 0; ecx < d5-1; ecx++) {
            eax -= d1;
            *edi++ = eax;
        }
    }
    char* esi = edi + d4;
    for (int ecx = 0; ecx < d5*2; ecx++) {
        *edi++ = *esi++;
        if (edi[-1] == 0x7f) edi[-1] = 0x80;
        else edi[-1] = -edi[-1];
    }
}

static void waves_generate_square(char* buffer) {
    char* edi = buffer;
    for (int ebx = 1; ebx <= 0x20; ebx++) {
        for (int ecx = 0; ecx < (0x40-ebx)*2; ecx++) *edi++ = 0x80;
        for (int ecx = 0; ecx < ebx*2; ecx++) *edi++ = 0x7f;
    }
}

static void waves_generate_sawtooth(char* buffer, int len) {
    char* edi = buffer;
    int ebx = 256 / (len-1);
    int eax = -128;
    for (int ecx = 0; ecx < len; ecx++) {
        *edi++ = eax;
        eax += ebx;
    }
}

static void waves_generate_white_noise(char* buffer, int len) {
    memcpy(buffer, WhiteNoiseBig, len);
}

static void waves_generate(AhxWaves* waves) {
    waves_generate_sawtooth(waves->Sawtooth04, 0x04);
    waves_generate_sawtooth(waves->Sawtooth08, 0x08);
    waves_generate_sawtooth(waves->Sawtooth10, 0x10);
    waves_generate_sawtooth(waves->Sawtooth20, 0x20);
    waves_generate_sawtooth(waves->Sawtooth40, 0x40);
    waves_generate_sawtooth(waves->Sawtooth80, 0x80);
    waves_generate_triangle(waves->Triangle04, 0x04);
    waves_generate_triangle(waves->Triangle08, 0x08);
    waves_generate_triangle(waves->Triangle10, 0x10);
    waves_generate_triangle(waves->Triangle20, 0x20);
    waves_generate_triangle(waves->Triangle40, 0x40);
    waves_generate_triangle(waves->Triangle80, 0x80);
    waves_generate_square(waves->Squares);
    waves_generate_white_noise(waves->WhiteNoiseBig, 0x280*3);
    waves_generate_filter_waveforms(waves->Triangle04, waves->LowPasses, waves->HighPasses);
}

// Initialize volume table
static void init_volume_table(AhxPlayer* player, float boost) {
    for (int i = 0; i < 65; i++) {
        for (int j = -128; j < 128; j++) {
            player->VolumeTable[i][j+128] = (int)(i * j * boost) / 64;
        }
    }
    player->Boost = boost;
}

// Song loading
static int load_song(AhxPlayer* player, const uint8_t* buffer, size_t len) {
    const uint8_t* song_buffer = buffer;
    const uint8_t* sb_ptr = &song_buffer[14];

    if (len < 14) return 0;
    if (song_buffer[0] != 'T' || song_buffer[1] != 'H' || song_buffer[2] != 'X') return 0;

    player->Song.Revision = song_buffer[3];
    if (player->Song.Revision > 1) return 0;

    // Header
    const char* name_ptr = (const char*)&song_buffer[(song_buffer[4]<<8) | song_buffer[5]];
    player->Song.Name = malloc(strlen(name_ptr) + 1);
    strcpy(player->Song.Name, name_ptr);
    name_ptr += strlen(name_ptr) + 1;

    player->Song.SpeedMultiplier = ((song_buffer[6]>>5)&3)+1;
    player->Song.PositionNr = ((song_buffer[6]&0xf)<<8) | song_buffer[7];
    player->Song.Restart = (song_buffer[8]<<8) | song_buffer[9];
    player->Song.TrackLength = song_buffer[10];
    player->Song.TrackNr = song_buffer[11];
    player->Song.InstrumentNr = song_buffer[12];
    player->Song.SubsongNr = song_buffer[13];

    // Subsongs
    player->Song.Subsongs = malloc(player->Song.SubsongNr * sizeof(int));
    for (int i = 0; i < player->Song.SubsongNr; i++) {
        if (sb_ptr - song_buffer > (int)len) return 0;
        player->Song.Subsongs[i] = (sb_ptr[0]<<8)|sb_ptr[1];
        sb_ptr += 2;
    }

    // Position List
    player->Song.Positions = malloc(player->Song.PositionNr * sizeof(AhxPosition));
    for (int i = 0; i < player->Song.PositionNr; i++) {
        for (int j = 0; j < 4; j++) {
            if (sb_ptr - song_buffer > (int)len) return 0;
            player->Song.Positions[i].Track[j] = *sb_ptr++;
            player->Song.Positions[i].Transpose[j] = *(signed char*)sb_ptr++;
        }
    }

    // Tracks
    int max_track = player->Song.TrackNr;
    player->Song.Tracks = malloc((max_track+1) * sizeof(AhxStep*));
    for (int i = 0; i < max_track+1; i++) {
        player->Song.Tracks[i] = malloc(player->Song.TrackLength * sizeof(AhxStep));
        if ((song_buffer[6]&0x80)==0x80 && i==0) {
            memset(player->Song.Tracks[i], 0, player->Song.TrackLength * sizeof(AhxStep));
            continue;
        }
        for (int j = 0; j < player->Song.TrackLength; j++) {
            if (sb_ptr - song_buffer > (int)len) return 0;
            player->Song.Tracks[i][j].Note = (sb_ptr[0]>>2)&0x3f;
            player->Song.Tracks[i][j].Instrument = ((sb_ptr[0]&0x3)<<4) | (sb_ptr[1]>>4);
            player->Song.Tracks[i][j].FX = sb_ptr[1]&0xf;
            player->Song.Tracks[i][j].FXParam = sb_ptr[2];
            sb_ptr += 3;
        }
    }

    // Instruments
    player->Song.Instruments = calloc(player->Song.InstrumentNr+1, sizeof(AhxInstrument));
    for (int i = 1; i < player->Song.InstrumentNr+1; i++) {
        player->Song.Instruments[i].Name = malloc(strlen(name_ptr)+1);
        strcpy(player->Song.Instruments[i].Name, name_ptr);
        name_ptr += strlen(name_ptr)+1;

        if (sb_ptr - song_buffer > (int)len) return 0;
        player->Song.Instruments[i].Volume = sb_ptr[0];
        player->Song.Instruments[i].FilterSpeed = ((sb_ptr[1]>>3)&0x1f) | ((sb_ptr[12]>>2)&0x20);
        player->Song.Instruments[i].WaveLength = sb_ptr[1]&0x7;
        player->Song.Instruments[i].Envelope.aFrames = sb_ptr[2];
        player->Song.Instruments[i].Envelope.aVolume = sb_ptr[3];
        player->Song.Instruments[i].Envelope.dFrames = sb_ptr[4];
        player->Song.Instruments[i].Envelope.dVolume = sb_ptr[5];
        player->Song.Instruments[i].Envelope.sFrames = sb_ptr[6];
        player->Song.Instruments[i].Envelope.rFrames = sb_ptr[7];
        player->Song.Instruments[i].Envelope.rVolume = sb_ptr[8];
        player->Song.Instruments[i].FilterLowerLimit = sb_ptr[12]&0x7f;
        player->Song.Instruments[i].VibratoDelay = sb_ptr[13];
        player->Song.Instruments[i].HardCutReleaseFrames = (sb_ptr[14]>>4)&7;
        player->Song.Instruments[i].HardCutRelease = sb_ptr[14]&0x80?1:0;
        player->Song.Instruments[i].VibratoDepth = sb_ptr[14]&0xf;
        player->Song.Instruments[i].VibratoSpeed = sb_ptr[15];
        player->Song.Instruments[i].SquareLowerLimit = sb_ptr[16];
        player->Song.Instruments[i].SquareUpperLimit = sb_ptr[17];
        player->Song.Instruments[i].SquareSpeed = sb_ptr[18];
        player->Song.Instruments[i].FilterUpperLimit = sb_ptr[19]&0x3f;
        player->Song.Instruments[i].PList.Speed = sb_ptr[20];
        player->Song.Instruments[i].PList.Length = sb_ptr[21];
        sb_ptr += 22;

        player->Song.Instruments[i].PList.Entries = malloc(player->Song.Instruments[i].PList.Length * sizeof(AhxPListEntry));
        for (int j = 0; j < player->Song.Instruments[i].PList.Length; j++) {
            if (sb_ptr - song_buffer > (int)len) return 0;
            player->Song.Instruments[i].PList.Entries[j].FX[1] = (sb_ptr[0]>>5)&7;
            player->Song.Instruments[i].PList.Entries[j].FX[0] = (sb_ptr[0]>>2)&7;
            player->Song.Instruments[i].PList.Entries[j].Waveform = ((sb_ptr[0]<<1)&6) | (sb_ptr[1]>>7);
            player->Song.Instruments[i].PList.Entries[j].Fixed = (sb_ptr[1]>>6)&1;
            player->Song.Instruments[i].PList.Entries[j].Note = sb_ptr[1]&0x3f;
            player->Song.Instruments[i].PList.Entries[j].FXParam[0] = sb_ptr[2];
            player->Song.Instruments[i].PList.Entries[j].FXParam[1] = sb_ptr[3];
            sb_ptr += 4;
        }
    }

    return 1;
}

// Init subsong
static void init_subsong(AhxPlayer* player, int nr) {
    if (nr > player->Song.SubsongNr) nr = 0;

    if (nr == 0) player->PosNr = 0;
    else player->PosNr = player->Song.Subsongs[nr-1];

    player->PosJump = 0;
    player->PatternBreak = 0;
    player->MainVolume = 0x40;
    player->Playing = 1;
    player->NoteNr = player->PosJumpNote = 0;
    player->Tempo = 6;
    player->StepWaitFrames = 0;
    player->GetNewPosition = 1;
    player->SongEndReached = 0;
    player->TimingValue = player->PlayingTime = 0;

    for (int v = 0; v < 4; v++) {
        voice_init(&player->Voices[v]);
    }

    // Set HVL-style panning (stereo mode 2 = standard stereo)
    int defpanleft = stereopan_left[2];   // 64
    int defpanright = stereopan_right[2]; // 193

    player->Voices[0].PanMultLeft = player->panning_left[defpanleft];
    player->Voices[0].PanMultRight = player->panning_right[defpanleft];
    player->Voices[1].PanMultLeft = player->panning_left[defpanright];
    player->Voices[1].PanMultRight = player->panning_right[defpanright];
    player->Voices[2].PanMultLeft = player->panning_left[defpanright];
    player->Voices[2].PanMultRight = player->panning_right[defpanright];
    player->Voices[3].PanMultLeft = player->panning_left[defpanleft];
    player->Voices[3].PanMultRight = player->panning_right[defpanleft];
}

// ProcessStep - from AHX.cpp line 399
static void player_process_step(AhxPlayer* player, int v) {
    if (!player->Voices[v].TrackOn) return;
    player->Voices[v].VolumeSlideUp = player->Voices[v].VolumeSlideDown = 0;

    int note = player->Song.Tracks[player->Song.Positions[player->PosNr].Track[v]][player->NoteNr].Note;
    int instrument = player->Song.Tracks[player->Song.Positions[player->PosNr].Track[v]][player->NoteNr].Instrument;
    int fx = player->Song.Tracks[player->Song.Positions[player->PosNr].Track[v]][player->NoteNr].FX;
    int fx_param = player->Song.Tracks[player->Song.Positions[player->PosNr].Track[v]][player->NoteNr].FXParam;

    switch (fx) {
        case 0x0: // Position Jump HI
            if ((fx_param & 0xf) > 0 && (fx_param & 0xf) <= 9)
                player->PosJump = fx_param & 0xf;
            break;
        case 0x5: // Volume Slide + Tone Portamento
        case 0xa: // Volume Slide
            player->Voices[v].VolumeSlideDown = fx_param & 0x0f;
            player->Voices[v].VolumeSlideUp = fx_param >> 4;
            break;
        case 0xb: // Position Jump
            player->PosJump = player->PosJump*100 + (fx_param & 0x0f) + (fx_param >> 4)*10;
            player->PatternBreak = 1;
            break;
        case 0xd: // Patternbreak
            player->PosJump = player->PosNr + 1;
            player->PosJumpNote = (fx_param & 0x0f) + (fx_param >> 4)*10;
            if (player->PosJumpNote > player->Song.TrackLength) player->PosJumpNote = 0;
            player->PatternBreak = 1;
            break;
        case 0xe: // Enhanced commands
            switch (fx_param >> 4) {
                case 0xc: // Note Cut
                    if ((fx_param & 0x0f) < player->Tempo) {
                        player->Voices[v].NoteCutWait = fx_param & 0x0f;
                        if (player->Voices[v].NoteCutWait) {
                            player->Voices[v].NoteCutOn = 1;
                            player->Voices[v].HardCutRelease = 0;
                        }
                    }
                    break;
                case 0xd: // Note Delay
                    if (player->Voices[v].NoteDelayOn) {
                        player->Voices[v].NoteDelayOn = 0;
                    } else {
                        if ((fx_param & 0x0f) < player->Tempo) {
                            player->Voices[v].NoteDelayWait = fx_param & 0x0f;
                            if (player->Voices[v].NoteDelayWait) {
                                player->Voices[v].NoteDelayOn = 1;
                                return;
                            }
                        }
                    }
                    break;
            }
            break;
        case 0xf: // Speed
            player->Tempo = fx_param;
            break;
    }

    if (instrument) {
        player->Voices[v].PerfSubVolume = 0x40;
        player->Voices[v].PeriodSlideSpeed = player->Voices[v].PeriodSlidePeriod = player->Voices[v].PeriodSlideLimit = 0;
        player->Voices[v].ADSRVolume = 0;
        player->Voices[v].Instrument = &player->Song.Instruments[instrument];
        voice_calc_adsr(&player->Voices[v]);

        // InitOnInstrument
        player->Voices[v].WaveLength = player->Voices[v].Instrument->WaveLength;
        player->Voices[v].NoteMaxVolume = player->Voices[v].Instrument->Volume;

        // InitVibrato
        player->Voices[v].VibratoCurrent = 0;
        player->Voices[v].VibratoDelay = player->Voices[v].Instrument->VibratoDelay;
        player->Voices[v].VibratoDepth = player->Voices[v].Instrument->VibratoDepth;
        player->Voices[v].VibratoSpeed = player->Voices[v].Instrument->VibratoSpeed;
        player->Voices[v].VibratoPeriod = 0;

        // InitHardCut
        player->Voices[v].HardCutRelease = player->Voices[v].Instrument->HardCutRelease;
        player->Voices[v].HardCut = player->Voices[v].Instrument->HardCutReleaseFrames;

        // InitSquare
        player->Voices[v].IgnoreSquare = player->Voices[v].SquareSlidingIn = 0;
        player->Voices[v].SquareWait = player->Voices[v].SquareOn = 0;
        int square_lower = player->Voices[v].Instrument->SquareLowerLimit >> (5-player->Voices[v].WaveLength);
        int square_upper = player->Voices[v].Instrument->SquareUpperLimit >> (5-player->Voices[v].WaveLength);
        if (square_upper < square_lower) {
            int t = square_upper;
            square_upper = square_lower;
            square_lower = t;
        }
        player->Voices[v].SquareUpperLimit = square_upper;
        player->Voices[v].SquareLowerLimit = square_lower;

        // Initialize generic square modulator
        tracker_modulator_set_limits(&player->Voices[v].square_mod, square_lower, square_upper);
        tracker_modulator_set_position(&player->Voices[v].square_mod, 0);
        tracker_modulator_set_active(&player->Voices[v].square_mod, false);

        // InitFilter
        player->Voices[v].IgnoreFilter = player->Voices[v].FilterWait = player->Voices[v].FilterOn = 0;
        player->Voices[v].FilterSlidingIn = 0;
        int d6 = player->Voices[v].Instrument->FilterSpeed;
        int d3 = player->Voices[v].Instrument->FilterLowerLimit;
        int d4 = player->Voices[v].Instrument->FilterUpperLimit;
        if (d3 & 0x80) d6 |= 0x20;
        if (d4 & 0x80) d6 |= 0x40;
        player->Voices[v].FilterSpeed = d6;
        d3 &= ~0x80;
        d4 &= ~0x80;
        if (d3 > d4) {
            int t = d3;
            d3 = d4;
            d4 = t;
        }
        player->Voices[v].FilterUpperLimit = d4;
        player->Voices[v].FilterLowerLimit = d3;
        player->Voices[v].FilterPos = 32;

        // Initialize generic filter modulator
        tracker_modulator_set_limits(&player->Voices[v].filter_mod, d3, d4);
        tracker_modulator_set_position(&player->Voices[v].filter_mod, 32);
        tracker_modulator_set_active(&player->Voices[v].filter_mod, false);

        // Init PerfList
        player->Voices[v].PerfWait = player->Voices[v].PerfCurrent = 0;
        player->Voices[v].PerfSpeed = player->Voices[v].Instrument->PList.Speed;
        player->Voices[v].PerfList = &player->Voices[v].Instrument->PList;

        // Initialize generic sequence (pointing to existing PList data)
        // Note: We cast AhxPListEntry to TrackerSequenceEntry - they have compatible layout
        player->Voices[v].plist_seq.entries = (TrackerSequenceEntry*)player->Voices[v].Instrument->PList.Entries;
        player->Voices[v].plist_seq.length = player->Voices[v].Instrument->PList.Length;
        player->Voices[v].plist_seq.speed = player->Voices[v].Instrument->PList.Speed;
        player->Voices[v].plist_seq.current = 0;
        player->Voices[v].plist_seq.wait = player->Voices[v].Instrument->PList.Speed;
        player->Voices[v].plist_seq.active = true;
    }

    // NoInstrument
    player->Voices[v].PeriodSlideOn = 0;

    switch (fx) {
        case 0x4: // Override filter
            break;
        case 0x9: // Set Squarewave-Offset
            player->Voices[v].SquarePos = fx_param >> (5 - player->Voices[v].WaveLength);
            player->Voices[v].PlantSquare = 1;
            player->Voices[v].IgnoreSquare = 1;

            // Sync position to generic square modulator
            tracker_modulator_set_position(&player->Voices[v].square_mod, player->Voices[v].SquarePos);
            break;
        case 0x5: // Tone Portamento + Volume Slide
        case 0x3: // Tone Portamento
            if (fx_param != 0) player->Voices[v].PeriodSlideSpeed = fx_param;
            if (note) {
                int neue = PeriodTable[note];
                int alte = PeriodTable[player->Voices[v].TrackPeriod];
                alte -= neue;
                neue = alte + player->Voices[v].PeriodSlidePeriod;
                if (neue) player->Voices[v].PeriodSlideLimit = -alte;
            }
            player->Voices[v].PeriodSlideOn = 1;
            player->Voices[v].PeriodSlideWithLimit = 1;
            goto no_note;
    }

    // Note trigger
    if (note) {
        player->Voices[v].TrackPeriod = note;
        player->Voices[v].PlantPeriod = 1;
    }

no_note:
    switch (fx) {
        case 0x1: // Portamento up
            player->Voices[v].PeriodSlideSpeed = -fx_param;
            player->Voices[v].PeriodSlideOn = 1;
            player->Voices[v].PeriodSlideWithLimit = 0;
            break;
        case 0x2: // Portamento down
            player->Voices[v].PeriodSlideSpeed = fx_param;
            player->Voices[v].PeriodSlideOn = 1;
            player->Voices[v].PeriodSlideWithLimit = 0;
            break;
        case 0xc: // Volume
            if (fx_param <= 0x40) {
                player->Voices[v].NoteMaxVolume = fx_param;
            } else {
                fx_param -= 0x50;
                if (fx_param <= 0x40) {
                    for (int i = 0; i < 4; i++)
                        player->Voices[i].TrackMasterVolume = fx_param;
                } else {
                    fx_param -= 0xa0 - 0x50;
                    if (fx_param <= 0x40)
                        player->Voices[v].TrackMasterVolume = fx_param;
                }
            }
            break;
        case 0xe: // Enhanced commands
            switch (fx_param >> 4) {
                case 0x1: // Fineslide up
                    player->Voices[v].PeriodSlidePeriod = -(fx_param & 0x0f);
                    player->Voices[v].PlantPeriod = 1;
                    break;
                case 0x2: // Fineslide down
                    player->Voices[v].PeriodSlidePeriod = fx_param & 0x0f;
                    player->Voices[v].PlantPeriod = 1;
                    break;
                case 0x4: // Vibrato control
                    player->Voices[v].VibratoDepth = fx_param & 0x0f;
                    break;
                case 0xa: // Finevolume up
                    player->Voices[v].NoteMaxVolume += fx_param & 0x0f;
                    if (player->Voices[v].NoteMaxVolume > 0x40)
                        player->Voices[v].NoteMaxVolume = 0x40;
                    break;
                case 0xb: // Finevolume down
                    player->Voices[v].NoteMaxVolume -= fx_param & 0x0f;
                    if (player->Voices[v].NoteMaxVolume < 0)
                        player->Voices[v].NoteMaxVolume = 0;
                    break;
            }
            break;
    }
}

// ProcessFrame - from AHX.cpp line 588
static void player_process_frame(AhxPlayer* player, int v) {
    if (!player->Voices[v].TrackOn) return;

    if (player->Voices[v].NoteDelayOn) {
        if (player->Voices[v].NoteDelayWait <= 0)
            player_process_step(player, v);
        else
            player->Voices[v].NoteDelayWait--;
    }

    if (player->Voices[v].HardCut) {
        int next_instrument;
        if (player->NoteNr+1 < player->Song.TrackLength)
            next_instrument = player->Song.Tracks[player->Voices[v].Track][player->NoteNr+1].Instrument;
        else
            next_instrument = player->Song.Tracks[player->Voices[v].NextTrack][0].Instrument;

        if (next_instrument) {
            int d1 = player->Tempo - player->Voices[v].HardCut;
            if (d1 < 0) d1 = 0;
            if (!player->Voices[v].NoteCutOn) {
                player->Voices[v].NoteCutOn = 1;
                player->Voices[v].NoteCutWait = d1;
                player->Voices[v].HardCutReleaseF = -(d1 - player->Tempo);
            } else {
                player->Voices[v].HardCut = 0;
            }
        }
    }

    if (player->Voices[v].NoteCutOn) {
        if (player->Voices[v].NoteCutWait <= 0) {
            player->Voices[v].NoteCutOn = 0;
            if (player->Voices[v].HardCutRelease) {
                player->Voices[v].ADSR.rVolume = -(player->Voices[v].ADSRVolume -
                    (player->Voices[v].Instrument->Envelope.rVolume << 8)) / player->Voices[v].HardCutReleaseF;
                player->Voices[v].ADSR.rFrames = player->Voices[v].HardCutReleaseF;
                player->Voices[v].ADSR.aFrames = player->Voices[v].ADSR.dFrames = player->Voices[v].ADSR.sFrames = 0;
            } else {
                player->Voices[v].NoteMaxVolume = 0;
            }
        } else {
            player->Voices[v].NoteCutWait--;
        }
    }

    // ADSR envelope
    if (player->Voices[v].ADSR.aFrames) {
        player->Voices[v].ADSRVolume += player->Voices[v].ADSR.aVolume;
        if (--player->Voices[v].ADSR.aFrames <= 0)
            player->Voices[v].ADSRVolume = player->Voices[v].Instrument->Envelope.aVolume << 8;
    } else if (player->Voices[v].ADSR.dFrames) {
        player->Voices[v].ADSRVolume += player->Voices[v].ADSR.dVolume;
        if (--player->Voices[v].ADSR.dFrames <= 0)
            player->Voices[v].ADSRVolume = player->Voices[v].Instrument->Envelope.dVolume << 8;
    } else if (player->Voices[v].ADSR.sFrames) {
        player->Voices[v].ADSR.sFrames--;
    } else if (player->Voices[v].ADSR.rFrames) {
        player->Voices[v].ADSRVolume += player->Voices[v].ADSR.rVolume;
        if (--player->Voices[v].ADSR.rFrames <= 0)
            player->Voices[v].ADSRVolume = player->Voices[v].Instrument->Envelope.rVolume << 8;
    }

    // Volume slide
    player->Voices[v].NoteMaxVolume = player->Voices[v].NoteMaxVolume +
        player->Voices[v].VolumeSlideUp - player->Voices[v].VolumeSlideDown;
    if (player->Voices[v].NoteMaxVolume < 0) player->Voices[v].NoteMaxVolume = 0;
    if (player->Voices[v].NoteMaxVolume > 0x40) player->Voices[v].NoteMaxVolume = 0x40;

    // Portamento
    if (player->Voices[v].PeriodSlideOn) {
        if (player->Voices[v].PeriodSlideWithLimit) {
            int d0 = player->Voices[v].PeriodSlidePeriod - player->Voices[v].PeriodSlideLimit;
            int d2 = player->Voices[v].PeriodSlideSpeed;
            if (d0 > 0) d2 = -d2;
            if (d0) {
                int d3 = (d0 + d2) ^ d0;
                if (d3 >= 0)
                    d0 = player->Voices[v].PeriodSlidePeriod + d2;
                else
                    d0 = player->Voices[v].PeriodSlideLimit;
                player->Voices[v].PeriodSlidePeriod = d0;
                player->Voices[v].PlantPeriod = 1;
            }
        } else {
            player->Voices[v].PeriodSlidePeriod += player->Voices[v].PeriodSlideSpeed;
            player->Voices[v].PlantPeriod = 1;
        }
    }

    // Vibrato
    if (player->Voices[v].VibratoDepth) {
        if (player->Voices[v].VibratoDelay <= 0) {
            player->Voices[v].VibratoPeriod = (VibratoTable[player->Voices[v].VibratoCurrent] *
                player->Voices[v].VibratoDepth) >> 7;
            player->Voices[v].PlantPeriod = 1;
            player->Voices[v].VibratoCurrent = (player->Voices[v].VibratoCurrent +
                player->Voices[v].VibratoSpeed) & 0x3f;
        } else {
            player->Voices[v].VibratoDelay--;
        }
    }

    // PList (using generic sequence component)
    const TrackerSequenceEntry* entry = tracker_sequence_update(&player->Voices[v].plist_seq);
    if (entry) {
        // Process waveform change
        if (entry->waveform) {
            player->Voices[v].Waveform = entry->waveform - 1;
            player->Voices[v].NewWaveform = 1;
            player->Voices[v].PeriodPerfSlideSpeed = player->Voices[v].PeriodPerfSlidePeriod = 0;
        }

        // Holdwave
        player->Voices[v].PeriodPerfSlideOn = 0;

        // Execute FX commands
        for (int i = 0; i < 2; i++) {
            player_plist_command_parse(player, v, entry->fx[i], entry->fx_param[i]);
        }

        // GetNote
        if (entry->note) {
            player->Voices[v].InstrPeriod = entry->note;
            player->Voices[v].PlantPeriod = 1;
            player->Voices[v].FixedNote = entry->fixed;
        }

        // Sync old fields for compatibility
        player->Voices[v].PerfCurrent = player->Voices[v].plist_seq.current;
        player->Voices[v].PerfWait = player->Voices[v].plist_seq.wait;
    } else {
        // No entry this frame - handle wait countdown
        if (player->Voices[v].plist_seq.current >= player->Voices[v].plist_seq.length) {
            if (player->Voices[v].PerfWait)
                player->Voices[v].PerfWait--;
            else
                player->Voices[v].PeriodPerfSlideSpeed = 0;
        }

        // Sync old fields for compatibility
        player->Voices[v].PerfWait = player->Voices[v].plist_seq.wait;
    }

    // PerfPortamento
    if (player->Voices[v].PeriodPerfSlideOn) {
        player->Voices[v].PeriodPerfSlidePeriod -= player->Voices[v].PeriodPerfSlideSpeed;
        if (player->Voices[v].PeriodPerfSlidePeriod)
            player->Voices[v].PlantPeriod = 1;
    }

    // Square modulation (using generic tracker modulator)
    if (player->Voices[v].Waveform == 3-1 && player->Voices[v].SquareOn) {
        if (--player->Voices[v].SquareWait <= 0) {
            // Use generic modulator for position updates
            tracker_modulator_update(&player->Voices[v].square_mod);

            // Get updated position
            int d3 = tracker_modulator_get_position(&player->Voices[v].square_mod);

            // Sync old field for compatibility
            player->Voices[v].SquarePos = d3;
            player->Voices[v].PlantSquare = 1;
            player->Voices[v].SquareWait = player->Voices[v].Instrument->SquareSpeed;
        }
    }

    // Filter modulation (using generic tracker modulator)
    if (player->Voices[v].FilterOn && --player->Voices[v].FilterWait <= 0) {
        // Use generic modulator for position updates
        int f_max = (player->Voices[v].FilterSpeed < 4) ? (5 - player->Voices[v].FilterSpeed) : 1;
        for (int i = 0; i < f_max; i++) {
            tracker_modulator_update(&player->Voices[v].filter_mod);
        }

        // Get updated position and clamp for AHX
        int d3 = tracker_modulator_get_position(&player->Voices[v].filter_mod);
        if (d3 < 1) {
            d3 = 1;
            tracker_modulator_set_position(&player->Voices[v].filter_mod, d3);
        }
        if (d3 > 63) {
            d3 = 63;
            tracker_modulator_set_position(&player->Voices[v].filter_mod, d3);
        }

        // Sync old field for compatibility
        player->Voices[v].FilterPos = d3;
        player->Voices[v].NewWaveform = 1;
        player->Voices[v].FilterWait = player->Voices[v].FilterSpeed - 3;
        if (player->Voices[v].FilterWait < 1) player->Voices[v].FilterWait = 1;
    }

    // Calculate square waveform
    if (player->Voices[v].Waveform == 3-1 || player->Voices[v].PlantSquare) {
        char* square_ptr = &player->Waves->Squares[(player->Voices[v].FilterPos-0x20) *
            (0xfc+0xfc+0x80*0x1f+0x80+0x280*3)];
        int x = player->Voices[v].SquarePos << (5 - player->Voices[v].WaveLength);

        if (x > 0x20) {
            x = 0x40 - x;
            player->Voices[v].SquareReverse = 1;
        }

        // OkDownSquare
        if (x > 0)
            square_ptr += (x-1) << 7;
        int delta = 32 >> player->Voices[v].WaveLength;
        player->WaveformTab[2] = player->Voices[v].SquareTempBuffer;

        for (int i = 0; i < (1 << player->Voices[v].WaveLength)*4; i++) {
            player->Voices[v].SquareTempBuffer[i] = *square_ptr;
            square_ptr += delta;
        }
        player->Voices[v].NewWaveform = 1;
        player->Voices[v].Waveform = 3-1;
        player->Voices[v].PlantSquare = 0;
    }

    if (player->Voices[v].Waveform == 4-1) player->Voices[v].NewWaveform = 1;

    if (player->Voices[v].NewWaveform) {
        char* audio_source = player->WaveformTab[player->Voices[v].Waveform];

        if (player->Voices[v].Waveform != 3-1) {
            audio_source += (player->Voices[v].FilterPos-0x20) * (0xfc+0xfc+0x80*0x1f+0x80+0x280*3);
        }

        if (player->Voices[v].Waveform < 3-1) {
            // GetWLWaveformlor2
            static int offsets[] = {0x00, 0x04, 0x04+0x08, 0x04+0x08+0x10, 0x04+0x08+0x10+0x20, 0x04+0x08+0x10+0x20+0x40};
            audio_source += offsets[player->Voices[v].WaveLength];
        }

        if (player->Voices[v].Waveform == 4-1) {
            // AddRandomMoving
            audio_source += (player->Voices[v].WNRandom & (2*0x280-1)) & ~1;
            // GoOnRandom
            player->Voices[v].WNRandom += 2239384;
            player->Voices[v].WNRandom = ((((player->Voices[v].WNRandom >> 8) | (player->Voices[v].WNRandom << 24)) + 782323) ^ 75) - 6735;
        }
        player->Voices[v].AudioSource = audio_source;
    }

    // AudioInitPeriod
    player->Voices[v].AudioPeriod = player->Voices[v].InstrPeriod;
    if (!player->Voices[v].FixedNote)
        player->Voices[v].AudioPeriod += player->Voices[v].Transpose + player->Voices[v].TrackPeriod - 1;
    if (player->Voices[v].AudioPeriod > 5*12) player->Voices[v].AudioPeriod = 5*12;
    if (player->Voices[v].AudioPeriod < 0) player->Voices[v].AudioPeriod = 0;
    player->Voices[v].AudioPeriod = PeriodTable[player->Voices[v].AudioPeriod];
    if (!player->Voices[v].FixedNote)
        player->Voices[v].AudioPeriod += player->Voices[v].PeriodSlidePeriod;
    player->Voices[v].AudioPeriod += player->Voices[v].PeriodPerfSlidePeriod + player->Voices[v].VibratoPeriod;
    if (player->Voices[v].AudioPeriod > 0x0d60) player->Voices[v].AudioPeriod = 0x0d60;
    if (player->Voices[v].AudioPeriod < 0x0071) player->Voices[v].AudioPeriod = 0x0071;

    // AudioInitVolume
    player->Voices[v].AudioVolume = ((((((((player->Voices[v].ADSRVolume >> 8) *
        player->Voices[v].NoteMaxVolume) >> 6) * player->Voices[v].PerfSubVolume) >> 6) *
        player->Voices[v].TrackMasterVolume) >> 6) * player->MainVolume) >> 6;
}

// SetAudio - from AHX.cpp line 808
static void player_set_audio(AhxPlayer* player, int v) {
    if (!player->Voices[v].TrackOn) {
        player->Voices[v].VoiceVolume = 0;
        return;
    }

    player->Voices[v].VoiceVolume = player->Voices[v].AudioVolume;

    if (player->Voices[v].PlantPeriod) {
        player->Voices[v].PlantPeriod = 0;
        player->Voices[v].VoicePeriod = player->Voices[v].AudioPeriod;
    }

    // Calculate delta for mixing (HVL: done once per frame, not per sample!)
    if (player->Voices[v].VoicePeriod) {
        double freq = Period2Freq(player->Voices[v].VoicePeriod);
        uint32_t delta = (uint32_t)(freq / (double)player->current_sample_rate);
        if (delta > (0x280 << 16)) delta -= (0x280 << 16);
        if (delta == 0) delta = 1;
        player->Voices[v].Delta = delta;
    }

    if (player->Voices[v].NewWaveform) {
        if (player->Voices[v].Waveform == 4-1) {
            memcpy(player->Voices[v].VoiceBuffer, player->Voices[v].AudioSource, 0x280);
        } else {
            int wave_loops = (1 << (5 - player->Voices[v].WaveLength)) * 5;
            for (int i = 0; i < wave_loops; i++) {
                memcpy(&player->Voices[v].VoiceBuffer[i*4*(1 << player->Voices[v].WaveLength)],
                    player->Voices[v].AudioSource, 4*(1 << player->Voices[v].WaveLength));
            }
        }
        player->Voices[v].VoiceBuffer[0x280] = player->Voices[v].VoiceBuffer[0];
    }
}

// PListCommandParse - from AHX.cpp line 831
static void player_plist_command_parse(AhxPlayer* player, int v, int fx, int fx_param) {
    switch (fx) {
        case 0:
            if (player->Song.Revision > 0 && fx_param != 0) {
                if (player->Voices[v].IgnoreFilter) {
                    player->Voices[v].FilterPos = player->Voices[v].IgnoreFilter;
                    player->Voices[v].IgnoreFilter = 0;
                } else {
                    player->Voices[v].FilterPos = fx_param;
                }
                player->Voices[v].NewWaveform = 1;

                // Sync position to generic filter modulator
                tracker_modulator_set_position(&player->Voices[v].filter_mod, player->Voices[v].FilterPos);
            }
            break;
        case 1:
            player->Voices[v].PeriodPerfSlideSpeed = fx_param;
            player->Voices[v].PeriodPerfSlideOn = 1;
            break;
        case 2:
            player->Voices[v].PeriodPerfSlideSpeed = -fx_param;
            player->Voices[v].PeriodPerfSlideOn = 1;
            break;
        case 3: // Init Square Modulation
            if (!player->Voices[v].IgnoreSquare) {
                player->Voices[v].SquarePos = fx_param >> (5 - player->Voices[v].WaveLength);

                // Sync position to generic square modulator
                tracker_modulator_set_position(&player->Voices[v].square_mod, player->Voices[v].SquarePos);
            } else {
                player->Voices[v].IgnoreSquare = 0;
            }
            break;
        case 4: // Start/Stop Modulation
            if (player->Song.Revision == 0 || fx_param == 0) {
                player->Voices[v].SquareInit = (player->Voices[v].SquareOn ^= 1);
                player->Voices[v].SquareSign = 1;

                // Activate generic square modulator (direction determined automatically by init)
                tracker_modulator_set_active(&player->Voices[v].square_mod, player->Voices[v].SquareOn);
            } else {
                if (fx_param & 0x0f) {
                    player->Voices[v].SquareInit = (player->Voices[v].SquareOn ^= 1);
                    player->Voices[v].SquareSign = 1;
                    if ((fx_param & 0x0f) == 0x0f)
                        player->Voices[v].SquareSign = -1;

                    // Activate generic square modulator (direction determined automatically by init)
                    tracker_modulator_set_active(&player->Voices[v].square_mod, player->Voices[v].SquareOn);
                }
                if (fx_param & 0xf0) {
                    player->Voices[v].FilterInit = (player->Voices[v].FilterOn ^= 1);
                    player->Voices[v].FilterSign = 1;
                    if ((fx_param & 0xf0) == 0xf0)
                        player->Voices[v].FilterSign = -1;

                    // Activate/configure generic filter modulator
                    tracker_modulator_set_active(&player->Voices[v].filter_mod, player->Voices[v].FilterOn);
                    tracker_modulator_set_direction(&player->Voices[v].filter_mod, player->Voices[v].FilterSign);
                }
            }
            break;
        case 5: // Jump to Step
            player->Voices[v].PerfCurrent = fx_param;
            tracker_sequence_jump(&player->Voices[v].plist_seq, fx_param);
            break;
        case 6: // Set Volume
            if (fx_param > 0x40) {
                if ((fx_param -= 0x50) >= 0) {
                    if (fx_param <= 0x40) {
                        player->Voices[v].PerfSubVolume = fx_param;
                    } else {
                        if ((fx_param -= 0xa0-0x50) >= 0) {
                            if (fx_param <= 0x40)
                                player->Voices[v].TrackMasterVolume = fx_param;
                        }
                    }
                }
            } else {
                player->Voices[v].NoteMaxVolume = fx_param;
            }
            break;
        case 7: // Set speed
            player->Voices[v].PerfSpeed = player->Voices[v].PerfWait = fx_param;
            tracker_sequence_set_speed(&player->Voices[v].plist_seq, fx_param);
            break;
    }
}

// PlayIRQ - from AHX.cpp line 338
static void player_play_irq(AhxPlayer* player) {
    if (player->StepWaitFrames <= 0) {
        if (player->GetNewPosition) {
            int next_pos = (player->PosNr+1 == player->Song.PositionNr) ? 0 : (player->PosNr+1);
            for (int i = 0; i < 4; i++) {
                player->Voices[i].Track = player->Song.Positions[player->PosNr].Track[i];
                player->Voices[i].Transpose = player->Song.Positions[player->PosNr].Transpose[i];
                player->Voices[i].NextTrack = player->Song.Positions[next_pos].Track[i];
                player->Voices[i].NextTranspose = player->Song.Positions[next_pos].Transpose[i];
            }
            player->GetNewPosition = 0;
        }

        for (int i = 0; i < 4; i++)
            player_process_step(player, i);
        player->StepWaitFrames = player->Tempo;
    }

    // DoFrameStuff
    for (int i = 0; i < 4; i++)
        player_process_frame(player, i);

    player->PlayingTime++;

    if (player->Tempo > 0 && --player->StepWaitFrames <= 0) {
        if (!player->PatternBreak) {
            player->NoteNr++;
            if (player->NoteNr >= player->Song.TrackLength) {
                player->PosJump = player->PosNr + 1;
                player->PosJumpNote = 0;
                player->PatternBreak = 1;
            }
        }

        if (player->PatternBreak) {
            player->PatternBreak = 0;
            player->NoteNr = player->PosJumpNote;
            player->PosJumpNote = 0;
            player->PosNr = player->PosJump;
            player->PosJump = 0;

            if (player->PosNr == player->Song.PositionNr) {
                player->SongEndReached = 1;
                player->PosNr = player->Song.Restart;
                if (player->disable_looping) {
                    player->Playing = 0;
                }
            }
            player->GetNewPosition = 1;
        }
    }

    // RemainPosition
    for (int a = 0; a < 4; a++)
        player_set_audio(player, a);

    // Call position callback if position changed
    if (player->position_callback &&
        (player->last_position != player->PosNr || player->last_row != player->NoteNr)) {
        player->last_position = player->PosNr;
        player->last_row = player->NoteNr;
        player->position_callback(0, player->PosNr, player->NoteNr, player->position_callback_userdata);
    }
}

// Public API implementations
AhxPlayer* ahx_player_create(void) {
    AhxPlayer* player = calloc(1, sizeof(AhxPlayer));
    if (!player) return NULL;

    // Create waveforms
    player->Waves = malloc(sizeof(AhxWaves));
    if (!player->Waves) {
        free(player);
        return NULL;
    }

    waves_generate(player->Waves);
    player->OurWaves = 1;

    // Initialize waveform table
    player->WaveformTab[0] = player->Waves->Triangle04;
    player->WaveformTab[1] = player->Waves->Sawtooth04;
    player->WaveformTab[3] = player->Waves->WhiteNoiseBig;

    // Initialize volume table (for compatibility, but not used in HVL mode)
    init_volume_table(player, 1.0f);
    player->Oversampling = 1;

    // Initialize HVL panning tables
    gen_panning_tables(player);

    // Initialize HVL mixgain (stereo mode 2 = standard stereo, defgain = 76)
    const int defgain[] = { 71, 72, 76, 85, 100 };
    player->mixgain = (defgain[2] * 256) / 100;  // = 194

    // Initialize default sample rate
    player->current_sample_rate = 48000;

    // Initialize voices
    for (int i = 0; i < 4; i++) {
        voice_init(&player->Voices[i]);
    }

    return player;
}

void ahx_player_destroy(AhxPlayer* player) {
    if (!player) return;

    // Free song data
    if (player->Song.Name) free(player->Song.Name);
    if (player->Song.Subsongs) free(player->Song.Subsongs);
    if (player->Song.Positions) free(player->Song.Positions);

    if (player->Song.Tracks) {
        for (int i = 0; i < player->Song.TrackNr+1; i++) {
            if (player->Song.Tracks[i]) free(player->Song.Tracks[i]);
        }
        free(player->Song.Tracks);
    }

    if (player->Song.Instruments) {
        for (int i = 1; i < player->Song.InstrumentNr+1; i++) {
            if (player->Song.Instruments[i].Name) free(player->Song.Instruments[i].Name);
            if (player->Song.Instruments[i].PList.Entries) free(player->Song.Instruments[i].PList.Entries);
        }
        free(player->Song.Instruments);
    }

    if (player->OurWaves && player->Waves) {
        free(player->Waves);
    }

    free(player);
}

bool ahx_player_load(AhxPlayer* player, const uint8_t* data, size_t size) {
    if (!player || !data) return false;
    if (!load_song(player, data, size)) return false;

    // Initialize subsong 0 after loading
    init_subsong(player, 0);
    return true;
}

void ahx_player_set_subsong(AhxPlayer* player, uint8_t subsong) {
    if (!player) return;
    init_subsong(player, subsong);
}

uint8_t ahx_player_get_current_subsong(const AhxPlayer* player) {
    if (!player) return 0;
    // Find which subsong we're at
    for (int i = 0; i < player->Song.SubsongNr; i++) {
        if (player->Song.Subsongs[i] == player->PosNr) return i + 1;
    }
    return 0;
}

uint8_t ahx_player_get_num_subsongs(const AhxPlayer* player) {
    if (!player) return 0;
    return player->Song.SubsongNr;
}

const char* ahx_player_get_title(const AhxPlayer* player) {
    if (!player) return NULL;
    return player->Song.Name;
}

void ahx_player_start(AhxPlayer* player) {
    if (!player) return;
    player->Playing = 1;
}

void ahx_player_stop(AhxPlayer* player) {
    if (!player) return;
    player->Playing = 0;
}

bool ahx_player_is_playing(const AhxPlayer* player) {
    if (!player) return false;
    return player->Playing != 0;
}

void ahx_player_set_position_callback(AhxPlayer* player, AhxPositionCallback callback, void* user_data) {
    if (!player) return;
    player->position_callback = callback;
    player->position_callback_userdata = user_data;
}

void ahx_player_get_position(const AhxPlayer* player, uint16_t* position, uint16_t* row) {
    if (!player) return;
    if (position) *position = player->PosNr;
    if (row) *row = player->NoteNr;
}

void ahx_player_process(AhxPlayer* player, float* left, float* right, size_t num_samples, int sample_rate) {
    if (!player || !left || !right) return;

    // Clear output buffers
    memset(left, 0, num_samples * sizeof(float));
    memset(right, 0, num_samples * sizeof(float));

    if (!player->Playing) return;

    // Update sample rate for delta calculations
    player->current_sample_rate = sample_rate;

    int samples_per_frame = sample_rate / 50 / player->Song.SpeedMultiplier;
    size_t output_pos = 0;

    while (output_pos < num_samples) {
        // Check if we need to process next frame (50Hz IRQ)
        if (player->frame_counter <= 0) {
            player_play_irq(player);
            player->frame_counter = samples_per_frame;
        }

        // Mix a chunk up to next frame boundary
        int chunk_samples = MIN((int)(num_samples - output_pos), player->frame_counter);

        // HVL mixing algorithm - exact match to hvl_mixchunk
        // Pre-load voice parameters into local arrays for efficiency
        uint32_t delta[4], pos[4];
        const int8_t* src[4];
        int vol[4], panl[4], panr[4];

        for (int i = 0; i < 4; i++) {
            delta[i] = player->Voices[i].Delta;
            vol[i] = player->Voices[i].VoiceVolume;
            pos[i] = player->pos[i];
            src[i] = (const int8_t*)player->Voices[i].VoiceBuffer;
            panl[i] = player->Voices[i].PanMultLeft;
            panr[i] = player->Voices[i].PanMultRight;
        }

        int samples_left = chunk_samples;
        int out_idx = 0;

        // Outer loop: batch processing to minimize wraparound checks
        while (samples_left > 0) {
            int loops = samples_left;

            // Calculate batch size: minimum samples before ANY voice wraps
            for (int i = 0; i < 4; i++) {
                if (player->channel_muted[i] || vol[i] == 0) continue;

                if (pos[i] >= (0x280 << 16))
                    pos[i] -= 0x280 << 16;

                if (delta[i] > 0) {
                    uint32_t cnt = ((0x280 << 16) - pos[i] - 1) / delta[i] + 1;
                    if (cnt < (uint32_t)loops) loops = cnt;
                }
            }

            samples_left -= loops;

            // Inner loop: process 'loops' samples without any wraparound checks
            for (int l = 0; l < loops; l++) {
                int a = 0, b = 0;

                for (int i = 0; i < 4; i++) {
                    if (player->channel_muted[i] || vol[i] == 0) continue;

                    // Direct multiplication (HVL style)
                    int j = src[i][pos[i] >> 16] * vol[i];

                    // Apply panning
                    a += (j * panl[i]) >> 7;
                    b += (j * panr[i]) >> 7;

                    pos[i] += delta[i];
                }

                // Apply mixgain
                a = (a * player->mixgain) >> 8;
                b = (b * player->mixgain) >> 8;

                // Clip to 16-bit range
                if (a < -32768) a = -32768;
                if (a >  32767) a =  32767;
                if (b < -32768) b = -32768;
                if (b >  32767) b =  32767;

                // Convert to float
                left[output_pos + out_idx] = a / 32768.0f;
                right[output_pos + out_idx] = b / 32768.0f;
                out_idx++;
            }
        }

        // Write back positions
        for (int i = 0; i < 4; i++) {
            player->pos[i] = pos[i];
        }

        output_pos += chunk_samples;
        player->frame_counter -= chunk_samples;
    }
}

void ahx_player_set_channel_mute(AhxPlayer* player, uint8_t channel, bool muted) {
    if (!player || channel >= 4) return;
    player->channel_muted[channel] = muted;
}

bool ahx_player_get_channel_mute(const AhxPlayer* player, uint8_t channel) {
    if (!player || channel >= 4) return false;
    return player->channel_muted[channel];
}

void ahx_player_set_boost(AhxPlayer* player, float boost) {
    if (!player) return;
    init_volume_table(player, boost);
}

void ahx_player_set_oversampling(AhxPlayer* player, bool enabled) {
    if (!player) return;
    player->Oversampling = enabled ? 1 : 0;
}

void ahx_player_set_disable_looping(AhxPlayer* player, bool disable) {
    if (!player) return;
    player->disable_looping = disable;
}
