/*
 * AHX Performance List (PList) - Shared Command Execution
 *
 * Generic PList command executor that works with any voice structure.
 * Used by both ahx_player and ahx_instrument.
 */

#ifndef AHX_PLIST_H
#define AHX_PLIST_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Execute a single PList command
 *
 * Commands:
 *   0: Set Filter Position
 *   1: Portamento Up
 *   2: Portamento Down
 *   3: Init Square Modulation
 *   4: Toggle Square/Filter Modulation
 *   5: Jump to PList Step
 *   6: Set Volume (Note/PerfSub/TrackMaster)
 *   7: Set Speed
 *
 * @param fx Effect command (0-7)
 * @param fx_param Effect parameter (0-255)
 * @param song_revision AHX song revision (0 = always apply filter, >0 = check fx_param)
 * @param filter_pos Pointer to filter position
 * @param ignore_filter Pointer to ignore filter flag
 * @param new_waveform Pointer to new waveform flag
 * @param square_pos Pointer to square position
 * @param ignore_square Pointer to ignore square flag
 * @param wave_length Pointer to wave length (0-5)
 * @param square_init Pointer to square init flag
 * @param square_on Pointer to square on/off state
 * @param square_sign Pointer to square sign
 * @param filter_init Pointer to filter init flag
 * @param filter_on Pointer to filter on/off state
 * @param filter_sign Pointer to filter sign
 * @param note_max_volume Pointer to note max volume
 * @param perf_sub_volume Pointer to performance sub volume
 * @param track_master_volume Pointer to track master volume
 * @param perf_current Pointer to PList current position
 * @param perf_speed Pointer to PList speed
 * @param perf_wait Pointer to PList wait counter
 * @param period_perf_slide_speed Pointer to portamento speed
 * @param period_perf_slide_on Pointer to portamento active flag
 */
void ahx_plist_execute_command(
    uint8_t fx,
    uint8_t fx_param,
    int song_revision,
    // Filter control
    int* filter_pos,
    int* ignore_filter,
    int* new_waveform,
    // Square modulation
    int* square_pos,
    int* ignore_square,
    int* wave_length,
    int* square_init,
    int* square_on,
    int* square_sign,
    // Filter modulation
    int* filter_init,
    int* filter_on,
    int* filter_sign,
    // Volume control
    int* note_max_volume,
    int* perf_sub_volume,
    int* track_master_volume,
    // PList control
    int* perf_current,
    int* perf_speed,
    int* perf_wait,
    // Portamento
    int* period_perf_slide_speed,
    int* period_perf_slide_on
);

#ifdef __cplusplus
}
#endif

#endif // AHX_PLIST_H
