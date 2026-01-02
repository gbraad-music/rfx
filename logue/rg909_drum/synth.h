#pragma once
/**
 * RG909 Drum Synth - TR-909 Bass Drum for Drumlogue
 */

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cmath>
#include <arm_neon.h>

#include "unit.h"

// Import RG909 drum synth
extern "C" {
#include "rg909_drum_synth.h"
}

// Parameter indices
enum {
  PARAM_LEVEL = 0,
  PARAM_TUNE,
  PARAM_DECAY,
  PARAM_ATTACK,
  PARAM_SWEEP,
  PARAM_TONE,
  PARAM_MASTER
};

class Synth {
public:
  Synth(void)
    : rg909_(nullptr)
    , sample_rate_(48000.0f)
    , level_(0.8f)
    , tune_(0.5f)
    , decay_(0.5f)
    , attack_(0.1f)
    , sweep_(0.5f)
    , tone_(0.3f)
    , master_(0.7f)
  {}

  ~Synth(void) {
    if (rg909_) rg909_synth_destroy(rg909_);
  }

  inline int8_t Init(const unit_runtime_desc_t * desc) {
    if (desc->samplerate != 48000)
      return k_unit_err_samplerate;

    if (desc->output_channels != 2)
      return k_unit_err_geometry;

    sample_rate_ = desc->samplerate;

    // Create RG909 synth
    rg909_ = rg909_synth_create();
    if (!rg909_) {
      return k_unit_err_memory;
    }

    // Initialize BD parameters
    updateParameters();

    return k_unit_err_none;
  }

  inline void Teardown() {
    if (rg909_) {
      rg909_synth_destroy(rg909_);
      rg909_ = nullptr;
    }
  }

  inline void Reset() {
    if (rg909_) rg909_synth_reset(rg909_);
  }

  inline void Resume() {}
  inline void Suspend() {}

  fast_inline void Render(float * out, size_t frames) {
    if (!rg909_) {
      // Clear buffer if synth not initialized
      for (size_t i = 0; i < frames * 2; i++) {
        out[i] = 0.0f;
      }
      return;
    }

    // Process RG909 synth (outputs interleaved stereo)
    rg909_synth_process_interleaved(rg909_, out, frames, sample_rate_);
  }

  inline void setParameter(uint8_t index, int32_t value) {
    switch (index) {
      case PARAM_LEVEL:
        level_ = value / 100.0f;
        if (rg909_) rg909_->bd_level = level_ * 2.0f; // Allow boost
        break;

      case PARAM_TUNE:
        tune_ = value / 100.0f;
        if (rg909_) rg909_->bd_tune = tune_;
        break;

      case PARAM_DECAY:
        decay_ = value / 100.0f;
        if (rg909_) rg909_->bd_decay = decay_;
        break;

      case PARAM_ATTACK:
        attack_ = value / 100.0f;
        if (rg909_) rg909_->bd_attack = attack_;
        break;

      case PARAM_SWEEP:
        sweep_ = value / 100.0f;
        updateSweepParameters();
        break;

      case PARAM_TONE:
        tone_ = value / 100.0f;
        updateToneParameters();
        break;

      case PARAM_MASTER:
        master_ = value / 100.0f;
        if (rg909_) rg909_->master_volume = master_;
        break;

      default:
        break;
    }
  }

  inline int32_t getParameterValue(uint8_t index) const {
    switch (index) {
      case PARAM_LEVEL: return (int32_t)(level_ * 100.0f);
      case PARAM_TUNE: return (int32_t)(tune_ * 100.0f);
      case PARAM_DECAY: return (int32_t)(decay_ * 100.0f);
      case PARAM_ATTACK: return (int32_t)(attack_ * 100.0f);
      case PARAM_SWEEP: return (int32_t)(sweep_ * 100.0f);
      case PARAM_TONE: return (int32_t)(tone_ * 100.0f);
      case PARAM_MASTER: return (int32_t)(master_ * 100.0f);
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

  inline void NoteOn(uint8_t note, uint8_t velocity) {
    if (rg909_) {
      // Trigger BD on any note
      rg909_synth_trigger_drum(rg909_, RG909_MIDI_NOTE_BD, velocity, sample_rate_);
    }
  }

  inline void NoteOff(uint8_t note) {
    (void)note;
    // BD doesn't need note-off
  }

  inline void GateOn(uint8_t velocity) {
    if (rg909_) {
      rg909_synth_trigger_drum(rg909_, RG909_MIDI_NOTE_BD, velocity, sample_rate_);
    }
  }

  inline void GateOff() {
    // BD doesn't need gate-off
  }

  inline void AllNoteOff() {
    // Can implement if needed
  }

  inline void PitchBend(uint16_t bend) { (void)bend; }
  inline void ChannelPressure(uint8_t pressure) { (void)pressure; }
  inline void Aftertouch(uint8_t note, uint8_t aftertouch) {
    (void)note;
    (void)aftertouch;
  }

  inline void LoadPreset(uint8_t idx) { (void)idx; }
  inline uint8_t getPresetIndex() const { return 0; }

  static inline const char * getPresetName(uint8_t idx) {
    (void)idx;
    return nullptr;
  }

private:
  void updateParameters() {
    if (!rg909_) return;

    rg909_->bd_level = level_ * 2.0f;
    rg909_->bd_tune = tune_;
    rg909_->bd_decay = decay_;
    rg909_->bd_attack = attack_;
    rg909_->master_volume = master_;

    updateSweepParameters();
    updateToneParameters();
  }

  void updateSweepParameters() {
    if (!rg909_) return;

    // Map sweep parameter to pitch sweep characteristics
    // 0 = less sweep, 1 = more sweep
    float sweep_mult = 0.5f + sweep_ * 1.5f; // Range: 0.5 to 2.0

    rg909_->bd_squiggly_freq = 90.0f * sweep_mult;
    rg909_->bd_fast_freq = 65.0f * sweep_mult;
    rg909_->bd_slow_freq = 52.0f * sweep_mult;
    rg909_->bd_tail_freq = 45.0f * sweep_mult;
  }

  void updateToneParameters() {
    if (!rg909_) return;

    // Map tone parameter to SAW width (affects harmonic content)
    // 0 = more sine-like, 1 = more saw-like (brighter)
    rg909_->bd_fast_saw_pct = 20.0f + tone_ * 60.0f; // Range: 20-80%
    rg909_->bd_slow_saw_pct = 15.0f + tone_ * 50.0f; // Range: 15-65%
  }

  RG909Synth* rg909_;
  float sample_rate_;

  // Parameters
  float level_;
  float tune_;
  float decay_;
  float attack_;
  float sweep_;
  float tone_;
  float master_;
};
