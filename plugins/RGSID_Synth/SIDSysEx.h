/*
 * MIDIbox SID V2 Compatible SysEx Format
 *
 * Enables preset dump/load via SysEx messages and .syx files
 * Full bidirectional compatibility with MIDIbox SID V2 hardware
 *
 * Copyright (C) 2024
 * SPDX-License-Identifier: ISC
 */

#ifndef SID_SYSEX_H
#define SID_SYSEX_H

#include <stdint.h>
#include <stddef.h>

// ============================================================================
// MIDIbox SID V2 SysEx Format
// ============================================================================

/*
 * SysEx Message Structure:
 *
 * F0              SysEx start
 * 00 20 32        Manufacturer ID (Educational/Development)
 * 00              Device ID (00 = broadcast/all devices)
 * 7F              Sub-ID (MIDIbox family)
 * 40              Device Type (MIDIbox SID V2)
 * [CMD]           Command byte
 * [DATA...]       Payload (7-bit encoded)
 * [CHECKSUM]      XOR checksum of all data bytes
 * F7              SysEx end
 */

// SysEx header constants
#define SYSEX_START           0xF0
#define SYSEX_END             0xF7
#define SYSEX_MANUFACTURER_1  0x00  // Educational/Research/Development
#define SYSEX_MANUFACTURER_2  0x20
#define SYSEX_MANUFACTURER_3  0x32
#define SYSEX_DEVICE_ID       0x00  // Broadcast (all devices)
#define SYSEX_FAMILY_ID       0x7F  // MIDIbox family
#define SYSEX_DEVICE_TYPE     0x40  // MIDIbox SID V2

// SysEx command bytes
#define SYSEX_CMD_DUMP_PATCH      0x01  // Dump single patch
#define SYSEX_CMD_DUMP_BANK       0x02  // Dump entire bank (128 patches)
#define SYSEX_CMD_DUMP_ALL        0x03  // Dump all banks (1024 patches)
#define SYSEX_CMD_LOAD_PATCH      0x10  // Load single patch
#define SYSEX_CMD_LOAD_BANK       0x11  // Load entire bank
#define SYSEX_CMD_LOAD_ALL        0x12  // Load all banks
#define SYSEX_CMD_REQUEST_PATCH   0x20  // Request patch dump
#define SYSEX_CMD_REQUEST_BANK    0x21  // Request bank dump
#define SYSEX_CMD_REQUEST_ALL     0x22  // Request all banks dump

// Header size: F0 00 20 32 00 7F 40 [CMD] = 8 bytes
#define SYSEX_HEADER_SIZE     8
// Footer size: [CHECKSUM] F7 = 2 bytes
#define SYSEX_FOOTER_SIZE     2

// Preset data size (30 parameters × 7-bit encoded = varies)
// Each float encoded as 14-bit (2 bytes in SysEx)
// 30 params × 2 bytes = 60 bytes data
// + 32 bytes name = 92 bytes total per preset
#define SYSEX_PRESET_DATA_SIZE  92

// ============================================================================
// Preset Structure (matches plugin SIDPreset)
// ============================================================================

struct SysExPreset {
    char name[32];

    // Voice 1
    uint8_t voice1Waveform;      // 0-15
    uint8_t voice1PulseWidth;    // 0-127
    uint8_t voice1Attack;        // 0-127
    uint8_t voice1Decay;         // 0-127
    uint8_t voice1Sustain;       // 0-127
    uint8_t voice1Release;       // 0-127
    uint8_t voice1RingMod;       // 0-127
    uint8_t voice1Sync;          // 0-127

    // Voice 2
    uint8_t voice2Waveform;
    uint8_t voice2PulseWidth;
    uint8_t voice2Attack;
    uint8_t voice2Decay;
    uint8_t voice2Sustain;
    uint8_t voice2Release;
    uint8_t voice2RingMod;
    uint8_t voice2Sync;

    // Voice 3
    uint8_t voice3Waveform;
    uint8_t voice3PulseWidth;
    uint8_t voice3Attack;
    uint8_t voice3Decay;
    uint8_t voice3Sustain;
    uint8_t voice3Release;
    uint8_t voice3RingMod;
    uint8_t voice3Sync;

