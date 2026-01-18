#ifndef SFZ_PLAYER_H
#define SFZ_PLAYER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Opaque player structure
typedef struct RGSFZPlayer RGSFZPlayer;

// Player management
RGSFZPlayer* rgsfz_player_create(uint32_t sample_rate);
void rgsfz_player_destroy(RGSFZPlayer* player);

// SFZ loading
int rgsfz_player_load_sfz_from_memory(RGSFZPlayer* player, const char* data, uint32_t length);

// Region management
int rgsfz_player_add_region(RGSFZPlayer* player,
                             uint8_t lokey, uint8_t hikey,
                             uint8_t lovel, uint8_t hivel,
                             uint8_t pitch_keycenter,
                             const char* sample_path,
                             float pan);

void rgsfz_player_load_region_sample(RGSFZPlayer* player, uint32_t region_index,
                                      int16_t* data, uint32_t num_samples,
                                      uint32_t sample_rate);

// Playback
void rgsfz_player_note_on(RGSFZPlayer* player, uint8_t note, uint8_t velocity);
void rgsfz_player_note_off(RGSFZPlayer* player, uint8_t note);
void rgsfz_player_all_notes_off(RGSFZPlayer* player);
void rgsfz_player_process_f32(RGSFZPlayer* player, float* buffer, uint32_t frames);

// Parameters
void rgsfz_player_set_volume(RGSFZPlayer* player, float volume);
void rgsfz_player_set_pan(RGSFZPlayer* player, float pan);
void rgsfz_player_set_decay(RGSFZPlayer* player, float decay);

// Queries
uint32_t rgsfz_player_get_num_regions(RGSFZPlayer* player);
uint32_t rgsfz_player_get_active_voices(RGSFZPlayer* player);

const char* rgsfz_player_get_region_sample(RGSFZPlayer* player, uint32_t index);
uint8_t rgsfz_player_get_region_lokey(RGSFZPlayer* player, uint32_t index);
uint8_t rgsfz_player_get_region_hikey(RGSFZPlayer* player, uint32_t index);
uint8_t rgsfz_player_get_region_lovel(RGSFZPlayer* player, uint32_t index);
uint8_t rgsfz_player_get_region_hivel(RGSFZPlayer* player, uint32_t index);

#ifdef __cplusplus
}
#endif

#endif // SFZ_PLAYER_H
