/*
 * AHX Wave Dump Tool
 *
 * Dumps waveform tables and renders presets to WAV files for analysis
 *
 * Usage:
 *   ahx_wave_dump --dump-waves output_dir/       # Dump all waveform tables
 *   ahx_wave_dump --render preset.ahxp note output.wav [--sustain]
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include "../../synth/ahx_waves.h"
#include "../../synth/ahx_preset.h"
#include "../../synth/ahx_instrument.h"
#include "../../synth/ahx_synth_core.h"

// Simple WAV header structure
typedef struct {
    char riff[4];           // "RIFF"
    uint32_t file_size;     // File size - 8
    char wave[4];           // "WAVE"
    char fmt[4];            // "fmt "
    uint32_t fmt_size;      // 16 for PCM
    uint16_t format;        // 1 = PCM
    uint16_t channels;      // 1 = mono, 2 = stereo
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;
    char data[4];           // "data"
    uint32_t data_size;
} WavHeader;

void write_wav_header(FILE* f, uint32_t sample_rate, uint16_t channels, uint32_t num_samples) {
    WavHeader header;
    memcpy(header.riff, "RIFF", 4);
    memcpy(header.wave, "WAVE", 4);
    memcpy(header.fmt, "fmt ", 4);
    memcpy(header.data, "data", 4);

    header.fmt_size = 16;
    header.format = 1;  // PCM
    header.channels = channels;
    header.sample_rate = sample_rate;
    header.bits_per_sample = 16;
    header.block_align = channels * 2;
    header.byte_rate = sample_rate * header.block_align;
    header.data_size = num_samples * header.block_align;
    header.file_size = header.data_size + sizeof(WavHeader) - 8;

    fwrite(&header, sizeof(WavHeader), 1, f);
}

// Dump a single waveform table to WAV
int dump_waveform_to_wav(const char* filename, const int16_t* buffer, uint32_t length, uint32_t sample_rate) {
    FILE* f = fopen(filename, "wb");
    if (!f) {
        printf("Error: Cannot create %s\n", filename);
        return 0;
    }

    write_wav_header(f, sample_rate, 1, length);
    fwrite(buffer, sizeof(int16_t), length, f);
    fclose(f);

    printf("Wrote %s (%u samples)\n", filename, length);
    return 1;
}

// Dump all waveform tables
int dump_all_waveforms(const char* output_dir) {
    char filepath[512];

    // Get waves instance
    AhxWaves* waves = ahx_waves_get();
    if (!waves) {
        printf("Error: Failed to initialize waves\n");
        return 0;
    }

    printf("Dumping waveform tables to %s\n", output_dir);

    // Dump triangles
    snprintf(filepath, sizeof(filepath), "%s/triangle_04.wav", output_dir);
    dump_waveform_to_wav(filepath, waves->Triangle04, 0x04, 48000);

    snprintf(filepath, sizeof(filepath), "%s/triangle_08.wav", output_dir);
    dump_waveform_to_wav(filepath, waves->Triangle08, 0x08, 48000);

    snprintf(filepath, sizeof(filepath), "%s/triangle_10.wav", output_dir);
    dump_waveform_to_wav(filepath, waves->Triangle10, 0x10, 48000);

    snprintf(filepath, sizeof(filepath), "%s/triangle_20.wav", output_dir);
    dump_waveform_to_wav(filepath, waves->Triangle20, 0x20, 48000);

    snprintf(filepath, sizeof(filepath), "%s/triangle_40.wav", output_dir);
    dump_waveform_to_wav(filepath, waves->Triangle40, 0x40, 48000);

    snprintf(filepath, sizeof(filepath), "%s/triangle_80.wav", output_dir);
    dump_waveform_to_wav(filepath, waves->Triangle80, 0x80, 48000);

    // Dump sawtooths
    snprintf(filepath, sizeof(filepath), "%s/sawtooth_04.wav", output_dir);
    dump_waveform_to_wav(filepath, waves->Sawtooth04, 0x04, 48000);

    snprintf(filepath, sizeof(filepath), "%s/sawtooth_08.wav", output_dir);
    dump_waveform_to_wav(filepath, waves->Sawtooth08, 0x08, 48000);

    snprintf(filepath, sizeof(filepath), "%s/sawtooth_10.wav", output_dir);
    dump_waveform_to_wav(filepath, waves->Sawtooth10, 0x10, 48000);

    snprintf(filepath, sizeof(filepath), "%s/sawtooth_20.wav", output_dir);
    dump_waveform_to_wav(filepath, waves->Sawtooth20, 0x20, 48000);

    snprintf(filepath, sizeof(filepath), "%s/sawtooth_40.wav", output_dir);
    dump_waveform_to_wav(filepath, waves->Sawtooth40, 0x40, 48000);

    snprintf(filepath, sizeof(filepath), "%s/sawtooth_80.wav", output_dir);
    dump_waveform_to_wav(filepath, waves->Sawtooth80, 0x80, 48000);

    // Dump squares (all 32 variations)
    snprintf(filepath, sizeof(filepath), "%s/squares_all.wav", output_dir);
    dump_waveform_to_wav(filepath, waves->Squares, 0x80 * 0x20, 48000);

    // Dump noise
    snprintf(filepath, sizeof(filepath), "%s/noise.wav", output_dir);
    dump_waveform_to_wav(filepath, waves->WhiteNoiseBig, 0x280 * 3, 48000);

    // Dump filtered triangles (all filter positions 32-63 to see the progression)
    printf("\nDumping filtered triangles (wave_length=4, FilterPos 32-63):\n");
    for (int filter_pos = 32; filter_pos <= 63; filter_pos++) {
        const int16_t* tri_filt = ahx_waves_get_waveform(waves, 0, 4, filter_pos);
        if (tri_filt) {
            snprintf(filepath, sizeof(filepath), "%s/triangle_40_filt%02d.wav", output_dir, filter_pos);
            dump_waveform_to_wav(filepath, tri_filt, 0x40, 48000);
        }
    }

    // Dump filtered sawtooths (all filter positions)
    printf("\nDumping filtered sawtooths (wave_length=4, FilterPos 32-63):\n");
    for (int filter_pos = 32; filter_pos <= 63; filter_pos++) {
        const int16_t* saw_filt = ahx_waves_get_waveform(waves, 1, 4, filter_pos);
        if (saw_filt) {
            snprintf(filepath, sizeof(filepath), "%s/sawtooth_40_filt%02d.wav", output_dir, filter_pos);
            dump_waveform_to_wav(filepath, saw_filt, 0x40, 48000);
        }
    }

    // Dump some squares with different filter positions
    printf("\nDumping filtered squares (wave_length=4, SquarePos=16, FilterPos 32/40/50/63):\n");
    int16_t square_buffer[0x281];
    int square_reverse;
    for (int filter_pos = 32; filter_pos <= 63; filter_pos += 10) {
        if (filter_pos > 63) filter_pos = 63;
        ahx_waves_generate_square(waves, square_buffer, 16, 4, filter_pos, &square_reverse);
        snprintf(filepath, sizeof(filepath), "%s/square_pos16_filt%02d.wav", output_dir, filter_pos);
        dump_waveform_to_wav(filepath, square_buffer, 0x40, 48000);
    }

    printf("\nDone! Dumped all waveforms to %s\n", output_dir);
    return 1;
}

// Render a preset to WAV
int render_preset(const char* preset_path, int midi_note, const char* output_path, int sustain) {
    // Load preset
    AhxPreset preset;
    if (!ahx_preset_load(&preset, preset_path)) {
        printf("Error: Failed to load preset %s\n", preset_path);
        return 0;
    }

    printf("Loaded preset: %s\n", preset.name);
    printf("Rendering note %d (%s)...\n", midi_note, sustain ? "with sustain" : "one-shot");

    // Create instrument
    AhxInstrument instrument;
    ahx_instrument_init(&instrument);
    ahx_instrument_set_params(&instrument, &preset.params);

    // Trigger note
    const uint32_t sample_rate = 48000;
    ahx_instrument_note_on(&instrument, midi_note, 127, sample_rate);

    // Allocate buffers (48kHz, 5 seconds max)
    const uint32_t max_samples = sample_rate * 5;  // 5 seconds
    float* float_buffer = (float*)malloc(max_samples * sizeof(float));
    int16_t* int16_buffer = (int16_t*)malloc(max_samples * sizeof(int16_t));
    if (!float_buffer || !int16_buffer) {
        printf("Error: Out of memory\n");
        free(float_buffer);
        free(int16_buffer);
        ahx_preset_free(&preset);
        return 0;
    }

    uint32_t num_samples = 0;

    if (sustain) {
        // Render 1 second, then note off, then render until silent
        uint32_t sustain_samples = sample_rate * 1;  // 1 second sustain

        // Sustain phase
        while (num_samples < sustain_samples && num_samples < max_samples) {
            uint32_t chunk_size = sustain_samples - num_samples;
            if (chunk_size > 1024) chunk_size = 1024;

            uint32_t rendered = ahx_instrument_process(&instrument,
                                                       float_buffer + num_samples,
                                                       chunk_size, sample_rate);
            num_samples += rendered;

            if (rendered < chunk_size) break;  // Instrument stopped early
        }

        // Note off
        ahx_instrument_note_off(&instrument);

        // Release phase - continue rendering until silent or max reached
        int silent_count = 0;
        while (num_samples < max_samples) {
            uint32_t chunk_size = 1024;
            if (num_samples + chunk_size > max_samples) {
                chunk_size = max_samples - num_samples;
            }

            uint32_t rendered = ahx_instrument_process(&instrument,
                                                       float_buffer + num_samples,
                                                       chunk_size, sample_rate);

            // Check if silent
            int chunk_silent = 1;
            for (uint32_t i = 0; i < rendered; i++) {
                if (fabs(float_buffer[num_samples + i]) > 0.0003f) {  // ~10/32768
                    chunk_silent = 0;
                    silent_count = 0;
                    break;
                }
            }

            num_samples += rendered;

            if (chunk_silent) {
                silent_count += rendered;
                if (silent_count > sample_rate / 10) {  // 0.1s of silence
                    break;
                }
            }

            if (rendered < chunk_size) break;  // Instrument stopped
        }
    } else {
        // Render until silent or max reached
        int silent_count = 0;
        while (num_samples < max_samples) {
            uint32_t chunk_size = 1024;
            if (num_samples + chunk_size > max_samples) {
                chunk_size = max_samples - num_samples;
            }

            uint32_t rendered = ahx_instrument_process(&instrument,
                                                       float_buffer + num_samples,
                                                       chunk_size, sample_rate);

            // Check if silent
            int chunk_silent = 1;
            for (uint32_t i = 0; i < rendered; i++) {
                if (fabs(float_buffer[num_samples + i]) > 0.0003f) {
                    chunk_silent = 0;
                    silent_count = 0;
                    break;
                }
            }

            num_samples += rendered;

            if (chunk_silent) {
                silent_count += rendered;
                if (silent_count > sample_rate / 10) {  // 0.1s of silence
                    break;
                }
            }

            if (rendered < chunk_size) break;  // Instrument stopped
        }
    }

    // Convert float to int16
    for (uint32_t i = 0; i < num_samples; i++) {
        float sample = float_buffer[i] * 32767.0f;
        if (sample > 32767.0f) sample = 32767.0f;
        if (sample < -32768.0f) sample = -32768.0f;
        int16_buffer[i] = (int16_t)sample;
    }

    // Write WAV file
    FILE* f = fopen(output_path, "wb");
    if (!f) {
        printf("Error: Cannot create %s\n", output_path);
        free(float_buffer);
        free(int16_buffer);
        ahx_preset_free(&preset);
        return 0;
    }

    write_wav_header(f, sample_rate, 1, num_samples);
    fwrite(int16_buffer, sizeof(int16_t), num_samples, f);
    fclose(f);

    printf("Wrote %s (%.2f seconds, %u samples)\n",
           output_path, (float)num_samples / sample_rate, num_samples);

    free(float_buffer);
    free(int16_buffer);
    ahx_preset_free(&preset);
    return 1;
}

void print_usage(const char* prog) {
    printf("AHX Wave Dump Tool\n");
    printf("Usage:\n");
    printf("  %s --dump-waves <output_dir>                    Dump all waveform tables\n", prog);
    printf("  %s --render <preset.ahxp> <note> <output.wav>  Render preset to WAV\n", prog);
    printf("  %s --render <preset.ahxp> <note> <output.wav> --sustain\n", prog);
    printf("\nExamples:\n");
    printf("  %s --dump-waves ./waves/\n", prog);
    printf("  %s --render chopper_03.ahxp 60 test.wav\n", prog);
    printf("  %s --render chopper_03.ahxp 60 test.wav --sustain\n", prog);
}

int main(int argc, char** argv) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "--dump-waves") == 0) {
        if (argc < 3) {
            printf("Error: Missing output directory\n");
            print_usage(argv[0]);
            return 1;
        }

        return dump_all_waveforms(argv[2]) ? 0 : 1;
    }
    else if (strcmp(argv[1], "--render") == 0) {
        if (argc < 5) {
            printf("Error: Missing arguments\n");
            print_usage(argv[0]);
            return 1;
        }

        const char* preset_path = argv[2];
        int midi_note = atoi(argv[3]);
        const char* output_path = argv[4];
        int sustain = (argc > 5 && strcmp(argv[5], "--sustain") == 0);

        return render_preset(preset_path, midi_note, output_path, sustain) ? 0 : 1;
    }
    else {
        printf("Error: Unknown command %s\n", argv[1]);
        print_usage(argv[0]);
        return 1;
    }
}
