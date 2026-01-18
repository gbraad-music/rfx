#pragma once
/**
 * RGSlicer - Slicing Sampler for Drumlogue
 *
 * Loads WAV samples from user storage and auto-slices them for keyboard playback.
 *
 * USAGE:
 * 1. Create /user/osc/rgslicer/ directory on your Drumlogue
 * 2. Copy sample_0.wav, sample_1.wav, ... sample_7.wav to that directory
 * 3. Load this unit
 * 4. Select preset 0-7 to load different samples
 * 5. Play MIDI notes C1-C5 (36-99) to trigger slices
 */

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cmath>
#include <cstring>
#include <cstdlib>
#include <cstdio>

#include "unit.h"

// Import RGSlicer engine
extern "C" {
#include "rgslicer.h"
}

// Parameter indices
enum {
  PARAM_VOLUME = 0,
  PARAM_PITCH,
  PARAM_TIME,
  PARAM_MODE,
  PARAM_SLICES,
  PARAM_SENSE
};

class Synth {
public:
  Synth(void)
    : slicer_(nullptr)
    , volume_(1.0f)
    , pitch_(0.0f)
    , time_(1.0f)
    , slice_mode_(0)
    , num_slices_(16)
    , sensitivity_(0.5f)
    , sample_loaded_(false)
    , current_preset_(0)
  {}

  ~Synth(void) {
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

  inline void Render(float *out, uint32_t frames) {
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

  inline void PitchBend(uint16_t bend) {
    // Not implemented yet
    (void)bend;
  }

  inline void ChannelPressure(uint8_t pressure) {
    (void)pressure;
  }

  inline void Aftertouch(uint8_t note, uint8_t aftertouch) {
    (void)note;
    (void)aftertouch;
  }

  inline void LoadPreset(uint8_t idx) {
    if (!slicer_ || idx >= 8) return;

    current_preset_ = idx;

    // Construct path to sample: /user/osc/rgslicer/sample_0.wav
    char sample_path[64];
    snprintf(sample_path, sizeof(sample_path), "/user/osc/rgslicer/sample_%d.wav", idx);

    // Try to load sample
    if (rgslicer_load_sample(slicer_, sample_path)) {
      // Auto-slice the sample
      SliceMode mode = (SliceMode)slice_mode_;
      uint8_t slices = rgslicer_auto_slice(slicer_, mode, num_slices_, sensitivity_);

      if (slices > 0) {
        sample_loaded_ = true;
      } else {
        sample_loaded_ = false;
      }
    } else {
      sample_loaded_ = false;
    }
  }

  inline uint8_t getPresetIndex() {
    return current_preset_;
  }

  static inline const char* getPresetName(uint8_t idx) {
    static const char* preset_names[8] = {
      "Sample 0",
      "Sample 1",
      "Sample 2",
      "Sample 3",
      "Sample 4",
      "Sample 5",
      "Sample 6",
      "Sample 7"
    };

    return (idx < 8) ? preset_names[idx] : "";
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
    }
  }

  inline int32_t getParameterValue(uint8_t id) {
    switch (id) {
      case PARAM_VOLUME:  return (int32_t)(volume_ * 100.0f);
      case PARAM_PITCH:   return (int32_t)pitch_;
      case PARAM_TIME:    return (int32_t)(time_ * 100.0f);
      case PARAM_MODE:    return (int32_t)slice_mode_;
      case PARAM_SLICES:  return (int32_t)num_slices_;
      case PARAM_SENSE:   return (int32_t)(sensitivity_ * 100.0f);
      default:            return 0;
    }
  }

  inline const char* getParameterStrValue(uint8_t id, int32_t value) {
    static char buf[16];

    switch (id) {
      case PARAM_MODE:
        switch (value) {
          case 0: return "TRANS";
          case 1: return "ZERO";
          case 2: return "GRID";
          case 3: return "BPM";
          default: return "";
        }

      case PARAM_PITCH:
        snprintf(buf, sizeof(buf), "%+d", (int)value);
        return buf;

      default:
        return nullptr;
    }
  }

  inline const uint8_t* getParameterBmpValue(uint8_t id, int32_t value) {
    (void)id;
    (void)value;
    return nullptr;
  }

private:
  RGSlicer* slicer_;
  float volume_;
  float pitch_;
  float time_;
  uint8_t slice_mode_;
  uint8_t num_slices_;
  float sensitivity_;
  bool sample_loaded_;
  uint8_t current_preset_;
};
