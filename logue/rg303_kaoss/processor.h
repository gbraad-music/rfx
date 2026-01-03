/*
 * Processor base class interface
 */

#pragma once

#include <stdint.h>

inline float param_10bit_to_f32(int32_t value)
{
  return value / 1023.0f;
}

class Processor
{
public:
  virtual ~Processor() {}

  virtual uint32_t getBufferSize() const = 0;
  virtual void init(float *buffer) = 0;
  virtual void teardown() = 0;
  virtual void process(const float *in, float *out, uint32_t frames) = 0;
  virtual void setParameter(uint8_t index, int32_t value) = 0;
  virtual const char *getParameterStrValue(uint8_t index, int32_t value) const = 0;
  virtual void touchEvent(uint8_t id, uint8_t phase, uint32_t x, uint32_t y) = 0;

  virtual uint32_t getSampleRate() const { return 48000; }
  virtual void reset() {}
  virtual void resume() {}
  virtual void suspend() {}
  virtual void setTempo(float bpm) { (void)bpm; }
  virtual void tempo4ppqnTick(uint32_t counter) { (void)counter; }
};
