/*
 * AHX Preset System - Implementation
 */

#include "ahx_preset.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PRESET_MAGIC "AHXP"
#define PRESET_VERSION 1

// Preset file header
typedef struct {
    char magic[4];          // "AHXP"
    uint32_t version;       // Version number
    uint32_t reserved[2];   // Reserved for future use
} PresetHeader;

// Save preset to file
bool ahx_preset_save(const AhxPreset* preset, const char* filepath) {
    if (!preset || !filepath) return false;

    FILE* f = fopen(filepath, "wb");
    if (!f) return false;

    // Write header
    PresetHeader header;
    memcpy(header.magic, PRESET_MAGIC, 4);
    header.version = PRESET_VERSION;
    header.reserved[0] = 0;
    header.reserved[1] = 0;

    if (fwrite(&header, sizeof(PresetHeader), 1, f) != 1) {
        fclose(f);
        return false;
    }

    // Write preset data field-by-field (don't use fwrite of whole struct - padding varies by platform!)
    // Write name (64 bytes)
    if (fwrite(preset->name, 64, 1, f) != 1) {
        fclose(f);
        return false;
    }

    // Write author (64 bytes)
    if (fwrite(preset->author, 64, 1, f) != 1) {
        fclose(f);
        return false;
    }

    // Write description (256 bytes)
    if (fwrite(preset->description, 256, 1, f) != 1) {
        fclose(f);
        return false;
    }

    // Write AhxInstrumentParams (WITHOUT the plist pointer!)
    // Pack parameters into fixed-size buffer (32 bytes)
    uint8_t params[32];
    memset(params, 0, sizeof(params));

    params[0] = (uint8_t)preset->params.waveform;
    params[1] = preset->params.wave_length;
    params[2] = preset->params.volume;
    params[3] = preset->params.envelope.attack_frames;
    params[4] = preset->params.envelope.attack_volume;
    params[5] = preset->params.envelope.decay_frames;
    params[6] = preset->params.envelope.decay_volume;
    params[7] = preset->params.envelope.sustain_frames;
    params[8] = preset->params.envelope.release_frames;
    params[9] = preset->params.envelope.release_volume;
    params[10] = preset->params.filter_lower;
    params[11] = preset->params.filter_upper;
    params[12] = preset->params.filter_speed;
    params[13] = preset->params.filter_enabled ? 1 : 0;
    params[14] = preset->params.square_lower;
    params[15] = preset->params.square_upper;
    params[16] = preset->params.square_speed;
    params[17] = preset->params.square_enabled ? 1 : 0;
    params[18] = preset->params.vibrato_delay;
    params[19] = preset->params.vibrato_depth;
    params[20] = preset->params.vibrato_speed;
    params[21] = preset->params.hard_cut_release ? 1 : 0;
    params[22] = preset->params.hard_cut_frames;
    // params[23-31] = reserved (already zeroed)

    if (fwrite(params, 32, 1, f) != 1) {
        fclose(f);
        return false;
    }

    // Write PList data after struct (if present)
    if (preset->params.plist && preset->params.plist->length > 0) {
        // Write speed and length
        if (fputc(preset->params.plist->speed, f) == EOF) {
            fclose(f);
            return false;
        }
        if (fputc(preset->params.plist->length, f) == EOF) {
            fclose(f);
            return false;
        }

        // Write entries (7 bytes each: note, fixed, waveform, fx[2], fx_param[2])
        for (int i = 0; i < preset->params.plist->length; i++) {
            AhxPListEntry* e = &preset->params.plist->entries[i];
            uint8_t entry[7];
            entry[0] = (uint8_t)(e->note & 0xFF);
            entry[1] = e->fixed ? 1 : 0;
            entry[2] = e->waveform;
            entry[3] = e->fx[0];
            entry[4] = e->fx_param[0];
            entry[5] = e->fx[1];
            entry[6] = e->fx_param[1];

            if (fwrite(entry, 7, 1, f) != 1) {
                fclose(f);
                return false;
            }
        }
    }

    fclose(f);
    return true;
}

