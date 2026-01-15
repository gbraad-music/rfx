/*
 * RGVPlayer - RegrooveController Test Tool
 *
 * A simple TUI player demonstrating RegrooveController features:
 * - Row-precise loop control
 * - Command queuing
 * - Pattern mode
 * - Channel mute/solo
 *
 * Usage: rgvplayer <file.mod|file.mmd|file.ahx>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <SDL2/SDL.h>

#include "../../players/deck_player.h"
#include "../../players/regroove_controller.h"

// ============================================================================
// Global State
// ============================================================================

static volatile int running = 1;
static struct termios orig_termios;

typedef struct {
    DeckPlayer* player;
    RegrooveController* controller;
    SDL_AudioDeviceID audio_device;
    int sample_rate;

    // Display state
    uint16_t current_order;
    uint16_t current_row;
    RegrooveLoopState loop_state;
    RegroovePatternMode pattern_mode;
    char song_name[256];
    int num_channels;
    bool channel_muted[32];
    bool channel_solo[32];
} PlayerState;

static PlayerState* g_state = NULL;

// ============================================================================
// Terminal Handling
// ============================================================================

static void handle_sigint(int sig) {
    (void)sig;
    running = 0;
}

static void tty_restore(void) {
    if (orig_termios.c_cflag) {
        tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios);
    }
}

static int tty_make_raw_nonblocking(void) {
    if (!isatty(STDIN_FILENO)) return -1;
    if (tcgetattr(STDIN_FILENO, &orig_termios) < 0) return -1;

    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ICANON | ECHO);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);

    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    if (flags != -1) fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);

    atexit(tty_restore);
    return 0;
}

static int read_key_nonblocking(void) {
    unsigned char c;
    ssize_t n = read(STDIN_FILENO, &c, 1);
    return (n == 1) ? (int)c : -1;
}

// ============================================================================
// RegrooveController Callbacks
// ============================================================================

static void on_loop_state_change(void* user_data, RegrooveLoopState old_state,
                                 RegrooveLoopState new_state) {
    PlayerState* state = (PlayerState*)user_data;
    state->loop_state = new_state;

    const char* state_names[] = {"OFF", "ARMED", "ACTIVE"};
    printf("\n[LOOP] State: %s -> %s\n", state_names[old_state], state_names[new_state]);
}

static void on_loop_trigger(void* user_data, uint16_t order, uint16_t row) {
    printf("\n[LOOP] Triggered at %d:%02d\n", order, row);
}

static void on_pattern_mode_change(void* user_data, RegroovePatternMode mode) {
    PlayerState* state = (PlayerState*)user_data;
    state->pattern_mode = mode;

    const char* mode_names[] = {"OFF", "SINGLE", "CHAIN"};
    printf("\n[PATTERN MODE] %s\n", mode_names[mode]);
}

static void on_command_executed(void* user_data, RegrooveCommandType command) {
    const char* cmd_names[] = {
        "NONE", "JUMP", "NEXT", "PREV", "RETRIGGER", "MUTE", "SOLO"
    };
    printf("\n[CMD] Executed: %s\n", cmd_names[command]);
}

// ============================================================================
// Display
// ============================================================================

static void clear_screen(void) {
    printf("\033[2J\033[H");  // Clear screen and move cursor to top
}

static void update_display(PlayerState* state) {
    // Get current position
    regroove_controller_get_position(state->controller,
                                     &state->current_order,
                                     &state->current_row);

    // Update channel mute states
    for (int i = 0; i < state->num_channels && i < 32; i++) {
        state->channel_muted[i] = regroove_controller_get_channel_mute(state->controller, i);
        state->channel_solo[i] = regroove_controller_get_channel_solo(state->controller, i);
    }

    clear_screen();

    printf("╔══════════════════════════════════════════════════════════════╗\n");
    printf("║  RGVPlayer - RegrooveController Test Tool                   ║\n");
    printf("╚══════════════════════════════════════════════════════════════╝\n\n");

    printf("  Song: %s\n", state->song_name);
    printf("  Position: Order %3d, Row %3d\n", state->current_order, state->current_row);

    // Loop state
    const char* loop_icons[] = {"○", "◔", "●"};
    const char* loop_names[] = {"OFF", "ARMED", "ACTIVE"};
    printf("  Loop: %s %s", loop_icons[state->loop_state], loop_names[state->loop_state]);

    if (state->loop_state != RG_LOOP_OFF) {
        uint16_t ls_ord, ls_row, le_ord, le_row;
        regroove_controller_get_loop_range_rows(state->controller,
                                                &ls_ord, &ls_row, &le_ord, &le_row);
        printf(" [%d:%02d → %d:%02d]", ls_ord, ls_row, le_ord, le_row);
    }
    printf("\n");

    // Pattern mode
    const char* pattern_mode_names[] = {"OFF", "SINGLE", "CHAIN"};
    if (state->pattern_mode != RG_PATTERN_MODE_OFF) {
        printf("  Pattern Mode: %s\n", pattern_mode_names[state->pattern_mode]);
    }

    // Channel states
    printf("\n  Channels: ");
    for (int i = 0; i < state->num_channels && i < 8; i++) {
        if (state->channel_solo[i]) {
            printf("[S%d]", i+1);
        } else if (state->channel_muted[i]) {
            printf("[M%d]", i+1);
        } else {
            printf(" %d ", i+1);
        }
    }
    printf("\n");

    printf("\n");
    printf("╔══════════════════════════════════════════════════════════════╗\n");
    printf("║  CONTROLS                                                    ║\n");
    printf("╠══════════════════════════════════════════════════════════════╣\n");
    printf("║  SPACE   Start/Stop playback                                ║\n");
    printf("║  N/P     Queue Next/Previous order                          ║\n");
    printf("║  R       Retrigger current pattern                          ║\n");
    printf("║                                                              ║\n");
    printf("║  L       Set loop (current position to end)                 ║\n");
    printf("║  A       Arm loop (play-to-loop)                            ║\n");
    printf("║  T       Trigger loop (immediate)                           ║\n");
    printf("║  F       Disable loop                                       ║\n");
    printf("║                                                              ║\n");
    printf("║  S       Toggle pattern mode (single pattern loop)          ║\n");
    printf("║                                                              ║\n");
    printf("║  1-8     Toggle channel mute                                ║\n");
    printf("║  M       Mute all channels                                  ║\n");
    printf("║  U       Unmute all channels                                ║\n");
    printf("║                                                              ║\n");
    printf("║  Q/ESC   Quit                                               ║\n");
    printf("╚══════════════════════════════════════════════════════════════╝\n");
}

// ============================================================================
// Audio Callback
// ============================================================================

static void audio_callback(void* userdata, Uint8* stream, int len) {
    PlayerState* state = (PlayerState*)userdata;
    if (!state) return;

    int16_t* buffer = (int16_t*)stream;
    int frames = len / (2 * sizeof(int16_t));  // Stereo

    // Process controller timing
    regroove_controller_process(state->controller, frames, state->sample_rate);

    // Render audio using unified deck player
    float* left = malloc(frames * sizeof(float));
    float* right = malloc(frames * sizeof(float));

    deck_player_process(state->player, left, right, frames, state->sample_rate);

    // Convert float to int16
    for (int i = 0; i < frames; i++) {
        buffer[i*2 + 0] = (int16_t)(left[i] * 32767.0f);
        buffer[i*2 + 1] = (int16_t)(right[i] * 32767.0f);
    }

    free(left);
    free(right);
}

// ============================================================================
// File Loading
// ============================================================================

static int load_file(PlayerState* state, const char* filename) {
    FILE* f = fopen(filename, "rb");
    if (!f) {
        fprintf(stderr, "Cannot open file: %s\n", filename);
        return -1;
    }

    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    fseek(f, 0, SEEK_SET);

    uint8_t* data = malloc(size);
    if (!data) {
        fclose(f);
        return -1;
    }

    fread(data, 1, size, f);
    fclose(f);

    // Create deck player and load file (auto-detects format)
    state->player = deck_player_create();
    if (!state->player) {
        free(data);
        fprintf(stderr, "Failed to create deck player\n");
        return -1;
    }

    if (!deck_player_load(state->player, data, size)) {
        free(data);
        deck_player_destroy(state->player);
        state->player = NULL;
        fprintf(stderr, "Unsupported file format\n");
        return -1;
    }

    free(data);

    // Get info from deck player
    state->num_channels = deck_player_get_num_channels(state->player);
    const char* title = deck_player_get_title(state->player);
    strncpy(state->song_name, title ? title : filename, 255);

    // Get sequencer and wrap in controller
    PatternSequencer* seq = deck_player_get_sequencer(state->player);
    state->controller = regroove_controller_create(seq);

    if (!state->controller) {
        deck_player_destroy(state->player);
        state->player = NULL;
        fprintf(stderr, "Failed to create regroove controller\n");
        return -1;
    }

    // Set up RegrooveController callbacks
    RegrooveControllerCallbacks callbacks = {
        .on_loop_state_change = on_loop_state_change,
        .on_loop_trigger = on_loop_trigger,
        .on_pattern_mode_change = on_pattern_mode_change,
        .on_command_executed = on_command_executed,
        .on_note = NULL
    };
    regroove_controller_set_callbacks(state->controller, &callbacks, state);

    // Start playback
    deck_player_start(state->player);

    return 0;
}

// ============================================================================
// Keyboard Handler
// ============================================================================

static void handle_key(PlayerState* state, int key) {
    switch (key) {
        case ' ':  // Toggle playback
            if (deck_player_is_playing(state->player)) {
                deck_player_stop(state->player);
                printf("\n[PAUSED]\n");
            } else {
                deck_player_start(state->player);
                printf("\n[PLAYING]\n");
            }
            break;

        case 'n':
        case 'N':
            regroove_controller_queue_next_order(state->controller);
            printf("\n[QUEUED] Next order\n");
            break;

        case 'p':
        case 'P':
            regroove_controller_queue_prev_order(state->controller);
            printf("\n[QUEUED] Previous order\n");
            break;

        case 'r':
        case 'R':
            regroove_controller_retrigger_pattern(state->controller);
            printf("\n[RETRIGGER] Pattern\n");
            break;

        case 'l':
        case 'L': {
            // Set loop from current position to end
            uint16_t cur_ord, cur_row;
            regroove_controller_get_position(state->controller, &cur_ord, &cur_row);
            uint16_t song_len = regroove_controller_get_song_length(state->controller);
            regroove_controller_set_loop_range_rows(state->controller,
                                                    cur_ord, cur_row,
                                                    song_len - 1, 63);
            printf("\n[LOOP] Set: %d:%02d to end\n", cur_ord, cur_row);
            break;
        }

        case 'a':
        case 'A':
            regroove_controller_arm_loop(state->controller);
            printf("\n[LOOP] Armed (play-to-loop)\n");
            break;

        case 't':
        case 'T':
            regroove_controller_trigger_loop(state->controller);
            printf("\n[LOOP] Triggered\n");
            break;

        case 'f':
        case 'F':
            regroove_controller_disable_loop(state->controller);
            printf("\n[LOOP] Disabled\n");
            break;

        case 's':
        case 'S':
            if (state->pattern_mode == RG_PATTERN_MODE_OFF) {
                regroove_controller_set_pattern_mode(state->controller, RG_PATTERN_MODE_SINGLE);
            } else {
                regroove_controller_set_pattern_mode(state->controller, RG_PATTERN_MODE_OFF);
            }
            break;

        case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': {
            int ch = key - '1';
            regroove_controller_toggle_channel_mute(state->controller, ch);
            printf("\n[CHANNEL %d] Mute toggled\n", ch + 1);
            break;
        }

        case 'm':
        case 'M':
            for (int i = 0; i < state->num_channels; i++) {
                if (!state->channel_muted[i]) {
                    regroove_controller_toggle_channel_mute(state->controller, i);
                }
            }
            printf("\n[MUTE ALL]\n");
            break;

        case 'u':
        case 'U':
            for (int i = 0; i < state->num_channels; i++) {
                if (state->channel_muted[i]) {
                    regroove_controller_toggle_channel_mute(state->controller, i);
                }
            }
            regroove_controller_clear_all_solo(state->controller);
            printf("\n[UNMUTE ALL]\n");
            break;

        case 'q':
        case 'Q':
        case 27:  // ESC
            running = 0;
            break;
    }
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <file.mod|file.mmd|file.ahx>\n", argv[0]);
        return 1;
    }

    signal(SIGINT, handle_sigint);

    // Create player state
    g_state = calloc(1, sizeof(PlayerState));
    g_state->sample_rate = 48000;

    // Load file
    if (load_file(g_state, argv[1]) != 0) {
        free(g_state);
        return 1;
    }

    // Initialize SDL audio
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    SDL_AudioSpec want, have;
    SDL_zero(want);
    want.freq = g_state->sample_rate;
    want.format = AUDIO_S16SYS;
    want.channels = 2;
    want.samples = 1024;
    want.callback = audio_callback;
    want.userdata = g_state;

    g_state->audio_device = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
    if (g_state->audio_device == 0) {
        fprintf(stderr, "SDL_OpenAudioDevice failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_PauseAudioDevice(g_state->audio_device, 0);  // Start audio

    // Setup terminal
    tty_make_raw_nonblocking();

    printf("\nRGVPlayer initialized. Playing: %s\n", argv[1]);
    printf("Press any key to start...\n");

    // Main loop
    while (running) {
        int key = read_key_nonblocking();
        if (key >= 0) {
            handle_key(g_state, key);
        }

        update_display(g_state);
        usleep(50000);  // 50ms update rate
    }

    // Cleanup
    printf("\n\nShutting down...\n");

    SDL_CloseAudioDevice(g_state->audio_device);
    SDL_Quit();

    if (g_state->controller) {
        regroove_controller_destroy(g_state->controller);
    }

    if (g_state->player) {
        deck_player_destroy(g_state->player);
    }

    free(g_state);

    return 0;
}
