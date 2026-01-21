/*
 * AHX Preset Tool
 *
 * Extract instruments from .ahx files and save as .ahxp presets
 * Create and manage AHX instrument presets
 *
 * Usage:
 *   ahx_preset_tool list <file.ahx>                       - List all instruments
 *   ahx_preset_tool extract <file.ahx> <index> <out>      - Extract single instrument (binary)
 *   ahx_preset_tool extract-all <file.ahx> <dir> [prefix] - Extract all instruments (binary)
 *   ahx_preset_tool export-text <file.ahx> <index> <out>  - Export instrument (text)
 *   ahx_preset_tool info <file.ahxp>                      - Show preset info
 *   ahx_preset_tool create <name> <out.ahxp>              - Create default preset
 *   ahx_preset_tool to-text <file.ahxp> <out.txt>         - Convert binary to text
 *   ahx_preset_tool from-text <file.txt> <out.ahxp>       - Convert text to binary
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
    printf("  %s export-text <file.ahx> <index> <output.txt>\n", program_name);
    printf("      Export instrument from AHX to text format\n\n");
    printf("  %s to-text <file.ahxp> <output.txt>\n", program_name);
    printf("      Convert binary preset to text format\n\n");
    printf("  %s from-text <file.txt> <output.ahxp>\n", program_name);
    printf("      Convert text format to binary preset\n\n");
    printf("Examples:\n");
    printf("  %s list mysong.ahx\n", program_name);
    printf("  %s extract mysong.ahx 1 bass.ahxp\n", program_name);
    printf("  %s extract-all mysong.ahx ./presets/\n", program_name);
    printf("  %s extract-all mysong.ahx ./presets/ Downstream\n", program_name);
    printf("  %s export-text mysong.ahx 1 bass.txt\n", program_name);
    printf("  %s to-text bass.ahxp bass.txt\n", program_name);
    printf("  %s from-text bass.txt bass_edit.ahxp\n", program_name);
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

// Note name table for text export
static const char* NOTE_NAMES[61] = {
    "---",
    "C-1", "C#1", "D-1", "D#1", "E-1", "F-1", "F#1", "G-1", "G#1", "A-1", "A#1", "B-1",
    "C-2", "C#2", "D-2", "D#2", "E-2", "F-2", "F#2", "G-2", "G#2", "A-2", "A#2", "B-2",
    "C-3", "C#3", "D-3", "D#3", "E-3", "F-3", "F#3", "G-3", "G#3", "A-3", "A#3", "B-3",
    "C-4", "C#4", "D-4", "D#4", "E-4", "F-4", "F#4", "G-4", "G#4", "A-4", "A#4", "B-4",
    "C-5", "C#5", "D-5", "D#5", "E-5", "F-5", "F#5", "G-5", "G#5", "A-5", "A#5", "B-5"
};

// Helper function: Write preset to text file
static int write_preset_to_text(FILE* fp, const AhxPreset* preset) {
    // Line 1: Name
    fprintf(fp, "Name: %s\n", preset->name);

    // Line 2: V(olume) W(avelength) F(ormwave)
    fprintf(fp, "V%d W%d F%d\n",
        preset->params.volume, preset->params.wave_length, preset->params.waveform);

    // Line 3: E(nvelope)
    fprintf(fp, "E%d,%d,%d,%d,%d,%d,%d\n",
        preset->params.envelope.attack_frames, preset->params.envelope.attack_volume,
        preset->params.envelope.decay_frames, preset->params.envelope.decay_volume,
        preset->params.envelope.sustain_frames,
        preset->params.envelope.release_frames, preset->params.envelope.release_volume);

    // Line 4: F(ilter)
    fprintf(fp, "F%d,%d,%d\n",
        preset->params.filter_speed, preset->params.filter_lower, preset->params.filter_upper);

    // Line 5: S(quare/PWM)
    fprintf(fp, "S%d,%d,%d\n",
        preset->params.square_speed, preset->params.square_lower, preset->params.square_upper);

    // Line 6: V(ibrato)
    fprintf(fp, "V%d,%d,%d\n",
        preset->params.vibrato_delay, preset->params.vibrato_depth, preset->params.vibrato_speed);

    // Line 7: P(erformance list)
    if (preset->params.plist) {
        fprintf(fp, "P%d,%d\n", preset->params.plist->speed, preset->params.plist->length);

        // PList entries (ahx2txt format)
        for (int i = 0; i < preset->params.plist->length; i++) {
            AhxPListEntry* e = &preset->params.plist->entries[i];

            // Get note name
            const char* note = (e->note <= 60) ? NOTE_NAMES[e->note] : "???";

            fprintf(fp, "%s%s %d %X%02X %X%02X\n",
                note,
                e->fixed ? "*" : " ",
                e->waveform,
                e->fx[0], e->fx_param[0],
                e->fx[1], e->fx_param[1]);
        }
    } else {
        fprintf(fp, "P0,0\n");
    }

    return 0;
}

// Command: Export instrument from AHX to text format
static int cmd_export_text(const char* ahx_file, uint8_t instrument_index, const char* output_file) {
    printf("Extracting instrument %d from: %s\n", instrument_index, ahx_file);

    AhxPreset preset;
    if (!ahx_preset_import_from_ahx(&preset, ahx_file, instrument_index)) {
        fprintf(stderr, "Error: Failed to import instrument %d from AHX file\n", instrument_index);
        return 1;
    }

    printf("  Name: %s\n", preset.name);

    FILE* fp = fopen(output_file, "w");
    if (!fp) {
        fprintf(stderr, "Error: Failed to open %s for writing\n", output_file);
        ahx_preset_free(&preset);
        return 1;
    }

    write_preset_to_text(fp, &preset);

    fclose(fp);
    ahx_preset_free(&preset);

    printf("✓ Exported to text: %s\n", output_file);
    return 0;
}

// Command: Convert binary preset to text format
static int cmd_to_text(const char* ahxp_file, const char* output_file) {
    AhxPreset preset;

    if (!ahx_preset_load(&preset, ahxp_file)) {
        fprintf(stderr, "Error: Failed to load preset from %s\n", ahxp_file);
        return 1;
    }

    FILE* fp = fopen(output_file, "w");
    if (!fp) {
        fprintf(stderr, "Error: Failed to open %s for writing\n", output_file);
        ahx_preset_free(&preset);
        return 1;
    }

    write_preset_to_text(fp, &preset);

    fclose(fp);
    ahx_preset_free(&preset);

    printf("✓ Converted to text: %s\n", output_file);
    return 0;
}

// Parse note name to note number
static int parse_note(const char* note_str) {
    for (int i = 0; i < 61; i++) {
        if (strncmp(note_str, NOTE_NAMES[i], 3) == 0) {
            return i;
        }
    }
    return 0; // Default to ---
}

// Command: Convert text format to binary preset
static int cmd_from_text(const char* text_file, const char* output_file) {
    FILE* fp = fopen(text_file, "r");
    if (!fp) {
        fprintf(stderr, "Error: Failed to open %s for reading\n", text_file);
        return 1;
    }

    AhxPreset preset = ahx_preset_create_default();
    char line[256];
    int plist_speed = 0, plist_length = 0;

    // Parse text file
    while (fgets(line, sizeof(line), fp)) {
        // Remove newline
        line[strcspn(line, "\n")] = 0;

        if (strncmp(line, "Name: ", 6) == 0) {
            strncpy(preset.name, line + 6, 63);
            preset.name[63] = '\0';
        }
        else if (line[0] == 'V' && strchr(line, 'W')) {
            sscanf(line, "V%d W%d F%d",
                &preset.params.volume, &preset.params.wave_length, &preset.params.waveform);
        }
        else if (line[0] == 'E') {
            sscanf(line + 1, "%d,%d,%d,%d,%d,%d,%d",
                &preset.params.envelope.attack_frames, &preset.params.envelope.attack_volume,
                &preset.params.envelope.decay_frames, &preset.params.envelope.decay_volume,
                &preset.params.envelope.sustain_frames,
                &preset.params.envelope.release_frames, &preset.params.envelope.release_volume);
        }
        else if (line[0] == 'F') {
            sscanf(line + 1, "%d,%d,%d",
                &preset.params.filter_speed, &preset.params.filter_lower, &preset.params.filter_upper);
            preset.params.filter_enabled = (preset.params.filter_speed > 0);
        }
        else if (line[0] == 'S') {
            sscanf(line + 1, "%d,%d,%d",
                &preset.params.square_speed, &preset.params.square_lower, &preset.params.square_upper);
            preset.params.square_enabled = (preset.params.square_speed > 0);
        }
        else if (line[0] == 'V' && strchr(line, ',')) {
            sscanf(line + 1, "%d,%d,%d",
                &preset.params.vibrato_delay, &preset.params.vibrato_depth, &preset.params.vibrato_speed);
        }
        else if (line[0] == 'P') {
            sscanf(line + 1, "%d,%d", &plist_speed, &plist_length);

            if (plist_length > 0) {
                preset.params.plist = (AhxPList*)malloc(sizeof(AhxPList));
                preset.params.plist->speed = plist_speed;
                preset.params.plist->length = plist_length;
                preset.params.plist->entries = (AhxPListEntry*)malloc(plist_length * sizeof(AhxPListEntry));

                // Read PList entries
                for (int i = 0; i < plist_length; i++) {
                    if (!fgets(line, sizeof(line), fp)) break;
                    line[strcspn(line, "\n")] = 0;

                    char note_str[4];
                    char fixed_char;
                    int waveform, fx1, fx1_param, fx2, fx2_param;

                    // Parse: "C-2* 1 302 105"
                    if (sscanf(line, "%3s%c %d %X%X %X%X",
                        note_str, &fixed_char, &waveform, &fx1, &fx1_param, &fx2, &fx2_param) == 7) {

                        preset.params.plist->entries[i].note = parse_note(note_str);
                        preset.params.plist->entries[i].fixed = (fixed_char == '*');
                        preset.params.plist->entries[i].waveform = waveform;
                        preset.params.plist->entries[i].fx[0] = fx1;
                        preset.params.plist->entries[i].fx_param[0] = fx1_param;
                        preset.params.plist->entries[i].fx[1] = fx2;
                        preset.params.plist->entries[i].fx_param[1] = fx2_param;
                    }
                }
            }
        }
    }

    fclose(fp);

    if (!ahx_preset_save(&preset, output_file)) {
        fprintf(stderr, "Error: Failed to save preset to %s\n", output_file);
        ahx_preset_free(&preset);
        return 1;
    }

    printf("✓ Imported from text: %s -> %s\n", text_file, output_file);
    ahx_preset_free(&preset);
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
    else if (strcmp(command, "export-text") == 0) {
        if (argc != 5) {
            fprintf(stderr, "Usage: %s export-text <file.ahx> <index> <output.txt>\n", argv[0]);
            return 1;
        }
        uint8_t index = (uint8_t)atoi(argv[3]);
        return cmd_export_text(argv[2], index, argv[4]);
    }
    else if (strcmp(command, "to-text") == 0) {
        if (argc != 4) {
            fprintf(stderr, "Usage: %s to-text <file.ahxp> <output.txt>\n", argv[0]);
            return 1;
        }
        return cmd_to_text(argv[2], argv[3]);
    }
    else if (strcmp(command, "from-text") == 0) {
        if (argc != 4) {
            fprintf(stderr, "Usage: %s from-text <file.txt> <output.ahxp>\n", argv[0]);
            return 1;
        }
        return cmd_from_text(argv[2], argv[3]);
    }
    else {
        fprintf(stderr, "Unknown command: %s\n\n", command);
        print_usage(argv[0]);
        return 1;
    }

    return 0;
}
