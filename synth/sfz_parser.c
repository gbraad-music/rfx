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

// Helper function to parse a single line and extract key=value pairs
static void parse_sfz_line(char* line, SFZRegion* current) {
    if (!current) return;

    char* pos = line;

    while (*pos) {
        // Skip whitespace
        while (*pos && isspace(*pos)) pos++;
        if (!*pos) break;

        // Find '=' to identify key
        char* equals = strchr(pos, '=');
        if (!equals) break;

        // Extract key
        char key_buf[64];
        size_t key_len = equals - pos;
        if (key_len >= 64) key_len = 63;
        memcpy(key_buf, pos, key_len);
        key_buf[key_len] = '\0';
        char* key = trim(key_buf);

        // Find value (from after '=' to next key= or end of string)
        char* value_start = equals + 1;
        char* value_end = value_start;

        // Scan for next key= pattern
        while (*value_end) {
            if (isspace(*value_end)) {
                // Check if next non-whitespace starts a key=value pattern
                char* check = value_end + 1;
                while (*check && isspace(*check)) check++;
                if (*check && isalpha(*check)) {
                    // Look ahead for '='
                    char* lookahead = check;
                    while (*lookahead && (isalnum(*lookahead) || *lookahead == '_')) lookahead++;
                    if (*lookahead == '=') {
                        // Found next key=, stop here
                        break;
                    }
                }
            }
            value_end++;
        }

        // Extract and trim value
        char value_buf[SFZ_MAX_PATH];
        size_t value_len = value_end - value_start;
        if (value_len >= SFZ_MAX_PATH) value_len = SFZ_MAX_PATH - 1;
        memcpy(value_buf, value_start, value_len);
        value_buf[value_len] = '\0';
        char* value = trim(value_buf);

        printf("[SFZ Parser C] Parsed: %s = '%s'\n", key, value);

        // Parse based on key
        if (strcmp(key, "sample") == 0) {
            strncpy(current->sample_path, value, SFZ_MAX_PATH - 1);
            current->sample_path[SFZ_MAX_PATH - 1] = '\0';
        } else if (strcmp(key, "key") == 0) {
            current->lokey = current->hikey = atoi(value);
        } else if (strcmp(key, "lokey") == 0) {
            current->lokey = atoi(value);
        } else if (strcmp(key, "hikey") == 0) {
            current->hikey = atoi(value);
        } else if (strcmp(key, "pitch_keycenter") == 0) {
            current->pitch_keycenter = atoi(value);
        } else if (strcmp(key, "pitch_keytrack") == 0) {
            current->pitch_keytrack = atoi(value);
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

        // Move to next key=value or end
        pos = value_end;
    }
}

SFZData* sfz_parse_from_memory(const char* content, size_t length) {
    SFZData* sfz = (SFZData*)calloc(1, sizeof(SFZData));
    if (!sfz) return NULL;

    sfz->base_dir[0] = '\0';  // No base directory for memory parsing

    // Current region being parsed
    SFZRegion* current = NULL;

    // Group defaults (opcodes in <group> apply to all subsequent regions)
    SFZRegion group_defaults;
    memset(&group_defaults, 0, sizeof(SFZRegion));
    group_defaults.lokey = 0;
    group_defaults.hikey = 127;
    group_defaults.pitch_keycenter = 255;
    group_defaults.pitch_keytrack = 100;  // Default: full pitch tracking
    group_defaults.lovel = 0;
    group_defaults.hivel = 127;
    group_defaults.offset = 0;
    group_defaults.end = 0;
    group_defaults.pan = 0.0f;
    group_defaults.loop_mode = false;
    group_defaults.sample_path[0] = '\0';

    // Parse line by line
    const char* line_start = content;
    const char* content_end = content + length;

    while (line_start < content_end) {
        // Find end of line
        const char* line_end = line_start;
        while (line_end < content_end && *line_end != '\n' && *line_end != '\r') {
            line_end++;
        }

        // Copy line to buffer
        size_t line_len = line_end - line_start;
        if (line_len >= 1024) line_len = 1023;

        char line[1024];
        memcpy(line, line_start, line_len);
        line[line_len] = '\0';

        char* trimmed = trim(line);

        // Skip comments and empty lines
        if (trimmed[0] != '\0' && trimmed[0] != '/' && trimmed[0] != ';') {
            // Check for <group> tag
            char* group_tag = strstr(trimmed, "<group>");
            if (group_tag) {
                printf("[SFZ Parser C] Found <group>\n");
                current = NULL;  // Reset current region when entering group

                // Parse any opcodes on the same line after <group>
                char* after_tag = group_tag + 7;  // Skip "<group>"
                if (*after_tag) {
                    printf("[SFZ Parser C] Parsing group opcodes: '%s'\n", after_tag);
                    parse_sfz_line(after_tag, &group_defaults);
                }
            }
            // Check for <region> tag
            else if (strstr(trimmed, "<region>")) {
                char* region_tag = strstr(trimmed, "<region>");
                if (sfz->num_regions >= SFZ_MAX_REGIONS) {
                    fprintf(stderr, "Too many regions in SFZ file\n");
                    break;
                }

                current = &sfz->regions[sfz->num_regions++];

                // Copy group defaults first
                memcpy(current, &group_defaults, sizeof(SFZRegion));

                // Then set/override region-specific defaults
                current->sample_data = NULL;
                current->sample_length = 0;
                current->sample_rate = 44100;

                printf("[SFZ Parser C] Found <region>, now at region %d\n", sfz->num_regions);

                // Parse any opcodes on the same line after <region>
                char* after_tag = region_tag + 8;  // Skip "<region>"
                if (*after_tag) {
                    printf("[SFZ Parser C] Parsing opcodes after <region>: '%s'\n", after_tag);
                    parse_sfz_line(after_tag, current);
                }
            } else if (current) {
                // Parse key=value pairs on this line (within a region)
                printf("[SFZ Parser C] Parsing line: '%s'\n", trimmed);
                parse_sfz_line(trimmed, current);
            } else {
                // Parse key=value pairs within group (before any region)
                printf("[SFZ Parser C] Parsing group line: '%s'\n", trimmed);
                parse_sfz_line(trimmed, &group_defaults);
            }
        }

        // Move to next line
        line_start = line_end;
        while (line_start < content_end && (*line_start == '\n' || *line_start == '\r')) {
            line_start++;
        }
    }

    // Note: pitch_keycenter is left at 255 if not explicitly set
    // This means no pitch shifting (samples play at natural rate)
    // Only melodic instruments should set pitch_keycenter explicitly

    printf("[SFZ Parser C] Parsed %d regions from memory\n", sfz->num_regions);
    return sfz;
}

SFZData* sfz_parse(const char* filepath) {
    FILE* f = fopen(filepath, "r");
    if (!f) {
        fprintf(stderr, "Failed to open SFZ file: %s\n", filepath);
        return NULL;
    }

    // Read entire file into memory
    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char* content = (char*)malloc(file_size + 1);
    if (!content) {
        fclose(f);
        return NULL;
    }

    fread(content, 1, file_size, f);
    content[file_size] = '\0';
    fclose(f);

    // Use memory parser
    SFZData* sfz = sfz_parse_from_memory(content, file_size);
    free(content);

    if (sfz) {
        get_base_dir(filepath, sfz->base_dir, SFZ_MAX_PATH);
    }

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
