#include "rg303_bass.h"
#include <stdlib.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

// Simple one-pole filter state
typedef struct {
    float z1;  // Previous output
} OnePoleFilter;

struct RG303Bass {
    float global_tempo_bpm;
    float filter_cutoff;     // 0.0 = closed, 1.0 = open
    float pattern_variation; // 0.0 = no notes, 1.0 = full pattern
    float slide_amount;      // 0.0 = no slide, 1.0 = full portamento
    float bass_mix;
    float input_mix;

    float metro_phase;
    float metro_increment;
    int step_count;          // 16th note counter (0-15, one bar)

    // Oscillator state
    int note_active;
    float osc_phase;         // Sawtooth phase (0 to 1)
    float current_freq;      // Current note frequency (with slide)
    float target_freq;       // Target frequency for slide
    float previous_freq;     // Previous note frequency for slide

    // Envelope state
    float env_time;
    float env_value;

    // Filter state
    OnePoleFilter filter;
    float filter_z1;         // For resonance feedback

    // Pattern state
    int current_note;        // MIDI note number
    int previous_note;       // Previous MIDI note for slide
    int is_accent;           // Whether current note is accented
    int has_slide;           // Whether current note has slide
};

// "Happy Birthday" melody as bass pattern
// Using MIDI note numbers: C2=36, D2=38, E2=40, F2=41, G2=43
// Pattern: C C D C F E - C C D C G F (first two phrases)
static const int PATTERN_NOTES[16] = {
    36, 0, 36, 38,   // C2, rest, C2, D2    (Ha-ppy birth-)
    36, 0, 41, 0,    // C2, rest, F2, rest  (-day to)
    40, 0, 36, 0,    // E2, rest, C2, rest  (you, Ha-)
    36, 38, 36, 43   // C2, D2, C2, G2      (-ppy birth-day to)
};

static const int PATTERN_ACCENTS[16] = {
    1, 0, 0, 0,    // Accent on first "Ha-"
    0, 0, 1, 0,    // Accent on "to"
    0, 0, 1, 0,    // Accent on "you"
    0, 0, 0, 1     // Accent on final "to"
};

// Slide pattern - add some acid weirdness to the birthday tune
static const int PATTERN_SLIDES[16] = {
    0, 0, 0, 1,    // Slide to D2
    0, 0, 1, 0,    // Slide to F2
    0, 0, 0, 0,    // No slide
    0, 0, 0, 1     // Slide to G2
};

// Convert MIDI note to frequency
static float midi_to_freq(int note) {
    return 440.0f * powf(2.0f, (note - 69) / 12.0f);
}

// Simple one-pole low-pass filter
static float filter_process(OnePoleFilter* flt, float input, float cutoff, float resonance) {
    float output = flt->z1 + cutoff * (input - flt->z1 + resonance * (input - flt->z1));
    flt->z1 = output;
    return output;
}

RG303Bass* rg303_bass_create(void)
{
    RG303Bass* bass = (RG303Bass*)malloc(sizeof(RG303Bass));
    if (!bass) return NULL;

    bass->global_tempo_bpm = 120.0f;
    bass->filter_cutoff = 0.3f;       // Moderate filter by default
    bass->pattern_variation = 0.5f;   // Some notes by default
    bass->slide_amount = 0.5f;        // Moderate slide by default
    bass->bass_mix = 0.7f;
    bass->input_mix = 0.3f;

    bass->metro_phase = 0.0f;
    bass->metro_increment = 0.0f;
    bass->step_count = 0;

    bass->note_active = 0;
    bass->osc_phase = 0.0f;
    bass->current_freq = 0.0f;
    bass->target_freq = 0.0f;
    bass->previous_freq = 0.0f;

    bass->env_time = 0.0f;
    bass->env_value = 0.0f;

    bass->filter.z1 = 0.0f;
    bass->filter_z1 = 0.0f;

    bass->current_note = 0;
    bass->previous_note = 0;
    bass->is_accent = 0;
    bass->has_slide = 0;

    // Calculate initial metro increment (16th notes = 4x beat rate)
    float beats_per_second = bass->global_tempo_bpm / 60.0f;
    float sixteenths_per_second = beats_per_second * 4.0f;
    bass->metro_increment = sixteenths_per_second / 48000.0f;

    return bass;
}

void rg303_bass_destroy(RG303Bass* bass)
{
    if (bass) {
        free(bass);
    }
}

void rg303_bass_set_filter(RG303Bass* bass, float cutoff)
{
    if (!bass) return;
    bass->filter_cutoff = cutoff;
}

void rg303_bass_set_pattern(RG303Bass* bass, float variation)
{
    if (!bass) return;
    bass->pattern_variation = variation;
}

void rg303_bass_set_slide(RG303Bass* bass, float slide)
{
    if (!bass) return;
    bass->slide_amount = slide;
}

void rg303_bass_set_mix(RG303Bass* bass, float mix)
{
    if (!bass) return;
    bass->bass_mix = mix;
}

