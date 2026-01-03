#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef struct RG303Bass RG303Bass;

// Create/destroy
RG303Bass* rg303_bass_create(void);
void rg303_bass_destroy(RG303Bass* bass);

// Parameters
void rg303_bass_set_filter(RG303Bass* bass, float cutoff);       // 0.0-1.0: filter cutoff/resonance
void rg303_bass_set_pattern(RG303Bass* bass, float variation);   // 0.0-1.0: pattern variation
void rg303_bass_set_slide(RG303Bass* bass, float slide);         // 0.0-1.0: slide/portamento amount
void rg303_bass_set_mix(RG303Bass* bass, float mix);

// Processing
void rg303_bass_process(RG303Bass* bass, const float* in_l, const float* in_r,
                        float* out_l, float* out_r, int sample_rate);

// Tempo
void rg303_bass_set_tempo(RG303Bass* bass, float bpm);

#ifdef __cplusplus
}
#endif
