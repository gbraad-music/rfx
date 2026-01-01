#include "audio_analysis.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifdef USE_AUBIO
#include <aubio.h>
#endif

bool analyze_audio_waveform(
    const float* audio_data,
    size_t num_samples,
    int sample_rate,
    WaveformFrame* waveform_out,
    size_t num_frames,
    ProgressCallback progress_cb,
    void* user_data
) {
    if (!audio_data || !waveform_out || num_samples == 0 || num_frames == 0) {
        return false;
    }

#ifdef USE_AUBIO
    // Create aubio phase vocoder for FFT analysis
    aubio_pvoc_t *pv = new_aubio_pvoc(AUBIO_WIN_SIZE, AUBIO_HOP_SIZE);
    if (!pv) {
        return false;
    }

    // Create input and FFT output buffers
    fvec_t *input_buf = new_fvec(AUBIO_HOP_SIZE);
    cvec_t *fft_buf = new_cvec(AUBIO_WIN_SIZE);

    if (!input_buf || !fft_buf) {
        if (pv) del_aubio_pvoc(pv);
        if (input_buf) del_fvec(input_buf);
        if (fft_buf) del_cvec(fft_buf);
        return false;
    }

    // Process audio in chunks to generate waveform frames
    const size_t samples_per_frame = WAVEFORM_DOWNSAMPLE;

    for (size_t frame = 0; frame < num_frames; frame++) {
        size_t frame_start = frame * samples_per_frame;
        size_t frame_end = frame_start + samples_per_frame;

        if (frame_end > num_samples) {
            frame_end = num_samples;
        }

        // Calculate peak amplitude for this frame (better for DJ visualization than RMS)
        float peak = 0.0f;
        for (size_t i = frame_start; i < frame_end; i++) {
            float abs_val = fabsf(audio_data[i]);
            if (abs_val > peak) peak = abs_val;
        }
        waveform_out[frame].amplitude = peak;

        // Perform FFT analysis on this frame
        SpectralBands bands = {0.0f, 0.0f, 0.0f};

        // Process in smaller hop-sized chunks for FFT
        for (size_t pos = frame_start; pos < frame_end && pos + AUBIO_HOP_SIZE <= num_samples; pos += AUBIO_HOP_SIZE) {
            // Copy samples to aubio buffer
            for (size_t i = 0; i < AUBIO_HOP_SIZE; i++) {
                input_buf->data[i] = audio_data[pos + i];
            }

            // Perform FFT
            aubio_pvoc_do(pv, input_buf, fft_buf);

            // Analyze spectral bands
            for (uint_t k = 0; k < fft_buf->length; k++) {
                float magnitude = fft_buf->norm[k];
                float freq = (float)k * sample_rate / AUBIO_WIN_SIZE;

                if (freq < 200.0f) {
                    bands.low += magnitude;
                } else if (freq < 2000.0f) {
                    bands.mid += magnitude;
                } else {
                    bands.high += magnitude;
                }
            }
        }

        // Normalize spectral bands
        float total_energy = bands.low + bands.mid + bands.high;
        if (total_energy > 0.0f) {
            bands.low /= total_energy;
            bands.mid /= total_energy;
            bands.high /= total_energy;
        }

        waveform_out[frame].bands = bands;

        // Report progress
        if (progress_cb && (frame % 100 == 0 || frame == num_frames - 1)) {
            float progress = (float)(frame + 1) / num_frames;
            progress_cb(progress, user_data);
        }
    }

    // Cleanup
    del_aubio_pvoc(pv);
    del_fvec(input_buf);
    del_cvec(fft_buf);

    return true;

#else
    // Fallback implementation without aubio - just calculate amplitude
    const size_t samples_per_frame = WAVEFORM_DOWNSAMPLE;

    for (size_t frame = 0; frame < num_frames; frame++) {
        size_t frame_start = frame * samples_per_frame;
        size_t frame_end = frame_start + samples_per_frame;

        if (frame_end > num_samples) {
            frame_end = num_samples;
        }

        // Calculate peak amplitude for this frame (better for DJ visualization than RMS)
        float peak = 0.0f;
        for (size_t i = frame_start; i < frame_end; i++) {
            float abs_val = fabsf(audio_data[i]);
            if (abs_val > peak) peak = abs_val;
        }
        waveform_out[frame].amplitude = peak;

        // No spectral analysis without aubio
        waveform_out[frame].bands.low = 0.0f;
        waveform_out[frame].bands.mid = 0.0f;
        waveform_out[frame].bands.high = 0.0f;

        // Report progress
        if (progress_cb && (frame % 100 == 0 || frame == num_frames - 1)) {
            float progress = (float)(frame + 1) / num_frames;
            progress_cb(progress, user_data);
        }
    }

    return true;
#endif
}

