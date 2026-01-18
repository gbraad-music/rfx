/**
 * WAV CUE Chunk Implementation
 */

#include "wav_cue.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// WAV chunk headers
typedef struct {
    char id[4];
    uint32_t size;
} ChunkHeader;

// CUE point in file format
typedef struct {
    uint32_t id;
    uint32_t position;
    char data_chunk_id[4];
    uint32_t chunk_start;
    uint32_t block_start;
    uint32_t sample_offset;
} CuePointFile;

// Label (adtl/labl) in file format
typedef struct {
    uint32_t cue_id;
    char text[1];  // Variable length
} LabelFile;

// ============================================================================
// CUE Reading
// ============================================================================

WAVCueData* wav_cue_read(const char* wav_path) {
    FILE* f = fopen(wav_path, "rb");
    if (!f) return NULL;

    // Read RIFF header
    char riff[4];
    fread(riff, 1, 4, f);
    if (memcmp(riff, "RIFF", 4) != 0) {
        fclose(f);
        return NULL;
    }

    uint32_t file_size;
    fread(&file_size, 4, 1, f);

    char wave[4];
    fread(wave, 1, 4, f);
    if (memcmp(wave, "WAVE", 4) != 0) {
        fclose(f);
        return NULL;
    }

    WAVCueData* cue_data = (WAVCueData*)calloc(1, sizeof(WAVCueData));
    if (!cue_data) {
        fclose(f);
        return NULL;
    }

    // Search for 'cue ' and 'adtl' chunks
    ChunkHeader chunk;
    bool found_cue = false;

    while (fread(&chunk, sizeof(ChunkHeader), 1, f) == 1) {
        if (memcmp(chunk.id, "cue ", 4) == 0) {
            // Read CUE chunk
            uint32_t num_cue_points;
            fread(&num_cue_points, 4, 1, f);

            if (num_cue_points > MAX_CUE_POINTS) {
                num_cue_points = MAX_CUE_POINTS;
            }

            for (uint32_t i = 0; i < num_cue_points; i++) {
                CuePointFile cp;
                fread(&cp, sizeof(CuePointFile), 1, f);

                cue_data->points[i].position = cp.sample_offset;
                cue_data->points[i].cue_id = cp.id;
                cue_data->points[i].label[0] = '\0';  // Will be filled from labl
                cue_data->num_points++;
            }

            found_cue = true;
        }
        else if (memcmp(chunk.id, "LIST", 4) == 0) {
            // Check if this is adtl list
            uint32_t list_start = ftell(f);
            char list_type[4];
            fread(list_type, 1, 4, f);

            if (memcmp(list_type, "adtl", 4) == 0) {
                // Read labels
                uint32_t list_end = list_start + chunk.size;

                while (ftell(f) < list_end) {
                    ChunkHeader subchunk;
                    if (fread(&subchunk, sizeof(ChunkHeader), 1, f) != 1) break;

                    if (memcmp(subchunk.id, "labl", 4) == 0) {
                        uint32_t cue_id;
                        fread(&cue_id, 4, 1, f);

                        // Read label text
                        char label_text[256];
                        uint32_t text_size = subchunk.size - 4;
                        if (text_size > 255) text_size = 255;
                        fread(label_text, 1, text_size, f);
                        label_text[text_size] = '\0';

                        // Find matching CUE point
                        for (uint32_t i = 0; i < cue_data->num_points; i++) {
                            if (cue_data->points[i].cue_id == cue_id) {
                                strncpy(cue_data->points[i].label, label_text, 31);
                                cue_data->points[i].label[31] = '\0';
                                break;
                            }
                        }

                        // Align to even byte
                        if (text_size & 1) fseek(f, 1, SEEK_CUR);
                    } else {
                        // Skip unknown subchunk
                        fseek(f, subchunk.size + (subchunk.size & 1), SEEK_CUR);
                    }
                }
            } else {
                // Skip non-adtl LIST
                fseek(f, chunk.size - 4, SEEK_CUR);
            }
        }
        else {
            // Skip unknown chunk
            fseek(f, chunk.size + (chunk.size & 1), SEEK_CUR);
        }
    }

    fclose(f);

    if (!found_cue) {
        free(cue_data);
        return NULL;
    }

    return cue_data;
}

// ============================================================================
// CUE Writing
// ============================================================================

