/*
 * WebAssembly Bindings for MOD/MED Player
 * Direct wrappers for mod_player and mmd_player
 */

#include <emscripten.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "../../synth/mod_player.h"
#include "../../synth/mmd_player.h"

typedef enum {
    PLAYER_TYPE_NONE = 0,
    PLAYER_TYPE_MOD = 1,
    PLAYER_TYPE_MED = 2
} PlayerType;

typedef struct {
    PlayerType type;
    union {
        ModPlayer* mod_player;
        MedPlayer* med_player;
    };

    bool playing;
    uint8_t current_order;
    uint16_t current_row;
    char filename[256];
} ModMedPlayer;

// Forward declarations
EMSCRIPTEN_KEEPALIVE int modmed_player_get_song_length(const ModMedPlayer* player);

// Detect file type
static PlayerType detect_file_type(const uint8_t* data, size_t size) {
    if (!data || size < 1084) return PLAYER_TYPE_NONE;

    // MOD signature at offset 1080
    const char* sig = (const char*)(data + 1080);
    if (memcmp(sig, "M.K.", 4) == 0 || memcmp(sig, "M!K!", 4) == 0 ||
        memcmp(sig, "FLT4", 4) == 0 || memcmp(sig, "4CHN", 4) == 0) {
        return PLAYER_TYPE_MOD;
    }

    // MMD signature at offset 0
    if (size >= 4) {
        uint32_t id = (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3];
        if (id == 0x4D4D4432 || id == 0x4D4D4433) {
            return PLAYER_TYPE_MED;
        }
    }

    return PLAYER_TYPE_NONE;
}

// Position callbacks
static void mod_callback(uint8_t order, uint8_t pattern, uint16_t row, void* user_data) {
    ModMedPlayer* p = (ModMedPlayer*)user_data;
    if (p) {
        p->current_order = order;
        p->current_row = row;
    }
}

static void med_callback(uint8_t order, uint8_t pattern, uint16_t row, void* user_data) {
    ModMedPlayer* p = (ModMedPlayer*)user_data;
    if (p) {
        p->current_order = order;
        p->current_row = row;
    }
}

EMSCRIPTEN_KEEPALIVE
ModMedPlayer* modmed_player_create(float sample_rate) {
    (void)sample_rate;
    ModMedPlayer* p = (ModMedPlayer*)calloc(1, sizeof(ModMedPlayer));
    if (p) {
        p->type = PLAYER_TYPE_NONE;
        p->playing = false;
    }
    return p;
}

EMSCRIPTEN_KEEPALIVE
void modmed_player_destroy(ModMedPlayer* player) {
    if (!player) return;
    if (player->type == PLAYER_TYPE_MOD && player->mod_player) {
        mod_player_destroy(player->mod_player);
    } else if (player->type == PLAYER_TYPE_MED && player->med_player) {
        med_player_destroy(player->med_player);
    }
    free(player);
}

