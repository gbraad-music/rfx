#pragma once
/*
 * RFX Lofi Effect for NTS-3
 */

#include "processor.h"
#include "../../effects/fx_lofi.h"
#include <cstdio>

enum {
  PARAM_BIT_DEPTH = 0,
  PARAM_SAMPLE_RATE,
  PARAM_FILTER,
  PARAM_SATURATION,
  PARAM_NOISE,
  PARAM_WOW_FLUTTER_DEPTH,
  PARAM_WOW_FLUTTER_RATE,
};

class Effect : public Processor
{
public:
  Effect() : fx_(nullptr) {}

  ~Effect() override {
    if (fx_) {
      fx_lofi_destroy(fx_);
      fx_ = nullptr;
    }
  }

  uint32_t getBufferSize() const override { return 0; }

  void init(float *buffer) override {
    (void)buffer;
    fx_ = fx_lofi_create(getSampleRate());
    if (fx_) {
      fx_lofi_set_enabled(fx_, true);
      // Set defaults (normalized 0-1 for MIDI compatibility)
      fx_lofi_set_bit_depth(fx_, 1.0f);           // 1.0 = 16-bit (clean)
      fx_lofi_set_sample_rate_ratio(fx_, 1.0f);   // 1.0 = 48kHz (no reduction)
      fx_lofi_set_filter_cutoff(fx_, 1.0f);       // 1.0 = 20kHz (no filtering)
      fx_lofi_set_saturation(fx_, 0.0f);          // 0.0 = no saturation
      fx_lofi_set_noise_level(fx_, 0.0f);         // 0.0 = no noise
      fx_lofi_set_wow_flutter_depth(fx_, 0.0f);   // 0.0 = no wow/flutter
      fx_lofi_set_wow_flutter_rate(fx_, 0.5f);    // 0.5 = 5Hz (mid-range)
    }
  }

  void teardown() override {
    if (fx_) {
      fx_lofi_destroy(fx_);
      fx_ = nullptr;
    }
  }

  void reset() override {
    if (fx_) {
      fx_lofi_reset(fx_);
    }
  }

  void process(const float *in, float *out, uint32_t frames) override {
    if (!fx_) {
      // Bypass
      for (uint32_t i = 0; i < frames * 2; ++i) {
        out[i] = in[i];
      }
      return;
    }

    // Copy input to output buffer
    for (uint32_t i = 0; i < frames * 2; ++i) {
      out[i] = in[i];
    }

    // Process in-place
    fx_lofi_process_f32(fx_, out, frames, getSampleRate());
  }

  void setParameter(uint8_t index, int32_t value) override {
    if (!fx_) return;

    switch (index) {
      case PARAM_BIT_DEPTH: {
        // Map index 0-3 to normalized 0-1 for MIDI compatibility
        int idx = (value < 0) ? 0 : (value > 3) ? 3 : value;
        fx_lofi_set_bit_depth(fx_, idx / 3.0f);
        break;
      }

      case PARAM_SAMPLE_RATE: {
        // Map index 0-7 to normalized 0-1 for MIDI compatibility
        int idx = (value < 0) ? 0 : (value > 7) ? 7 : value;
        fx_lofi_set_sample_rate_ratio(fx_, idx / 7.0f);
        break;
      }

      case PARAM_FILTER: {
        // Value is 0-100, send as normalized 0-1
        fx_lofi_set_filter_cutoff(fx_, value / 100.0f);
        break;
      }

      case PARAM_SATURATION: {
        // Value is 0-100, send as normalized 0-1
        fx_lofi_set_saturation(fx_, value / 100.0f);
        break;
      }

      case PARAM_NOISE: {
        // Value is 0-100, send as normalized 0-1
        fx_lofi_set_noise_level(fx_, value / 100.0f);
        break;
      }

      case PARAM_WOW_FLUTTER_DEPTH: {
        // Value is 0-100, send as normalized 0-1
        fx_lofi_set_wow_flutter_depth(fx_, value / 100.0f);
        break;
      }

      case PARAM_WOW_FLUTTER_RATE: {
        // Value is 1-100, map to normalized 0-1
        fx_lofi_set_wow_flutter_rate(fx_, (value - 1) / 99.0f);
        break;
      }
    }
  }

  const char *getParameterStrValue(uint8_t index, int32_t value) const override {
    static char str[16];

    switch (index) {
      case PARAM_BIT_DEPTH: {
        static const char *names[] = {"2-bit", "8-bit", "12-bit", "16-bit"};
        int idx = (value < 0) ? 0 : (value > 3) ? 3 : value;
        return names[idx];
      }

      case PARAM_SAMPLE_RATE: {
        static const char *names[] = {
          "7.5kHz", "8.3kHz", "10kHz", "15kHz",
          "16.7kHz", "22kHz", "32kHz", "48kHz"
        };
        int idx = (value < 0) ? 0 : (value > 7) ? 7 : value;
        return names[idx];
      }

      case PARAM_FILTER: {
        float norm = value / 100.0f;
        float freq = 200.0f * powf(20000.0f / 200.0f, norm);
        snprintf(str, sizeof(str), "%.0fHz", freq);
        return str;
      }

      case PARAM_SATURATION: {
        float sat = (value / 100.0f) * 2.0f;
        snprintf(str, sizeof(str), "%.1f", sat);
        return str;
      }

      case PARAM_WOW_FLUTTER_RATE: {
        float rate = 0.1f + ((value - 1) / 99.0f) * 9.9f;
        snprintf(str, sizeof(str), "%.1fHz", rate);
        return str;
      }

      default: {
        snprintf(str, sizeof(str), "%d%%", (int)value);
        return str;
      }
    }
  }

private:
  FX_Lofi *fx_;
};
