/*
 * Simple SDL2-based SID Player Test
 *
 * Usage: ./sid_player_test <filename.sid> [-o output.wav] [-v voices] [-s subsong] [-p] [-d]
 *
 * Options:
 *   -o <file>     Render to WAV file (use '-' for stdout)
 *   -v <123>      Render only specified voices (e.g., -v 13 for voices 1 and 3)
 *   -s <num>      Select subsong (default 0)
 *   -p            Force PAL timing (50Hz, overrides file header)
 *   -n            Force NTSC timing (60Hz, overrides file header)
 *   -d            Enable debug/tracker output
 *
 * Controls (interactive mode):
 *   Space - Play/Pause
 *   Q - Quit
 *   1-3 - Toggle voice mute
 *   W/S - Next/previous subsong
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <SDL2/SDL.h>
#include "../../players/sid_player.h"

#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#endif

#define SAMPLE_RATE 48000
#define BUFFER_SIZE 2048

typedef struct {
    SidPlayer* player;
    bool playing;
    SDL_AudioDeviceID audio_device;
} PlayerContext;

/* WAV file header structures */
typedef struct {
    char riff[4];           /* "RIFF" */
    uint32_t file_size;     /* File size - 8 */
    char wave[4];           /* "WAVE" */
} WAVHeader;

typedef struct {
    char fmt[4];            /* "fmt " */
    uint32_t chunk_size;    /* 16 for PCM */
    uint16_t format;        /* 1 = PCM, 3 = IEEE float */
    uint16_t channels;      /* 2 for stereo */
    uint32_t sample_rate;   /* 48000 */
    uint32_t byte_rate;     /* sample_rate * channels * bytes_per_sample */
    uint16_t block_align;   /* channels * bytes_per_sample */
    uint16_t bits_per_sample; /* 16 or 32 */
} WAVFmtChunk;

typedef struct {
    char data[4];           /* "data" */
    uint32_t data_size;     /* Size of audio data */
} WAVDataChunk;

