/*
 * Simple SDL2-based MED/OctaMED Player Test
 *
 * Usage: ./med_player_test <filename.med>
 *
 * Controls:
 *   Space - Play/Pause
 *   Q - Quit
 *   1-8 - Toggle channel mute (supports up to 64 channels)
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <SDL2/SDL.h>
#include "../../synth/mmd_player.h"

#define SAMPLE_RATE 48000
#define BUFFER_SIZE 2048

typedef struct {
    MedPlayer* player;
    bool playing;
    SDL_AudioDeviceID audio_device;
} PlayerContext;

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
    uint8_t pattern, row;
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
        printf("Usage: %s <filename.med>\n", argv[0]);
        printf("\nPlays an OctaMED (MMD2) file using SDL2 audio.\n");
        return 1;
    }

    const char* filename = argv[1];

    // Initialize SDL
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        fprintf(stderr, "SDL initialization failed: %s\n", SDL_GetError());
        return 1;
    }

    // Create MED player
    MedPlayer* player = med_player_create();
    if (!player) {
        fprintf(stderr, "Failed to create MED player\n");
        SDL_Quit();
        return 1;
    }

    // Load MED file
    printf("Loading file: %s\n", filename);
    if (!load_med_file(filename, player)) {
        fprintf(stderr, "Failed to load MED file\n");
        med_player_destroy(player);
        SDL_Quit();
        return 1;
    }

    printf("File loaded successfully!\n");
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
        med_player_destroy(player);
        SDL_Quit();
        return 1;
    }

    printf("Audio device opened successfully\n");
    printf("  Sample rate: %d Hz\n", have.freq);
    printf("  Channels: %d\n", have.channels);
    printf("  Buffer size: %d samples\n", have.samples);

    ctx.audio_device = audio_device;

    // Start playback
    printf("Starting playback...\n");
    med_player_start(player);
    SDL_PauseAudioDevice(audio_device, 0);  // Unpause

    printf("Playing: %s\n\n", filename);
    printf("Press Q or ESC to quit, SPACE to pause/unpause\n\n");

    // Main loop
    bool running = true;
    SDL_Event event;
    Uint32 last_update = SDL_GetTicks();

    printf("Entering main event loop...\n");

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

    printf("\n\nShutting down...\n");

    // Cleanup
    SDL_CloseAudioDevice(audio_device);
    med_player_destroy(player);
    SDL_Quit();

    return 0;
}