int wav_cue_write(const int16_t* sample_data, uint32_t num_samples,
                  uint32_t sample_rate, const WAVCueData* cue_data,
                  const char* output_path) {
    if (!sample_data || !output_path || num_samples == 0) return 0;

    FILE* f = fopen(output_path, "wb");
    if (!f) {
        printf("[WAV_CUE] Failed to create file: %s\n", output_path);
        return 0;
    }

    uint32_t data_size = num_samples * 2;  // 16-bit mono
    uint32_t cue_chunk_size = 0;
    uint32_t adtl_chunk_size = 0;

    // Calculate CUE chunk size
    if (cue_data && cue_data->num_points > 0) {
        cue_chunk_size = 4 + (cue_data->num_points * sizeof(CuePointFile));

        // Calculate adtl chunk size
        for (uint32_t i = 0; i < cue_data->num_points; i++) {
            uint32_t label_len = strlen(cue_data->points[i].label) + 1;  // +1 for null
            if (label_len & 1) label_len++;  // Pad to even
            adtl_chunk_size += 8 + 4 + label_len;  // chunk header + cue_id + text
        }
        adtl_chunk_size += 4;  // "adtl" identifier
    }

    uint32_t file_size = 4 + 8 + 16 + 8 + data_size;  // WAVE + fmt + data
    if (cue_chunk_size > 0) {
        file_size += 8 + cue_chunk_size;  // cue chunk
        file_size += 8 + adtl_chunk_size;  // LIST/adtl chunk
    }

    // Write RIFF header
    fwrite("RIFF", 1, 4, f);
    fwrite(&file_size, 4, 1, f);
    fwrite("WAVE", 1, 4, f);

    // Write fmt chunk
    fwrite("fmt ", 1, 4, f);
    uint32_t fmt_size = 16;
    fwrite(&fmt_size, 4, 1, f);
    uint16_t audio_format = 1;  // PCM
    uint16_t num_channels = 1;  // Mono
    uint16_t bits_per_sample = 16;
    uint32_t byte_rate = sample_rate * num_channels * (bits_per_sample / 8);
    uint16_t block_align = num_channels * (bits_per_sample / 8);

    fwrite(&audio_format, 2, 1, f);
    fwrite(&num_channels, 2, 1, f);
    fwrite(&sample_rate, 4, 1, f);
    fwrite(&byte_rate, 4, 1, f);
    fwrite(&block_align, 2, 1, f);
    fwrite(&bits_per_sample, 2, 1, f);

    // Write data chunk
    fwrite("data", 1, 4, f);
    fwrite(&data_size, 4, 1, f);
    fwrite(sample_data, 2, num_samples, f);

    // Write CUE chunk if present
    if (cue_data && cue_data->num_points > 0) {
        fwrite("cue ", 1, 4, f);
        fwrite(&cue_chunk_size, 4, 1, f);
        fwrite(&cue_data->num_points, 4, 1, f);

        for (uint32_t i = 0; i < cue_data->num_points; i++) {
            CuePointFile cp;
            cp.id = cue_data->points[i].cue_id;
            cp.position = 0;
            memcpy(cp.data_chunk_id, "data", 4);
            cp.chunk_start = 0;
            cp.block_start = 0;
            cp.sample_offset = cue_data->points[i].position;

            fwrite(&cp, sizeof(CuePointFile), 1, f);
        }

        // Write LIST/adtl chunk
        fwrite("LIST", 1, 4, f);
        fwrite(&adtl_chunk_size, 4, 1, f);
        fwrite("adtl", 1, 4, f);

        for (uint32_t i = 0; i < cue_data->num_points; i++) {
            uint32_t label_len = strlen(cue_data->points[i].label) + 1;
            uint32_t padded_len = label_len;
            if (padded_len & 1) padded_len++;

            uint32_t labl_size = 4 + label_len;
            if (labl_size & 1) labl_size++;

            fwrite("labl", 1, 4, f);
            fwrite(&labl_size, 4, 1, f);
            fwrite(&cue_data->points[i].cue_id, 4, 1, f);
            fwrite(cue_data->points[i].label, 1, label_len, f);

            // Pad to even byte
            if (label_len & 1) {
                uint8_t pad = 0;
                fwrite(&pad, 1, 1, f);
            }
        }
    }

    fclose(f);

    printf("[WAV_CUE] Wrote WAV file with %u CUE points: %s\n",
           cue_data ? cue_data->num_points : 0, output_path);

    return 1;
}

// ============================================================================
// CUE Utilities
// ============================================================================

WAVCueData* wav_cue_create_from_slices(const uint32_t* slice_offsets,
                                        uint8_t num_slices,
                                        const uint32_t* loop_offsets,
                                        uint8_t start_note) {
    if (!slice_offsets || num_slices == 0 || num_slices > 64) return NULL;

    WAVCueData* cue_data = (WAVCueData*)calloc(1, sizeof(WAVCueData));
    if (!cue_data) return NULL;

    uint32_t cue_id = 1;

    // Create CUE points for slices
    for (uint8_t i = 0; i < num_slices; i++) {
        cue_data->points[cue_data->num_points].position = slice_offsets[i];
        cue_data->points[cue_data->num_points].cue_id = cue_id++;
        snprintf(cue_data->points[cue_data->num_points].label, 32, "%d", start_note + i);
        cue_data->num_points++;
    }

    // Create loop points if provided
    if (loop_offsets) {
        for (uint8_t i = 0; i < num_slices; i++) {
            if (loop_offsets[i] > 0) {
                cue_data->points[cue_data->num_points].position = loop_offsets[i];
                cue_data->points[cue_data->num_points].cue_id = cue_id++;
                snprintf(cue_data->points[cue_data->num_points].label, 32, "%d-loop", start_note + i);
                cue_data->num_points++;
            }
        }
    }

    return cue_data;
}

uint8_t wav_cue_extract_slices(const WAVCueData* cue_data,
                                uint32_t* slice_offsets,
                                uint8_t max_slices) {
    if (!cue_data || !slice_offsets || max_slices == 0) return 0;

    uint8_t num_slices = 0;

    // Extract CUE points that are MIDI note labels (not loop points)
    for (uint32_t i = 0; i < cue_data->num_points && num_slices < max_slices; i++) {
        const char* label = cue_data->points[i].label;

        // Skip loop points (contain "-loop")
        if (strstr(label, "-loop") != NULL) continue;

        // Try to parse as MIDI note number
        int note = atoi(label);
        if (note >= 0 && note <= 127) {
            slice_offsets[num_slices] = cue_data->points[i].position;
            num_slices++;
        }
    }

    return num_slices;
}

void wav_cue_free(WAVCueData* cue_data) {
    if (cue_data) {
        free(cue_data);
    }
}
