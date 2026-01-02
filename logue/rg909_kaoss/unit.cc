/**
 *  @file unit.cc
 *  @brief nts-3 SDK unit interface for RG909 Kaoss Effect
 *
 *  Copyright (c) 2020-2022 KORG Inc. All rights reserved.
 *
 */

#include <cstddef>
#include <cstdint>

#include "unit_genericfx.h"  // Note: Include common definitions for generic FX units
#include "effect.h"           // Note: Include custom effect code

static Effect s_effect_instance;              // Note: Actual instance of custom effect object
static unit_runtime_desc_t s_runtime_desc;    // Note: Used to cache runtime descriptor obtained via init callback

// ---- Callback entry points from nts-3 runtime ----------------------------------------------

__unit_callback int8_t unit_init(const unit_runtime_desc_t * desc) {
  if (!desc)
    return k_unit_err_undef;

  if (desc->target != unit_header.common.target)
    return k_unit_err_target;
  if (!UNIT_API_IS_COMPAT(desc->api))
    return k_unit_err_api_version;

  s_runtime_desc = *desc;

  return s_effect_instance.Init(desc);
}

__unit_callback void unit_teardown() {
  s_effect_instance.Teardown();
}

__unit_callback void unit_reset() {
  s_effect_instance.Reset();
}

__unit_callback void unit_resume() {
  s_effect_instance.Resume();
}

__unit_callback void unit_suspend() {
  s_effect_instance.Suspend();
}

__unit_callback void unit_render(const float * in, float * out, uint32_t frames) {
  s_effect_instance.Render(in, out, frames);
}

__unit_callback void unit_set_param_value(uint8_t id, int32_t value) {
  s_effect_instance.setParameter(id, value);
}

__unit_callback int32_t unit_get_param_value(uint8_t id) {
  return s_effect_instance.getParameterValue(id);
}

__unit_callback const char * unit_get_param_str_value(uint8_t id, int32_t value) {
  return s_effect_instance.getParameterStrValue(id, value);
}

__unit_callback const uint8_t * unit_get_param_bmp_value(uint8_t id, int32_t value) {
  return s_effect_instance.getParameterBmpValue(id, value);
}

__unit_callback void unit_set_tempo(uint32_t tempo) {
  s_effect_instance.setTempo(tempo);
}

__unit_callback void unit_tempo_4ppqn_tick(uint32_t counter) {
  s_effect_instance.tempo4ppqnTick(counter);
}

__unit_callback void unit_touch_event(uint8_t id, uint8_t phase, uint32_t x, uint32_t y) {
  s_effect_instance.touchEvent(id, phase, x, y);
}

__unit_callback void unit_load_preset(uint8_t idx) {
  s_effect_instance.LoadPreset(idx);
}

__unit_callback uint8_t unit_get_preset_index() {
  return s_effect_instance.getPresetIndex();
}

__unit_callback const char * unit_get_preset_name(uint8_t idx) {
  return Effect::getPresetName(idx);
}
