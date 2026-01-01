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

        // Calculate RMS amplitude for this frame
        float sum_squares = 0.0f;
        for (size_t i = frame_start; i < frame_end; i++) {
            sum_squares += audio_data[i] * audio_data[i];
        }
        float rms = sqrtf(sum_squares / (frame_end - frame_start));
        waveform_out[frame].amplitude = rms;

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

        // Calculate RMS amplitude for this frame
        float sum_squares = 0.0f;
        for (size_t i = frame_start; i < frame_end; i++) {
            sum_squares += audio_data[i] * audio_data[i];
        }
        float rms = sqrtf(sum_squares / (frame_end - frame_start));
        waveform_out[frame].amplitude = rms;

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

float detect_bpm(
    const float* audio_data,
    size_t num_samples,
    int sample_rate
) {
    if (!audio_data || num_samples == 0) {
        return 0.0f;
    }

#ifdef USE_AUBIO
    // Create aubio tempo detector
    aubio_tempo_t *tempo = new_aubio_tempo("default", AUBIO_WIN_SIZE, AUBIO_HOP_SIZE, sample_rate);
    if (!tempo) {
        return 0.0f;
    }

    // Create input buffer
    fvec_t *input_buf = new_fvec(AUBIO_HOP_SIZE);
    fvec_t *output = new_fvec(1);

    if (!input_buf || !output) {
        if (tempo) del_aubio_tempo(tempo);
        if (input_buf) del_fvec(input_buf);
        if (output) del_fvec(output);
        return 0.0f;
    }

    // Process audio in chunks
    for (size_t pos = 0; pos + AUBIO_HOP_SIZE <= num_samples; pos += AUBIO_HOP_SIZE) {
        // Copy samples to aubio buffer
        for (size_t i = 0; i < AUBIO_HOP_SIZE; i++) {
            input_buf->data[i] = audio_data[pos + i];
        }

        // Process chunk
        aubio_tempo_do(tempo, input_buf, output);
    }

    // Get detected BPM
    float bpm = aubio_tempo_get_bpm(tempo);

    // Cleanup
    del_aubio_tempo(tempo);
    del_fvec(input_buf);
    del_fvec(output);

    return bpm;

#else
    // No BPM detection without aubio
    return 0.0f;
#endif
}