EMSCRIPTEN_KEEPALIVE
int modmed_player_load_from_memory(ModMedPlayer* player, const uint8_t* data, size_t size, const char* filename) {
    if (!player || !data) return 0;

    // Destroy existing
    if (player->type == PLAYER_TYPE_MOD && player->mod_player) {
        mod_player_destroy(player->mod_player);
        player->mod_player = NULL;
    } else if (player->type == PLAYER_TYPE_MED && player->med_player) {
        med_player_destroy(player->med_player);
        player->med_player = NULL;
    }

    player->type = detect_file_type(data, size);
    if (player->type == PLAYER_TYPE_NONE) return 0;

    bool success = false;
    if (player->type == PLAYER_TYPE_MOD) {
        player->mod_player = mod_player_create();
        if (player->mod_player) {
            success = mod_player_load(player->mod_player, data, size);
            if (success) {
                mod_player_set_position_callback(player->mod_player, mod_callback, player);
            }
        }
    } else if (player->type == PLAYER_TYPE_MED) {
        player->med_player = med_player_create();
        if (player->med_player) {
            success = med_player_load(player->med_player, data, size);
            if (success) {
                med_player_set_position_callback(player->med_player, med_callback, player);
            }
        }
    }

    if (success && filename) {
        strncpy(player->filename, filename, sizeof(player->filename) - 1);
    }

    return success ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE
void modmed_player_start(ModMedPlayer* player) {
    if (!player) return;
    player->playing = true;
    if (player->type == PLAYER_TYPE_MOD) {
        mod_player_start(player->mod_player);
    } else if (player->type == PLAYER_TYPE_MED) {
        med_player_start(player->med_player);
    }
}

EMSCRIPTEN_KEEPALIVE
void modmed_player_stop(ModMedPlayer* player) {
    if (!player) return;
    player->playing = false;
    if (player->type == PLAYER_TYPE_MOD) {
        mod_player_stop(player->mod_player);
    } else if (player->type == PLAYER_TYPE_MED) {
        med_player_stop(player->med_player);
    }
}

EMSCRIPTEN_KEEPALIVE
int modmed_player_is_playing(const ModMedPlayer* player) {
    return (player && player->playing) ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE
void modmed_player_process_f32(ModMedPlayer* player, float* buffer, int frames, float sample_rate) {
    if (!player || !buffer) return;

    if (player->type == PLAYER_TYPE_MOD) {
        // Process generates: LEFT samples in buffer[0..frames-1], RIGHT in buffer[frames..2*frames-1]
        // Keep PLANAR format for ScriptProcessor
        mod_player_process(player->mod_player, buffer, buffer + frames, frames, sample_rate);
    } else if (player->type == PLAYER_TYPE_MED) {
        // Process generates: LEFT samples in buffer[0..frames-1], RIGHT in buffer[frames..2*frames-1]
        // Keep PLANAR format for ScriptProcessor
        med_player_process(player->med_player, buffer, buffer + frames, frames, sample_rate);
    } else {
        memset(buffer, 0, frames * 2 * sizeof(float));
    }
}

EMSCRIPTEN_KEEPALIVE
void modmed_player_set_channel_mute(ModMedPlayer* player, int channel, int muted) {
    if (!player) return;
    if (player->type == PLAYER_TYPE_MOD) {
        mod_player_set_channel_mute(player->mod_player, channel, muted != 0);
    } else if (player->type == PLAYER_TYPE_MED) {
        med_player_set_channel_mute(player->med_player, channel, muted != 0);
    }
}

EMSCRIPTEN_KEEPALIVE
int modmed_player_get_channel_mute(const ModMedPlayer* player, int channel) {
    if (!player) return 0;
    if (player->type == PLAYER_TYPE_MOD) {
        return mod_player_get_channel_mute(player->mod_player, channel) ? 1 : 0;
    } else if (player->type == PLAYER_TYPE_MED) {
        return med_player_get_channel_mute(player->med_player, channel) ? 1 : 0;
    }
    return 0;
}

EMSCRIPTEN_KEEPALIVE
int modmed_player_get_current_order(const ModMedPlayer* player) {
    return player ? player->current_order : 0;
}

EMSCRIPTEN_KEEPALIVE
int modmed_player_get_current_row(const ModMedPlayer* player) {
    return player ? player->current_row : 0;
}

EMSCRIPTEN_KEEPALIVE
void modmed_player_set_position(ModMedPlayer* player, int order, int row) {
    if (!player) return;
    if (player->type == PLAYER_TYPE_MOD) {
        mod_player_set_position(player->mod_player, order, row);
    } else if (player->type == PLAYER_TYPE_MED) {
        med_player_set_position(player->med_player, order, row);
    }
}

EMSCRIPTEN_KEEPALIVE
void modmed_player_next_pattern(ModMedPlayer* player) {
    if (!player) return;
    uint8_t len = modmed_player_get_song_length(player);
    if (len == 0) return;
    uint8_t next = player->current_order + 1;
    if (next >= len) next = 0;
    modmed_player_set_position(player, next, 0);
}

EMSCRIPTEN_KEEPALIVE
void modmed_player_prev_pattern(ModMedPlayer* player) {
    if (!player) return;
    uint8_t len = modmed_player_get_song_length(player);
    if (len == 0) return;
    uint8_t prev = (player->current_order > 0) ? (player->current_order - 1) : (len - 1);
    modmed_player_set_position(player, prev, 0);
}

EMSCRIPTEN_KEEPALIVE
void modmed_player_set_loop_pattern(ModMedPlayer* player, int loop) {
    if (!player) return;
    if (loop) {
        if (player->type == PLAYER_TYPE_MOD) {
            mod_player_set_loop_range(player->mod_player, player->current_order, player->current_order);
        } else if (player->type == PLAYER_TYPE_MED) {
            med_player_set_loop_range(player->med_player, player->current_order, player->current_order);
        }
    } else {
        uint8_t len = modmed_player_get_song_length(player);
        if (len > 0) {
            if (player->type == PLAYER_TYPE_MOD) {
                mod_player_set_loop_range(player->mod_player, 0, len - 1);
            } else if (player->type == PLAYER_TYPE_MED) {
                med_player_set_loop_range(player->med_player, 0, len - 1);
            }
        }
    }
}

EMSCRIPTEN_KEEPALIVE
int modmed_player_get_song_length(const ModMedPlayer* player) {
    if (!player) return 0;
    if (player->type == PLAYER_TYPE_MOD) {
        return mod_player_get_song_length(player->mod_player);
    } else if (player->type == PLAYER_TYPE_MED) {
        return med_player_get_song_length(player->med_player);
    }
    return 0;
}

EMSCRIPTEN_KEEPALIVE
int modmed_player_get_num_channels(const ModMedPlayer* player) {
    if (!player) return 0;
    return 4;  // Both MOD and MED expose 4 channels for simplicity
}

EMSCRIPTEN_KEEPALIVE
int modmed_player_get_bpm(const ModMedPlayer* player) {
    if (!player) return 125;
    if (player->type == PLAYER_TYPE_MED) {
        return med_player_get_bpm(player->med_player);
    }
    return 125;  // MOD default
}

EMSCRIPTEN_KEEPALIVE
void modmed_player_set_bpm(ModMedPlayer* player, int bpm) {
    if (!player) return;
    if (player->type == PLAYER_TYPE_MOD) {
        mod_player_set_bpm(player->mod_player, bpm);
    } else if (player->type == PLAYER_TYPE_MED) {
        med_player_set_bpm(player->med_player, bpm);
    }
}

EMSCRIPTEN_KEEPALIVE
const char* modmed_player_get_filename(const ModMedPlayer* player) {
    return player ? player->filename : "";
}

EMSCRIPTEN_KEEPALIVE
const char* modmed_player_get_type_name(const ModMedPlayer* player) {
    if (!player) return "None";
    switch (player->type) {
        case PLAYER_TYPE_MOD: return "ProTracker MOD";
        case PLAYER_TYPE_MED: return "OctaMED";
        default: return "None";
    }
}

EMSCRIPTEN_KEEPALIVE
void* modmed_create_audio_buffer(int frames) {
    return malloc(frames * 2 * sizeof(float));
}

EMSCRIPTEN_KEEPALIVE
void modmed_destroy_audio_buffer(void* buffer) {
    free(buffer);
}

EMSCRIPTEN_KEEPALIVE
int modmed_get_buffer_size_bytes(int frames) {
    return frames * 2 * sizeof(float);
}
