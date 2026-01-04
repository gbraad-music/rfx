#pragma once
/**
 * RG1Piano - M1 Piano sampler for NTS-1 mkII
 */

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cmath>

#include "unit.h"

// Import synth components
extern "C" {
#include "synth_sample_player.h"
#include "sample_data.h"  // From ../../data/rg1piano/
}

// Parameter indices
enum {
  PARAM_DECAY = 0,
  PARAM_BRIGHTNESS,
  PARAM_VELOCITY_SENS,
  PARAM_VOLUME,
  PARAM_LFO_RATE,
  PARAM_LFO_DEPTH
};

class Synth {
public:
  Synth(void)
    : sample_player_(nullptr)
    , note_(-1)
    , velocity_(0)
    , gate_(false)
    , active_(false)
    , decay_(0.5f)
    , brightness_(0.5f)
    , velocity_sens_(0.8f)
    , volume_(0.7f)
    , lfo_rate_(0.3f)
    , lfo_depth_(0.2f)
    , lfo_phase_(0.0f)
    , filter_prev_sample_(0.0f)
  {}

  ~Synth(void) {
    if (sample_player_) synth_sample_player_destroy(sample_player_);
  }

  inline int8_t Init(const unit_runtime_desc_t * desc) {
    if (desc->samplerate != 48000)
      return k_unit_err_samplerate;

    if (desc->output_channels != 2)
      return k_unit_err_geometry;

    // Create sample player
    sample_player_ = synth_sample_player_create();
    if (!sample_player_) {
      return k_unit_err_memory;
    }

    // Initialize sample data
    sample_data_.attack_data = m1piano_onset;
    sample_data_.attack_length = m1piano_onset_length;
    sample_data_.loop_data = m1piano_tail;
    sample_data_.loop_length = m1piano_tail_length;
    sample_data_.sample_rate = 22050;
    sample_data_.root_note = 48;  // C3 (131.6 Hz verified with aubio)

    synth_sample_player_load_sample(sample_player_, &sample_data_);

    // Set default decay time
    synth_sample_player_set_loop_decay(sample_player_, 2.0f);

    return k_unit_err_none;
  }

  inline void Teardown() {
    if (sample_player_) {
      synth_sample_player_destroy(sample_player_);
      sample_player_ = nullptr;
    }
  }

  inline void Reset() {
    gate_ = false;
    active_ = false;
    note_ = -1;
    filter_prev_sample_ = 0.0f;

    if (sample_player_) synth_sample_player_reset(sample_player_);
  }

  inline void Resume() {}
  inline void Suspend() {}

  fast_inline void Render(float * out, size_t frames) {
    float * __restrict out_p = out;
    const float * out_e = out_p + (frames << 1);

    for (; out_p != out_e; out_p += 2) {
      float sample = renderSample();
      out_p[0] = out_p[1] = sample;
    }
  }

  inline void setParameter(uint8_t index, int32_t value) {
    switch (index) {
      case PARAM_DECAY:
        decay_ = value / 1023.0f;
        if (sample_player_) {
          // Map decay parameter to 0.5s - 8s range
          float decay_time = 0.5f + decay_ * 7.5f;
          synth_sample_player_set_loop_decay(sample_player_, decay_time);
        }
        break;

      case PARAM_BRIGHTNESS:
        brightness_ = value / 1023.0f;
        break;

      case PARAM_VELOCITY_SENS:
        velocity_sens_ = value / 1023.0f;
        break;

      case PARAM_VOLUME:
        volume_ = value / 1023.0f;
        break;

      case PARAM_LFO_RATE:
        lfo_rate_ = value / 1023.0f;
        break;

      case PARAM_LFO_DEPTH:
        lfo_depth_ = value / 1023.0f;
        break;

      default:
        break;
    }
  }

