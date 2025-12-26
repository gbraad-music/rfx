/*
 * Test program for RG909 kick drum synthesis
 * Renders a kick drum to a WAV file for testing
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include "../synth/rg909_drum_synth.h"

// WAV file header structure
typedef struct {
    char riff[4];          // "RIFF"
    uint32_t file_size;    // File size - 8
    char wave[4];          // "WAVE"
    char fmt[4];           // "fmt "
    uint32_t fmt_size;     // 16 for PCM
    uint16_t format;       // 1 for PCM
    uint16_t channels;     // 2 for stereo
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;
    char data[4];          // "data"
    uint32_t data_size;
} WAVHeader;

void write_wav(const char* filename, float* buffer, int frames, int sample_rate) {
    FILE* fp = fopen(filename, "wb");
    if (!fp) {
        fprintf(stderr, "Error: Could not open %s for writing\n", filename);
        return;
    }

    WAVHeader header;
    memcpy(header.riff, "RIFF", 4);
    memcpy(header.wave, "WAVE", 4);
    memcpy(header.fmt, "fmt ", 4);
    memcpy(header.data, "data", 4);

    header.fmt_size = 16;
    header.format = 1;  // PCM
    header.channels = 2;
    header.sample_rate = sample_rate;
    header.bits_per_sample = 16;
    header.block_align = header.channels * header.bits_per_sample / 8;
    header.byte_rate = sample_rate * header.block_align;
    header.data_size = frames * header.block_align;
    header.file_size = 36 + header.data_size;

    fwrite(&header, sizeof(WAVHeader), 1, fp);

    // Convert float to int16
    for (int i = 0; i < frames * 2; i++) {
        int16_t sample = (int16_t)(buffer[i] * 32767.0f);
        fwrite(&sample, sizeof(int16_t), 1, fp);
    }

    fclose(fp);
    printf("✓ Wrote %s: %d frames, %d Hz\n", filename, frames, sample_rate);
}

int main(int argc, char** argv) {
    printf("RG909 Kick Drum Test\n");
    printf("====================\n\n");

    // Parse command line arguments
    float bd_level = 0.96f;
    float bd_decay = 0.13f;
    float duration_ms = 200.0f;
    int sample_rate = 48000;

    if (argc > 1) bd_level = atof(argv[1]);
    if (argc > 2) bd_decay = atof(argv[2]);
    if (argc > 3) duration_ms = atof(argv[3]);

    printf("Parameters:\n");
    printf("  bd_level: %.2f\n", bd_level);
    printf("  bd_decay: %.2f\n", bd_decay);
    printf("  duration: %.1f ms\n", duration_ms);
    printf("  sample_rate: %d Hz\n\n", sample_rate);

    // Create synth
    RG909Synth* synth = rg909_synth_create();
    if (!synth) {
        fprintf(stderr, "Error: Could not create synth\n");
        return 1;
    }

    // Set parameters
    rg909_synth_set_parameter(synth, 0, bd_level);   // PARAM_BD_LEVEL
    rg909_synth_set_parameter(synth, 2, bd_decay);   // PARAM_BD_DECAY

    // Trigger bass drum (MIDI note 36)
    rg909_synth_trigger_drum(synth, 36, 127, sample_rate);

    // Render
    int frames = (int)(duration_ms * sample_rate / 1000);
    float* buffer = (float*)malloc(frames * 2 * sizeof(float));
    if (!buffer) {
        fprintf(stderr, "Error: Could not allocate buffer\n");
        rg909_synth_destroy(synth);
        return 1;
    }

    memset(buffer, 0, frames * 2 * sizeof(float));
    rg909_synth_process_interleaved(synth, buffer, frames, sample_rate);

    // Calculate statistics
    float peak = 0.0f;
    float sum_squares = 0.0f;
    for (int i = 0; i < frames * 2; i += 2) {  // Left channel only
        float abs_val = fabsf(buffer[i]);
        if (abs_val > peak) peak = abs_val;
        sum_squares += buffer[i] * buffer[i];
    }
    float rms = sqrtf(sum_squares / frames);

    printf("Output statistics:\n");
    printf("  Peak amplitude: %.6f\n", peak);
    printf("  RMS: %.6f\n", rms);
    printf("  Samples: %d\n\n", frames);

    // Save WAV
    write_wav("test_c_kick.wav", buffer, frames, sample_rate);

    // Print key sample values
    printf("\nKey transitions:\n");
    float key_times[] = {1.5f, 10.1f, 31.5f, 74.0f};
    for (int i = 0; i < 4; i++) {
        int idx = (int)(key_times[i] * sample_rate / 1000);
        if (idx < frames) {
            printf("  %5.1f ms: %+.6f\n", key_times[i], buffer[idx * 2]);
        }
    }

    // Cleanup
    free(buffer);
    rg909_synth_destroy(synth);

    printf("\n✓ Test complete\n");
    return 0;
}
