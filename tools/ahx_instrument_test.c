/*
 * AHX Instrument Test
 *
 * Simple test program to verify AHX instrument synthesis
 */

#include <stdio.h>
#include <stdlib.h>
#include "../synth/ahx_instrument.h"

#define SAMPLE_RATE 48000
#define BUFFER_SIZE 1024

int main(void) {
    printf("AHX Instrument Test\n");
    printf("===================\n\n");

    // Create instrument
    AhxInstrument inst;
    ahx_instrument_init(&inst);

    // Get default params
    AhxInstrumentParams params = ahx_instrument_default_params();

    printf("Default Parameters:\n");
    printf("  Waveform: %d\n", params.waveform);
    printf("  Wave Length: %d\n", params.wave_length);
    printf("  Volume: %d\n", params.volume);
    printf("  Envelope: A=%d/%d D=%d/%d S=%d R=%d/%d\n",
           params.envelope.attack_frames, params.envelope.attack_volume,
           params.envelope.decay_frames, params.envelope.decay_volume,
           params.envelope.sustain_frames,
           params.envelope.release_frames, params.envelope.release_volume);
    printf("\n");

    // Test 1: Triangle wave
    printf("Test 1: Triangle wave, note C4 (MIDI 60)\n");
    params.waveform = AHX_WAVE_TRIANGLE;
    ahx_instrument_set_params(&inst, &params);
    ahx_instrument_note_on(&inst, 60, 100, SAMPLE_RATE);

    float buffer[BUFFER_SIZE];
    uint32_t total_samples = 0;
    int frames = 0;

    while (ahx_instrument_is_active(&inst) && frames < 100) {
        uint32_t generated = ahx_instrument_process(&inst, buffer, BUFFER_SIZE, SAMPLE_RATE);
        total_samples += generated;
        frames++;
    }

    printf("  Generated %u samples (%u frames)\n", total_samples, frames);
    printf("  Duration: %.2f seconds\n", (float)total_samples / SAMPLE_RATE);
    printf("  Active: %s\n\n", ahx_instrument_is_active(&inst) ? "yes" : "no");

    // Test 2: Sawtooth with filter sweep
    printf("Test 2: Sawtooth with filter sweep\n");
    params.waveform = AHX_WAVE_SAWTOOTH;
    params.filter_enabled = true;
    params.filter_lower = 10;
    params.filter_upper = 50;
    params.filter_speed = 2;
    ahx_instrument_set_params(&inst, &params);
    ahx_instrument_note_on(&inst, 48, 127, SAMPLE_RATE);

    total_samples = 0;
    frames = 0;

    while (ahx_instrument_is_active(&inst) && frames < 100) {
        uint32_t generated = ahx_instrument_process(&inst, buffer, BUFFER_SIZE, SAMPLE_RATE);
        total_samples += generated;
        frames++;
    }

    printf("  Generated %u samples (%u frames)\n", total_samples, frames);
    printf("  Duration: %.2f seconds\n\n", (float)total_samples / SAMPLE_RATE);

    // Test 3: Square wave with PWM
    printf("Test 3: Square wave with PWM\n");
    params.waveform = AHX_WAVE_SQUARE;
    params.filter_enabled = false;
    params.square_enabled = true;
    params.square_lower = 32;
    params.square_upper = 224;
    params.square_speed = 8;
    ahx_instrument_set_params(&inst, &params);
    ahx_instrument_note_on(&inst, 72, 100, SAMPLE_RATE);

    // Play for 0.5 seconds then release
    uint32_t play_samples = SAMPLE_RATE / 2;
    total_samples = 0;

    while (total_samples < play_samples) {
        uint32_t to_generate = (play_samples - total_samples > BUFFER_SIZE) ? BUFFER_SIZE : (play_samples - total_samples);
        uint32_t generated = ahx_instrument_process(&inst, buffer, to_generate, SAMPLE_RATE);
        total_samples += generated;
    }

    printf("  Played for %.2f seconds\n", (float)total_samples / SAMPLE_RATE);

    // Release note
    ahx_instrument_note_off(&inst);
    printf("  Note released\n");

    // Process release
    frames = 0;
    while (ahx_instrument_is_active(&inst) && frames < 100) {
        uint32_t generated = ahx_instrument_process(&inst, buffer, BUFFER_SIZE, SAMPLE_RATE);
        total_samples += generated;
        frames++;
    }

    printf("  Total duration: %.2f seconds\n\n", (float)total_samples / SAMPLE_RATE);

    // Test 4: Noise
    printf("Test 4: White noise\n");
    params.waveform = AHX_WAVE_NOISE;
    params.square_enabled = false;
    params.envelope.release_frames = 5;  // Quick release
    ahx_instrument_set_params(&inst, &params);
    ahx_instrument_note_on(&inst, 60, 80, SAMPLE_RATE);
    ahx_instrument_note_off(&inst);

    total_samples = 0;
    while (ahx_instrument_is_active(&inst)) {
        uint32_t generated = ahx_instrument_process(&inst, buffer, BUFFER_SIZE, SAMPLE_RATE);
        total_samples += generated;
    }

    printf("  Duration: %.2f seconds\n\n", (float)total_samples / SAMPLE_RATE);

    printf("All tests completed successfully!\n");

    return 0;
}
