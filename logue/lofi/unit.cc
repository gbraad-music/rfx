/*
    BSD 3-Clause License

    Copyright (c) 2023, KORG INC.
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this
      list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.

    * Neither the name of the copyright holder nor the names of its
      contributors may be used to endorse or promote products derived from
      this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
    FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
    DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
    SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
    CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
    OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

//*/

/*
 *  File: unit.cc
 *
 *  NTS-3 kaoss pad kit generic effect unit interface
 *
 */
#include "effect.h"
#include "unit_genericfx.h" // base definitions for delfx units
#include "utils/int_math.h" // clipminmaxi32()
#include <algorithm>        // std::fill

static Effect s_effect_instance;

// Parameter cache
static int32_t cached_values[UNIT_GENERICFX_MAX_PARAM_COUNT];

// Runtime descriptor for background effect support
// Provides access to get_raw_input() API which returns audio input unaffected by effect on/off state
// This enables the effect to run continuously without requiring HOLD (XY Freeze) to be pressed
static unit_runtime_desc_t s_runtime_desc;

__unit_callback int8_t unit_init(const unit_runtime_desc_t *desc)
{
  if (!desc)
    return k_unit_err_undef;

  if (desc->target != unit_header.common.target)
    return k_unit_err_target;

  if (!UNIT_API_IS_COMPAT(desc->api))
    return k_unit_err_api_version;

  if (desc->samplerate != s_effect_instance.getSampleRate())
    return k_unit_err_samplerate;

  if (desc->input_channels != 2 || desc->output_channels != 2)
    return k_unit_err_geometry;

  if (!desc->hooks.sdram_alloc)
    return k_unit_err_memory;

  if (s_effect_instance.getBufferSize() > 0)
  {
    float *allocated_buffer_ = (float *)desc->hooks.sdram_alloc(s_effect_instance.getBufferSize() * sizeof(float));
    if (!allocated_buffer_)
      return k_unit_err_memory;

    std::fill(allocated_buffer_, allocated_buffer_ + s_effect_instance.getBufferSize(), 0.f);
    s_effect_instance.init(allocated_buffer_);
  }
  else
  {
    s_effect_instance.init(nullptr);
  }

  for (int id = 0; id < UNIT_GENERICFX_MAX_PARAM_COUNT; ++id)
  {
    cached_values[id] = static_cast<int32_t>(unit_header.common.params[id].init);
    s_effect_instance.setParameter(id, cached_values[id]);
  }

  s_runtime_desc = *desc;

  return k_unit_err_none;
}

__unit_callback void unit_teardown()
{
  s_effect_instance.teardown();
}

__unit_callback void unit_reset()
{
  s_effect_instance.reset();
}

__unit_callback void unit_resume()
{
  s_effect_instance.resume();
}

__unit_callback void unit_suspend()
{
  s_effect_instance.suspend();
}

__unit_callback void unit_render(const float *in, float *out, uint32_t frames)
{
  const unit_runtime_genericfx_context_t *ctxt =
    static_cast<const unit_runtime_genericfx_context_t *>(s_runtime_desc.hooks.runtime_context);

  const float *raw_input = ctxt->get_raw_input();
  s_effect_instance.process(raw_input, out, frames);
}

__unit_callback void unit_set_param_value(uint8_t id, int32_t value)
{
  value = clipminmaxi32(unit_header.common.params[id].min, value, unit_header.common.params[id].max);
  cached_values[id] = value;
  s_effect_instance.setParameter(id, value);
}

__unit_callback int32_t unit_get_param_value(uint8_t id)
{
  return cached_values[id];
}

__unit_callback const char *unit_get_param_str_value(uint8_t id, int32_t value)
{
  value = clipminmaxi32(unit_header.common.params[id].min, value, unit_header.common.params[id].max);
  return s_effect_instance.getParameterStrValue(id, value);
}

__unit_callback void unit_touch_event(uint8_t id, uint8_t phase, uint32_t x, uint32_t y)
{
  s_effect_instance.touchEvent(id, phase, x, y);
}

__unit_callback void unit_set_tempo(uint32_t tempo)
{
  float bpm = (tempo >> 16) + (tempo & 0xFFFF) / static_cast<float>(0x10000);
  s_effect_instance.setTempo(bpm);
}

__unit_callback void unit_tempo_4ppqn_tick(uint32_t counter)
{
  s_effect_instance.tempo4ppqnTick(counter);
}
