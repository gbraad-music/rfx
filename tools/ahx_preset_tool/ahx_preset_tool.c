/*
 * AHX Preset Tool
 *
 * Extract instruments from .ahx files and save as .ahxp presets
 * Create and manage AHX instrument presets
 *
 * Usage:
 *   ahx_preset_tool list <file.ahx>                       - List all instruments
 *   ahx_preset_tool extract <file.ahx> <index> <out>      - Extract single instrument
 *   ahx_preset_tool extract-all <file.ahx> <dir> [prefix] - Extract all instruments
 *   ahx_preset_tool info <file.ahxp>                      - Show preset info
 *   ahx_preset_tool create <name> <out.ahxp>              - Create default preset
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include "../../synth/ahx_preset.h"

#ifdef _WIN32
#include <direct.h>
#define mkdir(path, mode) _mkdir(path)
#else
#include <sys/stat.h>
#include <sys/types.h>
#endif

// Print usage information
static void print_usage(const char* program_name) {
    printf("AHX Preset Tool - Extract and manage AHX instrument presets\n\n");
    printf("Usage:\n");
    printf("  %s list <file.ahx>\n", program_name);
    printf("      List all instruments in an AHX file\n\n");
    printf("  %s extract <file.ahx> <index> <output.ahxp>\n", program_name);
    printf("      Extract a single instrument and save as preset\n");
    printf("      Index: 1-based instrument number\n\n");
    printf("  %s extract-all <file.ahx> <output-directory> [prefix]\n", program_name);
    printf("      Extract all instruments to separate .ahxp files\n");
    printf("      Optional prefix: Use {prefix}_{number}.ahxp instead of instrument names\n\n");
    printf("  %s info <file.ahxp>\n", program_name);
    printf("      Display information about a preset file\n\n");
    printf("  %s create <name> <output.ahxp>\n", program_name);
    printf("      Create a new preset with default parameters\n\n");
    printf("  %s builtin <index> <output.ahxp>\n", program_name);
    printf("      Save a built-in preset (0-5) to file\n\n");
    printf("Examples:\n");
    printf("  %s list mysong.ahx\n", program_name);
    printf("  %s extract mysong.ahx 1 bass.ahxp\n", program_name);
    printf("  %s extract-all mysong.ahx ./presets/\n", program_name);
    printf("  %s extract-all mysong.ahx ./presets/ Downstream\n", program_name);
    printf("  %s info bass.ahxp\n", program_name);
}

// Command: List instruments in AHX file
static int cmd_list(const char* ahx_file) {
    printf("Listing instruments in: %s\n\n", ahx_file);

    uint8_t count = ahx_preset_get_ahx_instrument_count(ahx_file);
    if (count == 0) {
        fprintf(stderr, "Error: Could not read AHX file or no instruments found\n");
        return 1;
    }

    printf("Found %d instruments:\n\n", count);

    for (uint8_t i = 1; i <= count; i++) {
        char name[64];
        if (ahx_preset_get_ahx_instrument_name(ahx_file, i, name)) {
            printf("  %2d. %s\n", i, name);
        } else {
            printf("  %2d. <error reading name>\n", i);
        }
    }

    printf("\n");
    return 0;
}

// Command: Extract single instrument
static int cmd_extract(const char* ahx_file, uint8_t index, const char* output_file) {
    printf("Extracting instrument %d from %s...\n", index, ahx_file);

    AhxPreset preset;
    if (!ahx_preset_import_from_ahx(&preset, ahx_file, index)) {
        fprintf(stderr, "Error: Failed to import instrument %d\n", index);
        fprintf(stderr, "  - Check that the index is valid (1-%d)\n",
                ahx_preset_get_ahx_instrument_count(ahx_file));
        return 1;
    }

    printf("  Name: %s\n", preset.name);
    printf("  Waveform: %d, Volume: %d, Wave Length: %d\n",
           preset.params.waveform, preset.params.volume, preset.params.wave_length);

    if (preset.params.plist) {
        printf("  PList: %d entries, speed: %d\n",
               preset.params.plist->length, preset.params.plist->speed);
    }

    if (!ahx_preset_save(&preset, output_file)) {
        fprintf(stderr, "Error: Failed to save preset to %s\n", output_file);
        ahx_preset_free(&preset);
        return 1;
    }

    printf("✓ Saved to: %s\n", output_file);

    ahx_preset_free(&preset);
    return 0;
}

// Command: Extract all instruments
static int cmd_extract_all(const char* ahx_file, const char* output_dir, const char* prefix) {
    printf("Extracting all instruments from: %s\n", ahx_file);
    printf("Output directory: %s\n", output_dir);
    if (prefix) {
        printf("Filename prefix: %s\n", prefix);
    }
    printf("\n");

    // Create output directory if it doesn't exist
    if (mkdir(output_dir, 0755) != 0) {
        // Only fail if error is not "already exists"
        if (errno != EEXIST) {
            fprintf(stderr, "Error: Failed to create directory '%s': %s\n", output_dir, strerror(errno));
            return 1;
        }
    }

    uint8_t count = ahx_preset_get_ahx_instrument_count(ahx_file);
    if (count == 0) {
        fprintf(stderr, "Error: Could not read AHX file or no instruments found\n");
        return 1;
    }

    printf("Found %d instruments\n\n", count);

    int success_count = 0;
    int fail_count = 0;

    for (uint8_t i = 1; i <= count; i++) {
        AhxPreset preset;
        if (!ahx_preset_import_from_ahx(&preset, ahx_file, i)) {
            fprintf(stderr, "✗ Failed to import instrument %d\n", i);
            fail_count++;
            continue;
        }

        // Create output filename
        char filename[256];

        if (prefix) {
            // Use prefix: {prefix}_{number}.ahxp
            snprintf(filename, sizeof(filename), "%s/%s_%02d.ahxp", output_dir, prefix, i);
        } else {
            // Legacy behavior: sanitize instrument name
            char sanitized_name[64];
            int j = 0;
            for (int k = 0; preset.name[k] && j < 63; k++) {
                char c = preset.name[k];
                if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                    (c >= '0' && c <= '9') || c == '_' || c == '-') {
                    sanitized_name[j++] = c;
                } else if (c == ' ') {
                    sanitized_name[j++] = '_';
                }
            }
            sanitized_name[j] = '\0';

            if (strlen(sanitized_name) == 0) {
                snprintf(filename, sizeof(filename), "%s/instrument_%02d.ahxp", output_dir, i);
            } else {
                snprintf(filename, sizeof(filename), "%s/%02d_%s.ahxp", output_dir, i, sanitized_name);
            }
        }

        if (!ahx_preset_save(&preset, filename)) {
            fprintf(stderr, "✗ Failed to save: %s\n", preset.name);
            ahx_preset_free(&preset);
            fail_count++;
            continue;
        }

        printf("✓ %2d. %s -> %s\n", i, preset.name, filename);
        ahx_preset_free(&preset);
        success_count++;
    }

    printf("\n");
    printf("Summary: %d succeeded, %d failed\n", success_count, fail_count);

    return (fail_count > 0) ? 1 : 0;
}

// Command: Show preset info
static int cmd_info(const char* preset_file) {
    printf("Preset information: %s\n\n", preset_file);

    AhxPreset preset;
    if (!ahx_preset_load(&preset, preset_file)) {
        fprintf(stderr, "Error: Failed to load preset file\n");
        return 1;
    }

    printf("Name:        %s\n", preset.name);
    printf("Author:      %s\n", preset.author);
    printf("Description: %s\n\n", preset.description);

    printf("Parameters:\n");
    printf("  Waveform:    %d (%s)\n", preset.params.waveform,
           preset.params.waveform == 0 ? "Triangle" :
           preset.params.waveform == 1 ? "Sawtooth" :
           preset.params.waveform == 2 ? "Square" : "Noise");
    printf("  Volume:      %d\n", preset.params.volume);
    printf("  Wave Length: %d\n", preset.params.wave_length);

    printf("\nEnvelope:\n");
    printf("  Attack:  %d frames @ volume %d\n",
           preset.params.envelope.attack_frames, preset.params.envelope.attack_volume);
    printf("  Decay:   %d frames @ volume %d\n",
           preset.params.envelope.decay_frames, preset.params.envelope.decay_volume);
    printf("  Sustain: %d frames\n", preset.params.envelope.sustain_frames);
    printf("  Release: %d frames @ volume %d\n",
           preset.params.envelope.release_frames, preset.params.envelope.release_volume);

    if (preset.params.filter_enabled) {
        printf("\nFilter Modulation:\n");
        printf("  Speed: %d, Range: %d-%d\n",
               preset.params.filter_speed, preset.params.filter_lower, preset.params.filter_upper);
    }

    if (preset.params.square_enabled) {
        printf("\nPWM Modulation:\n");
        printf("  Speed: %d, Range: %d-%d\n",
               preset.params.square_speed, preset.params.square_lower, preset.params.square_upper);
    }

    if (preset.params.vibrato_depth > 0) {
        printf("\nVibrato:\n");
        printf("  Delay: %d, Depth: %d, Speed: %d\n",
               preset.params.vibrato_delay, preset.params.vibrato_depth, preset.params.vibrato_speed);
    }

    if (preset.params.hard_cut_release) {
        printf("\nHard Cut Release: %d frames\n", preset.params.hard_cut_frames);
    }

    if (preset.params.plist) {
        printf("\nPerformance List:\n");
        printf("  Length: %d entries\n", preset.params.plist->length);
        printf("  Speed:  %d frames/entry\n", preset.params.plist->speed);

        printf("\n  Entries:\n");
        for (int i = 0; i < preset.params.plist->length && i < 10; i++) {
            AhxPListEntry* e = &preset.params.plist->entries[i];
            printf("    %2d: Waveform=%d Note=%2d Fixed=%d FX1=%d(%02X) FX2=%d(%02X)\n",
                   i, e->waveform, e->note, e->fixed,
                   e->fx[0], e->fx_param[0], e->fx[1], e->fx_param[1]);
        }
        if (preset.params.plist->length > 10) {
            printf("    ... (%d more entries)\n", preset.params.plist->length - 10);
        }
    }

    printf("\n");

    ahx_preset_free(&preset);
    return 0;
}

// Command: Create default preset
static int cmd_create(const char* name, const char* output_file) {
    printf("Creating preset: %s\n", name);

    AhxPreset preset = ahx_preset_create_default();
    strncpy(preset.name, name, 63);
    preset.name[63] = '\0';

    if (!ahx_preset_save(&preset, output_file)) {
        fprintf(stderr, "Error: Failed to save preset to %s\n", output_file);
        return 1;
    }

    printf("✓ Created: %s\n", output_file);
    return 0;
}

// Command: Save built-in preset
static int cmd_builtin(uint8_t index, const char* output_file) {
    uint8_t count = ahx_preset_get_builtin_count();
    if (index >= count) {
        fprintf(stderr, "Error: Invalid built-in preset index %d (valid: 0-%d)\n", index, count - 1);
        return 1;
    }

    AhxPreset preset = ahx_preset_get_builtin(index);
    printf("Saving built-in preset: %s\n", preset.name);

    if (!ahx_preset_save(&preset, output_file)) {
        fprintf(stderr, "Error: Failed to save preset to %s\n", output_file);
        return 1;
    }

    printf("✓ Saved: %s\n", output_file);
    return 0;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    const char* command = argv[1];

    if (strcmp(command, "list") == 0) {
        if (argc != 3) {
            fprintf(stderr, "Usage: %s list <file.ahx>\n", argv[0]);
            return 1;
        }
        return cmd_list(argv[2]);
    }
    else if (strcmp(command, "extract") == 0) {
        if (argc != 5) {
            fprintf(stderr, "Usage: %s extract <file.ahx> <index> <output.ahxp>\n", argv[0]);
            return 1;
        }
        uint8_t index = (uint8_t)atoi(argv[3]);
        return cmd_extract(argv[2], index, argv[4]);
    }
    else if (strcmp(command, "extract-all") == 0) {
        if (argc != 4 && argc != 5) {
            fprintf(stderr, "Usage: %s extract-all <file.ahx> <output-directory> [prefix]\n", argv[0]);
            return 1;
        }
        const char* prefix = (argc == 5) ? argv[4] : NULL;
        return cmd_extract_all(argv[2], argv[3], prefix);
    }
    else if (strcmp(command, "info") == 0) {
        if (argc != 3) {
            fprintf(stderr, "Usage: %s info <file.ahxp>\n", argv[0]);
            return 1;
        }
        return cmd_info(argv[2]);
    }
    else if (strcmp(command, "create") == 0) {
        if (argc != 4) {
            fprintf(stderr, "Usage: %s create <name> <output.ahxp>\n", argv[0]);
            return 1;
        }
        return cmd_create(argv[2], argv[3]);
    }
    else if (strcmp(command, "builtin") == 0) {
        if (argc != 4) {
            fprintf(stderr, "Usage: %s builtin <index> <output.ahxp>\n", argv[0]);
            return 1;
        }
        uint8_t index = (uint8_t)atoi(argv[2]);
        return cmd_builtin(index, argv[3]);
    }
    else {
        fprintf(stderr, "Unknown command: %s\n\n", command);
        print_usage(argv[0]);
        return 1;
    }

    return 0;
}
