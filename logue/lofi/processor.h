#pragma once
/*
 * Base Processor class for NTS-3 effects
 */

#include <cstdint>

class Processor
{
public:
  virtual ~Processor() {}

  virtual uint32_t getSampleRate() const { return 48000; }
  virtual uint32_t getBufferSize() const = 0;

  virtual void init(float *buffer) = 0;
  virtual void teardown() {}
  virtual void reset() {}
  virtual void resume() {}
  virtual void suspend() {}

  virtual void process(const float *in, float *out, uint32_t frames) = 0;
  virtual void setParameter(uint8_t index, int32_t value) = 0;
  virtual const char *getParameterStrValue(uint8_t index, int32_t value) const = 0;

  virtual void touchEvent(uint8_t id, uint8_t phase, uint32_t x, uint32_t y) {
    (void)id; (void)phase; (void)x; (void)y;
  }

  virtual void setTempo(float bpm) { (void)bpm; }
  virtual void tempo4ppqnTick(uint32_t counter) { (void)counter; }
};
