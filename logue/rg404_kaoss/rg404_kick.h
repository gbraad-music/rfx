#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef struct RG404Kick RG404Kick;

// Create/destroy
RG404Kick* rg404_kick_create(void);
void rg404_kick_destroy(RG404Kick* kick);

// Parameters
void rg404_kick_set_rhythm(RG404Kick* kick, float rhythm);
void rg404_kick_set_mix(RG404Kick* kick, float mix);

// Processing
void rg404_kick_process(RG404Kick* kick, const float* in_l, const float* in_r,
                        float* out_l, float* out_r, int sample_rate);

// Tempo
void rg404_kick_set_tempo(RG404Kick* kick, float bpm);

#ifdef __cplusplus
}
#endif