// Shared tempo detector result structure (for BPM and first beat)
typedef struct {
    float bpm;
    size_t first_beat;
    size_t num_beats_found;  // Track how many beats were detected
} TempoResult;

// Try a single aubio detection method and return number of beats found
static size_t try_aubio_method(
    const char* method,
    const float* audio_data,
    size_t num_samples,
    int sample_rate,
    size_t max_samples,
    size_t* beat_positions,
    size_t max_beats
) {
    aubio_tempo_t *tempo = new_aubio_tempo(method, AUBIO_WIN_SIZE, AUBIO_HOP_SIZE, sample_rate);
    if (!tempo) {
        return 0;
    }

    fvec_t *input_buf = new_fvec(AUBIO_HOP_SIZE);
    fvec_t *output = new_fvec(1);

    if (!input_buf || !output) {
        if (tempo) del_aubio_tempo(tempo);
        if (input_buf) del_fvec(input_buf);
        if (output) del_fvec(output);
        return 0;
    }

    size_t num_beats = 0;
    for (size_t pos = 0; pos + AUBIO_HOP_SIZE <= max_samples; pos += AUBIO_HOP_SIZE) {
        for (size_t i = 0; i < AUBIO_HOP_SIZE; i++) {
            input_buf->data[i] = audio_data[pos + i];
        }

        aubio_tempo_do(tempo, input_buf, output);

        if (output->data[0] != 0) {
            size_t beat_pos = aubio_tempo_get_last(tempo);
            if (num_beats < max_beats) {
                beat_positions[num_beats++] = beat_pos;
                if (num_beats >= 100) {
                    break;  // Early exit
                }
            }
        }
    }

    del_aubio_tempo(tempo);
    del_fvec(input_buf);
    del_fvec(output);

    return num_beats;
}

