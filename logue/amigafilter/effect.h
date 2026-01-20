#pragma once
/*
 * Regroove Amiga Filter - NTS-3 kaoss pad
 * RE-USES existing code from /rfx/effects/fx_amiga_filter.c
 * Background effect - runs continuously, XY controls parameters
 */

#include "processor.h"
#include "unit_genericfx.h"

extern "C" {
#include "fx_amiga_filter.h"
}

class Effect : public Processor
{
public:
  uint32_t getBufferSize() const override final { return 0; }

  enum
  {
    PARAM_TYPE = 0U,
    PARAM_DEPTH,
    NUM_PARAMS
  };

  inline void setParameter(uint8_t index, int32_t value) override final
  {
    if (!fx_) return;

    switch (index)
    {
    case PARAM_TYPE:
      // value is 0-3, map to filter types (skipping OFF=0)
      // 0->A500, 1->A500+LED, 2->A1200, 3->A1200+LED
      fx_amiga_filter_set_type(fx_, static_cast<AmigaFilterType>(value + 1));
      break;

    case PARAM_DEPTH:
      // value is 0-100, convert to 0.0-1.0
      fx_amiga_filter_set_mix(fx_, value / 100.0f);
      break;

    default:
      break;
    }
  }

  inline const char *getParameterStrValue(uint8_t index, int32_t value) const override final
  {
    static const char *type_names[] = {
      "A500 4.9k",
      "A500+LED 3.3k",
      "A1200 32k",
      "A1200+LED 3.3k"
    };

    if (index == PARAM_TYPE && value >= 0 && value <= 3) {
      return type_names[value];
    }

    return nullptr;
  }

  void init(float *allocated_buffer) override final
  {
    (void)allocated_buffer;

    // Use existing create function
    fx_ = fx_amiga_filter_create();
    if (fx_)
    {
      fx_amiga_filter_set_enabled(fx_, 1);
      fx_amiga_filter_set_type(fx_, AMIGA_FILTER_A500_LED_OFF);
      fx_amiga_filter_set_mix(fx_, 1.0f);
    }
  }

  void teardown() override final
  {
    // Use existing destroy function
    if (fx_)
    {
      fx_amiga_filter_destroy(fx_);
      fx_ = nullptr;
    }
  }

  void process(const float *__restrict in, float *__restrict out, uint32_t frames) override final
  {
    if (!fx_) return;

    const int sample_rate = 48000;

    for (uint32_t i = 0; i < frames; i++)
    {
      float left = *in++;
      float right = *in++;

      // Use existing process function
      fx_amiga_filter_process_frame(fx_, &left, &right, sample_rate);

      *out++ = left;
      *out++ = right;
    }
  }

  inline void touchEvent(uint8_t id, uint8_t phase, uint32_t x, uint32_t y) override final
  {
    // Explicitly ignore all touch events
    (void)id;
    (void)phase;
    (void)x;
    (void)y;
  }

private:
  FXAmigaFilter* fx_ = nullptr;
};
