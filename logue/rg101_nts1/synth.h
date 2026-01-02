#pragma once
/**
 * RG101 Synth - SH-101 inspired monophonic synthesizer for Drumlogue
 */

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cmath>
// ARM NEON not available on NTS-1 MKII

#include "unit.h"

// Import synth components
// Paths are relative to include directories set in Makefile
extern "C" {
#include "synth_oscillator.h"
#include "synth_filter_ladder.h"
#include "synth_envelope.h"
#include "synth_lfo.h"
#include "synth_noise.h"
}

// Parameter indices
enum {
  PARAM_SAW_LEVEL = 0,
  PARAM_SQUARE_LEVEL,
  PARAM_SUB_LEVEL,
  PARAM_NOISE_LEVEL,
  PARAM_PULSE_WIDTH,
  PARAM_PWM_DEPTH,
  PARAM_CUTOFF,
  PARAM_RESONANCE,
  PARAM_ENV_MOD,
  PARAM_KEYBOARD_TRACKING,
  PARAM_FILTER_ATTACK,
  PARAM_FILTER_DECAY,
  PARAM_FILTER_SUSTAIN,
  PARAM_FILTER_RELEASE,
  PARAM_AMP_ATTACK,
  PARAM_AMP_DECAY,
  PARAM_AMP_SUSTAIN,
  PARAM_AMP_RELEASE,
  PARAM_LFO_RATE,
  PARAM_LFO_FILTER_DEPTH,
  PARAM_VELOCITY_SENSITIVITY,
  PARAM_PORTAMENTO,
  PARAM_VOLUME
};

class Synth {
public:
  Synth(void)
    : osc_(nullptr)
    , sub_osc_(nullptr)
    , filter_(nullptr)
    , amp_env_(nullptr)
    , filter_env_(nullptr)
    , lfo_(nullptr)
    , noise_(nullptr)
    , note_(-1)
    , velocity_(0)
    , gate_(false)
    , active_(false)
    , current_freq_(440.0f)
    , target_freq_(440.0f)
    , sliding_(false)
    , saw_level_(0.8f)
    , square_level_(0.0f)
    , sub_level_(0.3f)
    , noise_level_(0.0f)
    , pulse_width_(0.5f)
    , pwm_depth_(0.0f)
    , cutoff_(0.5f)
    , resonance_(0.3f)
    , env_mod_(0.5f)
    , keyboard_tracking_(0.5f)
    , filter_attack_(0.003f)
    , filter_decay_(0.3f)
    , filter_sustain_(0.0f)
    , filter_release_(0.1f)
    , amp_attack_(0.003f)
    , amp_decay_(0.3f)
    , amp_sustain_(0.7f)
    , amp_release_(0.1f)
    , lfo_rate_(5.0f)
    , lfo_filter_depth_(0.0f)
    , velocity_sensitivity_(0.5f)
    , portamento_(0.0f)
    , volume_(0.7f)
  {}

  ~Synth(void) {
    if (osc_) synth_oscillator_destroy(osc_);
    if (sub_osc_) synth_oscillator_destroy(sub_osc_);
    if (filter_) synth_filter_ladder_destroy(filter_);
    if (amp_env_) synth_envelope_destroy(amp_env_);
    if (filter_env_) synth_envelope_destroy(filter_env_);
    if (lfo_) synth_lfo_destroy(lfo_);
    if (noise_) synth_noise_destroy(noise_);
  }

