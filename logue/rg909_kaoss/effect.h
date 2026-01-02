#pragma once
/**
 * RG909 Kaoss - Touch-based rhythmic kick effect for NTS-3
 */

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cmath>

#include "unit_genericfx.h"

// Import modular RG909 drums
extern "C" {
#include "rg909_bd.h"
#include "rg909_sd.h"
}

// Parameter indices
enum {
  PARAM_LEVEL = 0,
  PARAM_TUNE,
  PARAM_DECAY,
  PARAM_ATTACK,
  PARAM_DRY_WET,
  PARAM_PATTERN,
  PARAM_SWING,
  PARAM_TONE
};

class Effect {
public:
  Effect(void)
    : bd_()
    , sd_()
    , sample_rate_(48000.0f)
    , tempo_(120.0f)
    , ppqn_counter_(0)
    , beat_counter_(0)
    , touch_y_(1.0f)
    , touch_active_(false)
    , level_(0.8f)
    , tune_(0.5f)
    , decay_(0.5f)
    , attack_(0.1f)
    , dry_wet_(0.5f)
    , pattern_(1.0f)
    , swing_(0.5f)
    , tone_(0.3f)
  {
    // Drums will be initialized in Init()
  }

  ~Effect(void) {
    // No cleanup needed for static structs
  }

  inline int8_t Init(const unit_runtime_desc_t * desc) {
    if (desc->samplerate != 48000)
      return k_unit_err_samplerate;

    if (desc->input_channels != 2 || desc->output_channels != 2)
      return k_unit_err_geometry;

    sample_rate_ = desc->samplerate;

    // Initialize drums
    rg909_bd_init(&bd_);
    rg909_sd_init(&sd_);

    // Set initial drum parameters
    rg909_bd_set_level(&bd_, level_);
    rg909_bd_set_tune(&bd_, tune_);
    rg909_bd_set_decay(&bd_, decay_);
    rg909_bd_set_attack(&bd_, attack_);

    rg909_sd_set_level(&sd_, level_);
    rg909_sd_set_tone(&sd_, tone_);
    rg909_sd_set_snappy(&sd_, tone_);
    rg909_sd_set_tuning(&sd_, tune_);

    return k_unit_err_none;
  }

  inline void Teardown() {
    // Clean up SD's dynamically allocated components
    rg909_sd_destroy(&sd_);
  }

  inline void Reset() {
    rg909_bd_reset(&bd_);
    rg909_sd_reset(&sd_);
    ppqn_counter_ = 0;
    beat_counter_ = 0;
  }

  inline void Resume() {}
  inline void Suspend() {}

  inline void Render(const float * in, float * out, size_t frames) {
    // Process drums for each frame
    for (size_t f = 0; f < frames; f++) {
      float bd_sample = rg909_bd_process(&bd_, sample_rate_);
      float sd_sample = rg909_sd_process(&sd_, sample_rate_);
      float drum_mix = bd_sample + sd_sample;

      // Mix with input based on dry/wet (stereo)
      size_t idx = f * 2;
      out[idx] = in[idx] * (1.0f - dry_wet_) + drum_mix * dry_wet_;         // Left
      out[idx + 1] = in[idx + 1] * (1.0f - dry_wet_) + drum_mix * dry_wet_; // Right
    }
  }

  inline void setParameter(uint8_t index, int32_t value) {
    // NTS-3 uses 0-1023 range
    switch (index) {
      case PARAM_LEVEL:
        level_ = value / 1023.0f;
        rg909_bd_set_level(&bd_, level_);
        rg909_sd_set_level(&sd_, level_);
        break;

      case PARAM_TUNE:
        tune_ = value / 1023.0f;
        rg909_bd_set_tune(&bd_, tune_);
        rg909_sd_set_tuning(&sd_, tune_);
        break;

      case PARAM_DECAY:
        decay_ = value / 1023.0f;
        rg909_bd_set_decay(&bd_, decay_);
        break;

      case PARAM_ATTACK:
        attack_ = value / 1023.0f;
        rg909_bd_set_attack(&bd_, attack_);
        break;

      case PARAM_DRY_WET:
        dry_wet_ = value / 1023.0f;
        break;

      case PARAM_PATTERN:
        pattern_ = value / 1023.0f;
        break;

      case PARAM_SWING:
        swing_ = value / 1023.0f;
        break;

      case PARAM_TONE:
        tone_ = value / 1023.0f;
        rg909_sd_set_tone(&sd_, tone_);
        rg909_sd_set_snappy(&sd_, tone_);  // Use tone to control snappiness too
        break;

      default:
        break;
    }
  }

