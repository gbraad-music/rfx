/*
 * MIDIbox SID V2 Compatible SysEx Implementation
 *
 * Copyright (C) 2024
 * SPDX-License-Identifier: ISC
 */

#include "SIDSysEx.h"
#include <cstring>
#include <cstdio>

// ============================================================================
// Helper Functions
// ============================================================================

uint8_t sysex_calculate_checksum(const uint8_t* data, size_t size)
{
    uint8_t checksum = 0;
    for (size_t i = 0; i < size; i++) {
        checksum ^= data[i];
    }
    return checksum & 0x7F;  // Ensure 7-bit
}

bool sysex_verify_checksum(const uint8_t* sysex, size_t sysex_size)
{
    if (sysex_size < SYSEX_HEADER_SIZE + SYSEX_FOOTER_SIZE)
        return false;

    // Calculate checksum of data (everything between header and checksum)
    const uint8_t* data_start = sysex + SYSEX_HEADER_SIZE;
    size_t data_size = sysex_size - SYSEX_HEADER_SIZE - SYSEX_FOOTER_SIZE;

    uint8_t calculated = sysex_calculate_checksum(data_start, data_size);
    uint8_t received = sysex[sysex_size - 2];  // Checksum before F7

    return calculated == received;
}

bool sysex_is_valid_message(const uint8_t* data, size_t size)
{
    if (size < SYSEX_HEADER_SIZE + SYSEX_FOOTER_SIZE)
        return false;

    // Check header
    if (data[0] != SYSEX_START)
        return false;
    if (data[1] != SYSEX_MANUFACTURER_1)
        return false;
    if (data[2] != SYSEX_MANUFACTURER_2)
        return false;
    if (data[3] != SYSEX_MANUFACTURER_3)
        return false;
    if (data[4] != SYSEX_DEVICE_ID)
        return false;
    if (data[5] != SYSEX_FAMILY_ID)
        return false;
    if (data[6] != SYSEX_DEVICE_TYPE)
        return false;

    // Check footer
    if (data[size - 1] != SYSEX_END)
        return false;

    return true;
}

uint8_t sysex_get_command(const uint8_t* sysex, size_t size)
{
    if (!sysex_is_valid_message(sysex, size))
        return 0;

    return sysex[7];  // Command is at byte 7
}

// ============================================================================
// Preset Encoding/Decoding
// ============================================================================

size_t sysex_encode_patch_dump(const SysExPreset* preset,
                                uint8_t bank,
                                uint8_t patch,
                                uint8_t* buffer,
                                size_t buffer_size)
{
    if (!preset || !buffer)
        return 0;

    // Calculate required size
    // Header(8) + Bank(1) + Patch(1) + Name(32) + Params(30) + Checksum(1) + End(1) = 74
    size_t required_size = SYSEX_HEADER_SIZE + 1 + 1 + 32 + 30 + SYSEX_FOOTER_SIZE;

    if (buffer_size < required_size)
        return 0;

    size_t pos = 0;

    // Write header
    buffer[pos++] = SYSEX_START;
    buffer[pos++] = SYSEX_MANUFACTURER_1;
    buffer[pos++] = SYSEX_MANUFACTURER_2;
    buffer[pos++] = SYSEX_MANUFACTURER_3;
    buffer[pos++] = SYSEX_DEVICE_ID;
    buffer[pos++] = SYSEX_FAMILY_ID;
    buffer[pos++] = SYSEX_DEVICE_TYPE;
    buffer[pos++] = SYSEX_CMD_DUMP_PATCH;

    size_t data_start = pos;

    // Write bank and patch
    buffer[pos++] = bank & 0x7F;
    buffer[pos++] = patch & 0x7F;

    // Write name (32 bytes, 7-bit safe ASCII)
    for (int i = 0; i < 32; i++) {
        buffer[pos++] = preset->name[i] & 0x7F;
    }

    // Write parameters (all 7-bit safe)
    buffer[pos++] = preset->voice1Waveform & 0x7F;
    buffer[pos++] = preset->voice1PulseWidth & 0x7F;
    buffer[pos++] = preset->voice1Attack & 0x7F;
    buffer[pos++] = preset->voice1Decay & 0x7F;
    buffer[pos++] = preset->voice1Sustain & 0x7F;
    buffer[pos++] = preset->voice1Release & 0x7F;
    buffer[pos++] = preset->voice1RingMod & 0x7F;
    buffer[pos++] = preset->voice1Sync & 0x7F;

    buffer[pos++] = preset->voice2Waveform & 0x7F;
    buffer[pos++] = preset->voice2PulseWidth & 0x7F;
    buffer[pos++] = preset->voice2Attack & 0x7F;
    buffer[pos++] = preset->voice2Decay & 0x7F;
    buffer[pos++] = preset->voice2Sustain & 0x7F;
    buffer[pos++] = preset->voice2Release & 0x7F;
    buffer[pos++] = preset->voice2RingMod & 0x7F;
    buffer[pos++] = preset->voice2Sync & 0x7F;

    buffer[pos++] = preset->voice3Waveform & 0x7F;
    buffer[pos++] = preset->voice3PulseWidth & 0x7F;
    buffer[pos++] = preset->voice3Attack & 0x7F;
    buffer[pos++] = preset->voice3Decay & 0x7F;
    buffer[pos++] = preset->voice3Sustain & 0x7F;
    buffer[pos++] = preset->voice3Release & 0x7F;
    buffer[pos++] = preset->voice3RingMod & 0x7F;
    buffer[pos++] = preset->voice3Sync & 0x7F;

    buffer[pos++] = preset->filterMode & 0x7F;
    buffer[pos++] = preset->filterCutoff & 0x7F;
    buffer[pos++] = preset->filterResonance & 0x7F;
    buffer[pos++] = preset->filterVoice1 & 0x7F;
    buffer[pos++] = preset->filterVoice2 & 0x7F;
    buffer[pos++] = preset->filterVoice3 & 0x7F;

    buffer[pos++] = preset->volume & 0x7F;

    // Calculate and write checksum
    size_t data_size = pos - data_start;
    uint8_t checksum = sysex_calculate_checksum(&buffer[data_start], data_size);
    buffer[pos++] = checksum;

    // Write end
    buffer[pos++] = SYSEX_END;

    return pos;
}