    // Filter
    uint8_t filterMode;          // 0-3
    uint8_t filterCutoff;        // 0-127
    uint8_t filterResonance;     // 0-127
    uint8_t filterVoice1;        // 0-127
    uint8_t filterVoice2;        // 0-127
    uint8_t filterVoice3;        // 0-127

    // Global
    uint8_t volume;              // 0-127
};

// ============================================================================
// SysEx Functions
// ============================================================================

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Encode single preset to SysEx dump message
 * @param preset Preset data to encode
 * @param bank Bank number (0-7)
 * @param patch Patch number (0-127)
 * @param buffer Output buffer for SysEx message
 * @param buffer_size Size of output buffer
 * @return Number of bytes written, or 0 on error
 */
size_t sysex_encode_patch_dump(const SysExPreset* preset,
                                uint8_t bank,
                                uint8_t patch,
                                uint8_t* buffer,
                                size_t buffer_size);

/**
 * Decode single preset from SysEx message
 * @param sysex SysEx message data
 * @param sysex_size Size of SysEx message
 * @param preset Output preset structure
 * @param bank Output bank number
 * @param patch Output patch number
 * @return true if successfully decoded
 */
bool sysex_decode_patch(const uint8_t* sysex,
                        size_t sysex_size,
                        SysExPreset* preset,
                        uint8_t* bank,
                        uint8_t* patch);

/**
 * Encode entire bank to SysEx dump message
 * @param presets Array of 128 presets
 * @param bank Bank number (0-7)
 * @param buffer Output buffer
 * @param buffer_size Size of output buffer
 * @return Number of bytes written
 */
size_t sysex_encode_bank_dump(const SysExPreset* presets,
                               uint8_t bank,
                               uint8_t* buffer,
                               size_t buffer_size);

/**
 * Decode entire bank from SysEx message
 * @param sysex SysEx message data
 * @param sysex_size Size of SysEx message
 * @param presets Output array (must hold 128 presets)
 * @param bank Output bank number
 * @return true if successfully decoded
 */
bool sysex_decode_bank(const uint8_t* sysex,
                       size_t sysex_size,
                       SysExPreset* presets,
                       uint8_t* bank);

/**
 * Calculate XOR checksum of data
 * @param data Data buffer
 * @param size Data size
 * @return Checksum byte
 */
uint8_t sysex_calculate_checksum(const uint8_t* data, size_t size);

/**
 * Verify SysEx message checksum
 * @param sysex Complete SysEx message
 * @param sysex_size Message size
 * @return true if checksum valid
 */
bool sysex_verify_checksum(const uint8_t* sysex, size_t sysex_size);

/**
 * Check if data is a valid MIDIbox SID V2 SysEx message
 * @param data Data buffer
 * @param size Data size
 * @return true if valid header
 */
bool sysex_is_valid_message(const uint8_t* data, size_t size);

/**
 * Get command byte from SysEx message
 * @param sysex SysEx message
 * @param size Message size
 * @return Command byte, or 0 if invalid
 */
uint8_t sysex_get_command(const uint8_t* sysex, size_t size);

// ============================================================================
// File I/O Functions
// ============================================================================

/**
 * Load preset from .syx file
 * @param filename Path to .syx file
 * @param preset Output preset structure
 * @param bank Output bank number
 * @param patch Output patch number
 * @return true if loaded successfully
 */
bool sysex_load_patch_file(const char* filename,
                            SysExPreset* preset,
                            uint8_t* bank,
                            uint8_t* patch);

/**
 * Save preset to .syx file
 * @param filename Path to .syx file
 * @param preset Preset data
 * @param bank Bank number
 * @param patch Patch number
 * @return true if saved successfully
 */
bool sysex_save_patch_file(const char* filename,
                            const SysExPreset* preset,
                            uint8_t bank,
                            uint8_t patch);

/**
 * Load bank from .syx file
 * @param filename Path to .syx file
 * @param presets Output array (must hold 128 presets)
 * @param bank Output bank number
 * @return true if loaded successfully
 */
bool sysex_load_bank_file(const char* filename,
                           SysExPreset* presets,
                           uint8_t* bank);

/**
 * Save bank to .syx file
 * @param filename Path to .syx file
 * @param presets Array of 128 presets
 * @param bank Bank number
 * @return true if saved successfully
 */
bool sysex_save_bank_file(const char* filename,
                           const SysExPreset* presets,
                           uint8_t bank);

#ifdef __cplusplus
}
#endif

#endif // SID_SYSEX_H
