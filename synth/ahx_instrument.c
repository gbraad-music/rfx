/*
 * AHX Instrument Synth - Implementation
 *
 * Plugin-friendly wrapper around ahx_synth_core
 * Allows external parameter control while using authentic AHX algorithms
 */

#include "ahx_instrument.h"
#include "ahx_synth_core.h"
#include <stdlib.h>
#include <string.h>

// Convert plugin params to core instrument definition
static void params_to_core_instrument(AhxCoreInstrument* core, const AhxInstrumentParams* params) {
    core->Waveform = params->waveform;
    core->Volume = params->volume;
    core->WaveLength = params->wave_length;

    // Envelope
    core->Envelope.aFrames = params->envelope.attack_frames;
    core->Envelope.aVolume = params->envelope.attack_volume;
    core->Envelope.dFrames = params->envelope.decay_frames;
    core->Envelope.dVolume = params->envelope.decay_volume;
    core->Envelope.sFrames = params->envelope.sustain_frames;
    core->Envelope.rFrames = params->envelope.release_frames;
    core->Envelope.rVolume = params->envelope.release_volume;

    // Filter modulation
    if (params->filter_enabled) {
        core->FilterLowerLimit = params->filter_lower;
        core->FilterUpperLimit = params->filter_upper;
        core->FilterSpeed = params->filter_speed;
    } else {
        core->FilterLowerLimit = 0;
        core->FilterUpperLimit = 0;
        core->FilterSpeed = 0;
    }

    // PWM modulation
    if (params->square_enabled) {
        core->SquareLowerLimit = params->square_lower;
        core->SquareUpperLimit = params->square_upper;
        core->SquareSpeed = params->square_speed;
    } else {
        core->SquareLowerLimit = 0;
        core->SquareUpperLimit = 0;
        core->SquareSpeed = 0;
    }

    // Vibrato
    core->VibratoDelay = params->vibrato_delay;
    core->VibratoDepth = params->vibrato_depth;
    core->VibratoSpeed = params->vibrato_speed;

    // Hard cut release
    core->HardCutRelease = params->hard_cut_release ? 1 : 0;
    core->HardCutReleaseFrames = params->hard_cut_frames;
}

// Initialize instrument
void ahx_instrument_init(AhxInstrument* inst) {
    if (!inst) return;

    memset(inst, 0, sizeof(AhxInstrument));

    // Initialize with default params
    inst->params = ahx_instrument_default_params();

    // Convert to core instrument
    AhxCoreInstrument core_inst;
    params_to_core_instrument(&core_inst, &inst->params);

    // Initialize synthesis voice with core
    ahx_synth_voice_init(&inst->voice);
    ahx_synth_voice_calc_adsr(&inst->voice, &core_inst);

    // Store core instrument
    memcpy(&inst->core_inst, &core_inst, sizeof(AhxCoreInstrument));
    inst->voice.Instrument = &inst->core_inst;
}

// Set instrument parameters
void ahx_instrument_set_params(AhxInstrument* inst, const AhxInstrumentParams* params) {
    if (!inst || !params) return;

    // Copy params
    memcpy(&inst->params, params, sizeof(AhxInstrumentParams));

    // Convert to core instrument
    params_to_core_instrument(&inst->core_inst, params);

    // Recalculate ADSR if instrument has been initialized
    if (inst->voice.Instrument) {
        ahx_synth_voice_calc_adsr(&inst->voice, &inst->core_inst);
    }
}

// Get current parameters
void ahx_instrument_get_params(const AhxInstrument* inst, AhxInstrumentParams* params) {
    if (!inst || !params) return;
    memcpy(params, &inst->params, sizeof(AhxInstrumentParams));
}

// Create default parameters
AhxInstrumentParams ahx_instrument_default_params(void) {
    AhxInstrumentParams params;
    memset(&params, 0, sizeof(AhxInstrumentParams));

    // Default oscillator
    params.waveform = AHX_WAVE_SAWTOOTH;
    params.wave_length = 4;
    params.volume = 64;

    // Default ADSR envelope (basic shape)
    params.envelope.attack_frames = 1;
    params.envelope.attack_volume = 64;
    params.envelope.decay_frames = 20;
    params.envelope.decay_volume = 50;
    params.envelope.sustain_frames = 0;  // Infinite
    params.envelope.release_frames = 30;
    params.envelope.release_volume = 0;

    // No modulation by default
    params.filter_enabled = false;
    params.filter_lower = 0;
    params.filter_upper = 63;
    params.filter_speed = 4;

    params.square_enabled = false;
    params.square_lower = 32;
    params.square_upper = 224;
    params.square_speed = 4;

    // No vibrato by default
    params.vibrato_delay = 0;
    params.vibrato_depth = 0;
    params.vibrato_speed = 0;

    // No hard cut
    params.hard_cut_release = false;
    params.hard_cut_frames = 3;

    params.plist = NULL;

    return params;
}

// Trigger note on
void ahx_instrument_note_on(AhxInstrument* inst, uint8_t note, uint8_t velocity, uint32_t sample_rate) {
    if (!inst) return;

    // Use authentic AHX synthesis core
    ahx_synth_voice_note_on(&inst->voice, note, velocity, sample_rate);

    inst->note = note;
    inst->velocity = velocity;
    inst->active = true;
    inst->released = false;
}

// Trigger note off
void ahx_instrument_note_off(AhxInstrument* inst) {
    if (!inst) return;

    // Use authentic AHX synthesis core
    ahx_synth_voice_note_off(&inst->voice);

    inst->released = true;
}

// Process audio samples (uses authentic AHX algorithms)
uint32_t ahx_instrument_process(AhxInstrument* inst, float* output, uint32_t num_samples, uint32_t sample_rate) {
    (void)sample_rate;  // Reserved for future use

    if (!inst || !output) {
        if (output) {
            memset(output, 0, num_samples * sizeof(float));
        }
        return 0;
    }

    // Use authentic AHX synthesis core
    uint32_t generated = ahx_synth_voice_process(&inst->voice, output, num_samples, sample_rate);

    // Update active state
    inst->active = ahx_synth_voice_is_active(&inst->voice);

    return generated;
}

// Check if instrument is active
bool ahx_instrument_is_active(const AhxInstrument* inst) {
    if (!inst) return false;
    return ahx_synth_voice_is_active(&inst->voice);
}

// Reset instrument
void ahx_instrument_reset(AhxInstrument* inst) {
    if (!inst) return;

    ahx_synth_voice_reset(&inst->voice);
    inst->voice.Instrument = &inst->core_inst;

    inst->active = false;
    inst->released = false;
    inst->note = 0;
    inst->velocity = 0;
}

// Process frame (deprecated - now handled by ahx_synth_core)
void ahx_instrument_process_frame(AhxInstrument* inst) {
    if (!inst) return;
    ahx_synth_voice_process_frame(&inst->voice);
}
