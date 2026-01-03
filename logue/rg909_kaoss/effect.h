/*
 * RG-909 Kaotic Drum Generator
 * TR-909 style drum machine using RG909_BD for authentic kick
 */

#pragma once

#include "processor.h"
#include "unit_genericfx.h"

extern "C" {
#include "rg909_drum.h"
}

// WORKAROUND for "_ZdlPv", "ELF error: resolve symbol"
// The logue SDK doesn't provide operator delete, but the Effect destructor needs it
void operator delete(void *) noexcept {}

class Effect : public Processor
{
public:
  uint32_t getBufferSize() const override final { return 0; }

  enum
  {
    PARAM_KICK = 0U,    // Kick density (X-axis) - 0 = no kick
    PARAM_SNARE,        // Snare variation (Y-axis) - 0 = no snare
    PARAM_TONE,         // Kick tone/pitch (unmapped)
    NUM_PARAMS
  };

  inline void setParameter(uint8_t index, int32_t value) override final
  {
    if (!drum_) return;

    switch (index)
    {
    case PARAM_KICK:
      // Convert 0-1023 to kick density (0.0 to 1.0)
      // 0.0 = no kick, 1.0 = maximum kick density/variation
      {
        float density = value / 1023.0f;
        rg909_drum_set_kick_density(drum_, density);
      }
      break;

    case PARAM_SNARE:
      // Convert 0-1023 to snare variation (0.0 to 1.0)
      // 0.0 = no snare, 1.0 = maximum snare variation
      {
        float variation = value / 1023.0f;
        rg909_drum_set_snare_variation(drum_, variation);
      }
      break;

    case PARAM_TONE:
      // Convert 0-1023 to tone amount (0.0 to 1.0)
      {
        float tone = value / 1023.0f;
        rg909_drum_set_tone(drum_, tone);
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
    drum_ = rg909_drum_create();

    // Set default tempo to 120 BPM
    if (drum_)
    {
      rg909_drum_set_tempo(drum_, 120.0f);
    }
  }

  void teardown() override final
  {
    if (drum_)
    {
      rg909_drum_destroy(drum_);
      drum_ = nullptr;
    }
  }

  void process(const float *__restrict in, float *__restrict out, uint32_t frames) override final
  {
    if (!drum_)
    {
      // Fallback to pass-through if no drum instance
      for (uint32_t i = 0; i < frames * 2; i++)
      {
        out[i] = in[i];
      }
      return;
    }

    // Process drum machine and mix with input
    const uint32_t sample_rate = 48000; // NTS-3 sample rate

    for (uint32_t i = 0; i < frames; i++)
    {
      float in_l = in[i * 2];
      float in_r = in[i * 2 + 1];
      float out_l, out_r;

      rg909_drum_process(drum_, &in_l, &in_r, &out_l, &out_r, sample_rate);

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

  inline void setTempo(float bpm) override final
  {
    if (drum_)
    {
      rg909_drum_set_tempo(drum_, bpm);
    }
  }

private:
  RG909Drum* drum_ = nullptr;
};
