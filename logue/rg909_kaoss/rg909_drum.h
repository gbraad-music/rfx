#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef struct RG909Drum RG909Drum;

// Create/destroy
RG909Drum* rg909_drum_create(void);
void rg909_drum_destroy(RG909Drum* drum);

// Parameters
void rg909_drum_set_kick_density(RG909Drum* drum, float density);
void rg909_drum_set_snare_variation(RG909Drum* drum, float variation);
void rg909_drum_set_tone(RG909Drum* drum, float tone);
void rg909_drum_set_mix(RG909Drum* drum, float mix);

// Processing
void rg909_drum_process(RG909Drum* drum, const float* in_l, const float* in_r,
                        float* out_l, float* out_r, int sample_rate);

// Tempo
void rg909_drum_set_tempo(RG909Drum* drum, float bpm);

#ifdef __cplusplus
}
#endif
