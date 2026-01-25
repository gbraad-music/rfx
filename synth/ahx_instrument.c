/*
 * AHX Instrument Synth - Implementation
 *
 * Plugin-friendly wrapper around ahx_synth_core
 * Allows external parameter control while using authentic AHX algorithms
 */

#include "ahx_instrument.h"
#include "ahx_synth_core.h"
#include "ahx_plist.h"
#include <stdlib.h>
#include <string.h>

#ifdef EMSCRIPTEN
#include <emscripten.h>
#endif

#define AMIGA_PAULA_PAL_CLK 3546895

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

    // Initialize PList state
    inst->perf_current = 0;
    inst->perf_speed = 1;
    inst->perf_wait = 1;
    inst->perf_sub_volume = 64;  // Max volume
    inst->period_perf_slide_speed = 0;
    inst->period_perf_slide_period = 0;
    inst->period_perf_slide_on = false;
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

    // Default speed multiplier (standard AHX)
    params.speed_multiplier = 3;

    params.plist = NULL;

    return params;
}

// Trigger note on
void ahx_instrument_note_on(AhxInstrument* inst, uint8_t note, uint8_t velocity, uint32_t sample_rate) {
    if (!inst) return;

    // Reset PList playback to start
    inst->perf_current = 0;
    inst->voice.debug_frame_count = 0;

    if (inst->params.plist && inst->params.plist->speed > 0) {
        // PList speed is in 50Hz ticks (PAL timing)
        // Keep it as-is, but we'll decrement the wait counter by SpeedMultiplier each frame
        inst->perf_speed = inst->params.plist->speed;
        inst->perf_wait = 0;  // Apply entry 0 immediately, then wait for entry 1
    } else {
        inst->perf_speed = 1;
        inst->perf_wait = 0;
    }
    inst->perf_sub_volume = 64;
    inst->period_perf_slide_speed = 0;
    inst->period_perf_slide_period = 0;
    inst->period_perf_slide_on = false;

    // Set speed multiplier in voice for ADSR timing
    inst->voice.SpeedMultiplier = (inst->params.speed_multiplier > 0) ? inst->params.speed_multiplier : 1;

    // Use authentic AHX synthesis core with MIDI note (will be overridden by PList)
    ahx_synth_voice_note_on(&inst->voice, note, velocity, sample_rate);

    // If PList exists, override the period immediately with PList note
    if (inst->params.plist && inst->params.plist->length > 0) {
        AhxPListEntry* first_entry = &inst->params.plist->entries[0];
        if (first_entry->note > 0) {
            // PList note is already an AHX note index (1-60), not MIDI
            inst->voice.InstrPeriod = first_entry->note;
            inst->voice.FixedNote = first_entry->fixed;

            // Recalculate period immediately and reapply to voice playback
            inst->voice.VoicePeriod = ahx_synth_get_period_for_note(first_entry->note);

            tracker_voice_set_period(&inst->voice.voice_playback, inst->voice.VoicePeriod,
                                    AMIGA_PAULA_PAL_CLK, sample_rate);
        }
    }

    inst->note = note;  // Store original MIDI note for reference
    inst->velocity = velocity;
    inst->active = true;
    inst->released = false;

    // PList entry 0 will be applied on the first process_frame call (no manual application)
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
    if (!inst || !output) {
        if (output) {
            memset(output, 0, num_samples * sizeof(float));
        }
        return 0;
    }

    if (!inst->voice.TrackOn) {
        memset(output, 0, num_samples * sizeof(float));
        inst->active = false;
        return 0;
    }

    // Process each sample with proper 50Hz frame timing
    for (uint32_t i = 0; i < num_samples; i++) {
        // Check if we need to process a frame (50Hz timing)
        if (inst->voice.samples_in_frame >= inst->voice.samples_per_frame) {
            // Process PList logic + synthesis frame
            ahx_instrument_process_frame(inst);
            inst->voice.samples_in_frame = 0;

            // Update voice period if changed
            tracker_voice_set_period(&inst->voice.voice_playback, inst->voice.VoicePeriod,
                                    AMIGA_PAULA_PAL_CLK, sample_rate);
        }
        inst->voice.samples_in_frame++;

        // Get raw sample from voice playback
        int32_t sample = tracker_voice_get_sample(&inst->voice.voice_playback);

        // Apply volume and convert to float
        float sample_f = sample / 32768.0f;
        float vol = (inst->voice.VoiceVolume / 64.0f) * 0.5f;
        output[i] = sample_f * vol;

        if (!inst->voice.TrackOn) {
            // Voice stopped - clear remaining buffer
            if (i + 1 < num_samples) {
                memset(&output[i + 1], 0, (num_samples - i - 1) * sizeof(float));
            }
            inst->active = false;
            return i + 1;
        }
    }

    // Update active state
    inst->active = ahx_synth_voice_is_active(&inst->voice);

    return num_samples;
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

// PList command wrapper - calls shared implementation
static void plist_command_parse(AhxInstrument* inst, AhxSynthVoice* voice, uint8_t fx, uint8_t fx_param) {
    if (!inst || !voice) return;

    // Map modulator fields to individual variables for PList execution
    int square_init = voice->square_mod.init_pending ? 1 : 0;
    int square_on = voice->square_mod.active ? 1 : 0;
    int square_sign = voice->square_mod.sign;
    int filter_init = voice->filter_mod.init_pending ? 1 : 0;
    int filter_on = voice->filter_mod.active ? 1 : 0;
    int filter_sign = voice->filter_mod.sign;
    int square_pos = voice->square_mod.position;
    int filter_pos = voice->filter_mod.position;
    int period_perf_slide_on = inst->period_perf_slide_on ? 1 : 0;

    // Use shared PList command executor
    ahx_plist_execute_command(
        fx,
        fx_param,
        0,  // song_revision = 0 for synth mode (always apply filter pos)
        // Filter control
        &filter_pos,
        &voice->IgnoreFilter,
        &voice->NewWaveform,
        // Square modulation
        &square_pos,
        &voice->IgnoreSquare,
        &voice->WaveLength,
        &square_init,
        &square_on,
        &square_sign,
        // Filter modulation
        &filter_init,
        &filter_on,
        &filter_sign,
        // Volume control
        &voice->NoteMaxVolume,
        &voice->PerfSubVolume,
        &voice->TrackMasterVolume,
        // PList control
        &inst->perf_current,
        &inst->perf_speed,
        &inst->perf_wait,
        // Portamento
        &inst->period_perf_slide_speed,
        &period_perf_slide_on,
        // Note state
        inst->released ? 1 : 0  // Prevent PList jumps after note-off
    );

    // Map results back to modulator fields
    voice->square_mod.init_pending = square_init != 0;
    voice->square_mod.active = square_on != 0;
    voice->square_mod.sign = square_sign;
    voice->filter_mod.init_pending = filter_init != 0;
    voice->filter_mod.active = filter_on != 0;
    voice->filter_mod.sign = filter_sign;
    voice->square_mod.position = square_pos;
    voice->filter_mod.position = filter_pos;
    inst->period_perf_slide_on = period_perf_slide_on != 0;
}

// Process frame (deprecated - now handled by ahx_synth_core)
void ahx_instrument_process_frame(AhxInstrument* inst) {
    if (!inst) return;

    // Update PList active state (keeps voice alive even after envelope finishes)
    inst->voice.PListActive = (inst->params.plist && inst->perf_current < inst->params.plist->length);

    // Process PList if active
    if (inst->voice.PListActive) {
        // Decrement wait normally - SpeedMultiplier affects frame rate, not counters
        if (--inst->perf_wait <= 0) {
            uint8_t cur = inst->perf_current++;
            AhxPListEntry* entry = &inst->params.plist->entries[cur];
            inst->perf_wait = inst->perf_speed;

            // Apply waveform change
            if (entry->waveform > 0) {
                inst->voice.Waveform = entry->waveform - 1;  // 1-4 maps to 0-3
                inst->voice.NewWaveform = 1;
                inst->period_perf_slide_speed = inst->period_perf_slide_period = 0;

                // Initialize square modulation when switching to square waveform
                if (inst->voice.Waveform == 2 && !tracker_modulator_is_active(&inst->voice.square_mod)) {
                    // Enable square modulation automatically
                    tracker_modulator_set_active(&inst->voice.square_mod, true);
                    inst->voice.SquarePos = tracker_modulator_get_position(&inst->voice.square_mod);
                }
            }

            // Reset portamento flag (will be set by commands if needed)
            inst->period_perf_slide_on = false;

            // Execute FX commands
            for (int i = 0; i < 2; i++) {
                plist_command_parse(inst, &inst->voice, entry->fx[i], entry->fx_param[i]);
            }

            // Apply note change
            if (entry->note > 0) {
                inst->voice.InstrPeriod = entry->note;
                inst->voice.PlantPeriod = 1;
                inst->voice.FixedNote = entry->fixed;
            }
        }
    } else {
        // PList finished - decay portamento
        if (inst->perf_wait) {
            inst->perf_wait--;
            if (inst->perf_wait < 0) inst->perf_wait = 0;
        } else {
            inst->period_perf_slide_speed = 0;
        }
    }

    // Apply PList portamento
    if (inst->period_perf_slide_on) {
        inst->period_perf_slide_period -= inst->period_perf_slide_speed;
        if (inst->period_perf_slide_period) {
            inst->voice.PlantPeriod = 1;
        }
    }

    // Process core synthesis frame
    ahx_synth_voice_process_frame(&inst->voice);
}
