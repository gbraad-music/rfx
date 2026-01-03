/*
 * RG-404 Background Kick Generator
 * Four-on-the-floor kick using C-based implementation
 */

#pragma once

#include "processor.h"
#include "unit_genericfx.h"

extern "C" {
#include "rg404_kick.h"
}

class Effect : public Processor
{
public:
  uint32_t getBufferSize() const override final { return 0; }

  enum
  {
    PARAM_TEMPO = 0U,
    PARAM_KICK_MIX,
    NUM_PARAMS
  };

  inline void setParameter(uint8_t index, int32_t value) override final
  {
    if (!kick_) return;

    switch (index)
    {
    case PARAM_TEMPO:
      // Convert 0-1023 to tempo multiplier (0.5x to 2.0x)
      {
        float mult = 0.5f + (value / 1023.0f) * 1.5f;
        rg404_kick_set_tempo_mult(kick_, mult);
      }
      break;

    case PARAM_KICK_MIX:
      // Convert 0-1023 to mix level (0.0 to 1.0)
      {
        float mix = value / 1023.0f;
        rg404_kick_set_mix(kick_, mix);
      }
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
    kick_ = rg404_kick_create();

    // Set default tempo to 120 BPM
    if (kick_)
    {
      rg404_kick_set_tempo(kick_, 120.0f);
    }
  }

  void teardown() override final
  {
    if (kick_)
    {
      rg404_kick_destroy(kick_);
      kick_ = nullptr;
    }
  }

  void process(const float *__restrict in, float *__restrict out, uint32_t frames) override final
  {
    if (!kick_)
    {
      // Fallback to pass-through if no kick instance
      for (uint32_t i = 0; i < frames * 2; i++)
      {
        out[i] = in[i];
      }
      return;
    }

    // Process kick drum and mix with input
    const uint32_t sample_rate = 48000; // NTS-3 sample rate

    for (uint32_t i = 0; i < frames; i++)
    {
      float in_l = in[i * 2];
      float in_r = in[i * 2 + 1];
      float out_l, out_r;

      rg404_kick_process(kick_, &in_l, &in_r, &out_l, &out_r, sample_rate);

      out[i * 2] = out_l;
      out[i * 2 + 1] = out_r;
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
  RG404Kick* kick_ = nullptr;
};