/* Write WAV header (16-bit PCM) */
void write_wav_header(FILE* f, uint32_t num_samples) {
    uint32_t data_size = num_samples * 2 * 2;  /* 2 channels * 2 bytes per sample */

    WAVHeader header = {
        .riff = {'R', 'I', 'F', 'F'},
        .file_size = 36 + data_size,
        .wave = {'W', 'A', 'V', 'E'}
    };

    WAVFmtChunk fmt = {
        .fmt = {'f', 'm', 't', ' '},
        .chunk_size = 16,
        .format = 1,  /* PCM */
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

/* Render SID file to WAV */
bool render_to_wav(SidPlayer* player, const char* output_file) {
    FILE* f = NULL;
    bool use_stdout = (strcmp(output_file, "-") == 0);

    if (use_stdout) {
        f = stdout;
        /* Set binary mode on Windows */
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

    /* Reserve space for header (we'll update it later with actual size) */
    long header_pos = ftell(f);
    write_wav_header(f, 0);

    /* Render audio */
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

    /* Disable looping so playback stops at end */
    sid_player_set_disable_looping(player, true);

    sid_player_start(player);

    uint32_t total_samples = 0;
    uint32_t max_samples = SAMPLE_RATE * 60 * 5;  /* Max 5 minutes */

    if (!use_stdout) {
        fprintf(stderr, "Rendering");
    }

    while (sid_player_is_playing(player) && total_samples < max_samples) {
        /* Process audio */
        sid_player_process(player, left, right, render_frames, SAMPLE_RATE);

        /* Convert float to 16-bit PCM and interleave */
        for (size_t i = 0; i < render_frames; i++) {
            /* Clamp to [-1.0, 1.0] and convert to 16-bit */
            float l = left[i];
            float r = right[i];
            if (l > 1.0f) l = 1.0f;
            if (l < -1.0f) l = -1.0f;
            if (r > 1.0f) r = 1.0f;
            if (r < -1.0f) r = -1.0f;

            output[i * 2] = (int16_t)(l * 32767.0f);
            output[i * 2 + 1] = (int16_t)(r * 32767.0f);
        }

        /* Write to file */
        fwrite(output, sizeof(int16_t), render_frames * 2, f);
        total_samples += render_frames;

        /* Progress indicator (every second) */
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

    /* Update header with actual size */
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

/* SDL audio callback */
void audio_callback(void* userdata, Uint8* stream, int len) {
    PlayerContext* ctx = (PlayerContext*)userdata;

    size_t frames = len / (2 * sizeof(float));  /* Stereo float samples */
    float* left = (float*)malloc(frames * sizeof(float));
    float* right = (float*)malloc(frames * sizeof(float));
    float* output = (float*)stream;

    if (!left || !right) {
        memset(stream, 0, len);
        return;
    }

    if (ctx->playing) {
        sid_player_process(ctx->player, left, right, frames, SAMPLE_RATE);

        /* Interleave left and right */
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

/* Position callback for display */
void position_callback(uint8_t subsong, uint32_t time_ms, void* user_data) {
    /* Optional: could print position updates */
    (void)subsong;
    (void)time_ms;
    (void)user_data;
}

/* Interactive player mode */
void run_interactive(SidPlayer* player) {
    /* Setup SDL Audio */
    SDL_AudioSpec want, have;
    SDL_zero(want);
    want.freq = SAMPLE_RATE;
    want.format = AUDIO_F32SYS;  /* Float32 system byte order */
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

    /* Set position callback */
    sid_player_set_position_callback(player, position_callback, NULL);

    /* Start playback */
    sid_player_start(player);
    SDL_PauseAudioDevice(device, 0);  /* Unpause */

    printf("SID Player - Interactive Mode\n");
    printf("Title: %s\n", sid_player_get_title(player));
    printf("Author: %s\n", sid_player_get_author(player));
    printf("Copyright: %s\n", sid_player_get_copyright(player));
    printf("Subsongs: %d\n", sid_player_get_num_subsongs(player));
    printf("Current subsong: %d\n", sid_player_get_current_subsong(player));
    printf("\nControls:\n");
    printf("  Space - Play/Pause\n");
    printf("  Q - Quit\n");
    printf("  1-3 - Toggle voice mute\n");
    printf("  W/S - Next/Previous subsong\n");
    printf("  D - Dump current SID state (snapshot)\n");
    printf("\n");

    /* Event loop */
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
                            sid_player_start(player);
                            printf("Playing...\n");
                        } else {
                            sid_player_stop(player);
                            printf("Paused\n");
                        }
                        break;

                    case SDLK_q:
                        running = false;
                        break;

                    case SDLK_d:
                        sid_player_print_state(player);
                        break;

                    case SDLK_1:
                    case SDLK_2:
                    case SDLK_3: {
                        uint8_t voice = event.key.keysym.sym - SDLK_1;
                        bool muted = !sid_player_get_voice_mute(player, voice);
                        sid_player_set_voice_mute(player, voice, muted);
                        printf("Voice %d: %s\n", voice + 1, muted ? "MUTED" : "UNMUTED");
                        break;
                    }

                    case SDLK_w: {
                        uint8_t current = sid_player_get_current_subsong(player);
                        uint8_t num_subsongs = sid_player_get_num_subsongs(player);
                        if (current + 1 < num_subsongs) {
                            sid_player_set_subsong(player, current + 1);
                            printf("Subsong: %d\n", current + 1);
                            if (ctx.playing) {
                                sid_player_start(player);
                            }
                        }
                        break;
                    }

                    case SDLK_s: {
                        uint8_t current = sid_player_get_current_subsong(player);
                        if (current > 0) {
                            sid_player_set_subsong(player, current - 1);
                            printf("Subsong: %d\n", current - 1);
                            if (ctx.playing) {
                                sid_player_start(player);
                            }
                        }
                        break;
                    }
                }
            }
        }

        /* Print position status */
        if (ctx.playing) {
            uint32_t time_ms = sid_player_get_time_ms(player);
            uint8_t subsong = sid_player_get_current_subsong(player);
            uint32_t seconds = time_ms / 1000;
            uint32_t minutes = seconds / 60;
            seconds %= 60;
            printf("\rSubsong %d | Time %02u:%02u ", subsong, minutes, seconds);
            fflush(stdout);
        }

        SDL_Delay(100);
    }

    printf("\nStopping...\n");

    SDL_CloseAudioDevice(device);
}

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <filename.sid> [-o output.wav] [-v voices] [-s subsong] [-p] [-n] [-d]\n", argv[0]);
        return 1;
    }

    const char* filename = argv[1];
    const char* output_file = NULL;
    const char* voice_mask = NULL;
    int subsong = 0;
    bool debug_mode = false;
    bool force_pal = false;
    bool force_ntsc = false;

    /* Parse arguments */
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            output_file = argv[++i];
        } else if (strcmp(argv[i], "-v") == 0 && i + 1 < argc) {
            voice_mask = argv[++i];
        } else if (strcmp(argv[i], "-s") == 0 && i + 1 < argc) {
            subsong = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-p") == 0) {
            force_pal = true;
        } else if (strcmp(argv[i], "-n") == 0) {
            force_ntsc = true;
        } else if (strcmp(argv[i], "-d") == 0) {
            debug_mode = true;
        }
    }

    /* Load file */
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

    /* Create player */
    SidPlayer* player = sid_player_create();
    if (!player) {
        fprintf(stderr, "Error: Failed to create player\n");
        free(file_data);
        return 1;
    }

    /* Enable debug mode if requested */
    if (debug_mode) {
        sid_player_set_debug_output(player, true);
    }

    /* Load SID file */
    if (!sid_player_load(player, file_data, file_size)) {
        fprintf(stderr, "Error: Failed to load SID file (invalid format or corrupt file)\n");
        sid_player_destroy(player);
        free(file_data);
        return 1;
    }

    free(file_data);

    /* Force PAL or NTSC timing if requested */
    if (force_pal) {
        fprintf(stderr, "Forcing PAL timing (50Hz)\n");
        sid_player_set_pal_mode(player, true);
    } else if (force_ntsc) {
        fprintf(stderr, "Forcing NTSC timing (60Hz)\n");
        sid_player_set_pal_mode(player, false);
    }

    /* Set subsong */
    if (subsong > 0 && subsong < sid_player_get_num_subsongs(player)) {
        sid_player_set_subsong(player, subsong);
    }

    /* Apply voice mask */
    if (voice_mask) {
        for (int i = 0; i < 3; i++) {
            bool enabled = false;
            char ch = '1' + i;
            for (const char* p = voice_mask; *p; p++) {
                if (*p == ch) {
                    enabled = true;
                    break;
                }
            }
            sid_player_set_voice_mute(player, i, !enabled);
        }
    }

    bool success = false;

    if (output_file) {
        /* Render mode */
        success = render_to_wav(player, output_file);
    } else {
        /* Interactive mode */
        if (SDL_Init(SDL_INIT_AUDIO) != 0) {
            fprintf(stderr, "Error: Failed to initialize SDL: %s\n", SDL_GetError());
            sid_player_destroy(player);
            return 1;
        }

        run_interactive(player);

        SDL_Quit();
        success = true;
    }

    sid_player_destroy(player);

    return success ? 0 : 1;
}
