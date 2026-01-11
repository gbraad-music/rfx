#include "sfz_parser.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

// WAV file header structure
typedef struct {
    char riff[4];           // "RIFF"
    uint32_t file_size;     // File size - 8
    char wave[4];           // "WAVE"
} WAVHeader;

typedef struct {
    char fmt[4];            // "fmt "
    uint32_t chunk_size;    // 16 for PCM
    uint16_t audio_format;  // 1 for PCM
    uint16_t num_channels;  // 1 = Mono, 2 = Stereo
    uint32_t sample_rate;   // 44100, 48000, etc.
    uint32_t byte_rate;     // sample_rate * num_channels * bits_per_sample/8
    uint16_t block_align;   // num_channels * bits_per_sample/8
    uint16_t bits_per_sample; // 8, 16, 24, etc.
} FMTChunk;

typedef struct {
    char data[4];           // "data"
    uint32_t data_size;     // Size of data section
} DATAChunk;

// Helper: trim whitespace
static char* trim(char* str) {
    if (!str) return NULL;

    // Trim leading
    while (*str == ' ' || *str == '\t' || *str == '\n' || *str == '\r') str++;
    if (*str == 0) return str;

    // Trim trailing
    char* end = str + strlen(str) - 1;
    while (end > str && (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r')) end--;
    end[1] = '\0';

    return str;
}

// Helper: get base directory from file path
static void get_base_dir(const char* filepath, char* base_dir, size_t size) {
    strncpy(base_dir, filepath, size - 1);
    base_dir[size - 1] = '\0';

    // Find last slash or backslash
    char* last_slash = strrchr(base_dir, '/');
    char* last_backslash = strrchr(base_dir, '\\');
    char* sep = last_slash > last_backslash ? last_slash : last_backslash;

    if (sep) {
        sep[1] = '\0';  // Keep the slash
    } else {
        base_dir[0] = '\0';  // No directory
    }
}

SFZData* sfz_parse(const char* filepath) {
    FILE* f = fopen(filepath, "r");
    if (!f) {
        fprintf(stderr, "Failed to open SFZ file: %s\n", filepath);
        return NULL;
    }

    SFZData* sfz = (SFZData*)calloc(1, sizeof(SFZData));
    if (!sfz) {
        fclose(f);
        return NULL;
    }

    get_base_dir(filepath, sfz->base_dir, SFZ_MAX_PATH);

    // Current region being parsed
    SFZRegion* current = NULL;

    char line[1024];
    while (fgets(line, sizeof(line), f)) {
        char* trimmed = trim(line);

        // Skip comments and empty lines
        if (trimmed[0] == '\0' || trimmed[0] == '/' || trimmed[0] == ';') {
            continue;
        }

        // Check for <region> tag
        if (strstr(trimmed, "<region>")) {
            if (sfz->num_regions >= SFZ_MAX_REGIONS) {
                fprintf(stderr, "Too many regions in SFZ file\n");
                break;
            }

            current = &sfz->regions[sfz->num_regions++];

            // Set defaults
            current->lokey = 0;
            current->hikey = 127;
            current->pitch_keycenter = 60;  // Middle C
            current->lovel = 0;
            current->hivel = 127;
            current->offset = 0;
            current->end = 0;  // 0 = use full sample
            current->pan = 0.0f;
            current->loop_mode = false;  // Default to one-shot
            current->sample_data = NULL;
            current->sample_length = 0;
            current->sample_rate = 44100;  // Default
            continue;
        }

        if (!current) continue;  // Skip until we have a region

        // Parse key=value pairs
        char* key = trimmed;
        char* value = strchr(trimmed, '=');
        if (!value) continue;

        *value = '\0';
        value++;
        key = trim(key);
        value = trim(value);

        if (strcmp(key, "sample") == 0) {
            strncpy(current->sample_path, value, SFZ_MAX_PATH - 1);
        } else if (strcmp(key, "key") == 0) {
            current->lokey = current->hikey = atoi(value);
        } else if (strcmp(key, "lokey") == 0) {
            current->lokey = atoi(value);
        } else if (strcmp(key, "hikey") == 0) {
            current->hikey = atoi(value);
        } else if (strcmp(key, "pitch_keycenter") == 0) {
            current->pitch_keycenter = atoi(value);
        } else if (strcmp(key, "lovel") == 0) {
            current->lovel = atoi(value);
        } else if (strcmp(key, "hivel") == 0) {
            current->hivel = atoi(value);
        } else if (strcmp(key, "offset") == 0) {
            current->offset = atoi(value);
        } else if (strcmp(key, "end") == 0) {
            current->end = atoi(value);
        } else if (strcmp(key, "pan") == 0) {
            current->pan = atof(value);
        } else if (strcmp(key, "loop_mode") == 0) {
            current->loop_mode = (strcmp(value, "loop_continuous") == 0);
        }
    }

    fclose(f);
    return sfz;
}

bool sfz_load_samples(SFZData* sfz) {
    if (!sfz) return false;

    for (uint32_t i = 0; i < sfz->num_regions; i++) {
        SFZRegion* region = &sfz->regions[i];

        // Build full path
        char full_path[SFZ_MAX_PATH * 2];
        snprintf(full_path, sizeof(full_path), "%s%s", sfz->base_dir, region->sample_path);

        FILE* f = fopen(full_path, "rb");
        if (!f) {
            fprintf(stderr, "Failed to open WAV file: %s\n", full_path);
            continue;
        }

        // Read WAV header
        WAVHeader wav_header;
        fread(&wav_header, sizeof(WAVHeader), 1, f);

        if (memcmp(wav_header.riff, "RIFF", 4) != 0 ||
            memcmp(wav_header.wave, "WAVE", 4) != 0) {
            fprintf(stderr, "Not a valid WAV file: %s\n", full_path);
            fclose(f);
            continue;
        }

        // Read fmt chunk
        FMTChunk fmt;
        fread(&fmt, sizeof(FMTChunk), 1, f);

        region->sample_rate = fmt.sample_rate;

        // Find data chunk
        DATAChunk data_chunk;
        fread(&data_chunk, sizeof(DATAChunk), 1, f);

        // Allocate and read sample data
        uint32_t num_samples = data_chunk.data_size / (fmt.bits_per_sample / 8);

        if (fmt.num_channels == 2) {
            num_samples /= 2;  // Convert stereo to mono by taking left channel
        }

        region->sample_length = num_samples;
        region->sample_data = (int16_t*)malloc(num_samples * sizeof(int16_t));

        if (!region->sample_data) {
            fprintf(stderr, "Failed to allocate memory for sample: %s\n", full_path);
            fclose(f);
            continue;
        }

        if (fmt.num_channels == 1) {
            // Mono - read directly
            fread(region->sample_data, sizeof(int16_t), num_samples, f);
        } else {
            // Stereo - read and mix to mono
            for (uint32_t j = 0; j < num_samples; j++) {
                int16_t left, right;
                fread(&left, sizeof(int16_t), 1, f);
                fread(&right, sizeof(int16_t), 1, f);
                region->sample_data[j] = (left + right) / 2;
            }
        }

        fclose(f);
    }

    return true;
}

const SFZRegion* sfz_find_region(const SFZData* sfz, uint8_t note, uint8_t velocity) {
    if (!sfz) return NULL;

    for (uint32_t i = 0; i < sfz->num_regions; i++) {
        const SFZRegion* region = &sfz->regions[i];

        if (note >= region->lokey && note <= region->hikey &&
            velocity >= region->lovel && velocity <= region->hivel) {
            return region;
        }
    }

    return NULL;
}

void sfz_free(SFZData* sfz) {
    if (!sfz) return;

    for (uint32_t i = 0; i < sfz->num_regions; i++) {
        if (sfz->regions[i].sample_data) {
            free(sfz->regions[i].sample_data);
        }
    }

    free(sfz);
}