bool sysex_decode_patch(const uint8_t* sysex,
                        size_t sysex_size,
                        SysExPreset* preset,
                        uint8_t* bank,
                        uint8_t* patch)
{
    if (!sysex || !preset || !bank || !patch)
        return false;

    // Validate message
    if (!sysex_is_valid_message(sysex, sysex_size))
        return false;

    if (sysex_get_command(sysex, sysex_size) != SYSEX_CMD_DUMP_PATCH &&
        sysex_get_command(sysex, sysex_size) != SYSEX_CMD_LOAD_PATCH)
        return false;

    // Verify checksum
    if (!sysex_verify_checksum(sysex, sysex_size))
        return false;

    size_t pos = SYSEX_HEADER_SIZE;

    // Read bank and patch
    *bank = sysex[pos++];
    *patch = sysex[pos++];

    // Read name
    for (int i = 0; i < 32; i++) {
        preset->name[i] = sysex[pos++];
    }

    // Read parameters
    preset->voice1Waveform = sysex[pos++];
    preset->voice1PulseWidth = sysex[pos++];
    preset->voice1Attack = sysex[pos++];
    preset->voice1Decay = sysex[pos++];
    preset->voice1Sustain = sysex[pos++];
    preset->voice1Release = sysex[pos++];
    preset->voice1RingMod = sysex[pos++];
    preset->voice1Sync = sysex[pos++];

    preset->voice2Waveform = sysex[pos++];
    preset->voice2PulseWidth = sysex[pos++];
    preset->voice2Attack = sysex[pos++];
    preset->voice2Decay = sysex[pos++];
    preset->voice2Sustain = sysex[pos++];
    preset->voice2Release = sysex[pos++];
    preset->voice2RingMod = sysex[pos++];
    preset->voice2Sync = sysex[pos++];

    preset->voice3Waveform = sysex[pos++];
    preset->voice3PulseWidth = sysex[pos++];
    preset->voice3Attack = sysex[pos++];
    preset->voice3Decay = sysex[pos++];
    preset->voice3Sustain = sysex[pos++];
    preset->voice3Release = sysex[pos++];
    preset->voice3RingMod = sysex[pos++];
    preset->voice3Sync = sysex[pos++];

    preset->filterMode = sysex[pos++];
    preset->filterCutoff = sysex[pos++];
    preset->filterResonance = sysex[pos++];
    preset->filterVoice1 = sysex[pos++];
    preset->filterVoice2 = sysex[pos++];
    preset->filterVoice3 = sysex[pos++];

    preset->volume = sysex[pos++];

    return true;
}

