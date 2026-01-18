/**
 * RGSlicer - SFZ Import/Export
 */

#include "rgslicer.h"
#include "sfz_parser.h"
#include "wav_loader.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

// External access to RGSlicer internals
extern int rgslicer_add_slice(RGSlicer* slicer, uint32_t offset);
extern int rgslicer_load_sample(RGSlicer* slicer, const char* wav_path);

// ============================================================================
// SFZ Import
// ============================================================================

int rgslicer_import_sfz(RGSlicer* slicer, const char* sfz_path) {
    if (!slicer || !sfz_path) return 0;

    // Parse SFZ file
    SFZData* sfz = sfz_parse(sfz_path);
    if (!sfz) {
        printf("[RGSlicer] Failed to parse SFZ: %s\n", sfz_path);
        return 0;
    }

    if (sfz->num_regions == 0) {
        printf("[RGSlicer] No regions found in SFZ\n");
        sfz_free(sfz);
        return 0;
    }

    printf("[RGSlicer] Loaded SFZ with %d regions\n", sfz->num_regions);

    // Get WAV file path from first region (all should use same sample)
    const char* sample_path = sfz->regions[0].sample_path;
    if (!sample_path || sample_path[0] == '\0') {
        printf("[RGSlicer] No sample path in SFZ\n");
        sfz_free(sfz);
        return 0;
    }

    // Construct full path to WAV (assume same directory as SFZ)
    char wav_path[512];
    strncpy(wav_path, sfz_path, sizeof(wav_path) - 1);

    char* last_slash = strrchr(wav_path, '/');
    if (!last_slash) last_slash = strrchr(wav_path, '\\');
    if (last_slash) {
        *(last_slash + 1) = '\0';
        strncat(wav_path, sample_path, sizeof(wav_path) - strlen(wav_path) - 1);
    } else {
        strncpy(wav_path, sample_path, sizeof(wav_path) - 1);
    }

    // Load WAV sample
    if (!rgslicer_load_sample(slicer, wav_path)) {
        printf("[RGSlicer] Failed to load sample: %s\n", wav_path);
        sfz_free(sfz);
        return 0;
    }

    // Clear existing slices
    rgslicer_clear_slices(slicer);

    // Create slices from SFZ regions
    uint8_t loaded = 0;
    for (uint32_t i = 0; i < sfz->num_regions && i < RGSLICER_MAX_SLICES; i++) {
        SFZRegion* region = &sfz->regions[i];

        // Create slice at offset
        int slice_idx = rgslicer_add_slice(slicer, region->offset);
        if (slice_idx >= 0) {
            // Set slice parameters from SFZ (if available)
            // Note: Standard SFZ doesn't have pitch/time per-region for slicing
            // but we can read tune, volume, pan if present

            // For now, just use default parameters
            loaded++;

            printf("[RGSlicer] Slice %d: offset=%u end=%u lokey=%d hikey=%d\n",
                   slice_idx, region->offset, region->end,
                   region->lokey, region->hikey);
        }
    }

    sfz_free(sfz);

    printf("[RGSlicer] Imported %d slices from SFZ\n", loaded);

    return loaded;
}

// ============================================================================
// SFZ Export
// ============================================================================

int rgslicer_export_sfz(RGSlicer* slicer, const char* sfz_path, const char* wav_path) {
    if (!slicer || !sfz_path || !wav_path) return 0;
    if (!slicer->sample_loaded || slicer->num_slices == 0) {
        printf("[RGSlicer] Cannot export: no sample or slices\n");
        return 0;
    }

    FILE* f = fopen(sfz_path, "w");
    if (!f) {
        printf("[RGSlicer] Failed to create SFZ file: %s\n", sfz_path);
        return 0;
    }

    // Write SFZ header
    fprintf(f, "// RGSlicer Export: %s\n", slicer->sample_name);
    fprintf(f, "// Sample: %s\n", wav_path);
    fprintf(f, "// BPM: %.1f\n", slicer->bpm);
    fprintf(f, "// Slices: %d\n", slicer->num_slices);
    fprintf(f, "// Root Note: %d\n\n", slicer->root_note);

    // Global group settings
    fprintf(f, "<group>\n");
    fprintf(f, "amp_veltrack=100\n");
    fprintf(f, "ampeg_attack=0.001\n");
    fprintf(f, "ampeg_release=0.05\n");
    fprintf(f, "\n");

    // Write each slice as a region
    for (uint8_t i = 0; i < slicer->num_slices; i++) {
        uint8_t midi_note = 36 + i;  // C1 = 36

        fprintf(f, "<region> sample=%s lokey=%d hikey=%d pitch_keycenter=%d\n",
                wav_path, midi_note, midi_note, midi_note);

        fprintf(f, "         offset=%u end=%u\n",
                slicer->slices[i].offset,
                slicer->slices[i].end);

        // Export pitch/volume/pan if non-default
        if (fabsf(slicer->slices[i].pitch_semitones) > 0.01f) {
            fprintf(f, "         tune=%d\n", (int)(slicer->slices[i].pitch_semitones * 100));
        }

        if (fabsf(slicer->slices[i].volume - 1.0f) > 0.01f) {
            float volume_db = 20.0f * log10f(slicer->slices[i].volume);
            fprintf(f, "         volume=%.1f\n", volume_db);
        }

        if (fabsf(slicer->slices[i].pan) > 0.01f) {
            float pan_percent = slicer->slices[i].pan * 100.0f;
            fprintf(f, "         pan=%.0f\n", pan_percent);
        }

        if (slicer->slices[i].loop) {
            fprintf(f, "         loop_mode=loop_continuous\n");
        }

        fprintf(f, "\n");
    }

    fclose(f);

    printf("[RGSlicer] Exported %d slices to %s\n", slicer->num_slices, sfz_path);

    return 1;
}