// Load preset from file
bool ahx_preset_load(AhxPreset* preset, const char* filepath) {
    if (!preset || !filepath) return false;

    FILE* f = fopen(filepath, "rb");
    if (!f) return false;

    // Read header
    PresetHeader header;
    if (fread(&header, sizeof(PresetHeader), 1, f) != 1) {
        fclose(f);
        return false;
    }

    // Verify magic
    if (memcmp(header.magic, PRESET_MAGIC, 4) != 0) {
        fclose(f);
        return false;
    }

    // Check version
    if (header.version != PRESET_VERSION) {
        fclose(f);
        return false;
    }

    // Read preset data field-by-field (matches save format)
    // Read name (64 bytes)
    if (fread(preset->name, 64, 1, f) != 1) {
        fclose(f);
        return false;
    }

    // Read author (64 bytes)
    if (fread(preset->author, 64, 1, f) != 1) {
        fclose(f);
        return false;
    }

    // Read description (256 bytes)
    if (fread(preset->description, 256, 1, f) != 1) {
        fclose(f);
        return false;
    }

    // Read packed parameters (32 bytes)
    uint8_t params[32];
    if (fread(params, 32, 1, f) != 1) {
        fclose(f);
        return false;
    }

    // Unpack parameters
    preset->params.waveform = (AhxWaveform)params[0];
    preset->params.wave_length = params[1];
    preset->params.volume = params[2];

    // CRITICAL: ADSR values in .ahxp are in CIA TICKS, need to convert to 50Hz frames
    // The PList speed determines tick-to-frame conversion
    // NOTE: PList will be loaded later, so we can't access speed here
    // For now, load values as-is and divide during PList import
    preset->params.envelope.attack_frames = params[3];
    preset->params.envelope.attack_volume = params[4];
    preset->params.envelope.decay_frames = params[5];
    preset->params.envelope.decay_volume = params[6];
    preset->params.envelope.sustain_frames = params[7];
    preset->params.envelope.release_frames = params[8];
    preset->params.envelope.release_volume = params[9];
    preset->params.filter_lower = params[10];
    preset->params.filter_upper = params[11];
    preset->params.filter_speed = params[12];
    preset->params.filter_enabled = params[13] != 0;
    preset->params.square_lower = params[14];
    preset->params.square_upper = params[15];
    preset->params.square_speed = params[16];
    preset->params.square_enabled = params[17] != 0;
    preset->params.vibrato_delay = params[18];
    preset->params.vibrato_depth = params[19];
    preset->params.vibrato_speed = params[20];
    preset->params.hard_cut_release = params[21] != 0;
    preset->params.hard_cut_frames = params[22];

    // Clear PList pointer (will be loaded separately if present)
    preset->params.plist = NULL;

    // Try to read PList data (if present)
    int c = fgetc(f);
    if (c != EOF) {
        uint8_t speed = (uint8_t)c;
        uint8_t length = (uint8_t)fgetc(f);

        if (length > 0 && length <= 255) {
            // Allocate PList
            preset->params.plist = (AhxPList*)malloc(sizeof(AhxPList));
            if (preset->params.plist) {
                preset->params.plist->speed = speed;
                preset->params.plist->length = length;
                preset->params.plist->entries = (AhxPListEntry*)malloc(length * sizeof(AhxPListEntry));

                if (preset->params.plist->entries) {
                    // Read entries
                    for (int i = 0; i < length; i++) {
                        uint8_t entry[7];
                        if (fread(entry, 7, 1, f) == 1) {
                            preset->params.plist->entries[i].note = (int16_t)entry[0];
                            preset->params.plist->entries[i].fixed = entry[1] != 0;
                            preset->params.plist->entries[i].waveform = entry[2];
                            preset->params.plist->entries[i].fx[0] = entry[3];
                            preset->params.plist->entries[i].fx_param[0] = entry[4];
                            preset->params.plist->entries[i].fx[1] = entry[5];
                            preset->params.plist->entries[i].fx_param[1] = entry[6];
                        }
                    }
                } else {
                    free(preset->params.plist);
                    preset->params.plist = NULL;
                }
            }
        }
    }

    fclose(f);
    return true;
}