size_t sysex_encode_bank_dump(const SysExPreset* presets,
                               uint8_t bank,
                               uint8_t* buffer,
                               size_t buffer_size)
{
    if (!presets || !buffer)
        return 0;

    // Bank dump = header + bank + 128 presets + checksum + end
    // Each preset = 64 bytes (32 name + 30 params + 2 padding)
    size_t required_size = SYSEX_HEADER_SIZE + 1 + (128 * 64) + SYSEX_FOOTER_SIZE;

    if (buffer_size < required_size)
        return 0;

    size_t pos = 0;

    // Write header
    buffer[pos++] = SYSEX_START;
    buffer[pos++] = SYSEX_MANUFACTURER_1;
    buffer[pos++] = SYSEX_MANUFACTURER_2;
    buffer[pos++] = SYSEX_MANUFACTURER_3;
    buffer[pos++] = SYSEX_DEVICE_ID;
    buffer[pos++] = SYSEX_FAMILY_ID;
    buffer[pos++] = SYSEX_DEVICE_TYPE;
    buffer[pos++] = SYSEX_CMD_DUMP_BANK;

    size_t data_start = pos;

    // Write bank number
    buffer[pos++] = bank & 0x7F;

    // Write all 128 presets
    for (int i = 0; i < 128; i++) {
        const SysExPreset* preset = &presets[i];

        // Name (32 bytes)
        for (int j = 0; j < 32; j++) {
            buffer[pos++] = preset->name[j] & 0x7F;
        }

        // Parameters (30 bytes)
        buffer[pos++] = preset->voice1Waveform & 0x7F;
        buffer[pos++] = preset->voice1PulseWidth & 0x7F;
        buffer[pos++] = preset->voice1Attack & 0x7F;
        buffer[pos++] = preset->voice1Decay & 0x7F;
        buffer[pos++] = preset->voice1Sustain & 0x7F;
        buffer[pos++] = preset->voice1Release & 0x7F;
        buffer[pos++] = preset->voice1RingMod & 0x7F;
        buffer[pos++] = preset->voice1Sync & 0x7F;

        buffer[pos++] = preset->voice2Waveform & 0x7F;
        buffer[pos++] = preset->voice2PulseWidth & 0x7F;
        buffer[pos++] = preset->voice2Attack & 0x7F;
        buffer[pos++] = preset->voice2Decay & 0x7F;
        buffer[pos++] = preset->voice2Sustain & 0x7F;
        buffer[pos++] = preset->voice2Release & 0x7F;
        buffer[pos++] = preset->voice2RingMod & 0x7F;
        buffer[pos++] = preset->voice2Sync & 0x7F;

        buffer[pos++] = preset->voice3Waveform & 0x7F;
        buffer[pos++] = preset->voice3PulseWidth & 0x7F;
        buffer[pos++] = preset->voice3Attack & 0x7F;
        buffer[pos++] = preset->voice3Decay & 0x7F;
        buffer[pos++] = preset->voice3Sustain & 0x7F;
        buffer[pos++] = preset->voice3Release & 0x7F;
        buffer[pos++] = preset->voice3RingMod & 0x7F;
        buffer[pos++] = preset->voice3Sync & 0x7F;

        buffer[pos++] = preset->filterMode & 0x7F;
        buffer[pos++] = preset->filterCutoff & 0x7F;
        buffer[pos++] = preset->filterResonance & 0x7F;
        buffer[pos++] = preset->filterVoice1 & 0x7F;
        buffer[pos++] = preset->filterVoice2 & 0x7F;
        buffer[pos++] = preset->filterVoice3 & 0x7F;

        buffer[pos++] = preset->volume & 0x7F;

        // Padding (2 bytes for 64-byte alignment)
        buffer[pos++] = 0x00;
        buffer[pos++] = 0x00;
    }

    // Calculate and write checksum
    size_t data_size = pos - data_start;
    uint8_t checksum = sysex_calculate_checksum(&buffer[data_start], data_size);
    buffer[pos++] = checksum;

    // Write end
    buffer[pos++] = SYSEX_END;

    return pos;
}

