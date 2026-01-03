#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef struct RG404Drum RG404Drum;

// Create/destroy
RG404Drum* rg404_drum_create(void);
void rg404_drum_destroy(RG404Drum* drum);

// Parameters
void rg404_drum_set_kick_density(RG404Drum* drum, float density);
void rg404_drum_set_snare_variation(RG404Drum* drum, float variation);
void rg404_drum_set_mix(RG404Drum* drum, float mix);
void rg404_drum_set_drive(RG404Drum* drum, float drive);

// Processing
void rg404_drum_process(RG404Drum* drum, const float* in_l, const float* in_r,
                        float* out_l, float* out_r, int sample_rate);

// Tempo
void rg404_drum_set_tempo(RG404Drum* drum, float bpm);

#ifdef __cplusplus
}
#endif
