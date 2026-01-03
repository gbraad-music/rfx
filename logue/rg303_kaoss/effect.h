/*
 * RG-303 Kaotic Bass Generator
 * TB-303 style bass synthesizer for NTS-3 Kaoss Pad
 */

#pragma once

#include "processor.h"
#include "unit_genericfx.h"

extern "C" {
#include "rg303_bass.h"
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
    PARAM_FILTER = 0U,  // Filter cutoff/resonance (X-axis)
    PARAM_PATTERN,      // Pattern variation (Y-axis)
    PARAM_ACCENT,       // Accent amount (unmapped)
    NUM_PARAMS
  };

  inline void setParameter(uint8_t index, int32_t value) override final
  {
    if (!bass_) return;

    switch (index)
    {
    case PARAM_FILTER:
      // Convert 0-1023 to filter cutoff (0.0 to 1.0)
      // 0.0 = filter closed (dark), 1.0 = filter open (bright)
      {
        float cutoff = value / 1023.0f;
        rg303_bass_set_filter(bass_, cutoff);
      }
      break;

    case PARAM_PATTERN:
      // Convert 0-1023 to pattern variation (0.0 to 1.0)
      // 0.0 = sparse pattern, 1.0 = dense pattern
      {
        float variation = value / 1023.0f;
        rg303_bass_set_pattern(bass_, variation);
      }
      break;

    case PARAM_ACCENT:
      // Convert 0-1023 to accent amount (0.0 to 1.0)
      {
        float accent = value / 1023.0f;
        rg303_bass_set_accent(bass_, accent);
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
    bass_ = rg303_bass_create();

    // Set default tempo to 120 BPM
    if (bass_)
    {
      rg303_bass_set_tempo(bass_, 120.0f);
    }
  }

  void teardown() override final
  {
    if (bass_)
    {
      rg303_bass_destroy(bass_);
      bass_ = nullptr;
    }
  }

  void process(const float *__restrict in, float *__restrict out, uint32_t frames) override final
  {
    if (!bass_)
    {
      // Fallback to pass-through if no bass instance
      for (uint32_t i = 0; i < frames * 2; i++)
      {
        out[i] = in[i];
      }
      return;
    }

    // Process bass synthesizer and mix with input
    const uint32_t sample_rate = 48000; // NTS-3 sample rate

    for (uint32_t i = 0; i < frames; i++)
    {
      float in_l = in[i * 2];
      float in_r = in[i * 2 + 1];
      float out_l, out_r;

      rg303_bass_process(bass_, &in_l, &in_r, &out_l, &out_r, sample_rate);

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
    if (bass_)
    {
      rg303_bass_set_tempo(bass_, bpm);
    }
  }

private:
  RG303Bass* bass_ = nullptr;
};