bool sysex_decode_bank(const uint8_t* sysex,
                       size_t sysex_size,
                       SysExPreset* presets,
                       uint8_t* bank)
{
    if (!sysex || !presets || !bank)
        return false;

    // Validate message
    if (!sysex_is_valid_message(sysex, sysex_size))
        return false;

    if (sysex_get_command(sysex, sysex_size) != SYSEX_CMD_DUMP_BANK &&
        sysex_get_command(sysex, sysex_size) != SYSEX_CMD_LOAD_BANK)
        return false;

    // Verify checksum
    if (!sysex_verify_checksum(sysex, sysex_size))
        return false;

    size_t pos = SYSEX_HEADER_SIZE;

    // Read bank number
    *bank = sysex[pos++];

    // Read all 128 presets
    for (int i = 0; i < 128; i++) {
        SysExPreset* preset = &presets[i];

        // Name (32 bytes)
        for (int j = 0; j < 32; j++) {
            preset->name[j] = sysex[pos++];
        }

        // Parameters (30 bytes)
        preset->voice1Waveform = sysex[pos++];
        preset->voice1PulseWidth = sysex[pos++];
        preset->voice1Attack = sysex[pos++];
        preset->voice1Decay = sysex[pos++];
        preset->voice1Sustain = sysex[pos++];
        preset->voice1Release = sysex[pos++];
        preset->voice1RingMod = sysex[pos++];
        preset->voice1Sync = sysex[pos++];

        preset->voice2Waveform = sysex[pos++];
        preset->voice2PulseWidth = sysex[pos++];
        preset->voice2Attack = sysex[pos++];
        preset->voice2Decay = sysex[pos++];
        preset->voice2Sustain = sysex[pos++];
        preset->voice2Release = sysex[pos++];
        preset->voice2RingMod = sysex[pos++];
        preset->voice2Sync = sysex[pos++];

        preset->voice3Waveform = sysex[pos++];
        preset->voice3PulseWidth = sysex[pos++];
        preset->voice3Attack = sysex[pos++];
        preset->voice3Decay = sysex[pos++];
        preset->voice3Sustain = sysex[pos++];
        preset->voice3Release = sysex[pos++];
        preset->voice3RingMod = sysex[pos++];
        preset->voice3Sync = sysex[pos++];

        preset->filterMode = sysex[pos++];
        preset->filterCutoff = sysex[pos++];
        preset->filterResonance = sysex[pos++];
        preset->filterVoice1 = sysex[pos++];
        preset->filterVoice2 = sysex[pos++];
        preset->filterVoice3 = sysex[pos++];

        preset->volume = sysex[pos++];

        // Skip padding
        pos += 2;
    }

    return true;
}

// ============================================================================
// File I/O
// ============================================================================

bool sysex_load_patch_file(const char* filename,
                            SysExPreset* preset,
                            uint8_t* bank,
                            uint8_t* patch)
{
    FILE* file = std::fopen(filename, "rb");
    if (!file)
        return false;

    // Get file size
    std::fseek(file, 0, SEEK_END);
    long file_size = std::ftell(file);
    std::fseek(file, 0, SEEK_SET);

    if (file_size <= 0 || file_size > 65536) {
        std::fclose(file);
        return false;
    }

    // Read file
    uint8_t* buffer = new uint8_t[file_size];
    size_t read = std::fread(buffer, 1, file_size, file);
    std::fclose(file);

    if (read != (size_t)file_size) {
        delete[] buffer;
        return false;
    }

    // Decode
    bool success = sysex_decode_patch(buffer, file_size, preset, bank, patch);
    delete[] buffer;

    return success;
}

bool sysex_save_patch_file(const char* filename,
                            const SysExPreset* preset,
                            uint8_t bank,
                            uint8_t patch)
{
    uint8_t buffer[256];
    size_t size = sysex_encode_patch_dump(preset, bank, patch, buffer, sizeof(buffer));

    if (size == 0)
        return false;

    FILE* file = std::fopen(filename, "wb");
    if (!file)
        return false;

    size_t written = std::fwrite(buffer, 1, size, file);
    std::fclose(file);

    return written == size;
}

bool sysex_load_bank_file(const char* filename,
                           SysExPreset* presets,
                           uint8_t* bank)
{
    FILE* file = std::fopen(filename, "rb");
    if (!file)
        return false;

    // Get file size
    std::fseek(file, 0, SEEK_END);
    long file_size = std::ftell(file);
    std::fseek(file, 0, SEEK_SET);

    if (file_size <= 0 || file_size > 1048576) {  // Max 1MB
        std::fclose(file);
        return false;
    }

    // Read file
    uint8_t* buffer = new uint8_t[file_size];
    size_t read = std::fread(buffer, 1, file_size, file);
    std::fclose(file);

    if (read != (size_t)file_size) {
        delete[] buffer;
        return false;
    }

    // Decode
    bool success = sysex_decode_bank(buffer, file_size, presets, bank);
    delete[] buffer;

    return success;
}

bool sysex_save_bank_file(const char* filename,
                           const SysExPreset* presets,
                           uint8_t bank)
{
    // Allocate buffer for bank dump (128 presets × 64 bytes + overhead)
    size_t buffer_size = SYSEX_HEADER_SIZE + 1 + (128 * 64) + SYSEX_FOOTER_SIZE;
    uint8_t* buffer = new uint8_t[buffer_size];

    size_t size = sysex_encode_bank_dump(presets, bank, buffer, buffer_size);

    if (size == 0) {
        delete[] buffer;
        return false;
    }

    FILE* file = std::fopen(filename, "wb");
    if (!file) {
        delete[] buffer;
        return false;
    }

    size_t written = std::fwrite(buffer, 1, size, file);
    std::fclose(file);
    delete[] buffer;

    return written == size;
}