  inline int32_t getParameterValue(uint8_t index) const {
    switch (index) {
      case PARAM_DECAY: return (int32_t)(decay_ * 1023.0f);
      case PARAM_BRIGHTNESS: return (int32_t)(brightness_ * 1023.0f);
      case PARAM_VELOCITY_SENS: return (int32_t)(velocity_sens_ * 1023.0f);
      case PARAM_VOLUME: return (int32_t)(volume_ * 1023.0f);
      case PARAM_LFO_RATE: return (int32_t)(lfo_rate_ * 1023.0f);
      case PARAM_LFO_DEPTH: return (int32_t)(lfo_depth_ * 1023.0f);
      default: return 0;
    }
  }

  inline const char * getParameterStrValue(uint8_t index, int32_t value) const {
    (void)index;
    (void)value;
    return nullptr;
  }

  inline const uint8_t * getParameterBmpValue(uint8_t index, int32_t value) const {
    (void)value;
    (void)index;
    return nullptr;
  }

  inline void NoteOn(uint8_t note, uint8_t velocity) {
    note_ = note;
    velocity_ = velocity;
    gate_ = true;
    active_ = true;

    // Reset LFO phase for each note
    lfo_phase_ = 0.0f;

    if (sample_player_) {
      // Apply velocity sensitivity
      uint8_t effective_velocity = (uint8_t)(127.0f * (1.0f - velocity_sens_ + velocity_sens_ * (velocity / 127.0f)));
      synth_sample_player_trigger(sample_player_, note, effective_velocity);
    }
  }

  inline void NoteOff(uint8_t note) {
    if (note == note_) {
      gate_ = false;

      if (sample_player_) {
        synth_sample_player_release(sample_player_);
      }
    }
  }

  inline void GateOn(uint8_t velocity) {
    velocity_ = velocity;
    gate_ = true;
    active_ = true;
  }

  inline void GateOff() {
    gate_ = false;

    if (sample_player_) {
      synth_sample_player_release(sample_player_);
    }
  }

  inline void AllNoteOff() {
    gate_ = false;
    active_ = false;

    if (sample_player_) {
      synth_sample_player_release(sample_player_);
    }
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
  float renderSample() {
    if (!active_ || !sample_player_) {
      return 0.0f;
    }

    const int sampleRate = 48000;

    // Process sample player
    float sample = synth_sample_player_process(sample_player_, sampleRate);

    // Apply LFO modulation (tremolo/vibrato effect)
    // Map rate: 0-1 -> 0.5Hz-8Hz
    float lfo_freq = 0.5f + lfo_rate_ * 7.5f;
    lfo_phase_ += (2.0f * M_PI * lfo_freq) / sampleRate;
    if (lfo_phase_ >= 2.0f * M_PI) {
      lfo_phase_ -= 2.0f * M_PI;
    }

    // LFO creates tremolo effect: amplitude modulation
    // depth 0 = no effect, depth 1 = full tremolo
    float lfo_value = sinf(lfo_phase_);
    float lfo_mod = 1.0f - (lfo_depth_ * 0.3f * (1.0f - lfo_value));
    sample *= lfo_mod;

    // Apply simple brightness filter (low-pass)
    // This is a basic one-pole filter
    // Map brightness to 0.3-1.0 range to prevent signal loss at low values
    float cutoff = 0.3f + brightness_ * 0.7f;
    sample = filter_prev_sample_ + cutoff * (sample - filter_prev_sample_);
    filter_prev_sample_ = sample;

    // Apply master volume
    sample *= volume_;

    // Soft clipping to prevent harsh digital clipping and squealing
    // Tanh-style soft saturation
    if (sample > 1.0f) {
      sample = 1.0f - expf(-(sample - 1.0f));
    } else if (sample < -1.0f) {
      sample = -1.0f + expf(sample + 1.0f);
    }

    // Check if sample player is still active
    if (!synth_sample_player_is_active(sample_player_)) {
      active_ = false;
    }

    return sample;
  }

  SynthSamplePlayer* sample_player_;
  SampleData sample_data_;

  int note_;
  int velocity_;
  bool gate_;
  bool active_;

  // Parameters
  float decay_;
  float brightness_;
  float velocity_sens_;
  float volume_;
  float lfo_rate_;
  float lfo_depth_;

  // LFO state
  float lfo_phase_;

  // Filter state
  float filter_prev_sample_;
};