static TempoResult detect_tempo_and_beats(
    const float* audio_data,
    size_t num_samples,
    int sample_rate
) {
    TempoResult result = {0.0f, 0, 0};

    if (!audio_data || num_samples == 0) {
        return result;
    }

#ifdef USE_AUBIO
    // Limit to first 3 minutes of audio to avoid excessive processing on long files
    const size_t max_samples_to_analyze = (size_t)(3 * 60 * sample_rate);  // 3 minutes
    const size_t samples_to_analyze = (num_samples < max_samples_to_analyze) ? num_samples : max_samples_to_analyze;

    size_t* beat_positions = malloc(sizeof(size_t) * 1000);
    size_t* best_positions = malloc(sizeof(size_t) * 1000);
    if (!beat_positions || !best_positions) {
        if (beat_positions) free(beat_positions);
        if (best_positions) free(best_positions);
        return result;
    }

    // Try multiple detection methods and pick the one that finds the most beats
    const char* methods[] = {"default", "specflux", "hfc", "complex"};
    size_t num_methods = sizeof(methods) / sizeof(methods[0]);
    size_t best_beat_count = 0;

    for (size_t m = 0; m < num_methods; m++) {
        size_t num_beats = try_aubio_method(
            methods[m],
            audio_data,
            num_samples,
            sample_rate,
            samples_to_analyze,
            beat_positions,
            1000
        );

        // Keep the result with the most beats detected
        if (num_beats > best_beat_count) {
            best_beat_count = num_beats;
            memcpy(best_positions, beat_positions, num_beats * sizeof(size_t));
        }

        // If we found enough beats, no need to try other methods
        if (best_beat_count >= 50) {
            break;
        }
    }

    size_t num_beats = best_beat_count;
    memcpy(beat_positions, best_positions, num_beats * sizeof(size_t));
    free(best_positions);

    result.num_beats_found = num_beats;

    // Robust beat grid alignment using multiple beats
    if (num_beats >= 4) {
        // Skip first 1-2 beats - they're often noisy (Mixxx does this too)
        size_t beats_to_skip = (num_beats > 10) ? 2 : 1;

        // Calculate BPM from TOTAL TIME SPAN (first to last beat)
        // This is more accurate than median spacing and prevents drift!
        // Formula: BPM = (num_intervals * 60 * sample_rate) / total_samples
        size_t first_beat_pos = beat_positions[beats_to_skip];
        size_t last_beat_pos = beat_positions[num_beats - 1];
        size_t total_samples = last_beat_pos - first_beat_pos;
        size_t num_intervals = (num_beats - 1) - beats_to_skip;  // Number of gaps between beats

        double avg_spacing = (double)total_samples / num_intervals;
        result.bpm = (60.0 * sample_rate) / avg_spacing;

        // Find best offset by testing different phases
            // Use avg_spacing for grid (accurate), median_spacing for scoring (robust)
            double best_offset = 0;
            double best_score = 0;
            int test_steps = 400;  // Increased resolution for better accuracy

            for (int step = 0; step < test_steps; step++) {
                double test_offset = (avg_spacing * step) / test_steps;
                double score = 0;

                // Score based on how close detected beats are to grid positions
                // Use beats after skipping the noisy ones
                for (size_t i = beats_to_skip; i < num_beats; i++) {
                    double beat_phase = fmod(beat_positions[i] - test_offset + avg_spacing, avg_spacing);
                    // Distance to nearest grid line
                    double distance = fmin(beat_phase, avg_spacing - beat_phase);
                    // Score: closer = higher (exponential weighting for precision)
                    double normalized_distance = distance / avg_spacing;
                    score += 1.0 / (1.0 + normalized_distance * normalized_distance * 100.0);
                }

                if (score > best_score) {
                    best_score = score;
                    best_offset = test_offset;
                }
            }

            // CRITICAL FIX (from Mixxx): If track starts very close to 50% between beats,
            // the actual first beat is likely at that position (common off-by-half-beat error)
            double first_sample_phase = fmod(best_offset, avg_spacing) / avg_spacing;
            if (first_sample_phase > 0.4 && first_sample_phase < 0.6) {
                // Track starts near halfway between beats - shift grid by half beat
                best_offset += avg_spacing * 0.5;
                if (best_offset >= avg_spacing) {
                    best_offset -= avg_spacing;
                }
            }

            // Ensure offset is within one beat period
            while (best_offset >= avg_spacing) {
                best_offset -= avg_spacing;
            }
            while (best_offset < 0) {
                best_offset += avg_spacing;
            }

            result.first_beat = (size_t)best_offset;
            free(spacings);
        } else {
            // Memory allocation failed, use simple spacing from first two beats
            if (num_beats >= 2) {
                double spacing = (double)(beat_positions[1] - beat_positions[0]);
                result.bpm = (60.0 * sample_rate) / spacing;
            }
            result.first_beat = beat_positions[beats_to_skip];
        }
    } else if (num_beats >= 2) {
        // Not enough beats for robust analysis, calculate from first two beats
        double spacing = (double)(beat_positions[1] - beat_positions[0]);
        result.bpm = (60.0 * sample_rate) / spacing;
        result.first_beat = beat_positions[0];
    } else if (num_beats == 1) {
        // Only one beat - can't calculate BPM
        result.bpm = 0.0f;
        result.first_beat = beat_positions[0];
    } else {
        // No beats detected at all
        result.bpm = 0.0f;
        result.first_beat = 0;
    }

    free(beat_positions);

    return result;

#else
    return result;
#endif
}

float detect_bpm(
    const float* audio_data,
    size_t num_samples,
    int sample_rate
) {
    TempoResult result = detect_tempo_and_beats(audio_data, num_samples, sample_rate);
    return result.bpm;
}

size_t detect_first_beat(
    const float* audio_data,
    size_t num_samples,
    int sample_rate
) {
    TempoResult result = detect_tempo_and_beats(audio_data, num_samples, sample_rate);
    return result.first_beat;
}