  inline int8_t Init(const unit_runtime_desc_t * desc) {
    if (desc->samplerate != 48000)
      return k_unit_err_samplerate;

    if (desc->output_channels != 2)
      return k_unit_err_geometry;

    // Create synth components
    osc_ = synth_oscillator_create();
    sub_osc_ = synth_oscillator_create();
    filter_ = synth_filter_ladder_create();
    amp_env_ = synth_envelope_create();
    filter_env_ = synth_envelope_create();
    lfo_ = synth_lfo_create();
    noise_ = synth_noise_create();

    if (!osc_ || !sub_osc_ || !filter_ || !amp_env_ || !filter_env_ || !lfo_ || !noise_) {
      return k_unit_err_memory;
    }

    // Setup oscillators
    synth_oscillator_set_waveform(osc_, SYNTH_OSC_SAW);
    synth_oscillator_set_waveform(sub_osc_, SYNTH_OSC_SQUARE);

    // Setup envelopes
    updateEnvelopes();

    // Setup LFO (sine wave default)
    synth_lfo_set_waveform(lfo_, SYNTH_LFO_SINE);
    synth_lfo_set_frequency(lfo_, lfo_rate_);

    // Setup filter
    synth_filter_ladder_set_cutoff(filter_, cutoff_);
    synth_filter_ladder_set_resonance(filter_, resonance_);

    return k_unit_err_none;
  }

