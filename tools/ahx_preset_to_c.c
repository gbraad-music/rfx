/*
 * Convert AHX preset to C header file
 * Generates C code with preset data embedded
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "ahx_preset.h"

void sanitize_name(const char* input, char* output) {
    int j = 0;
    for (int i = 0; input[i] && j < 63; i++) {
        if (isalnum(input[i])) {
            output[j++] = toupper(input[i]);
        } else if (input[i] == ' ' || input[i] == '-' || input[i] == '_') {
            output[j++] = '_';
        }
    }
    output[j] = '\0';
}

int main(int argc, char** argv) {
    if (argc != 3) {
        printf("Usage: %s <preset.ahxp> <output.h>\n", argv[0]);
        return 1;
    }
    
    const char* preset_path = argv[1];
    const char* output_path = argv[2];
    
    // Load preset
    AhxPreset preset;
    if (!ahx_preset_load(&preset, preset_path)) {
        fprintf(stderr, "Error: Failed to load preset %s\n", preset_path);
        return 1;
    }
    
    // Generate sanitized name
    char name[64];
    sanitize_name(preset.name, name);
    if (name[0] == '\0') {
        strcpy(name, "PRESET");
    }
    
    // Open output file
    FILE* out = fopen(output_path, "w");
    if (!out) {
        fprintf(stderr, "Error: Cannot create %s\n", output_path);
        ahx_preset_free(&preset);
        return 1;
    }
    
    // Write header
    fprintf(out, "// Auto-generated from %s\n", preset_path);
    fprintf(out, "#ifndef PRESET_%s_H\n", name);
    fprintf(out, "#define PRESET_%s_H\n\n", name);
    fprintf(out, "#include \"ahx_preset.h\"\n\n");
    
    // Write PList data if present
    if (preset.params.plist && preset.params.plist->length > 0) {
        fprintf(out, "static AhxPListEntry preset_%s_plist_entries[] = {\n", name);
        for (int i = 0; i < preset.params.plist->length; i++) {
            AhxPListEntry* e = &preset.params.plist->entries[i];
            fprintf(out, "    {%d, %d, %d, {%d, %d}, {%d, %d}},\n",
                e->note, e->fixed ? 1 : 0, e->waveform,
                e->fx[0], e->fx[1], e->fx_param[0], e->fx_param[1]);
        }
        fprintf(out, "};\n\n");
        
        fprintf(out, "static AhxPList preset_%s_plist = {\n", name);
        fprintf(out, "    .speed = %d,\n", preset.params.plist->speed);
        fprintf(out, "    .length = %d,\n", preset.params.plist->length);
        fprintf(out, "    .entries = preset_%s_plist_entries\n", name);
        fprintf(out, "};\n\n");
    }
    
    // Write preset params
    fprintf(out, "static AhxInstrumentParams preset_%s_params = {\n", name);
    fprintf(out, "    .waveform = %d,\n", preset.params.waveform);
    fprintf(out, "    .wave_length = %d,\n", preset.params.wave_length);
    fprintf(out, "    .volume = %d,\n", preset.params.volume);
    fprintf(out, "    .envelope = {\n");
    fprintf(out, "        .attack_frames = %d,\n", preset.params.envelope.attack_frames);
    fprintf(out, "        .attack_volume = %d,\n", preset.params.envelope.attack_volume);
    fprintf(out, "        .decay_frames = %d,\n", preset.params.envelope.decay_frames);
    fprintf(out, "        .decay_volume = %d,\n", preset.params.envelope.decay_volume);
    fprintf(out, "        .sustain_frames = %d,\n", preset.params.envelope.sustain_frames);
    fprintf(out, "        .release_frames = %d,\n", preset.params.envelope.release_frames);
    fprintf(out, "        .release_volume = %d\n", preset.params.envelope.release_volume);
    fprintf(out, "    },\n");
    fprintf(out, "    .filter_lower = %d,\n", preset.params.filter_lower);
    fprintf(out, "    .filter_upper = %d,\n", preset.params.filter_upper);
    fprintf(out, "    .filter_speed = %d,\n", preset.params.filter_speed);
    fprintf(out, "    .filter_enabled = %d,\n", preset.params.filter_enabled ? 1 : 0);
    fprintf(out, "    .square_lower = %d,\n", preset.params.square_lower);
    fprintf(out, "    .square_upper = %d,\n", preset.params.square_upper);
    fprintf(out, "    .square_speed = %d,\n", preset.params.square_speed);
    fprintf(out, "    .square_enabled = %d,\n", preset.params.square_enabled ? 1 : 0);
    fprintf(out, "    .vibrato_delay = %d,\n", preset.params.vibrato_delay);
    fprintf(out, "    .vibrato_depth = %d,\n", preset.params.vibrato_depth);
    fprintf(out, "    .vibrato_speed = %d,\n", preset.params.vibrato_speed);
    fprintf(out, "    .hard_cut_release = %d,\n", preset.params.hard_cut_release ? 1 : 0);
    fprintf(out, "    .hard_cut_frames = %d,\n", preset.params.hard_cut_frames);
    fprintf(out, "    .speed_multiplier = %d,\n", preset.params.speed_multiplier);
    
    if (preset.params.plist && preset.params.plist->length > 0) {
        fprintf(out, "    .plist = &preset_%s_plist\n", name);
    } else {
        fprintf(out, "    .plist = NULL\n");
    }
    
    fprintf(out, "};\n\n");
    
    // Write preset struct
    fprintf(out, "static AhxPreset preset_%s = {\n", name);
    fprintf(out, "    .name = \"%s\",\n", preset.name);
    fprintf(out, "    .author = \"%s\",\n", preset.author);
    fprintf(out, "    .description = \"%s\",\n", preset.description);
    fprintf(out, "    .params = preset_%s_params\n", name);
    fprintf(out, "};\n\n");
    
    fprintf(out, "#endif // PRESET_%s_H\n", name);
    
    fclose(out);
    ahx_preset_free(&preset);
    
    printf("Generated %s\n", output_path);
    return 0;
}
