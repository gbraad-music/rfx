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
#include "synth_modal_piano.h"
#include "sample_data.h"  // From ../../data/rg1piano/
}

// Parameter indices
enum {
  PARAM_DECAY = 0,
  PARAM_RESONANCE,      // Modal resonator amount
  PARAM_BRIGHTNESS,     // Filter envelope sustain
  PARAM_VELOCITY_SENS,
  PARAM_VOLUME,
  PARAM_LFO_RATE,
  PARAM_LFO_DEPTH
};

class Synth {
public:
  Synth(void)
    : modal_piano_(nullptr)
    , note_(-1)
    , velocity_(0)
    , gate_(false)
    , active_(false)
    , decay_(0.5f)
    , resonance_(0.0f)
    , brightness_(0.6f)
    , velocity_sens_(0.8f)
    , volume_(0.7f)
    , lfo_rate_(0.3f)
    , lfo_depth_(0.2f)
  {}

  ~Synth(void) {
    if (modal_piano_) modal_piano_destroy(modal_piano_);
  }

  inline int8_t Init(const unit_runtime_desc_t * desc) {
    if (desc->samplerate != 48000)
      return k_unit_err_samplerate;

    if (desc->output_channels != 2)
      return k_unit_err_geometry;

    // Create modal piano
    modal_piano_ = modal_piano_create();
    if (!modal_piano_) {
      return k_unit_err_memory;
    }

    // Initialize sample data
    sample_data_.attack_data = m1piano_onset;
    sample_data_.attack_length = m1piano_onset_length;
    sample_data_.loop_data = m1piano_tail;
    sample_data_.loop_length = m1piano_tail_length;
    sample_data_.sample_rate = 22050;
    sample_data_.root_note = 48;  // C3 (131.6 Hz verified with aubio)

    modal_piano_load_sample(modal_piano_, &sample_data_);

    // Set default parameters
    modal_piano_set_decay(modal_piano_, 2.0f);
    modal_piano_set_resonance(modal_piano_, 0.0f);  // Start with resonators off
    modal_piano_set_filter_envelope(modal_piano_, 0.01f, 0.3f, 0.6f);  // 10ms attack, 300ms decay, 60% sustain
    modal_piano_set_velocity_sensitivity(modal_piano_, 0.8f);

    return k_unit_err_none;
  }

  inline void Teardown() {
    if (modal_piano_) {
      modal_piano_destroy(modal_piano_);
      modal_piano_ = nullptr;
    }
  }

  inline void Reset() {
    gate_ = false;
    active_ = false;
    note_ = -1;

    if (modal_piano_) modal_piano_reset(modal_piano_);
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
        if (modal_piano_) {
          // Map decay parameter to 0.5s - 8s range
          float decay_time = 0.5f + decay_ * 7.5f;
          modal_piano_set_decay(modal_piano_, decay_time);
        }
        break;

      case PARAM_RESONANCE:
        resonance_ = value / 1023.0f;
        if (modal_piano_) {
          modal_piano_set_resonance(modal_piano_, resonance_);
        }
        break;

      case PARAM_BRIGHTNESS:
        brightness_ = value / 1023.0f;
        if (modal_piano_) {
          // Brightness controls filter envelope sustain level
          // Keep attack/decay times constant, update sustain
          modal_piano_set_filter_envelope(modal_piano_, 0.01f, 0.3f, brightness_);
        }
        break;

      case PARAM_VELOCITY_SENS:
        velocity_sens_ = value / 1023.0f;
        if (modal_piano_) {
          modal_piano_set_velocity_sensitivity(modal_piano_, velocity_sens_);
        }
        break;

      case PARAM_VOLUME:
        volume_ = value / 1023.0f;
        break;

      case PARAM_LFO_RATE:
        lfo_rate_ = value / 1023.0f;
        if (modal_piano_) {
          // Map rate: 0-1 -> 0.5Hz-8Hz
          float lfo_freq = 0.5f + lfo_rate_ * 7.5f;
          modal_piano_set_lfo(modal_piano_, lfo_freq, lfo_depth_);
        }
        break;

      case PARAM_LFO_DEPTH:
        lfo_depth_ = value / 1023.0f;
        if (modal_piano_) {
          // Map rate: 0-1 -> 0.5Hz-8Hz
          float lfo_freq = 0.5f + lfo_rate_ * 7.5f;
          modal_piano_set_lfo(modal_piano_, lfo_freq, lfo_depth_);
        }
        break;

      default:
        break;
    }
  }

  inline int32_t getParameterValue(uint8_t index) const {
    switch (index) {
      case PARAM_DECAY: return (int32_t)(decay_ * 1023.0f);
      case PARAM_RESONANCE: return (int32_t)(resonance_ * 1023.0f);
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

    if (modal_piano_) {
      modal_piano_trigger(modal_piano_, note, velocity);
    }
  }

  inline void NoteOff(uint8_t note) {
    if (note == note_) {
      gate_ = false;

      if (modal_piano_) {
        modal_piano_release(modal_piano_);
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

    if (modal_piano_) {
      modal_piano_release(modal_piano_);
    }
  }

  inline void AllNoteOff() {
    gate_ = false;
    active_ = false;

    if (modal_piano_) {
      modal_piano_release(modal_piano_);
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
    if (!active_ || !modal_piano_) {
      return 0.0f;
    }

    const int sampleRate = 48000;

    // Process modal piano (handles sample playback, resonators, filter envelope, LFO)
    float sample = modal_piano_process(modal_piano_, sampleRate);

    // Apply master volume
    sample *= volume_;

    // Soft clipping to prevent harsh digital clipping and squealing
    // Tanh-style soft saturation
    if (sample > 1.0f) {
      sample = 1.0f - expf(-(sample - 1.0f));
    } else if (sample < -1.0f) {
      sample = -1.0f + expf(sample + 1.0f);
    }

    // Check if modal piano is still active
    if (!modal_piano_is_active(modal_piano_)) {
      active_ = false;
    }

    return sample;
  }

  ModalPiano* modal_piano_;
  SampleData sample_data_;

  int note_;
  int velocity_;
  bool gate_;
  bool active_;

  // Parameters
  float decay_;
  float resonance_;
  float brightness_;
  float velocity_sens_;
  float volume_;
  float lfo_rate_;
  float lfo_depth_;
};