void rg303_bass_set_tempo(RG303Bass* bass, float bpm)
{
    if (!bass) return;
    bass->global_tempo_bpm = bpm;

    if (bpm < 30.0f) bpm = 30.0f;
    if (bpm > 300.0f) bpm = 300.0f;
    float beats_per_second = bpm / 60.0f;
    float sixteenths_per_second = beats_per_second * 4.0f;
    bass->metro_increment = sixteenths_per_second / 48000.0f;
}

void rg303_bass_process(RG303Bass* bass, const float* in_l, const float* in_r,
                        float* out_l, float* out_r, int sample_rate)
{
    if (!bass) return;

    // Metro - runs at 16th note resolution
    bass->metro_phase += bass->metro_increment;
    if (bass->metro_phase >= 1.0f) {
        bass->metro_phase -= 1.0f;

        // Determine which notes to play based on pattern_variation
        int should_trigger = 0;

        if (bass->pattern_variation > 0.01f) {
            int note = PATTERN_NOTES[bass->step_count];

            if (note > 0) {  // 0 = rest
                // Pattern variation controls note density
                if (bass->pattern_variation > 0.8f) {
                    // Play all notes
                    should_trigger = 1;
                } else if (bass->pattern_variation > 0.5f) {
                    // Play most notes (skip some rests)
                    if (bass->step_count % 4 != 3) {
                        should_trigger = 1;
                    }
                } else if (bass->pattern_variation > 0.2f) {
                    // Play main notes (steps 0, 2, 4, 6, 8, 10, 12, 14)
                    if (bass->step_count % 2 == 0) {
                        should_trigger = 1;
                    }
                } else {
                    // Sparse pattern - only strong beats
                    if (bass->step_count == 0 || bass->step_count == 8) {
                        should_trigger = 1;
                    }
                }
            }
        }

        if (should_trigger) {
            bass->note_active = 1;
            bass->env_time = 0.0f;

            // Store previous note for slide
            bass->previous_note = bass->current_note;
            bass->previous_freq = bass->target_freq;

            // Set new note
            bass->current_note = PATTERN_NOTES[bass->step_count];
            bass->is_accent = PATTERN_ACCENTS[bass->step_count];
            bass->has_slide = PATTERN_SLIDES[bass->step_count];

            // Calculate target frequency
            bass->target_freq = midi_to_freq(bass->current_note);

            // If slide is enabled and this note has slide flag, start from previous freq
            if (bass->has_slide && bass->slide_amount > 0.01f && bass->previous_freq > 0.0f) {
                bass->current_freq = bass->previous_freq;  // Start at previous note
            } else {
                bass->current_freq = bass->target_freq;  // Jump immediately
            }
        }

        // Advance step counter (16 steps = 1 bar)
        bass->step_count++;
        if (bass->step_count >= 16) {
            bass->step_count = 0;
        }
    }

    // Generate bass output
    float bass_out = 0.0f;

    if (bass->note_active) {
        const float note_duration = 0.45f;  // 450ms per note (longer, more legato)

        if (bass->env_time < note_duration) {
            // Slide/portamento - glide from current_freq to target_freq
            if (bass->has_slide && bass->slide_amount > 0.01f) {
                // Slide time controlled by slide_amount (20ms to 80ms)
                float slide_time = 0.02f + (bass->slide_amount * 0.06f);

                if (bass->env_time < slide_time) {
                    // Linear interpolation from current to target
                    float t = bass->env_time / slide_time;
                    bass->current_freq = bass->previous_freq + (bass->target_freq - bass->previous_freq) * t;
                } else {
                    bass->current_freq = bass->target_freq;
                }
            }

            // Generate sawtooth wave
            bass->osc_phase += bass->current_freq / (float)sample_rate;
            if (bass->osc_phase >= 1.0f) bass->osc_phase -= 1.0f;

            // Sawtooth: ramp from -1 to +1
            float saw = (bass->osc_phase * 2.0f) - 1.0f;

            // Amplitude envelope - quick attack, exponential decay
            float amp_env = 1.0f - (bass->env_time / note_duration);
            amp_env = amp_env * amp_env;  // Quadratic decay

            // Filter envelope - starts high, decays to cutoff value
            // TB-303 style: envelope modulates filter cutoff
            float filter_env = 1.0f - (bass->env_time / 0.08f);  // 80ms filter envelope (longer)
            if (filter_env < 0.0f) filter_env = 0.0f;
            filter_env = filter_env * filter_env;  // Exponential curve

            // Calculate filter cutoff with envelope
            float cutoff = bass->filter_cutoff + filter_env * (1.0f - bass->filter_cutoff);

            // Resonance based on filter cutoff (more resonance = more squelch)
            float resonance = bass->filter_cutoff * 0.8f;

            // Apply resonant low-pass filter
            bass_out = filter_process(&bass->filter, saw, cutoff * 0.5f, resonance);

            // Apply amplitude envelope and accent
            float accent_mult = bass->is_accent ? 1.5f : 1.0f;  // Accents are 50% louder
            bass_out *= amp_env * 0.5f * accent_mult;

            bass->env_time += 1.0f / (float)sample_rate;
        } else {
            bass->note_active = 0;
        }
    }

    // Mix bass with input
    *out_l = (*in_l) * bass->input_mix + bass_out * bass->bass_mix;
    *out_r = (*in_r) * bass->input_mix + bass_out * bass->bass_mix;
}
