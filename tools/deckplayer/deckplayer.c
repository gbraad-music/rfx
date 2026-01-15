/*
 * Deck Player Test Tool
 * Tests the unified deck player with MOD/MED/AHX file support
 *
 * Usage: ./deckplayer <filename> [-o output.wav] [-t seconds]
 *
 * Options:
 *   -o <file>     Render to WAV file (use '-' for stdout)
 *   -t <seconds>  Time limit in seconds (default: play full song)
 *
 * Controls (interactive mode):
 *   Space - Play/Pause
 *   Q - Quit
 *   1-4 - Toggle channel mute
 */

#include "../../players/deck_player.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <SDL2/SDL.h>

#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#endif

#define SAMPLE_RATE 48000
#define BUFFER_SIZE 2048

typedef struct {
    DeckPlayer* player;
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
    uint16_t format;        // 1 = PCM
    uint16_t channels;      // 2 for stereo
    uint32_t sample_rate;   // 48000
    uint32_t byte_rate;     // sample_rate * channels * bytes_per_sample
    uint16_t block_align;   // channels * bytes_per_sample
    uint16_t bits_per_sample; // 16
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

// Render to WAV file
bool render_to_wav(DeckPlayer* player, const char* output_file, int time_limit_seconds) {
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
            fprintf(stderr, "Error: Could not open '%s' for writing\n", output_file);
            return false;
        }
    }

    // Allocate buffers
    float* left = (float*)malloc(BUFFER_SIZE * sizeof(float));
    float* right = (float*)malloc(BUFFER_SIZE * sizeof(float));

    if (!left || !right) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        if (!use_stdout && f) fclose(f);
        free(left);
        free(right);
        return false;
    }

    // Calculate total samples
    size_t total_samples;
    if (time_limit_seconds > 0) {
        total_samples = SAMPLE_RATE * time_limit_seconds;
    } else {
        // Default: 5 minutes for unlimited mode
        total_samples = SAMPLE_RATE * 300;
    }

    // Write header (placeholder, will update if not stdout)
    write_wav_header(f, total_samples);

    // Start playback
    deck_player_start(player);

    size_t samples_written = 0;
    bool done = false;

    while (samples_written < total_samples && !done) {
        size_t samples_to_render = BUFFER_SIZE;
        if (samples_written + samples_to_render > total_samples) {
            samples_to_render = total_samples - samples_written;
        }

        // Render audio
        deck_player_process(player, left, right, samples_to_render, SAMPLE_RATE);

        // Check if playback stopped (song ended)
        if (!deck_player_is_playing(player)) {
            done = true;
        }

        // Convert to 16-bit PCM and write interleaved
        for (size_t i = 0; i < samples_to_render; i++) {
            int16_t l = (int16_t)(left[i] * 32767.0f);
            int16_t r = (int16_t)(right[i] * 32767.0f);
            fwrite(&l, 2, 1, f);
            fwrite(&r, 2, 1, f);
        }

        samples_written += samples_to_render;

        // Print progress every second (only to stderr if using stdout)
        if ((samples_written % SAMPLE_RATE) == 0) {
            int seconds = samples_written / SAMPLE_RATE;
            if (use_stdout) {
                fprintf(stderr, "Progress: %d seconds\n", seconds);
            } else {
                printf("Progress: %d seconds\n", seconds);
            }
        }
    }

    // Update header with actual size if not stdout
    if (!use_stdout) {
        fseek(f, 0, SEEK_SET);
        write_wav_header(f, samples_written);
        fclose(f);
        printf("Wrote %zu samples to %s\n", samples_written, output_file);
    }

    free(left);
    free(right);

    return true;
}

// SDL audio callback
void audio_callback(void* userdata, Uint8* stream, int len) {
    PlayerContext* ctx = (PlayerContext*)userdata;
    size_t frames = len / (2 * sizeof(float));  // Stereo float32

    float* left = (float*)malloc(frames * sizeof(float));
    float* right = (float*)malloc(frames * sizeof(float));
    float* output = (float*)stream;

    if (!left || !right) {
        memset(stream, 0, len);
        return;
    }

    if (ctx->playing) {
        deck_player_process(ctx->player, left, right, frames, SAMPLE_RATE);

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
void position_callback(uint8_t order, uint16_t pattern, uint16_t row, void* user_data) {
    static uint16_t last_pattern = 0xFFFF;

    // Only print when pattern changes to reduce spam
    if (pattern != last_pattern) {
        printf("Position: Order=%d Pattern=%d Row=%d\n", order, pattern, row);
        last_pattern = pattern;
    }
}

// Interactive player mode
void run_interactive(DeckPlayer* player) {
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
    deck_player_set_position_callback(player, position_callback, NULL);

    // Start playback
    deck_player_start(player);
    SDL_PauseAudioDevice(device, 0);  // Unpause

    printf("Deck Player - Interactive Mode\n");
    printf("Format: %s\n", deck_player_get_type_name(player));

    const char* title = deck_player_get_title(player);
    if (title) {
        printf("Title: %s\n", title);
    }

    printf("\nControls:\n");
    printf("  Space - Play/Pause\n");
    printf("  Q - Quit\n");
    printf("  1-4 - Toggle channel mute\n");
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
                            deck_player_start(player);
                            printf("Playing...\n");
                        } else {
                            deck_player_stop(player);
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
                        int ch = event.key.keysym.sym - SDLK_1;
                        bool muted = deck_player_get_channel_mute(player, ch);
                        deck_player_set_channel_mute(player, ch, !muted);
                        printf("Channel %d: %s\n", ch + 1, muted ? "Unmuted" : "Muted");
                        break;
                    }

                    default:
                        break;
                }
            }
        }

        SDL_Delay(10);
    }

    // Cleanup
    SDL_CloseAudioDevice(device);
}

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Deck Player Test Tool\n");
        fprintf(stderr, "Usage: %s <filename> [-o output.wav] [-t seconds]\n", argv[0]);
        fprintf(stderr, "\nOptions:\n");
        fprintf(stderr, "  -o <file>     Render to WAV file (use '-' for stdout)\n");
        fprintf(stderr, "  -t <seconds>  Time limit in seconds (default: play full song)\n");
        fprintf(stderr, "\nSupported formats: MOD, MED, AHX\n");
        return 1;
    }

    const char* filename = argv[1];
    const char* output_file = NULL;
    int time_limit = 0;

    // Parse arguments
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            output_file = argv[++i];
        } else if (strcmp(argv[i], "-t") == 0 && i + 1 < argc) {
            time_limit = atoi(argv[++i]);
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
    DeckPlayer* player = deck_player_create();
    if (!player) {
        fprintf(stderr, "Error: Failed to create player\n");
        free(file_data);
        return 1;
    }

    // Load file (auto-detects format)
    if (!deck_player_load(player, file_data, file_size)) {
        fprintf(stderr, "Error: Could not load file (unsupported format or corrupt)\n");
        deck_player_destroy(player);
        free(file_data);
        return 1;
    }

    free(file_data);

    // Choose mode based on arguments
    bool success = true;
    if (output_file) {
        // WAV rendering mode - disable looping so song stops at end
        deck_player_set_disable_looping(player, true);
        success = render_to_wav(player, output_file, time_limit);
    } else {
        // Interactive SDL mode
        if (SDL_Init(SDL_INIT_AUDIO) < 0) {
            fprintf(stderr, "Error: SDL initialization failed: %s\n", SDL_GetError());
            deck_player_destroy(player);
            return 1;
        }

        run_interactive(player);
        SDL_Quit();
    }

    // Cleanup
    deck_player_destroy(player);

    return success ? 0 : 1;
}
