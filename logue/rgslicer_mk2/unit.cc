/*
 *  File: unit.cc
 *
 *  RGSlicer Slicing Sampler unit interface for MicroKorg2
 *
 *  2026 (c) Regroove Project
 */

#include "unit.h"
#include "runtime.h"

#include <cstddef>
#include <cstdint>

#include "osc.h"

// Static member definitions for preset configuration
char RGSlicer::s_preset_names[8][32];
char RGSlicer::s_preset_files[8][64];
uint8_t RGSlicer::s_num_presets = 8;

static RGSlicer s_rgslicer_instance;
static unit_runtime_desc_t s_runtime_desc;

__attribute__((used)) int8_t unit_init(const unit_runtime_desc_t * desc) {
  if (!desc)
    return k_unit_err_undef;

  if (desc->target != unit_header.target)
    return k_unit_err_target;
  if (!UNIT_API_IS_COMPAT(desc->api))
    return k_unit_err_api_version;

  s_runtime_desc = *desc;

  return s_rgslicer_instance.Init(desc);
}

__attribute__((used)) void unit_teardown() {
  s_rgslicer_instance.Teardown();
}

__attribute__((used)) void unit_reset() {
  s_rgslicer_instance.Reset();
}

__attribute__((used)) void unit_resume() {
  s_rgslicer_instance.Resume();
}

__attribute__((used)) void unit_suspend() {
  s_rgslicer_instance.Suspend();
}

__attribute__((used)) void unit_render(const float * in, float * out, uint32_t frames) {
  (void)in;
  s_rgslicer_instance.Process(out, frames);
}

__attribute__((used)) void unit_set_param_value(uint8_t id, int32_t value) {
  s_rgslicer_instance.setParameter(id, value);
}

__attribute__((used)) int32_t unit_get_param_value(uint8_t id) {
  return s_rgslicer_instance.getParameterValue(id);
}

__attribute__((used)) const char * unit_get_param_str_value(uint8_t id, int32_t value) {
  return s_rgslicer_instance.getParameterStrValue(id, value);
}

__attribute__((used)) const uint8_t * unit_get_param_bmp_value(uint8_t id, int32_t value) {
  return s_rgslicer_instance.getParameterBmpValue(id, value);
}

__attribute__((used)) void unit_set_tempo(uint32_t tempo) {
  s_rgslicer_instance.SetTempo(tempo);
}

__attribute__((used)) void unit_load_preset(uint8_t idx) {
  return s_rgslicer_instance.LoadPreset(idx);
}

__attribute__((used)) uint8_t unit_get_preset_index() {
  return s_rgslicer_instance.getPresetIndex();
}

__attribute__((used)) const char * unit_get_preset_name(uint8_t idx) {
  return RGSlicer::getPresetName(idx);
}

__attribute__((used)) void unit_platform_exclusive(uint8_t messageId, void * data, uint32_t dataSize) {
  s_rgslicer_instance.unit_platform_exclusive(messageId, data, dataSize);
}

// Note on/off handlers for synth units
__attribute__((used)) void unit_note_on(uint8_t note, uint8_t velocity) {
  s_rgslicer_instance.NoteOn(note, velocity);
}

__attribute__((used)) void unit_note_off(uint8_t note) {
  s_rgslicer_instance.NoteOff(note);
}

__attribute__((used)) void unit_all_note_off(void) {
  s_rgslicer_instance.AllNoteOff();
}

__attribute__((used)) void unit_pitch_bend(uint16_t bend) {
  (void)bend;
}

__attribute__((used)) void unit_channel_pressure(uint8_t pressure) {
  (void)pressure;
}

__attribute__((used)) void unit_aftertouch(uint8_t note, uint8_t aftertouch) {
  (void)note;
  (void)aftertouch;
}