// Import from AHX file
bool ahx_preset_import_from_ahx(AhxPreset* preset, const char* ahx_filepath, uint8_t instrument_index) {
    if (!preset || !ahx_filepath) return false;

    FILE* f = fopen(ahx_filepath, "rb");
    if (!f) return false;

    // Get file size
    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (file_size < 14) {
        fclose(f);
        return false;
    }

    // Read file into buffer
    uint8_t* buffer = (uint8_t*)malloc(file_size);
    if (!buffer) {
        fclose(f);
        return false;
    }

    if (fread(buffer, 1, file_size, f) != (size_t)file_size) {
        free(buffer);
        fclose(f);
        return false;
    }
    fclose(f);

    // Verify AHX signature
    if (buffer[0] != 'T' || buffer[1] != 'H' || buffer[2] != 'X') {
        free(buffer);
        return false;
    }

    uint8_t revision = buffer[3];
    if (revision > 1) {
        free(buffer);
        return false;
    }

    // Parse header
    uint16_t name_offset = (buffer[4] << 8) | buffer[5];
    uint8_t position_nr = ((buffer[6] & 0xf) << 8) | buffer[7];
    uint8_t track_length = buffer[10];
    uint8_t track_nr = buffer[11];
    uint8_t instrument_nr = buffer[12];
    uint8_t subsong_nr = buffer[13];

    // Validate instrument index
    if (instrument_index == 0 || instrument_index > instrument_nr) {
        free(buffer);
        return false;
    }

    // Skip to instrument data
    const uint8_t* ptr = &buffer[14];

    // Skip subsongs
    ptr += subsong_nr * 2;

    // Skip positions
    ptr += position_nr * 8;  // 4 channels * 2 bytes

    // Skip tracks
    for (int i = 0; i <= track_nr; i++) {
        if ((buffer[6] & 0x80) == 0x80 && i == 0) {
            continue;  // Empty track
        }
        ptr += track_length * 3;  // 3 bytes per step
    }

    // Read instrument names
    const char* name_ptr = (const char*)&buffer[name_offset];
    name_ptr += strlen(name_ptr) + 1;  // Skip song name

    // Skip to target instrument
    for (int i = 1; i < instrument_index; i++) {
        if (ptr - buffer > file_size) {
            free(buffer);
            return false;
        }

        // Skip instrument name
        name_ptr += strlen(name_ptr) + 1;

        // Skip instrument data (22 bytes header)
        uint8_t plist_length = ptr[21];
        ptr += 22 + (plist_length * 4);  // Header + PList entries
    }

    // Read target instrument name
    strncpy(preset->name, name_ptr, 63);
    preset->name[63] = '\0';
    snprintf(preset->author, 64, "Imported from AHX");
    snprintf(preset->description, 256, "Instrument %d from %s", instrument_index, ahx_filepath);

    // Parse instrument parameters
    if (ptr + 22 - buffer > file_size) {
        free(buffer);
        return false;
    }

    preset->params.volume = ptr[0];
    preset->params.wave_length = ptr[1] & 0x7;

    // Envelope
    // IMPORTANT: ADSR values in HivelyTracker .hvl format are stored in CIA ticks
    // Convert to 50Hz frames by dividing by 3 (default AHX song speed)
    preset->params.envelope.attack_frames = (ptr[2] + 2) / 3;
    preset->params.envelope.attack_volume = ptr[3];
    preset->params.envelope.decay_frames = (ptr[4] + 2) / 3;
    preset->params.envelope.decay_volume = ptr[5];
    preset->params.envelope.sustain_frames = (ptr[6] + 2) / 3;
    preset->params.envelope.release_frames = (ptr[7] + 2) / 3;
    preset->params.envelope.release_volume = ptr[8];

    // Filter
    uint8_t filter_speed = ((ptr[1] >> 3) & 0x1f) | ((ptr[12] >> 2) & 0x20);
    uint8_t filter_lower = ptr[12] & 0x7f;
    uint8_t filter_upper = ptr[19] & 0x3f;
    preset->params.filter_enabled = (filter_speed > 0 || filter_lower > 0 || filter_upper > 0);
    preset->params.filter_speed = filter_speed;
    preset->params.filter_lower = filter_lower;
    preset->params.filter_upper = filter_upper;

    // Vibrato
    preset->params.vibrato_delay = ptr[13];
    preset->params.vibrato_depth = ptr[14] & 0xf;
    preset->params.vibrato_speed = ptr[15];

    // Square modulation (PWM)
    preset->params.square_lower = ptr[16];
    preset->params.square_upper = ptr[17];
    preset->params.square_speed = ptr[18];
    preset->params.square_enabled = (preset->params.square_speed > 0 ||
                                     preset->params.square_lower != preset->params.square_upper);

    // Hard cut
    preset->params.hard_cut_frames = (ptr[14] >> 4) & 7;
    preset->params.hard_cut_release = (ptr[14] & 0x80) ? true : false;

    // Default waveform (will be overridden by PList if used)
    preset->params.waveform = AHX_WAVE_SAWTOOTH;

    // Parse PList
    uint8_t plist_speed = ptr[20];
    uint8_t plist_length = ptr[21];
    ptr += 22;

    if (plist_length > 0) {
        // Allocate PList
        preset->params.plist = (AhxPList*)malloc(sizeof(AhxPList));
        if (!preset->params.plist) {
            free(buffer);
            return false;
        }

        preset->params.plist->speed = plist_speed;
        preset->params.plist->length = plist_length;
        preset->params.plist->entries = (AhxPListEntry*)malloc(plist_length * sizeof(AhxPListEntry));

        if (!preset->params.plist->entries) {
            free(preset->params.plist);
            preset->params.plist = NULL;
            free(buffer);
            return false;
        }

        // Parse PList entries
        for (int j = 0; j < plist_length; j++) {
            if (ptr + 4 - buffer > file_size) {
                free(preset->params.plist->entries);
                free(preset->params.plist);
                preset->params.plist = NULL;
                free(buffer);
                return false;
            }

            preset->params.plist->entries[j].fx[1] = (ptr[0] >> 5) & 7;
            preset->params.plist->entries[j].fx[0] = (ptr[0] >> 2) & 7;
            preset->params.plist->entries[j].waveform = ((ptr[0] << 1) & 6) | (ptr[1] >> 7);
            preset->params.plist->entries[j].fixed = (ptr[1] >> 6) & 1;
            preset->params.plist->entries[j].note = ptr[1] & 0x3f;
            preset->params.plist->entries[j].fx_param[0] = ptr[2];
            preset->params.plist->entries[j].fx_param[1] = ptr[3];

            ptr += 4;

            // If first entry has a waveform, use it as default
            if (j == 0 && preset->params.plist->entries[j].waveform > 0) {
                preset->params.waveform = preset->params.plist->entries[j].waveform - 1;
            }
        }
    } else {
        preset->params.plist = NULL;
    }

    free(buffer);
    return true;
}

