#pragma once
/**
 * RGSlicer - Slicing Sampler for MicroKorg2
 *
 * Loads WAV samples from storage and auto-slices them for keyboard playback.
 * Uses MicroKorg2 device tempo for BPM mode.
 *
 * USAGE:
 * 1. Copy sample_0.wav, sample_1.wav, ... sample_7.wav to:
 *    /var/lib/microkorgd/userfs/Regroove/
 * 2. Load this unit
 * 3. Select preset 0-7 to load different samples
 * 4. Play MIDI notes C1-C5 (36-99) to trigger slices
 */

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cmath>
#include <cstring>
#include <cstdlib>
#include <cstdio>

#include "unit.h"
#include "runtime.h"
#include "utils/mk2_utils.h"

// Import RGSlicer engine
extern "C" {
#include "rgslicer.h"
}

// Regroove resource paths
#include "../../common/regroove_paths.h"

// Parameter indices
enum {
  PARAM_VOLUME = 0,
  PARAM_PITCH,
  PARAM_TIME,
  PARAM_MODE,
  PARAM_SLICES,
  PARAM_SENSE,
  PARAM_NOTE_DIVISION,
  PARAM_PITCH_ALGO,
  PARAM_TIME_ALGO
};

class RGSlicer {
public:
  RGSlicer(void)
    : slicer_(nullptr)
    , volume_(1.0f)
    , pitch_(0.0f)
    , time_(1.0f)
    , slice_mode_(0)
    , num_slices_(16)
    , sensitivity_(0.5f)
    , note_division_(4)       // 16th notes default
    , pitch_algorithm_(0)     // Simple (rate) default
    , time_algorithm_(1)      // AKAI/Amiga default
    , sample_loaded_(false)
    , current_preset_(0)
  {}

  ~RGSlicer(void) {
    if (slicer_) rgslicer_destroy(slicer_);
  }

  inline int8_t Init(const unit_runtime_desc_t * desc) {
    if (desc->samplerate != 48000)
      return k_unit_err_samplerate;

    if (desc->output_channels != 2)
      return k_unit_err_geometry;

    // Create RGSlicer instance
    slicer_ = rgslicer_create(48000);
    if (!slicer_) {
      return k_unit_err_memory;
    }

    // Set default parameters
    rgslicer_set_global_volume(slicer_, volume_);
    rgslicer_set_global_pitch(slicer_, pitch_);
    rgslicer_set_global_time(slicer_, time_);
    rgslicer_set_note_division(slicer_, note_division_);
    rgslicer_set_pitch_algorithm(slicer_, pitch_algorithm_);
    rgslicer_set_time_algorithm(slicer_, time_algorithm_);

    // Load preset configuration
    LoadConfig();

    // Try to load default preset
    LoadPreset(0);

    return k_unit_err_none;
  }

  inline void Teardown() {
    if (slicer_) {
      rgslicer_destroy(slicer_);
      slicer_ = nullptr;
    }
  }

  inline void Reset() {
    if (slicer_) rgslicer_all_notes_off(slicer_);
  }

  inline void Resume() {}
  inline void Suspend() {}

  fast_inline void Process(float *out, size_t frames) {
    if (!slicer_ || !sample_loaded_) {
      // No sample loaded, output silence
      for (uint32_t i = 0; i < frames * 2; i++) {
        out[i] = 0.0f;
      }
      return;
    }

    // Process audio (stereo interleaved)
    rgslicer_process_f32(slicer_, out, frames);
  }

  inline void NoteOn(uint8_t note, uint8_t velocity) {
    if (slicer_ && sample_loaded_) {
      rgslicer_note_on(slicer_, note, velocity);
    }
  }

  inline void NoteOff(uint8_t note) {
    if (slicer_ && sample_loaded_) {
      rgslicer_note_off(slicer_, note);
    }
  }

  inline void AllNoteOff() {
    if (slicer_) {
      rgslicer_all_notes_off(slicer_);
    }
  }

  inline void LoadPreset(uint8_t idx) {
    if (!slicer_ || idx >= 8) return;

    current_preset_ = idx;
    sample_loaded_ = false;

    // MicroKorg2 doesn't have Sample API - always use file loading with config
    char sample_path[256];
    snprintf(sample_path, sizeof(sample_path), "%s/%s", REGROOVE_RESOURCE_PATH, s_preset_files[idx]);

    if (rgslicer_load_sample(slicer_, sample_path)) {
      SliceMode mode = (SliceMode)slice_mode_;
      uint8_t slices = rgslicer_auto_slice(slicer_, mode, num_slices_, sensitivity_);
      sample_loaded_ = (slices > 0);
    }
  }

  inline uint8_t getPresetIndex() const {
    return current_preset_;
  }

  static inline const char* getPresetName(uint8_t idx) {
    return (idx < 8) ? s_preset_names[idx] : "";
  }

