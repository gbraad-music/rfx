#pragma once
/**
 * RG303 Synth - TB-303 inspired bass synthesizer for Drumlogue
 */

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cmath>
#include <arm_neon.h>

#include "unit.h"

// Import synth components
// Paths are relative to include directories set in Makefile
extern "C" {
#include "synth_oscillator.h"
#include "synth_filter.h"
#include "synth_envelope.h"
}

// Parameter indices
enum {
  PARAM_WAVEFORM = 0,
  PARAM_CUTOFF,
  PARAM_RESONANCE,
  PARAM_ENV_MOD,
  PARAM_DECAY,
  PARAM_ACCENT,
  PARAM_SLIDE,
  PARAM_VOLUME
};

class Synth {
public:
  Synth(void)
    : osc_(nullptr)
    , filter_(nullptr)
    , amp_env_(nullptr)
    , filter_env_(nullptr)
    , note_(-1)
    , velocity_(0)
    , gate_(false)
    , active_(false)
    , current_freq_(440.0f)
    , target_freq_(440.0f)
    , sliding_(false)
    , waveform_(0.0f)
    , cutoff_(0.5f)
    , resonance_(0.5f)
    , env_mod_(0.5f)
    , decay_(0.3f)
    , accent_(0.0f)
    , slide_time_(0.1f)
    , volume_(0.7f)
  {}

  ~Synth(void) {
    if (osc_) synth_oscillator_destroy(osc_);
    if (filter_) synth_filter_destroy(filter_);
    if (amp_env_) synth_envelope_destroy(amp_env_);
    if (filter_env_) synth_envelope_destroy(filter_env_);
  }

  inline int8_t Init(const unit_runtime_desc_t * desc) {
    if (desc->samplerate != 48000)
      return k_unit_err_samplerate;

    if (desc->output_channels != 2)
      return k_unit_err_geometry;

    // Create synth components
    osc_ = synth_oscillator_create();
    filter_ = synth_filter_create();
    amp_env_ = synth_envelope_create();
    filter_env_ = synth_envelope_create();

    if (!osc_ || !filter_ || !amp_env_ || !filter_env_) {
      return k_unit_err_memory;
    }

    // Setup TB-303 style envelopes
    synth_envelope_set_attack(amp_env_, 0.003f);
    synth_envelope_set_decay(amp_env_, 0.2f);
    synth_envelope_set_sustain(amp_env_, 0.0f);  // No sustain
    synth_envelope_set_release(amp_env_, 0.01f);

    synth_envelope_set_attack(filter_env_, 0.003f);
    synth_envelope_set_decay(filter_env_, 0.01f + decay_ * 2.0f);
    synth_envelope_set_sustain(filter_env_, 0.0f);
    synth_envelope_set_release(filter_env_, 0.01f);

    // Setup filter
    synth_filter_set_type(filter_, SYNTH_FILTER_LPF);
    synth_filter_set_cutoff(filter_, cutoff_);
    synth_filter_set_resonance(filter_, resonance_);

    // Setup oscillator
    synth_oscillator_set_waveform(osc_, SYNTH_OSC_SAW);

    return k_unit_err_none;
  }

  inline void Teardown() {
    if (osc_) {
      synth_oscillator_destroy(osc_);
      osc_ = nullptr;
    }
    if (filter_) {
      synth_filter_destroy(filter_);
      filter_ = nullptr;
    }
    if (amp_env_) {
      synth_envelope_destroy(amp_env_);
      amp_env_ = nullptr;
    }
    if (filter_env_) {
      synth_envelope_destroy(filter_env_);
      filter_env_ = nullptr;
    }
  }

  inline void Reset() {
    gate_ = false;
    active_ = false;
    note_ = -1;
    sliding_ = false;
    current_freq_ = 440.0f;

    if (amp_env_) synth_envelope_reset(amp_env_);
    if (filter_env_) synth_envelope_reset(filter_env_);
  }

  inline void Resume() {}
  inline void Suspend() {}

  fast_inline void Render(float * out, size_t frames) {
    float * __restrict out_p = out;
    const float * out_e = out_p + (frames << 1);

    for (; out_p != out_e; out_p += 2) {
      float sample = renderSample();
      vst1_f32(out_p, vdup_n_f32(sample));
    }
  }

  inline void setParameter(uint8_t index, int32_t value) {
    switch (index) {
      case PARAM_WAVEFORM:
        waveform_ = (float)value;
        if (osc_) {
          synth_oscillator_set_waveform(osc_,
            (value == 0) ? SYNTH_OSC_SAW : SYNTH_OSC_SQUARE);
        }
        break;

      case PARAM_CUTOFF:
        cutoff_ = value / 100.0f;
        if (filter_) synth_filter_set_cutoff(filter_, cutoff_);
        break;

      case PARAM_RESONANCE:
        resonance_ = value / 100.0f;
        if (filter_) synth_filter_set_resonance(filter_, resonance_);
        break;

      case PARAM_ENV_MOD:
        env_mod_ = value / 100.0f;
        break;

      case PARAM_DECAY:
        decay_ = value / 100.0f;
        if (filter_env_) {
          synth_envelope_set_decay(filter_env_, 0.01f + decay_ * 2.0f);
        }
        break;

      case PARAM_ACCENT:
        accent_ = value / 100.0f;
        break;

      case PARAM_SLIDE:
        slide_time_ = (value / 100.0f) * 0.5f + 0.01f;
        break;

      case PARAM_VOLUME:
        volume_ = value / 100.0f;
        break;

      default:
        break;
    }
  }