// Get instrument count from AHX file
uint8_t ahx_preset_get_ahx_instrument_count(const char* ahx_filepath) {
    if (!ahx_filepath) return 0;

    FILE* f = fopen(ahx_filepath, "rb");
    if (!f) return 0;

    // Read AHX header
    uint8_t header[14];
    if (fread(header, 1, 14, f) != 14) {
        fclose(f);
        return 0;
    }

    // Verify signature
    if (header[0] != 'T' || header[1] != 'H' || header[2] != 'X') {
        fclose(f);
        return 0;
    }

    // Instrument count at offset 12
    uint8_t instrument_nr = header[12];

    fclose(f);
    return instrument_nr;
}

// Get instrument name from AHX file
bool ahx_preset_get_ahx_instrument_name(const char* ahx_filepath, uint8_t instrument_index, char* name_out) {
    if (!ahx_filepath || !name_out) return false;

    FILE* f = fopen(ahx_filepath, "rb");
    if (!f) return false;

    // Get file size
    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (file_size < 14) {
        fclose(f);
        return false;
    }

    // Read header
    uint8_t header[14];
    if (fread(header, 1, 14, f) != 14) {
        fclose(f);
        return false;
    }

    // Verify signature
    if (header[0] != 'T' || header[1] != 'H' || header[2] != 'X') {
        fclose(f);
        return false;
    }

    // Get name offset
    uint16_t name_offset = (header[4] << 8) | header[5];
    uint8_t instrument_nr = header[12];

    // Validate instrument index
    if (instrument_index == 0 || instrument_index > instrument_nr) {
        fclose(f);
        return false;
    }

    // Seek to names section
    fseek(f, name_offset, SEEK_SET);

    // Read names (null-terminated strings)
    char name_buffer[128];

    // Skip song name
    int c;
    while ((c = fgetc(f)) != EOF && c != '\0');

    // Read instrument names
    for (int i = 1; i <= instrument_index; i++) {
        int idx = 0;
        while ((c = fgetc(f)) != EOF && c != '\0' && idx < 127) {
            name_buffer[idx++] = c;
        }
        name_buffer[idx] = '\0';

        if (i == instrument_index) {
            strncpy(name_out, name_buffer, 63);
            name_out[63] = '\0';
            fclose(f);
            return true;
        }
    }

    fclose(f);
    return false;
}

// Create default preset
AhxPreset ahx_preset_create_default(void) {
    AhxPreset preset;
    memset(&preset, 0, sizeof(AhxPreset));

    strcpy(preset.name, "Default");
    strcpy(preset.author, "Regroove");
    strcpy(preset.description, "Default AHX instrument");

    preset.params = ahx_instrument_default_params();

    return preset;
}

