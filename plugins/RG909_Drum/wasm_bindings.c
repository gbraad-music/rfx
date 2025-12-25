/*
 * WebAssembly Bindings for RG909 Drum Synthesizer
 * Thin wrapper around shared core implementation
 */

#include <emscripten.h>
#include "synth/rg909_drum_synth.h"

// Thin wrappers for WASM export
EMSCRIPTEN_KEEPALIVE
RG909Synth* rg909_create(float sample_rate) {
    (void)sample_rate;  // Unused currently
    return rg909_synth_create();
}

EMSCRIPTEN_KEEPALIVE
void rg909_destroy(RG909Synth* synth) {
    rg909_synth_destroy(synth);
}

EMSCRIPTEN_KEEPALIVE
void rg909_reset(RG909Synth* synth) {
    rg909_synth_reset(synth);
}

EMSCRIPTEN_KEEPALIVE
void rg909_trigger_drum(RG909Synth* synth, uint8_t note, uint8_t velocity, float sample_rate) {
    rg909_synth_trigger_drum(synth, note, velocity, sample_rate);
}

EMSCRIPTEN_KEEPALIVE
void rg909_process_f32(RG909Synth* synth, float* buffer, int frames, float sample_rate) {
    rg909_synth_process_interleaved(synth, buffer, frames, sample_rate);
}

EMSCRIPTEN_KEEPALIVE
void rg909_set_parameter(RG909Synth* synth, int param, float value) {
    rg909_synth_set_parameter(synth, param, value);
}

EMSCRIPTEN_KEEPALIVE
float rg909_get_parameter(RG909Synth* synth, int param) {
    return rg909_synth_get_parameter(synth, param);
}
