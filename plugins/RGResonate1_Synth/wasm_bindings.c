/*
 * WebAssembly Bindings for RGResonate1 Synthesizer
 * This file provides helper functions for JavaScript integration
 */

#include <emscripten.h>
#include <stdlib.h>

// The actual synth headers are included in the Makefile compilation
// All regroove_synth.h functions are automatically exported

// Helper: Create audio buffer for JavaScript
EMSCRIPTEN_KEEPALIVE
void* synth_create_audio_buffer(int frames) {
    // Stereo interleaved buffer
    return malloc(frames * 2 * sizeof(float));
}

// Helper: Destroy audio buffer
EMSCRIPTEN_KEEPALIVE
void synth_destroy_audio_buffer(void* buffer) {
    free(buffer);
}

// Helper: Get buffer size in bytes
EMSCRIPTEN_KEEPALIVE
int synth_get_buffer_size_bytes(int frames) {
    return frames * 2 * sizeof(float);  // Stereo interleaved
}