// Built-in presets
AhxPreset ahx_preset_get_builtin(uint8_t index) {
    AhxPreset preset = ahx_preset_create_default();

    switch (index) {
        case 0: // Default
            strcpy(preset.name, "Default");
            strcpy(preset.description, "Basic sawtooth synth");
            preset.params = ahx_instrument_default_params();
            break;

        case 1: // Bass - Classic AHX
            strcpy(preset.name, "Bass - Classic AHX");
            strcpy(preset.description, "Thick bass with filter and PWM");
            preset.params.waveform = AHX_WAVE_SQUARE;
            preset.params.wave_length = 5;
            preset.params.filter_enabled = true;
            preset.params.filter_lower = 10;
            preset.params.filter_upper = 40;
            preset.params.filter_speed = 3;
            preset.params.square_enabled = true;
            preset.params.square_lower = 40;
            preset.params.square_upper = 200;
            preset.params.square_speed = 6;
            preset.params.envelope.attack_frames = 1;
            preset.params.envelope.attack_volume = 64;
            preset.params.envelope.decay_frames = 20;
            preset.params.envelope.decay_volume = 50;
            preset.params.envelope.sustain_frames = 0;
            preset.params.envelope.release_frames = 10;
            break;

        case 2: // Lead - Sawtooth
            strcpy(preset.name, "Lead - Sawtooth");
            strcpy(preset.description, "Bright lead with vibrato");
            preset.params.waveform = AHX_WAVE_SAWTOOTH;
            preset.params.wave_length = 4;
            preset.params.filter_enabled = true;
            preset.params.filter_lower = 25;
            preset.params.filter_upper = 55;
            preset.params.filter_speed = 5;
            preset.params.vibrato_delay = 10;
            preset.params.vibrato_depth = 4;
            preset.params.vibrato_speed = 30;
            preset.params.envelope.attack_frames = 2;
            preset.params.envelope.attack_volume = 64;
            preset.params.envelope.decay_frames = 15;
            preset.params.envelope.decay_volume = 56;
            preset.params.envelope.sustain_frames = 0;
            preset.params.envelope.release_frames = 25;
            break;

        case 3: // Pad - PWM
            strcpy(preset.name, "Pad - PWM");
            strcpy(preset.description, "Evolving pad with pulse width modulation");
            preset.params.waveform = AHX_WAVE_SQUARE;
            preset.params.wave_length = 4;
            preset.params.square_enabled = true;
            preset.params.square_lower = 32;
            preset.params.square_upper = 224;
            preset.params.square_speed = 8;
            preset.params.filter_enabled = true;
            preset.params.filter_lower = 20;
            preset.params.filter_upper = 45;
            preset.params.filter_speed = 6;
            preset.params.envelope.attack_frames = 50;
            preset.params.envelope.attack_volume = 64;
            preset.params.envelope.decay_frames = 30;
            preset.params.envelope.decay_volume = 52;
            preset.params.envelope.sustain_frames = 0;
            preset.params.envelope.release_frames = 60;
            break;

        case 4: // Hit - Percussion
            strcpy(preset.name, "Hit - Percussion");
            strcpy(preset.description, "Percussive noise hit");
            preset.params.waveform = AHX_WAVE_NOISE;
            preset.params.wave_length = 2;
            preset.params.volume = 64;
            preset.params.hard_cut_release = true;
            preset.params.hard_cut_frames = 3;
            preset.params.filter_enabled = true;
            preset.params.filter_lower = 5;
            preset.params.filter_upper = 50;
            preset.params.filter_speed = 1;
            preset.params.envelope.attack_frames = 0;
            preset.params.envelope.attack_volume = 64;
            preset.params.envelope.decay_frames = 0;
            preset.params.envelope.decay_volume = 64;
            preset.params.envelope.sustain_frames = 0;
            preset.params.envelope.release_frames = 0;
            break;

        case 5: // Noise - Cymbal
            strcpy(preset.name, "Noise - Cymbal");
            strcpy(preset.description, "Cymbal-like noise");
            preset.params.waveform = AHX_WAVE_NOISE;
            preset.params.wave_length = 3;
            preset.params.filter_enabled = true;
            preset.params.filter_lower = 40;
            preset.params.filter_upper = 60;
            preset.params.filter_speed = 2;
            preset.params.envelope.attack_frames = 0;
            preset.params.envelope.attack_volume = 64;
            preset.params.envelope.decay_frames = 30;
            preset.params.envelope.decay_volume = 40;
            preset.params.envelope.sustain_frames = 0;
            preset.params.envelope.release_frames = 40;
            break;

        default:
            preset = ahx_preset_create_default();
            break;
    }

    return preset;
}

// Get number of built-in presets
uint8_t ahx_preset_get_builtin_count(void) {
    return 6;
}

// Free preset resources
void ahx_preset_free(AhxPreset* preset) {
    if (!preset) return;

    // Free PList if allocated
    if (preset->params.plist) {
        if (preset->params.plist->entries) {
            free(preset->params.plist->entries);
            preset->params.plist->entries = NULL;
        }
        free(preset->params.plist);
        preset->params.plist = NULL;
    }
}