  inline void Teardown() {
    if (osc_) {
      synth_oscillator_destroy(osc_);
      osc_ = nullptr;
    }
    if (sub_osc_) {
      synth_oscillator_destroy(sub_osc_);
      sub_osc_ = nullptr;
    }
    if (filter_) {
      synth_filter_ladder_destroy(filter_);
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
    if (lfo_) {
      synth_lfo_destroy(lfo_);
      lfo_ = nullptr;
    }
    if (noise_) {
      synth_noise_destroy(noise_);
      noise_ = nullptr;
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
    if (lfo_) synth_lfo_reset(lfo_);
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
      case PARAM_SAW_LEVEL:
        saw_level_ = value / 100.0f;
        break;

      case PARAM_SQUARE_LEVEL:
        square_level_ = value / 100.0f;
        break;

      case PARAM_SUB_LEVEL:
        sub_level_ = value / 100.0f;
        break;

      case PARAM_NOISE_LEVEL:
        noise_level_ = value / 100.0f;
        break;

      case PARAM_PULSE_WIDTH:
        pulse_width_ = value / 100.0f;
        if (osc_) synth_oscillator_set_pulse_width(osc_, pulse_width_);
        break;

      case PARAM_PWM_DEPTH:
        pwm_depth_ = value / 100.0f;
        break;

      case PARAM_CUTOFF:
        cutoff_ = value / 100.0f;
        if (filter_) synth_filter_ladder_set_cutoff(filter_, cutoff_);
        break;

      case PARAM_RESONANCE:
        resonance_ = value / 100.0f;
        if (filter_) synth_filter_ladder_set_resonance(filter_, resonance_);
        break;

      case PARAM_ENV_MOD:
        env_mod_ = value / 100.0f;
        break;

      case PARAM_KEYBOARD_TRACKING:
        keyboard_tracking_ = value / 100.0f;
        break;

      case PARAM_FILTER_ATTACK:
        filter_attack_ = (value / 100.0f) * 2.0f;
        updateEnvelopes();
        break;

      case PARAM_FILTER_DECAY:
        filter_decay_ = (value / 100.0f) * 3.0f;
        updateEnvelopes();
        break;

      case PARAM_FILTER_SUSTAIN:
        filter_sustain_ = value / 100.0f;
        updateEnvelopes();
        break;

      case PARAM_FILTER_RELEASE:
        filter_release_ = (value / 100.0f) * 2.0f;
        updateEnvelopes();
        break;

      case PARAM_AMP_ATTACK:
        amp_attack_ = (value / 100.0f) * 2.0f;
        updateEnvelopes();
        break;

      case PARAM_AMP_DECAY:
        amp_decay_ = (value / 100.0f) * 3.0f;
        updateEnvelopes();
        break;

      case PARAM_AMP_SUSTAIN:
        amp_sustain_ = value / 100.0f;
        updateEnvelopes();
        break;

      case PARAM_AMP_RELEASE:
        amp_release_ = (value / 100.0f) * 2.0f;
        updateEnvelopes();
        break;

      case PARAM_LFO_RATE:
        lfo_rate_ = (value / 100.0f) * 20.0f; // 0-20 Hz
        if (lfo_) synth_lfo_set_frequency(lfo_, lfo_rate_);
        break;

      case PARAM_LFO_FILTER_DEPTH:
        lfo_filter_depth_ = value / 100.0f;
        break;

      case PARAM_VELOCITY_SENSITIVITY:
        velocity_sensitivity_ = value / 100.0f;
        break;

      case PARAM_PORTAMENTO:
        portamento_ = (value / 100.0f) * 0.5f;
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
      case PARAM_SAW_LEVEL: return (int32_t)(saw_level_ * 100.0f);
      case PARAM_SQUARE_LEVEL: return (int32_t)(square_level_ * 100.0f);
      case PARAM_SUB_LEVEL: return (int32_t)(sub_level_ * 100.0f);
      case PARAM_NOISE_LEVEL: return (int32_t)(noise_level_ * 100.0f);
      case PARAM_PULSE_WIDTH: return (int32_t)(pulse_width_ * 100.0f);
      case PARAM_PWM_DEPTH: return (int32_t)(pwm_depth_ * 100.0f);
      case PARAM_CUTOFF: return (int32_t)(cutoff_ * 100.0f);
      case PARAM_RESONANCE: return (int32_t)(resonance_ * 100.0f);
      case PARAM_ENV_MOD: return (int32_t)(env_mod_ * 100.0f);
      case PARAM_KEYBOARD_TRACKING: return (int32_t)(keyboard_tracking_ * 100.0f);
      case PARAM_FILTER_ATTACK: return (int32_t)((filter_attack_ / 2.0f) * 100.0f);
      case PARAM_FILTER_DECAY: return (int32_t)((filter_decay_ / 3.0f) * 100.0f);
      case PARAM_FILTER_SUSTAIN: return (int32_t)(filter_sustain_ * 100.0f);
      case PARAM_FILTER_RELEASE: return (int32_t)((filter_release_ / 2.0f) * 100.0f);
      case PARAM_AMP_ATTACK: return (int32_t)((amp_attack_ / 2.0f) * 100.0f);
      case PARAM_AMP_DECAY: return (int32_t)((amp_decay_ / 3.0f) * 100.0f);
      case PARAM_AMP_SUSTAIN: return (int32_t)(amp_sustain_ * 100.0f);
      case PARAM_AMP_RELEASE: return (int32_t)((amp_release_ / 2.0f) * 100.0f);
      case PARAM_LFO_RATE: return (int32_t)((lfo_rate_ / 20.0f) * 100.0f);
      case PARAM_LFO_FILTER_DEPTH: return (int32_t)(lfo_filter_depth_ * 100.0f);
      case PARAM_VELOCITY_SENSITIVITY: return (int32_t)(velocity_sensitivity_ * 100.0f);
      case PARAM_PORTAMENTO: return (int32_t)((portamento_ / 0.5f) * 100.0f);
      case PARAM_VOLUME: return (int32_t)(volume_ * 100.0f);
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
    note_ = note;
    velocity_ = velocity;

    // Calculate target frequency
    target_freq_ = 440.0f * powf(2.0f, (note - 69) / 12.0f);

    // If gate is already on and portamento is enabled, slide to new note
    if (gate_ && portamento_ > 0.001f) {
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
    if (sub_osc_) synth_oscillator_set_frequency(sub_osc_, current_freq_ * 0.5f); // One octave down
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
  void updateEnvelopes() {
    if (amp_env_) {
      synth_envelope_set_attack(amp_env_, amp_attack_);
      synth_envelope_set_decay(amp_env_, amp_decay_);
      synth_envelope_set_sustain(amp_env_, amp_sustain_);
      synth_envelope_set_release(amp_env_, amp_release_);
    }

    if (filter_env_) {
      synth_envelope_set_attack(filter_env_, filter_attack_);
      synth_envelope_set_decay(filter_env_, filter_decay_);
      synth_envelope_set_sustain(filter_env_, filter_sustain_);
      synth_envelope_set_release(filter_env_, filter_release_);
    }
  }

  float renderSample() {
    if (!active_ || !osc_ || !sub_osc_ || !filter_ || !amp_env_ || !filter_env_ || !lfo_ || !noise_) {
      return 0.0f;
    }

    const int sampleRate = 48000;

    // Handle portamento/slide
    if (sliding_ && portamento_ > 0.001f) {
      float slide_speed = 1.0f - expf(-1.0f / (portamento_ * sampleRate));
      current_freq_ += (target_freq_ - current_freq_) * slide_speed;

      if (fabsf(current_freq_ - target_freq_) < 0.1f) {
        current_freq_ = target_freq_;
        sliding_ = false;

        // Trigger envelopes when slide completes
        if (amp_env_) synth_envelope_trigger(amp_env_);
        if (filter_env_) synth_envelope_trigger(filter_env_);
      }

      synth_oscillator_set_frequency(osc_, current_freq_);
      synth_oscillator_set_frequency(sub_osc_, current_freq_ * 0.5f);
    }

    // Process LFO
    float lfo_value = synth_lfo_process(lfo_, sampleRate);

    // Apply PWM if enabled
    if (pwm_depth_ > 0.001f) {
      float modulated_pw = pulse_width_ + lfo_value * pwm_depth_ * 0.4f;
      modulated_pw = fmaxf(0.05f, fminf(0.95f, modulated_pw));
      synth_oscillator_set_pulse_width(osc_, modulated_pw);
    }

    // Generate oscillator samples
    float saw_sample = 0.0f;
    float square_sample = 0.0f;

    if (saw_level_ > 0.001f || square_level_ > 0.001f) {
      synth_oscillator_set_waveform(osc_, SYNTH_OSC_SAW);
      saw_sample = synth_oscillator_process(osc_, sampleRate) * saw_level_;

      synth_oscillator_set_waveform(osc_, SYNTH_OSC_SQUARE);
      square_sample = synth_oscillator_process(osc_, sampleRate) * square_level_;
    }

    float sub_sample = synth_oscillator_process(sub_osc_, sampleRate) * sub_level_;
    float noise_sample = synth_noise_process(noise_) * noise_level_;

    // Mix oscillators
    float sample = saw_sample + square_sample + sub_sample + noise_sample;

    // Process envelopes
    float amp_env_value = synth_envelope_process(amp_env_, sampleRate);
    float filter_env_value = synth_envelope_process(filter_env_, sampleRate);

    // Apply velocity sensitivity
    float velocity_amt = 1.0f - velocity_sensitivity_ + (velocity_ / 127.0f) * velocity_sensitivity_;
    filter_env_value *= velocity_amt;

    // Modulate filter cutoff with envelope and LFO
    float modulated_cutoff = cutoff_;
    modulated_cutoff += filter_env_value * env_mod_;
    modulated_cutoff += lfo_value * lfo_filter_depth_ * 0.3f;

    // Add keyboard tracking
    if (note_ >= 0) {
      float key_track = ((note_ - 60) / 60.0f) * keyboard_tracking_;
      modulated_cutoff += key_track * 0.5f;
    }

    modulated_cutoff = fmaxf(0.0f, fminf(1.0f, modulated_cutoff));

    synth_filter_ladder_set_cutoff(filter_, modulated_cutoff);

    // Apply filter
    sample = synth_filter_ladder_process(filter_, sample, sampleRate);

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
  SynthOscillator* sub_osc_;
  SynthFilterLadder* filter_;
  SynthEnvelope* amp_env_;
  SynthEnvelope* filter_env_;
  SynthLFO* lfo_;
  SynthNoise* noise_;

  int note_;
  int velocity_;
  bool gate_;
  bool active_;

  float current_freq_;
  float target_freq_;
  bool sliding_;

  // Parameters
  float saw_level_;
  float square_level_;
  float sub_level_;
  float noise_level_;
  float pulse_width_;
  float pwm_depth_;
  float cutoff_;
  float resonance_;
  float env_mod_;
  float keyboard_tracking_;
  float filter_attack_;
  float filter_decay_;
  float filter_sustain_;
  float filter_release_;
  float amp_attack_;
  float amp_decay_;
  float amp_sustain_;
  float amp_release_;
  float lfo_rate_;
  float lfo_filter_depth_;
  float velocity_sensitivity_;
  float portamento_;
  float volume_;
};
