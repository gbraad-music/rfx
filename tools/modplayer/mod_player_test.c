/*
 * Simple SDL2-based MOD Player Test
 *
 * Usage: ./mod_player_test <filename.mod>
 *
 * Controls:
 *   Space - Play/Pause
 *   Q - Quit
 *   1-4 - Toggle channel mute
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <SDL2/SDL.h>
#include "../synth/mod_player.h"

#define SAMPLE_RATE 48000
#define BUFFER_SIZE 2048

typedef struct {
    ModPlayer* player;
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
    uint8_t pattern, row;
    mod_player_get_position(player, &pattern, &row);

    printf("\r[%s] Pattern: %3d  Row: %2d  | Ch: ",
           playing ? ">" : "||", pattern, row);

    for (int i = 0; i < 4; i++) {
        bool muted = mod_player_get_channel_mute(player, i);
        printf("%d:%s ", i+1, muted ? "MUTE" : "ON");
    }

    printf("    ");  // Clear any leftover text
    fflush(stdout);
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: %s <filename.mod>\n", argv[0]);
        printf("\nPlays a ProTracker MOD file using SDL2 audio.\n");
        return 1;
    }

    const char* filename = argv[1];

    // Initialize SDL
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        fprintf(stderr, "SDL initialization failed: %s\n", SDL_GetError());
        return 1;
    }

    // Create MOD player
    ModPlayer* player = mod_player_create();
    if (!player) {
        fprintf(stderr, "Failed to create MOD player\n");
        SDL_Quit();
        return 1;
    }

    // Load MOD file
    if (!load_mod_file(filename, player)) {
        mod_player_destroy(player);
        SDL_Quit();
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