  inline int32_t getParameterValue(uint8_t index) const {
    switch (index) {
      case PARAM_WAVEFORM: return (int32_t)waveform_;
      case PARAM_CUTOFF: return (int32_t)(cutoff_ * 100.0f);
      case PARAM_RESONANCE: return (int32_t)(resonance_ * 100.0f);
      case PARAM_ENV_MOD: return (int32_t)(env_mod_ * 100.0f);
      case PARAM_DECAY: return (int32_t)(decay_ * 100.0f);
      case PARAM_ACCENT: return (int32_t)(accent_ * 100.0f);
      case PARAM_SLIDE: return (int32_t)((slide_time_ - 0.01f) / 0.5f * 100.0f);
      case PARAM_VOLUME: return (int32_t)(volume_ * 100.0f);
      default: return 0;
    }
  }

  inline const char * getParameterStrValue(uint8_t index, int32_t value) const {
    static const char* waveforms[] = {"SAW", "SQUARE"};

    switch (index) {
      case PARAM_WAVEFORM:
        return (value == 0) ? waveforms[0] : waveforms[1];
      default:
        return nullptr;
    }
  }

  inline const uint8_t * getParameterBmpValue(uint8_t index, int32_t value) const {
    (void)value;
    (void)index;
    return nullptr;
  }

  inline void NoteOn(uint8_t note, uint8_t velocity) {
    note_ = note;
    velocity_ = velocity;

    // Calculate target frequency
    target_freq_ = 440.0f * powf(2.0f, (note - 69) / 12.0f);

    // If gate is already on and slide is enabled, slide to new note
    if (gate_ && slide_time_ > 0.01f) {
      sliding_ = true;
    } else {
      // Jump to frequency immediately
      current_freq_ = target_freq_;
      sliding_ = false;

      // Trigger envelopes
      if (amp_env_) synth_envelope_trigger(amp_env_);
      if (filter_env_) synth_envelope_trigger(filter_env_);
    }

    gate_ = true;
    active_ = true;

    // Update oscillator frequency
    if (osc_) synth_oscillator_set_frequency(osc_, current_freq_);
  }

  inline void NoteOff(uint8_t note) {
    if (note == note_) {
      gate_ = false;
      sliding_ = false;

      if (amp_env_) synth_envelope_release(amp_env_);
      if (filter_env_) synth_envelope_release(filter_env_);
    }
  }

  inline void GateOn(uint8_t velocity) {
    velocity_ = velocity;
    gate_ = true;
    active_ = true;

    if (amp_env_) synth_envelope_trigger(amp_env_);
    if (filter_env_) synth_envelope_trigger(filter_env_);
  }

  inline void GateOff() {
    gate_ = false;
    sliding_ = false;

    if (amp_env_) synth_envelope_release(amp_env_);
    if (filter_env_) synth_envelope_release(filter_env_);
  }

  inline void AllNoteOff() {
    gate_ = false;
    active_ = false;
    sliding_ = false;

    if (amp_env_) synth_envelope_release(amp_env_);
    if (filter_env_) synth_envelope_release(filter_env_);
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
    if (!active_ || !osc_ || !filter_ || !amp_env_ || !filter_env_) {
      return 0.0f;
    }

    const int sampleRate = 48000;

    // Handle portamento/slide
    if (sliding_) {
      float slide_speed = 1.0f - expf(-1.0f / (slide_time_ * sampleRate));
      current_freq_ += (target_freq_ - current_freq_) * slide_speed;

      if (fabsf(current_freq_ - target_freq_) < 0.1f) {
        current_freq_ = target_freq_;
        sliding_ = false;

        // Trigger envelopes when slide completes
        if (amp_env_) synth_envelope_trigger(amp_env_);
        if (filter_env_) synth_envelope_trigger(filter_env_);
      }

      synth_oscillator_set_frequency(osc_, current_freq_);
    }

    // Generate oscillator sample
    float sample = synth_oscillator_process(osc_, sampleRate);

    // Process envelopes
    float amp_env_value = synth_envelope_process(amp_env_, sampleRate);
    float filter_env_value = synth_envelope_process(filter_env_, sampleRate);

    // Apply accent (boosts envelope and filter)
    float accent_amt = (velocity_ / 127.0f) * accent_;
    filter_env_value += accent_amt;
    amp_env_value = fminf(amp_env_value + accent_amt * 0.3f, 1.0f);

    // Modulate filter cutoff with envelope
    float modulated_cutoff = cutoff_ + filter_env_value * env_mod_;
    modulated_cutoff = fmaxf(0.0f, fminf(1.0f, modulated_cutoff));

    synth_filter_set_cutoff(filter_, modulated_cutoff);

    // Apply filter
    sample = synth_filter_process(filter_, sample, sampleRate);

    // Apply amplitude envelope
    sample *= amp_env_value;

    // Apply master volume
    sample *= volume_;

    // Check if voice should still be active
    if (!gate_ && amp_env_value < 0.001f) {
      active_ = false;
    }

    return sample;
  }

  SynthOscillator* osc_;
  SynthFilter* filter_;
  SynthEnvelope* amp_env_;
  SynthEnvelope* filter_env_;

  int note_;
  int velocity_;
  bool gate_;
  bool active_;

  float current_freq_;
  float target_freq_;
  bool sliding_;

  // Parameters
  float waveform_;
  float cutoff_;
  float resonance_;
  float env_mod_;
  float decay_;
  float accent_;
  float slide_time_;
  float volume_;
};
