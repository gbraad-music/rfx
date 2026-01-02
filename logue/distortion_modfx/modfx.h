/*
 * Regroove Distortion - NTS-1 MKII Modulation Effect
 * Analog-style saturation
 */

#pragma once

#include "processor.h"
#include "unit_modfx.h"
#include "fx_distortion.h"

class Modfx : public Processor {
public:
  Modfx() : fx_(nullptr), drive_(0.5f), mix_(0.5f) {}

  ~Modfx() {
    if (fx_) {
      fx_distortion_destroy(fx_);
      fx_ = nullptr;
    }
  }

  // No buffer needed for this effect
  uint32_t getBufferSize() const override final { return 0; }

  enum {
    DRIVE = 0U,
    MIX,
    NUM_PARAMS
  };

  void init(float *allocated_buffer) override final {
    (void)allocated_buffer;

    fx_ = fx_distortion_create();
    if (fx_) {
      fx_distortion_reset(fx_);
      fx_distortion_set_drive(fx_, drive_);
      fx_distortion_set_mix(fx_, mix_);
    }
  }

  void teardown() override final {
    if (fx_) {
      fx_distortion_destroy(fx_);
      fx_ = nullptr;
    }
  }

  void reset() override final {
    if (fx_) {
      fx_distortion_reset(fx_);
    }
  }

  void resume() override final {}
  void suspend() override final {}

  void process(const float *__restrict in, float *__restrict out, uint32_t frames) override final {
    if (!fx_) {
      // Passthrough if not initialized
      for (uint32_t i = 0; i < frames * 2; i++) {
        out[i] = in[i];
      }
      return;
    }

    const float * __restrict in_p = in;
    float * __restrict out_p = out;
    const float * out_e = out_p + (frames << 1);

    for (; out_p != out_e; in_p += 2, out_p += 2) {
      float left = in_p[0];
      float right = in_p[1];

      fx_distortion_process_frame(fx_, &left, &right, 48000);

      out_p[0] = left;
      out_p[1] = right;
    }
  }

  void setParameter(uint8_t index, int32_t value) override final {
    float normalized = value / 1023.0f;

    switch (index) {
      case DRIVE:
        drive_ = normalized;
        if (fx_) fx_distortion_set_drive(fx_, drive_);
        break;

      case MIX:
        mix_ = normalized;
        if (fx_) fx_distortion_set_mix(fx_, mix_);
        break;

      default:
        break;
    }
  }

  const char *getParameterStrValue(uint8_t index, int32_t value) const override final {
    (void)index;
    (void)value;
    return nullptr;
  }

private:
  FXDistortion *fx_;
  float drive_;
  float mix_;
};
