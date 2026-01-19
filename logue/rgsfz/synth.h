#pragma once
/**
 * RGSFZ Player - SFZ sampler for Drumlogue / Microkrog2
 *
 * Loads SFZ files and WAV samples from user storage using standard C file I/O.
 *
 * USAGE:
 * 1. Create /user/osc/rgsfz/ directory on your device
 * 2. Copy preset_0.sfz and referenced WAV files to that directory
 * 3. Load this unit
 * 4. Select preset 0-7 to load different SFZ files
 */

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cmath>
#include <cstring>
#include <cstdlib>
#include <cstdio>

#include "unit.h"

// Import SFZ player components
extern "C" {
#include "sfz_player.h"
#include "sfz_parser.h"
#include "../../common/sample_loader.h"
}

// Parameter indices
enum {
  PARAM_VOLUME = 0,
  PARAM_PAN,
  PARAM_DECAY
};

class Synth {
public:
  Synth(void)
    : player_(nullptr)
    , volume_(2.0f)
    , pan_(0.0f)
    , decay_(0.5f)
    , sfz_loaded_(false)
    , current_preset_(0)
  {}

  ~Synth(void) {
    if (player_) rgsfz_player_destroy(player_);
  }

  inline int8_t Init(const unit_runtime_desc_t * desc) {
    if (desc->samplerate != 48000)
      return k_unit_err_samplerate;

    if (desc->output_channels != 2)
      return k_unit_err_geometry;

    // Create RGSFZ player
    player_ = rgsfz_player_create(48000);
    if (!player_) {
      return k_unit_err_memory;
    }

    // Set default parameters
    rgsfz_player_set_volume(player_, volume_);
    rgsfz_player_set_pan(player_, pan_);
    rgsfz_player_set_decay(player_, decay_);

    // Try to load default preset
    LoadPreset(0);

    return k_unit_err_none;
  }

  inline void Teardown() {
    if (player_) {
      rgsfz_player_destroy(player_);
      player_ = nullptr;
    }
  }

  inline void Reset() {
    if (player_) rgsfz_player_all_notes_off(player_);
  }

  inline void Resume() {}
  inline void Suspend() {}

  inline void Render(float *out, uint32_t frames) {
    if (!player_ || !sfz_loaded_) {
      // No SFZ loaded, output silence
      for (uint32_t i = 0; i < frames * 2; i++) {
        out[i] = 0.0f;
      }
      return;
    }

    // Process audio
    rgsfz_player_process_f32(player_, out, frames);
  }

  inline void NoteOn(uint8_t note, uint8_t velocity) {
    if (!player_ || !sfz_loaded_) return;
    rgsfz_player_note_on(player_, note, velocity);
  }

  inline void NoteOff(uint8_t note) {
    if (!player_ || !sfz_loaded_) return;
    rgsfz_player_note_off(player_, note);
  }

  inline void AllNoteOff() {
    if (!player_) return;
    rgsfz_player_all_notes_off(player_);
  }

  inline void PitchBend(uint16_t bend) {
    // TODO: Implement pitch bend
    (void)bend;
  }

  inline void ChannelPressure(uint8_t press) {
    // TODO: Implement aftertouch
    (void)press;
  }

  inline void Aftertouch(uint8_t note, uint8_t press) {
    // TODO: Implement poly aftertouch
    (void)note;
    (void)press;
  }

  inline void setParameter(uint8_t id, int32_t value) {
    if (!player_) return;

    switch (id) {
      case PARAM_VOLUME:
        // 0-200 -> 0.0-2.0
        volume_ = value / 100.0f;
        rgsfz_player_set_volume(player_, volume_);
        break;

      case PARAM_PAN:
        // 0-200 -> -1.0 to +1.0
        pan_ = (value - 100) / 100.0f;
        rgsfz_player_set_pan(player_, pan_);
        break;

      case PARAM_DECAY:
        // 0-100 -> 0.0-1.0
        decay_ = value / 100.0f;
        rgsfz_player_set_decay(player_, decay_);
        break;
    }
  }

  inline int32_t getParameterValue(uint8_t id) const {
    switch (id) {
      case PARAM_VOLUME:
        return (int32_t)(volume_ * 100.0f);
      case PARAM_PAN:
        return (int32_t)(pan_ * 100.0f) + 100;
      case PARAM_DECAY:
        return (int32_t)(decay_ * 100.0f);
      default:
        return 0;
    }
  }

