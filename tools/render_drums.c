/*
 * RG909 Drum Renderer - Direct WAV output
 * Compile: gcc -o render_drums render_drums.c rg909_drum_synth.c synth_*.c -lm -O3
 * Usage: ./render_drums
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "rg909_drum_synth.h"

// WAV file header structure
typedef struct {
    char riff[4];           // "RIFF"
    uint32_t file_size;     // File size - 8
    char wave[4];           // "WAVE"
    char fmt[4];            // "fmt "
    uint32_t fmt_size;      // 16 for PCM
    uint16_t audio_format;  // 1 for PCM
    uint16_t channels;      // 1 = mono, 2 = stereo
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;
    char data[4];           // "data"
    uint32_t data_size;
} WavHeader;

void write_wav_file(const char* filename, float* samples, int num_samples, int sample_rate) {
    FILE* f = fopen(filename, "wb");
    if (!f) {
        fprintf(stderr, "Failed to open %s\n", filename);
        return;
    }

    // Convert float to 16-bit PCM
    int16_t* pcm = malloc(num_samples * sizeof(int16_t));
    for (int i = 0; i < num_samples; i++) {
        float s = samples[i];
        if (s > 1.0f) s = 1.0f;
        if (s < -1.0f) s = -1.0f;
        pcm[i] = (int16_t)(s * 32767.0f);
    }

    // Write WAV header
    WavHeader header;
    memcpy(header.riff, "RIFF", 4);
    header.file_size = 36 + num_samples * 2;
    memcpy(header.wave, "WAVE", 4);
    memcpy(header.fmt, "fmt ", 4);
    header.fmt_size = 16;
    header.audio_format = 1;
    header.channels = 1;
    header.sample_rate = sample_rate;
    header.byte_rate = sample_rate * 2;
    header.block_align = 2;
    header.bits_per_sample = 16;
    memcpy(header.data, "data", 4);
    header.data_size = num_samples * 2;

    fwrite(&header, sizeof(WavHeader), 1, f);
    fwrite(pcm, sizeof(int16_t), num_samples, f);

    fclose(f);
    free(pcm);
}

void render_drum(RG909Synth* synth, uint8_t note, const char* name, float duration, int sample_rate) {
    int num_frames = (int)(duration * sample_rate);
    float* buffer = calloc(num_frames * 2, sizeof(float));  // Stereo interleaved

    // Trigger drum
    rg909_synth_trigger_drum(synth, note, 127, sample_rate);

    // Process audio
    rg909_synth_process_interleaved(synth, buffer, num_frames, sample_rate);

    // Mix stereo to mono
    float* mono = malloc(num_frames * sizeof(float));
    for (int i = 0; i < num_frames; i++) {
        mono[i] = (buffer[i * 2] + buffer[i * 2 + 1]) * 0.5f;
    }

    // Write to file
    char filename[256];
    snprintf(filename, sizeof(filename), "RG909_%s.wav", name);
    write_wav_file(filename, mono, num_frames, sample_rate);

    printf("✓ Rendered: %s\n", filename);

    free(buffer);
    free(mono);

    // Reset for next drum
    rg909_synth_reset(synth);
}

int main() {
    const int sample_rate = 44100;
    const float duration = 0.5f;  // 500ms - matches real TR-909 samples

    printf("RG909 Drum Renderer\n");
    printf("===================\n\n");

    RG909Synth* synth = rg909_synth_create();
    if (!synth) {
        fprintf(stderr, "Failed to create synth\n");
        return 1;
    }

    // Render all drums
    render_drum(synth, 36, "BD_BassDrum", duration, sample_rate);
    render_drum(synth, 38, "SD_Snare", duration, sample_rate);
    render_drum(synth, 37, "RS_Rimshot", duration, sample_rate);
    render_drum(synth, 39, "HC_HandClap", duration, sample_rate);
    render_drum(synth, 41, "LT_TomLow", duration, sample_rate);
    render_drum(synth, 47, "MT_TomMid", duration, sample_rate);
    render_drum(synth, 50, "HT_TomHigh", duration, sample_rate);

    rg909_synth_destroy(synth);

    printf("\n✅ All drums rendered successfully!\n");
    return 0;
}
