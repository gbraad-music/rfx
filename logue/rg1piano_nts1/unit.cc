/*
    BSD 3-Clause License

    Copyright (c) 2023, KORG INC.
    All rights reserved.
*/

/*
 *  File: unit.cc
 *
 *  NTS-1 mkII oscillator unit interface for RG1Piano
 *
 */

#include "unit_osc.h"
#include "synth.h"
#include "utils/int_math.h"

static Synth s_synth_instance;
static int32_t cached_values[UNIT_OSC_MAX_PARAM_COUNT];
static const unit_runtime_osc_context_t *context;

__unit_callback int8_t unit_init(const unit_runtime_desc_t *desc)
{
  if (!desc)
    return k_unit_err_undef;

  if (desc->target != unit_header.target)
    return k_unit_err_target;

  if (!UNIT_API_IS_COMPAT(desc->api))
    return k_unit_err_api_version;

  if (desc->samplerate != 48000)
    return k_unit_err_samplerate;

  if (desc->input_channels != 2 || desc->output_channels != 1)
    return k_unit_err_geometry;

  context = static_cast<const unit_runtime_osc_context_t *>(desc->hooks.runtime_context);

  for (int id = 0; id < UNIT_OSC_MAX_PARAM_COUNT; ++id)
  {
    cached_values[id] = static_cast<int32_t>(unit_header.params[id].init);
  }

  // Initialize synth with modified descriptor for stereo output compatibility
  unit_runtime_desc_t synth_desc = *desc;
  synth_desc.output_channels = 2;
  int8_t result = s_synth_instance.Init(&synth_desc);

  // Apply initial parameter values
  if (result == k_unit_err_none) {
    for (int id = 0; id < unit_header.num_params; ++id) {
      s_synth_instance.setParameter(id, cached_values[id]);
    }
  }

  return result;
}

__unit_callback void unit_teardown()
{
  s_synth_instance.Teardown();
}

__unit_callback void unit_reset()
{
  s_synth_instance.Reset();
}

__unit_callback void unit_resume()
{
  s_synth_instance.Resume();
}

__unit_callback void unit_suspend()
{
  s_synth_instance.Suspend();
}

__unit_callback void unit_render(const float *in, float *out, uint32_t frames)
{
  (void)in;

  // Render to temporary stereo buffer
  float stereo_buffer[frames * 2];
  s_synth_instance.Render(stereo_buffer, frames);

  // Convert stereo to mono output
  for (uint32_t i = 0; i < frames; i++)
  {
    out[i] = stereo_buffer[i * 2]; // Use left channel
  }
}

__unit_callback void unit_set_param_value(uint8_t id, int32_t value)
{
  value = clipminmaxi32(unit_header.params[id].min, value, unit_header.params[id].max);
  cached_values[id] = value;
  s_synth_instance.setParameter(id, value);
}

__unit_callback int32_t unit_get_param_value(uint8_t id)
{
  return cached_values[id];
}

__unit_callback const char *unit_get_param_str_value(uint8_t id, int32_t value)
{
  value = clipminmaxi32(unit_header.params[id].min, value, unit_header.params[id].max);
  return s_synth_instance.getParameterStrValue(id, value);
}

__unit_callback void unit_note_on(uint8_t note, uint8_t velo)
{
  s_synth_instance.NoteOn(note, velo);
}

__unit_callback void unit_note_off(uint8_t note)
{
  s_synth_instance.NoteOff(note);
}

__unit_callback void unit_all_note_off()
{
  s_synth_instance.AllNoteOff();
}

__unit_callback void unit_set_tempo(uint32_t tempo)
{
  (void)tempo;
}

__unit_callback void unit_tempo_4ppqn_tick(uint32_t counter)
{
  (void)counter;
}

__unit_callback void unit_pitch_bend(uint16_t bend)
{
  s_synth_instance.PitchBend(bend);
}

__unit_callback void unit_channel_pressure(uint8_t press)
{
  s_synth_instance.ChannelPressure(press);
}

__unit_callback void unit_aftertouch(uint8_t note, uint8_t press)
{
  s_synth_instance.Aftertouch(note, press);
}