  inline int32_t getParameterValue(uint8_t index) const {
    // NTS-3 uses 0-1023 range
    switch (index) {
      case PARAM_LEVEL: return (int32_t)(level_ * 1023.0f);
      case PARAM_TUNE: return (int32_t)(tune_ * 1023.0f);
      case PARAM_DECAY: return (int32_t)(decay_ * 1023.0f);
      case PARAM_ATTACK: return (int32_t)(attack_ * 1023.0f);
      case PARAM_DRY_WET: return (int32_t)(dry_wet_ * 1023.0f);
      case PARAM_PATTERN: return (int32_t)(pattern_ * 1023.0f);
      case PARAM_SWING: return (int32_t)(swing_ * 1023.0f);
      case PARAM_TONE: return (int32_t)(tone_ * 1023.0f);
      default: return 0;
    }
  }

  inline const char * getParameterStrValue(uint8_t index, int32_t value) const {
    (void)value;
    (void)index;
    return nullptr;
  }

  inline const uint8_t * getParameterBmpValue(uint8_t index, int32_t value) const {
    (void)value;
    (void)index;
    return nullptr;
  }

  inline void setTempo(uint32_t tempo) {
    // Tempo is in fixed point: high 16 bits = integer, low 16 bits = fraction
    tempo_ = (tempo >> 16) + (tempo & 0xFFFF) / static_cast<float>(0x10000);
  }

  inline void tempo4ppqnTick(uint32_t counter) {
    ppqn_counter_ = counter;

    // Trigger on quarter note boundaries (every 4 ticks)
    if ((counter % 4) == 0) {
      uint32_t quarter_note = counter / 4;
      beat_counter_ = quarter_note % 4; // Beat within bar (0-3)

      // Determine if we should trigger based on pattern density
      float density = touch_active_ ? touch_y_ : pattern_;

      bool should_trigger = false;

      if (density > 0.875f) {
        // Four-on-the-floor (all beats)
        should_trigger = true;
      } else if (density > 0.625f) {
        // Beats 1, 2, 3, 4 (full)
        should_trigger = true;
      } else if (density > 0.375f) {
        // Beats 1 and 3
        should_trigger = (beat_counter_ == 0 || beat_counter_ == 2);
      } else if (density > 0.125f) {
        // Beat 1 only
        should_trigger = (beat_counter_ == 0);
      }
      // Below 0.125f = no triggers

      if (should_trigger) {
        // Trigger bass drum on pattern beats
        rg909_bd_trigger(&bd_, 100, sample_rate_);

        // Trigger snare on beats 2 and 4 (backbeat)
        if (beat_counter_ == 1 || beat_counter_ == 3) {
          rg909_sd_trigger(&sd_, 100, sample_rate_);
        }
      }
    }
  }

  inline void touchEvent(uint8_t id, uint8_t phase, uint32_t x, uint32_t y) {
    (void)id;
    (void)x;

    // Touch Y position controls pattern density
    // Y range is 0-1023, convert to 0.0-1.0
    touch_y_ = y / 1023.0f;

    // Track touch state
    if (phase == 0) {
      // Touch began - trigger drums for testing
      touch_active_ = true;
      rg909_bd_trigger(&bd_, 100, sample_rate_);
      rg909_sd_trigger(&sd_, 100, sample_rate_);
    } else if (phase == 2) {
      // Touch ended
      touch_active_ = false;
    }
    // phase == 1 is touch moved, keeps touch_active_ = true
  }

  inline void LoadPreset(uint8_t idx) { (void)idx; }
  inline uint8_t getPresetIndex() const { return 0; }

  static inline const char * getPresetName(uint8_t idx) {
    (void)idx;
    return nullptr;
  }

private:
  RG909_BD bd_;
  RG909_SD sd_;
  float sample_rate_;
  float tempo_;
  uint32_t ppqn_counter_;
  uint32_t beat_counter_;
  float touch_y_;
  bool touch_active_;

  // Parameters
  float level_;
  float tune_;
  float decay_;
  float attack_;
  float dry_wet_;
  float pattern_;
  float swing_;
  float tone_;
};
