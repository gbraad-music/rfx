/*
 * Simple SDL2-based MED/OctaMED Player Test
 *
 * Usage: ./med_player_test <filename.med> [-o output.wav]
 *
 * Controls (interactive mode):
 *   Space - Play/Pause
 *   Q - Quit
 *   1-8 - Toggle channel mute (supports up to 64 channels)
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <SDL2/SDL.h>
#include "../../synth/mmd_player.h"

#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#endif

#define SAMPLE_RATE 48000
#define BUFFER_SIZE 2048

typedef struct {
    MedPlayer* player;
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

// Position callback to track when song loops
typedef struct {
    uint8_t max_order;      // Maximum order we've seen
    uint8_t prev_order;     // Previous order
    bool has_looped;
} LoopTracker;

void position_callback(uint8_t order, uint8_t pattern, uint16_t row, void* user_data) {
    LoopTracker* tracker = (LoopTracker*)user_data;

    // Detect loop: when order jumps backwards
    if (row == 0 && tracker->prev_order != 255) {  // Skip first call
        if (order < tracker->prev_order) {
            // Order went backwards - we've looped
            tracker->has_looped = true;
        }
    }

    // Track maximum order seen
    if (order > tracker->max_order) {
        tracker->max_order = order;
    }

    tracker->prev_order = order;
}

// Render MED file to WAV
bool render_to_wav(MedPlayer* player, const char* output_file) {
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

    // Set up loop tracking
    LoopTracker tracker = {0};
    tracker.prev_order = 255;  // Sentinel value to skip first callback
    tracker.has_looped = false;

    med_player_set_position_callback(player, position_callback, &tracker);

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

    med_player_start(player);

    uint32_t total_samples = 0;

    if (!use_stdout) {
        fprintf(stderr, "Rendering");
    }

    while (!tracker.has_looped) {
        // Process audio
        med_player_process(player, left, right, render_frames, SAMPLE_RATE);

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
        med_player_process(ctx->player, left, right, frames, SAMPLE_RATE);

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

bool load_med_file(const char* filename, MedPlayer* player) {
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

    // Load MED
    bool success = med_player_load(player, data, (size_t)size);
    free(data);

    if (!success) {
        fprintf(stderr, "Error: Failed to parse MED file (not a valid MMD2 file?)\n");
        return false;
    }

    return true;
}

void print_info(MedPlayer* player) {
    printf("\n");
    printf("============================================================\n");
    printf("  RGMedPlayer - OctaMED Module Player (MMD2)\n");
    printf("============================================================\n");
    printf("\n");
    printf("Song Length: %d patterns\n", med_player_get_song_length(player));
    printf("BPM: %d\n", med_player_get_bpm(player));
    printf("\n");
    printf("Controls:\n");
    printf("  Space    - Play/Pause\n");
    printf("  1-8      - Toggle channel mute\n");
    printf("  +/-      - Adjust BPM\n");
    printf("  Q/Esc    - Quit\n");
    printf("\n");
}

void print_status(MedPlayer* player, bool playing) {
    uint8_t pattern;
    uint16_t row;
    med_player_get_position(player, &pattern, &row);

    printf("\r[%s] Pattern: %3d  Row: %2d  | Ch: ",
           playing ? ">" : "||", pattern, row);

    for (int i = 0; i < 8; i++) {
        bool muted = med_player_get_channel_mute(player, i);
        printf("%d:%s ", i+1, muted ? "M" : "O");
    }

    printf("    ");  // Clear any leftover text
    fflush(stdout);
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: %s <filename.med> [-o output.wav]\n", argv[0]);
        printf("\nPlays or renders an OctaMED (MMD2) file.\n");
        printf("\nOptions:\n");
        printf("  -o <file>    Render to WAV file (use '-' for stdout)\n");
        printf("\nInteractive mode (no -o):\n");
        printf("  Plays the file using SDL2 audio with keyboard controls.\n");
        return 1;
    }

    const char* filename = argv[1];
    const char* output_file = NULL;
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
        }
    }

    // Create MED player
    MedPlayer* player = med_player_create();
    if (!player) {
        fprintf(stderr, "Failed to create MED player\n");
        return 1;
    }

    // Load MED file
    if (!load_med_file(filename, player)) {
        fprintf(stderr, "Failed to load MED file\n");
        med_player_destroy(player);
        return 1;
    }

    // Render mode
    if (render_mode) {
        bool success = render_to_wav(player, output_file);
        med_player_destroy(player);
        return success ? 0 : 1;
    }

    // Interactive playback mode
    print_info(player);

    // Initialize SDL
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        fprintf(stderr, "SDL initialization failed: %s\n", SDL_GetError());
        med_player_destroy(player);
        return 1;
    }

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
        med_player_destroy(player);
        SDL_Quit();
        return 1;
    }

    ctx.audio_device = audio_device;

    // Start playback
    med_player_start(player);
    SDL_PauseAudioDevice(audio_device, 0);  // Unpause

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
                            med_player_start(player);
                        } else {
                            med_player_stop(player);
                        }
                        break;

                    case SDLK_1:
                    case SDLK_2:
                    case SDLK_3:
                    case SDLK_4:
                    case SDLK_5:
                    case SDLK_6:
                    case SDLK_7:
                    case SDLK_8: {
                        int ch = event.key.keysym.sym - SDLK_1;
                        bool muted = med_player_get_channel_mute(player, ch);
                        med_player_set_channel_mute(player, ch, !muted);
                        break;
                    }

                    case SDLK_PLUS:
                    case SDLK_EQUALS: {
                        uint16_t bpm = med_player_get_bpm(player);
                        med_player_set_bpm(player, bpm + 5);
                        break;
                    }

                    case SDLK_MINUS: {
                        uint16_t bpm = med_player_get_bpm(player);
                        if (bpm > 5) {
                            med_player_set_bpm(player, bpm - 5);
                        }
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

    printf("\n");

    // Cleanup
    SDL_CloseAudioDevice(audio_device);
    med_player_destroy(player);
    SDL_Quit();

    return 0;
}
