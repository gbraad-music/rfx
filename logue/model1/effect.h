#pragma once
/*
 * Regroove RM1 - NTS-3 kaoss pad
 * Model1 Channel Strip combining all Model1 effects
 * RE-USES existing code from /rfx/effects/fx_model1_*.c
 */

#include "processor.h"
#include "unit_genericfx.h"

extern "C" {
#include "fx_model1_trim.h"
#include "fx_model1_hpf.h"
#include "fx_model1_lpf.h"
#include "fx_model1_sculpt.h"
}

class Effect : public Processor
{
public:
  uint32_t getBufferSize() const override final { return 0; }

  enum
  {
    PARAM_TRIM = 0U,
    PARAM_CONTOUR_HI,
    PARAM_SCULPT_FREQ,
    PARAM_SCULPT_GAIN,
    PARAM_CONTOUR_LO,
    NUM_PARAMS
  };

  inline void setParameter(uint8_t index, int32_t value) override final
  {
    float valf = param_10bit_to_f32(value);

    switch (index)
    {
    case PARAM_TRIM:
      if (trim_) fx_model1_trim_set_drive(trim_, valf);
      break;

    case PARAM_CONTOUR_HI:
      if (hpf_) fx_model1_hpf_set_cutoff(hpf_, valf);
      break;

    case PARAM_SCULPT_FREQ:
      if (sculpt_) fx_model1_sculpt_set_frequency(sculpt_, valf);
      break;

    case PARAM_SCULPT_GAIN:
      if (sculpt_) fx_model1_sculpt_set_gain(sculpt_, valf);
      break;

    case PARAM_CONTOUR_LO:
      if (lpf_) fx_model1_lpf_set_cutoff(lpf_, valf);
      break;

    default:
      break;
    }
  }

  inline const char *getParameterStrValue(uint8_t index, int32_t value) const override final
  {
    (void)index;
    (void)value;
    return nullptr;
  }

  void init(float *allocated_buffer) override final
  {
    (void)allocated_buffer;

    // Create all Model1 effects
    trim_ = fx_model1_trim_create();
    if (trim_)
    {
      fx_model1_trim_set_enabled(trim_, 1);
      fx_model1_trim_set_drive(trim_, 0.5f);
    }

    hpf_ = fx_model1_hpf_create();
    if (hpf_)
    {
      fx_model1_hpf_set_enabled(hpf_, 1);
      fx_model1_hpf_set_cutoff(hpf_, 0.0f);
    }

    lpf_ = fx_model1_lpf_create();
    if (lpf_)
    {
      fx_model1_lpf_set_enabled(lpf_, 1);
      fx_model1_lpf_set_cutoff(lpf_, 1.0f);
    }

    sculpt_ = fx_model1_sculpt_create();
    if (sculpt_)
    {
      fx_model1_sculpt_set_enabled(sculpt_, 1);
      fx_model1_sculpt_set_frequency(sculpt_, 0.5f);
      fx_model1_sculpt_set_gain(sculpt_, 0.5f);
    }
  }

  void teardown() override final
  {
    if (trim_)
    {
      fx_model1_trim_destroy(trim_);
      trim_ = nullptr;
    }
    if (hpf_)
    {
      fx_model1_hpf_destroy(hpf_);
      hpf_ = nullptr;
    }
    if (lpf_)
    {
      fx_model1_lpf_destroy(lpf_);
      lpf_ = nullptr;
    }
    if (sculpt_)
    {
      fx_model1_sculpt_destroy(sculpt_);
      sculpt_ = nullptr;
    }
  }

  void process(const float *__restrict in, float *__restrict out, uint32_t frames) override final
  {
    const int sample_rate = 48000;

    for (uint32_t i = 0; i < frames; i++)
    {
      float left = *in++;
      float right = *in++;

      // Process chain: Trim -> HPF -> LPF -> Sculpt
      if (trim_) fx_model1_trim_process_frame(trim_, &left, &right, sample_rate);
      if (hpf_) fx_model1_hpf_process_frame(hpf_, &left, &right, sample_rate);
      if (lpf_) fx_model1_lpf_process_frame(lpf_, &left, &right, sample_rate);
      if (sculpt_) fx_model1_sculpt_process_frame(sculpt_, &left, &right, sample_rate);

      *out++ = left;
      *out++ = right;
    }
  }

  inline void touchEvent(uint8_t id, uint8_t phase, uint32_t x, uint32_t y) override final
  {
    (void)id;
    (void)phase;
    (void)x;
    (void)y;
  }

private:
  FXModel1Trim* trim_ = nullptr;
  FXModel1HPF* hpf_ = nullptr;
  FXModel1LPF* lpf_ = nullptr;
  FXModel1Sculpt* sculpt_ = nullptr;
};