  inline const char* getParameterStrValue(uint8_t id, int32_t value) {
    static char str_buffer[11];

    switch (id) {
      case PARAM_VOLUME:
        snprintf(str_buffer, sizeof(str_buffer), "%d%%", (int)value);
        break;

      case PARAM_PAN:
        {
          int pan_val = value - 100;
          if (pan_val == 0) {
            snprintf(str_buffer, sizeof(str_buffer), "C");
          } else if (pan_val < 0) {
            snprintf(str_buffer, sizeof(str_buffer), "L%d", -pan_val);
          } else {
            snprintf(str_buffer, sizeof(str_buffer), "R%d", pan_val);
          }
        }
        break;

      case PARAM_DECAY:
        snprintf(str_buffer, sizeof(str_buffer), "%d%%", (int)value);
        break;

      default:
        str_buffer[0] = '\0';
        break;
    }

    return str_buffer;
  }

  inline void LoadPreset(uint8_t preset_idx) {
    if (!player_) return;

    current_preset_ = preset_idx;
    sfz_loaded_ = false;

    // Construct path to SFZ file
    char sfz_path[256];
    snprintf(sfz_path, sizeof(sfz_path), "/user/osc/rgsfz/preset_%d.sfz", preset_idx);

    // Open and read SFZ file
    FILE* sfz_file = fopen(sfz_path, "r");
    if (!sfz_file) {
      return;  // File not found
    }

    // Get file size
    fseek(sfz_file, 0, SEEK_END);
    long sfz_size = ftell(sfz_file);
    fseek(sfz_file, 0, SEEK_SET);

    if (sfz_size <= 0 || sfz_size > 65536) {
      fclose(sfz_file);
      return;  // Invalid size
    }

    // Read SFZ content
    char* sfz_buffer = (char*)malloc(sfz_size + 1);
    if (!sfz_buffer) {
      fclose(sfz_file);
      return;  // Out of memory
    }

    size_t read_size = fread(sfz_buffer, 1, sfz_size, sfz_file);
    fclose(sfz_file);

    if (read_size != (size_t)sfz_size) {
      free(sfz_buffer);
      return;  // Read error
    }

    sfz_buffer[sfz_size] = '\0';

    // Parse SFZ file
    int result = rgsfz_player_load_sfz_from_memory(player_, sfz_buffer, sfz_size);
    free(sfz_buffer);

    if (!result) {
      return;  // Parse error
    }

    // Load WAV samples for each region
    uint32_t num_regions = rgsfz_player_get_num_regions(player_);
    for (uint32_t i = 0; i < num_regions; i++) {
      const char* sample_path = rgsfz_player_get_region_sample(player_, i);

      if (!sample_path || sample_path[0] == '\0') {
        continue;  // No sample for this region
      }

      // Construct full path to WAV file
      char wav_path[512];
      snprintf(wav_path, sizeof(wav_path), "/user/osc/rgsfz/%s", sample_path);

      // Load WAV file
      LoadWAVSample(i, wav_path);
    }

    sfz_loaded_ = (num_regions > 0);
  }

  inline uint8_t getPresetIndex() const {
    return current_preset_;
  }

  static inline const char* getPresetName(uint8_t idx) {
    static const char* preset_names[8] = {
      "Preset 0",
      "Preset 1",
      "Preset 2",
      "Preset 3",
      "Preset 4",
      "Preset 5",
      "Preset 6",
      "Preset 7"
    };

    if (idx < 8) {
      return preset_names[idx];
    }
    return nullptr;
  }

  inline const uint8_t* getParameterBmpValue(uint8_t index, int32_t value) const {
    (void)index;
    (void)value;
    return nullptr;
  }

private:
  inline void LoadWAVSample(uint32_t region_idx, const char* wav_path) {
    // Use shared WAV loader
    WAVSample* sample = wav_load_file(wav_path);
    if (!sample) {
      return;  // Load failed
    }

    // Load into RGSFZ player
    rgsfz_player_load_region_sample(player_, region_idx, sample->pcm_data,
                                     sample->num_samples, sample->sample_rate);

    // Free WAV sample
    wav_free_sample(sample);
  }

  RGSFZPlayer* player_;
  float volume_;
  float pan_;
  float decay_;
  bool sfz_loaded_;
  uint8_t current_preset_;
};
