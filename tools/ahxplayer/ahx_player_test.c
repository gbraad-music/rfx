/*
 * Simple SDL2-based AHX Player Test
 *
 * Usage: ./ahx_player_test <filename.ahx> [-o output.wav] [-c channels] [-s subsong]
 *
 * Options:
 *   -o <file>     Render to WAV file (use '-' for stdout)
 *   -c <1234>     Render only specified channels (e.g., -c 13 for channels 1 and 3)
 *   -s <num>      Select subsong (default 0)
 *
 * Controls (interactive mode):
 *   Space - Play/Pause
 *   Q - Quit
 *   1-4 - Toggle channel mute
 *   W/S - Next/previous subsong
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <SDL2/SDL.h>
#include "../../players/ahx_player.h"

#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#endif

#define SAMPLE_RATE 48000
#define BUFFER_SIZE 2048

typedef struct {
    AhxPlayer* player;
    bool playing;
    SDL_AudioDeviceID audio_device;
} PlayerContext;

// WAV file header structures
typedef struct {
    char riff[4];           // "RIFF"
    uint32_t file_size;     // File size - 8
    char wave[4];           // "WAVE"
} WAVHeader;

typedef struct {
    char fmt[4];            // "fmt "
    uint32_t chunk_size;    // 16 for PCM
    uint16_t format;        // 1 = PCM, 3 = IEEE float
    uint16_t channels;      // 2 for stereo
    uint32_t sample_rate;   // 48000
    uint32_t byte_rate;     // sample_rate * channels * bytes_per_sample
    uint16_t block_align;   // channels * bytes_per_sample
    uint16_t bits_per_sample; // 16 or 32
} WAVFmtChunk;

typedef struct {
    char data[4];           // "data"
    uint32_t data_size;     // Size of audio data
} WAVDataChunk;

// Write WAV header (16-bit PCM)
void write_wav_header(FILE* f, uint32_t num_samples) {
    uint32_t data_size = num_samples * 2 * 2;  // 2 channels * 2 bytes per sample

    WAVHeader header = {
        .riff = {'R', 'I', 'F', 'F'},
        .file_size = 36 + data_size,
        .wave = {'W', 'A', 'V', 'E'}
    };

    WAVFmtChunk fmt = {
        .fmt = {'f', 'm', 't', ' '},
        .chunk_size = 16,
        .format = 1,  // PCM
        .channels = 2,
        .sample_rate = SAMPLE_RATE,
        .byte_rate = SAMPLE_RATE * 2 * 2,
        .block_align = 4,
        .bits_per_sample = 16
    };

    WAVDataChunk data = {
        .data = {'d', 'a', 't', 'a'},
        .data_size = data_size
    };

    fwrite(&header, sizeof(header), 1, f);
    fwrite(&fmt, sizeof(fmt), 1, f);
    fwrite(&data, sizeof(data), 1, f);
}

// Render AHX file to WAV
bool render_to_wav(AhxPlayer* player, const char* output_file) {
    FILE* f = NULL;
    bool use_stdout = (strcmp(output_file, "-") == 0);

    if (use_stdout) {
        f = stdout;
        // Set binary mode on Windows
        #ifdef _WIN32
        _setmode(_fileno(stdout), _O_BINARY);
        #endif
    } else {
        f = fopen(output_file, "wb");
        if (!f) {
            fprintf(stderr, "Error: Could not create output file '%s'\n", output_file);
            return false;
        }
    }

    // Reserve space for header (we'll update it later with actual size)
    long header_pos = ftell(f);
    write_wav_header(f, 0);

    // Render audio
    const size_t render_frames = 4096;
    float* left = (float*)malloc(render_frames * sizeof(float));
    float* right = (float*)malloc(render_frames * sizeof(float));
    int16_t* output = (int16_t*)malloc(render_frames * 2 * sizeof(int16_t));

    if (!left || !right || !output) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        if (!use_stdout) fclose(f);
        free(left);
        free(right);
        free(output);
        return false;
    }

    // Disable looping so playback stops at end
    ahx_player_set_disable_looping(player, true);

    ahx_player_start(player);

    uint32_t total_samples = 0;
    uint32_t max_samples = SAMPLE_RATE * 60 * 5;  // Max 5 minutes

    if (!use_stdout) {
        fprintf(stderr, "Rendering");
    }

    while (ahx_player_is_playing(player) && total_samples < max_samples) {
        // Process audio
        ahx_player_process(player, left, right, render_frames, SAMPLE_RATE);

        // Convert float to 16-bit PCM and interleave
        for (size_t i = 0; i < render_frames; i++) {
            // Clamp to [-1.0, 1.0] and convert to 16-bit
            float l = left[i];
            float r = right[i];
            if (l > 1.0f) l = 1.0f;
            if (l < -1.0f) l = -1.0f;
            if (r > 1.0f) r = 1.0f;
            if (r < -1.0f) r = -1.0f;

            output[i * 2] = (int16_t)(l * 32767.0f);
            output[i * 2 + 1] = (int16_t)(r * 32767.0f);
        }

        // Write to file
        fwrite(output, sizeof(int16_t), render_frames * 2, f);
        total_samples += render_frames;

        // Progress indicator (every second)
        if (!use_stdout && (total_samples % SAMPLE_RATE) == 0) {
            fprintf(stderr, ".");
            fflush(stderr);
        }
    }

    if (!use_stdout) {
        fprintf(stderr, " done\n");
        fprintf(stderr, "Rendered %u samples (%.1f seconds)\n",
                total_samples, (float)total_samples / SAMPLE_RATE);
    }

    // Update header with actual size
    if (!use_stdout) {
        fseek(f, header_pos, SEEK_SET);
        write_wav_header(f, total_samples);
    }

    free(left);
    free(right);
    free(output);

    if (!use_stdout) fclose(f);

    return true;
}

// SDL audio callback
void audio_callback(void* userdata, Uint8* stream, int len) {
    PlayerContext* ctx = (PlayerContext*)userdata;

    size_t frames = len / (2 * sizeof(float));  // Stereo float samples
    float* left = (float*)malloc(frames * sizeof(float));
    float* right = (float*)malloc(frames * sizeof(float));
    float* output = (float*)stream;

    if (!left || !right) {
        memset(stream, 0, len);
        return;
    }

    if (ctx->playing) {
        ahx_player_process(ctx->player, left, right, frames, SAMPLE_RATE);

        // Interleave left and right
        for (size_t i = 0; i < frames; i++) {
            output[i * 2] = left[i];
            output[i * 2 + 1] = right[i];
        }
    } else {
        memset(stream, 0, len);
    }

    free(left);
    free(right);
}

// Position callback for display
void position_callback(uint8_t subsong, uint16_t position, uint16_t row, void* user_data) {
    // Optional: could print position updates
}

// Interactive player mode
void run_interactive(AhxPlayer* player) {
    // Setup SDL Audio
    SDL_AudioSpec want, have;
    SDL_zero(want);
    want.freq = SAMPLE_RATE;
    want.format = AUDIO_F32SYS;  // Float32 system byte order
    want.channels = 2;
    want.samples = BUFFER_SIZE;
    want.callback = audio_callback;

    PlayerContext ctx = {
        .player = player,
        .playing = true,
        .audio_device = 0
    };

    want.userdata = &ctx;

    SDL_AudioDeviceID device = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
    if (device == 0) {
        fprintf(stderr, "Error: Failed to open audio device: %s\n", SDL_GetError());
        return;
    }

    ctx.audio_device = device;

    // Set position callback
    ahx_player_set_position_callback(player, position_callback, NULL);

    // Start playback
    ahx_player_start(player);
    SDL_PauseAudioDevice(device, 0);  // Unpause

    printf("AHX Player - Interactive Mode\n");
    printf("Title: %s\n", ahx_player_get_title(player));
    printf("Subsongs: %d\n", ahx_player_get_num_subsongs(player));
    printf("Current subsong: %d\n", ahx_player_get_current_subsong(player));
    printf("\nControls:\n");
    printf("  Space - Play/Pause\n");
    printf("  Q - Quit\n");
    printf("  1-4 - Toggle channel mute\n");
    printf("  W/S - Next/Previous subsong\n");
    printf("\n");

    // Event loop
    SDL_Event event;
    bool running = true;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else if (event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                    case SDLK_SPACE:
                        ctx.playing = !ctx.playing;
                        if (ctx.playing) {
                            ahx_player_start(player);
                            printf("Playing...\n");
                        } else {
                            ahx_player_stop(player);
                            printf("Paused\n");
                        }
                        break;

                    case SDLK_q:
                        running = false;
                        break;

                    case SDLK_1:
                    case SDLK_2:
                    case SDLK_3:
                    case SDLK_4: {
                        uint8_t ch = event.key.keysym.sym - SDLK_1;
                        bool muted = !ahx_player_get_channel_mute(player, ch);
                        ahx_player_set_channel_mute(player, ch, muted);
                        printf("Channel %d: %s\n", ch + 1, muted ? "MUTED" : "UNMUTED");
                        break;
                    }

                    case SDLK_w: {
                        uint8_t current = ahx_player_get_current_subsong(player);
                        uint8_t num_subsongs = ahx_player_get_num_subsongs(player);
                        if (current + 1 < num_subsongs) {
                            ahx_player_set_subsong(player, current + 1);
                            printf("Subsong: %d\n", current + 1);
                            if (ctx.playing) {
                                ahx_player_start(player);
                            }
                        }
                        break;
                    }

                    case SDLK_s: {
                        uint8_t current = ahx_player_get_current_subsong(player);
                        if (current > 0) {
                            ahx_player_set_subsong(player, current - 1);
                            printf("Subsong: %d\n", current - 1);
                            if (ctx.playing) {
                                ahx_player_start(player);
                            }
                        }
                        break;
                    }
                }
            }
        }

        // Print position status
        if (ctx.playing) {
            uint16_t position, row;
            ahx_player_get_position(player, &position, &row);
            uint8_t subsong = ahx_player_get_current_subsong(player);
            printf("\rSubsong %d | Position %03d | Row %02d ", subsong, position, row);
            fflush(stdout);
        }

        SDL_Delay(100);
    }

    printf("\nStopping...\n");

    SDL_CloseAudioDevice(device);
}

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <filename.ahx> [-o output.wav] [-c channels] [-s subsong]\n", argv[0]);
        return 1;
    }

    const char* filename = argv[1];
    const char* output_file = NULL;
    const char* channel_mask = NULL;
    int subsong = 0;

    // Parse arguments
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            output_file = argv[++i];
        } else if (strcmp(argv[i], "-c") == 0 && i + 1 < argc) {
            channel_mask = argv[++i];
        } else if (strcmp(argv[i], "-s") == 0 && i + 1 < argc) {
            subsong = atoi(argv[++i]);
        }
    }

    // Load file
    FILE* f = fopen(filename, "rb");
    if (!f) {
        fprintf(stderr, "Error: Could not open file '%s'\n", filename);
        return 1;
    }

    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    fseek(f, 0, SEEK_SET);

    uint8_t* file_data = (uint8_t*)malloc(file_size);
    if (!file_data) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        fclose(f);
        return 1;
    }

    fread(file_data, 1, file_size, f);
    fclose(f);

    // Create player
    AhxPlayer* player = ahx_player_create();
    if (!player) {
        fprintf(stderr, "Error: Failed to create player\n");
        free(file_data);
        return 1;
    }

    // Load AHX file
    if (!ahx_player_load(player, file_data, file_size)) {
        fprintf(stderr, "Error: Failed to load AHX file (invalid format or corrupt file)\n");
        ahx_player_destroy(player);
        free(file_data);
        return 1;
    }

    free(file_data);

    // Set subsong
    if (subsong > 0 && subsong < ahx_player_get_num_subsongs(player)) {
        ahx_player_set_subsong(player, subsong);
    }

    // Apply channel mask
    if (channel_mask) {
        for (int i = 0; i < 4; i++) {
            bool enabled = false;
            char ch = '1' + i;
            for (const char* p = channel_mask; *p; p++) {
                if (*p == ch) {
                    enabled = true;
                    break;
                }
            }
            ahx_player_set_channel_mute(player, i, !enabled);
        }
    }

    bool success = false;

    if (output_file) {
        // Render mode
        success = render_to_wav(player, output_file);
    } else {
        // Interactive mode
        if (SDL_Init(SDL_INIT_AUDIO) != 0) {
            fprintf(stderr, "Error: Failed to initialize SDL: %s\n", SDL_GetError());
            ahx_player_destroy(player);
            return 1;
        }

        run_interactive(player);

        SDL_Quit();
        success = true;
    }

    ahx_player_destroy(player);

    return success ? 0 : 1;
}