  inline void setParameter(uint8_t id, int32_t value) {
    if (!slicer_) return;

    switch (id) {
      case PARAM_VOLUME:
        volume_ = value / 100.0f;
        rgslicer_set_global_volume(slicer_, volume_);
        break;

      case PARAM_PITCH:
        pitch_ = (float)value;
        rgslicer_set_global_pitch(slicer_, pitch_);
        break;

      case PARAM_TIME:
        time_ = value / 100.0f;
        rgslicer_set_global_time(slicer_, time_);
        break;

      case PARAM_MODE:
        slice_mode_ = (uint8_t)value;
        if (sample_loaded_) {
          SliceMode mode = (SliceMode)slice_mode_;
          rgslicer_auto_slice(slicer_, mode, num_slices_, sensitivity_);
        }
        break;

      case PARAM_SLICES:
        num_slices_ = (uint8_t)value;
        if (sample_loaded_) {
          SliceMode mode = (SliceMode)slice_mode_;
          rgslicer_auto_slice(slicer_, mode, num_slices_, sensitivity_);
        }
        break;

      case PARAM_SENSE:
        sensitivity_ = value / 100.0f;
        if (sample_loaded_) {
          SliceMode mode = (SliceMode)slice_mode_;
          rgslicer_auto_slice(slicer_, mode, num_slices_, sensitivity_);
        }
        break;

      case PARAM_NOTE_DIVISION:
        note_division_ = (uint8_t)value;
        rgslicer_set_note_division(slicer_, note_division_);
        break;

      case PARAM_PITCH_ALGO:
        pitch_algorithm_ = (uint8_t)value;
        rgslicer_set_pitch_algorithm(slicer_, pitch_algorithm_);
        break;

      case PARAM_TIME_ALGO:
        time_algorithm_ = (uint8_t)value;
        rgslicer_set_time_algorithm(slicer_, time_algorithm_);
        break;
    }
  }

  inline int32_t getParameterValue(uint8_t id) const {
    switch (id) {
      case PARAM_VOLUME:        return (int32_t)(volume_ * 100.0f);
      case PARAM_PITCH:         return (int32_t)pitch_;
      case PARAM_TIME:          return (int32_t)(time_ * 100.0f);
      case PARAM_MODE:          return (int32_t)slice_mode_;
      case PARAM_SLICES:        return (int32_t)num_slices_;
      case PARAM_SENSE:         return (int32_t)(sensitivity_ * 100.0f);
      case PARAM_NOTE_DIVISION: return (int32_t)note_division_;
      case PARAM_PITCH_ALGO:    return (int32_t)pitch_algorithm_;
      case PARAM_TIME_ALGO:     return (int32_t)time_algorithm_;
      default:                  return 0;
    }
  }

  inline const char* getParameterStrValue(uint8_t id, int32_t value) const {
    (void)id;
    (void)value;
    return nullptr;
  }

  inline const uint8_t* getParameterBmpValue(uint8_t index, int32_t value) const {
    (void)index;
    (void)value;
    return nullptr;
  }

  inline void SetTempo(uint32_t tempo) {
    if (!slicer_) return;

    // Tempo is in fixed-point format: integer part in upper 16 bits, fractional in lower 16 bits
    float bpm = (tempo >> 16) + (tempo & 0xFFFF) / (float)0x10000;
    rgslicer_set_bpm(slicer_, bpm);
  }

  void unit_platform_exclusive(uint8_t messageId, void * data, uint32_t dataSize) {
    (void)messageId;
    (void)data;
    (void)dataSize;
  }

private:
  RGSlicer* slicer_;
  float volume_;
  float pitch_;
  float time_;
  uint8_t slice_mode_;
  uint8_t num_slices_;
  float sensitivity_;
  uint8_t note_division_;   // 1=quarter, 2=8th, 4=16th, 8=32nd
  uint8_t pitch_algorithm_; // 0=Simple (rate), 1=Time-Preserving
  uint8_t time_algorithm_;  // 0=Granular, 1=AKAI/Amiga
  bool sample_loaded_;
  uint8_t current_preset_;

  // Static config data
  static char s_preset_names[8][32];
  static char s_preset_files[8][64];
  static uint8_t s_num_presets;

  static inline void LoadConfig() {
    // Initialize defaults
    s_num_presets = 8;
    for (int i = 0; i < 8; i++) {
      snprintf(s_preset_names[i], sizeof(s_preset_names[i]), "Sample %d", i);
      snprintf(s_preset_files[i], sizeof(s_preset_files[i]), "sample_%d.wav", i);
    }

    // Try to load presets file
    char config_path[256];
    snprintf(config_path, sizeof(config_path), "%s/.rgslicer_presets", REGROOVE_RESOURCE_PATH);
    FILE* cfg = fopen(config_path, "r");
    if (!cfg) return;

    // Parse config: filename.wav = Display Name
    char line[128];
    int preset_idx = 0;
    while (fgets(line, sizeof(line), cfg) && preset_idx < 8) {
      // Skip empty lines and comments
      if (line[0] == '\n' || line[0] == '#' || line[0] == ';') continue;

      // Find '=' separator
      char* eq = strchr(line, '=');
      if (!eq) continue;

      // Extract filename (key)
      *eq = '\0';
      char* filename = line;
      // Trim trailing whitespace from filename
      char* end = eq - 1;
      while (end > filename && (*end == ' ' || *end == '\t')) {
        *end = '\0';
        end--;
      }

      // Extract display name (value)
      char* display_name = eq + 1;
      // Trim leading whitespace from display name
      while (*display_name == ' ' || *display_name == '\t') display_name++;
      // Trim trailing whitespace and newline
      end = display_name + strlen(display_name) - 1;
      while (end > display_name && (*end == '\n' || *end == '\r' || *end == ' ' || *end == '\t')) {
        *end = '\0';
        end--;
      }

      // Store config entry
      snprintf(s_preset_files[preset_idx], sizeof(s_preset_files[preset_idx]), "%s", filename);
      snprintf(s_preset_names[preset_idx], sizeof(s_preset_names[preset_idx]), "%s", display_name);
      preset_idx++;
    }

    fclose(cfg);
    s_num_presets = preset_idx;
  }
};
