#pragma once
/*
 * Regroove Phaser - NTS-3 kaoss pad
 * RE-USES existing code from /rfx/effects/fx_phaser.c
 * Always-on version (ignores XY touch parameters)
 */

#include "processor.h"
#include "unit_genericfx.h"

extern "C" {
#include "fx_phaser.h"
}

class Effect : public Processor
{
public:
  uint32_t getBufferSize() const override final { return 0; }

  enum
  {
    PARAM_RATE = 0U,
    PARAM_DEPTH,
    PARAM_FEEDBACK,
    NUM_PARAMS
  };

  inline void setParameter(uint8_t index, int32_t value) override final
  {
    if (!fx_) return;

    float valf = param_10bit_to_f32(value);

    switch (index)
    {
    case PARAM_RATE:
      fx_phaser_set_rate(fx_, valf);
      break;

    case PARAM_DEPTH:
      fx_phaser_set_depth(fx_, valf);
      break;

    case PARAM_FEEDBACK:
      fx_phaser_set_feedback(fx_, valf);
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

    fx_ = fx_phaser_create();
    if (fx_)
    {
      fx_phaser_set_enabled(fx_, 1);
      fx_phaser_set_rate(fx_, 0.3f);
      fx_phaser_set_depth(fx_, 0.5f);
      fx_phaser_set_feedback(fx_, 0.4f);
    }
  }

  void teardown() override final
  {
    if (fx_)
    {
      fx_phaser_destroy(fx_);
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

      fx_phaser_process_frame(fx_, &left, &right, sample_rate);

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
  FXPhaser* fx_ = nullptr;
};
