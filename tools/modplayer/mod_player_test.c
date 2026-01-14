/*
 * Simple SDL2-based MOD Player Test
 *
 * Usage: ./mod_player_test <filename.mod> [-o output.wav] [-c channels]
 *
 * Options:
 *   -o <file>     Render to WAV file (use '-' for stdout)
 *   -c <1234>     Render only specified channels (e.g., -c 13 for channels 1 and 3)
 *
 * Controls (interactive mode):
 *   Space - Play/Pause
 *   Q - Quit
 *   1-4 - Toggle channel mute
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <SDL2/SDL.h>
#include "../../players/mod_player.h"

#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#endif

#define SAMPLE_RATE 48000
#define BUFFER_SIZE 2048

typedef struct {
    ModPlayer* player;
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

// Render MOD file to WAV
bool render_to_wav(ModPlayer* player, const char* output_file) {
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

    // Disable looping so playback stops at end instead of looping
    mod_player_set_disable_looping(player, true);

    mod_player_start(player);

    uint32_t total_samples = 0;
    uint32_t max_samples = SAMPLE_RATE * 60 * 5;  // Max 5 minutes to prevent infinite loops

    if (!use_stdout) {
        fprintf(stderr, "Rendering");
    }

    while (mod_player_is_playing(player) && total_samples < max_samples) {
        // Process audio
        mod_player_process(player, left, right, render_frames, SAMPLE_RATE);

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

        if (!use_stdout && (total_samples % (SAMPLE_RATE * 2)) == 0) {
            fprintf(stderr, ".");
            fflush(stderr);
        }
    }

    if (!use_stdout) {
        fprintf(stderr, "\n");

        // Update header with actual size
        fseek(f, header_pos, SEEK_SET);
        write_wav_header(f, total_samples);

        fprintf(stderr, "Rendered %u samples (%.1f seconds) to %s\n",
                total_samples, (float)total_samples / SAMPLE_RATE, output_file);

        fclose(f);
    }

    free(left);
    free(right);
    free(output);

    return true;
}

// SDL audio callback
void audio_callback(void* userdata, Uint8* stream, int len) {
    PlayerContext* ctx = (PlayerContext*)userdata;
    int frames = len / (2 * sizeof(float));  // Stereo, float samples

    float* left = (float*)malloc(frames * sizeof(float));
    float* right = (float*)malloc(frames * sizeof(float));

    if (ctx->playing && ctx->player) {
        mod_player_process(ctx->player, left, right, frames, SAMPLE_RATE);

        // Interleave left/right channels
        float* output = (float*)stream;
        for (int i = 0; i < frames; i++) {
            output[i * 2] = left[i];
            output[i * 2 + 1] = right[i];
        }
    } else {
        // Silence
        memset(stream, 0, len);
    }

    free(left);
    free(right);
}

bool load_mod_file(const char* filename, ModPlayer* player) {
    FILE* f = fopen(filename, "rb");
    if (!f) {
        fprintf(stderr, "Error: Could not open file '%s'\n", filename);
        return false;
    }

    // Get file size
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (size <= 0) {
        fprintf(stderr, "Error: Invalid file size\n");
        fclose(f);
        return false;
    }

    // Read file
    uint8_t* data = (uint8_t*)malloc(size);
    size_t read = fread(data, 1, size, f);
    fclose(f);

    if (read != (size_t)size) {
        fprintf(stderr, "Error: Could not read entire file\n");
        free(data);
        return false;
    }

    // Load MOD
    bool success = mod_player_load(player, data, (uint32_t)size);
    free(data);

    if (!success) {
        fprintf(stderr, "Error: Failed to parse MOD file (not a valid ProTracker MOD?)\n");
        return false;
    }

    return true;
}

void print_info(ModPlayer* player) {
    printf("\n");
    printf("============================================================\n");
    printf("  RGModPlayer - ProTracker Module Player\n");
    printf("============================================================\n");
    printf("\n");
    printf("Title: %s\n", mod_player_get_title(player));
    printf("Song Length: %d patterns\n", mod_player_get_song_length(player));
    printf("\n");
    printf("Controls:\n");
    printf("  Space    - Play/Pause\n");
    printf("  1-4      - Toggle channel mute\n");
    printf("  +/-      - Adjust BPM\n");
    printf("  Q/Esc    - Quit\n");
    printf("\n");
}

void print_status(ModPlayer* player, bool playing) {
    uint8_t order;
    uint16_t row;
    mod_player_get_position(player, &order, &row);

    printf("\r[%s] Song order: %3d  Row: %2d  | Ch: ",
           playing ? ">" : "||", order, row);

    for (int i = 0; i < 4; i++) {
        bool muted = mod_player_get_channel_mute(player, i);
        printf("%d:%s ", i+1, muted ? "MUTE" : "ON");
    }

    printf("    ");  // Clear any leftover text
    fflush(stdout);
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: %s <filename.mod> [-o output.wav] [-c channels]\n", argv[0]);
        printf("\nPlays or renders a ProTracker MOD file.\n");
        printf("\nOptions:\n");
        printf("  -o <file>      Render to WAV file (use '-' for stdout)\n");
        printf("  -c <1234>      Render only specified channels (e.g., -c 13 for channels 1 and 3)\n");
        printf("\nInteractive mode (no -o):\n");
        printf("  Plays the file using SDL2 audio with keyboard controls.\n");
        return 1;
    }

    const char* filename = argv[1];
    const char* output_file = NULL;
    const char* channel_spec = NULL;
    bool render_mode = false;

    // Parse command-line arguments
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0) {
            if (i + 1 < argc) {
                output_file = argv[i + 1];
                render_mode = true;
                i++;  // Skip next argument
            } else {
                fprintf(stderr, "Error: -o requires an output filename\n");
                return 1;
            }
        } else if (strcmp(argv[i], "-c") == 0) {
            if (i + 1 < argc) {
                channel_spec = argv[i + 1];
                i++;  // Skip next argument
            } else {
                fprintf(stderr, "Error: -c requires a channel specification (e.g., 1234 or 13)\n");
                return 1;
            }
        }
    }

    // Create MOD player
    ModPlayer* player = mod_player_create();
    if (!player) {
        fprintf(stderr, "Failed to create MOD player\n");
        return 1;
    }

    // Load MOD file
    if (!load_mod_file(filename, player)) {
        fprintf(stderr, "Failed to load MOD file\n");
        mod_player_destroy(player);
        return 1;
    }

    // Apply channel selection
    if (channel_spec) {
        // First mute all channels
        for (int ch = 0; ch < 4; ch++) {
            mod_player_set_channel_mute(player, ch, true);
        }

        // Then unmute specified channels
        for (const char* p = channel_spec; *p; p++) {
            if (*p >= '1' && *p <= '4') {
                int ch = *p - '1';  // Convert '1'-'4' to 0-3
                mod_player_set_channel_mute(player, ch, false);
                fprintf(stderr, "Enabled channel %d\n", ch + 1);
            }
        }
    }

    // Render mode
    if (render_mode) {
        bool success = render_to_wav(player, output_file);
        mod_player_destroy(player);
        return success ? 0 : 1;
    }

    // Interactive playback mode
    // Initialize SDL
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        fprintf(stderr, "SDL initialization failed: %s\n", SDL_GetError());
        mod_player_destroy(player);
        return 1;
    }

    print_info(player);

    // Setup audio
    SDL_AudioSpec want, have;
    SDL_zero(want);
    want.freq = SAMPLE_RATE;
    want.format = AUDIO_F32SYS;
    want.channels = 2;
    want.samples = BUFFER_SIZE;
    want.callback = audio_callback;

    PlayerContext ctx;
    ctx.player = player;
    ctx.playing = true;
    want.userdata = &ctx;

    SDL_AudioDeviceID audio_device = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
    if (audio_device == 0) {
        fprintf(stderr, "Failed to open audio device: %s\n", SDL_GetError());
        mod_player_destroy(player);
        SDL_Quit();
        return 1;
    }

    ctx.audio_device = audio_device;

    // Start playback
    mod_player_start(player);
    SDL_PauseAudioDevice(audio_device, 0);  // Unpause

    printf("Playing: %s\n\n", filename);

    // Main loop
    bool running = true;
    SDL_Event event;
    Uint32 last_update = SDL_GetTicks();

    while (running) {
        // Poll events
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else if (event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                    case SDLK_SPACE:
                        ctx.playing = !ctx.playing;
                        if (ctx.playing) {
                            mod_player_start(player);
                        } else {
                            mod_player_stop(player);
                        }
                        break;

                    case SDLK_1:
                    case SDLK_2:
                    case SDLK_3:
                    case SDLK_4: {
                        int ch = event.key.keysym.sym - SDLK_1;
                        bool muted = mod_player_get_channel_mute(player, ch);
                        mod_player_set_channel_mute(player, ch, !muted);
                        break;
                    }

                    case SDLK_PLUS:
                    case SDLK_EQUALS: {
                        // Increase BPM (get current, add 5, set)
                        // For now just set to 130
                        mod_player_set_bpm(player, 130);
                        break;
                    }

                    case SDLK_MINUS: {
                        // Decrease BPM
                        mod_player_set_bpm(player, 120);
                        break;
                    }

                    case SDLK_q:
                    case SDLK_ESCAPE:
                        running = false;
                        break;
                }
            }
        }

        // Update status display (10 times per second)
        Uint32 now = SDL_GetTicks();
        if (now - last_update > 100) {
            print_status(player, ctx.playing);
            last_update = now;
        }

        SDL_Delay(10);
    }

    printf("\n\nShutting down...\n");

    // Cleanup
    SDL_CloseAudioDevice(audio_device);
    mod_player_destroy(player);
    SDL_Quit();

    return 0;
}
