/*
 * AHX Performance List (PList) - Shared Command Execution
 */

#include "ahx_plist.h"
#include <stdio.h>

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
    int* period_perf_slide_on,
    // Note state
    int note_off
) {
    switch (fx) {
        case 0: // Set Filter Position
            if (fx_param != 0) {
                // For song_revision > 0: check ignore_filter flag
                // For synth mode (song_revision == 0): always apply
                if (song_revision > 0 && ignore_filter && *ignore_filter) {
                    if (filter_pos) {
                        *filter_pos = *ignore_filter;
                    }
                    *ignore_filter = 0;
                } else {
                    if (filter_pos) {
                        *filter_pos = fx_param;
                    }
                }
                if (new_waveform) *new_waveform = 1;
            }
            break;

        case 1: // Portamento Up
            if (period_perf_slide_speed) *period_perf_slide_speed = fx_param;
            if (period_perf_slide_on) *period_perf_slide_on = 1;
            break;

        case 2: // Portamento Down
            if (period_perf_slide_speed) *period_perf_slide_speed = -fx_param;
            if (period_perf_slide_on) *period_perf_slide_on = 1;
            break;

        case 3: // Init Square Modulation Position
            if (ignore_square && wave_length && square_pos) {
                if (!*ignore_square) {
                    *square_pos = fx_param >> (5 - *wave_length);
                } else {
                    *ignore_square = 0;
                }
            }
            break;

        case 4: // Toggle Square/Filter Modulation
            if (fx_param == 0) {
                // fx_param=0: toggle square only (old AHX behavior)
                if (square_on && square_init && square_sign) {
                    *square_init = (*square_on ^= 1);
                    *square_sign = 1;
                }
            } else {
                // fx_param != 0: check nibbles for square/filter
                if (fx_param & 0x0f) {
                    if (square_on && square_init && square_sign) {
                        *square_init = (*square_on ^= 1);
                        *square_sign = 1;
                        if ((fx_param & 0x0f) == 0x0f) {
                            *square_sign = -1;
                        }
                    }
                }
                if (fx_param & 0xf0) {
                    if (filter_on && filter_init && filter_sign) {
                        *filter_init = (*filter_on ^= 1);
                        *filter_sign = 1;
                        if ((fx_param & 0xf0) == 0xf0) {
                            *filter_sign = -1;
                        }
                    }
                }
            }
            break;

        case 5: // Jump to PList Step (only during sustain, not after note-off)
            if (!note_off && perf_current) *perf_current = fx_param;
            break;

        case 6: // Set Volume
            if (fx_param <= 0x40) {
                // Note volume (0x00-0x40)
                if (note_max_volume) {
                    *note_max_volume = fx_param;
                }
            } else if (fx_param >= 0x50 && fx_param <= 0x90) {
                // PerfSub volume (0x50-0x90)
                if (perf_sub_volume) {
                    *perf_sub_volume = fx_param - 0x50;
                }
            } else if (fx_param >= 0xA0 && fx_param <= 0xE0) {
                // TrackMaster volume (0xA0-0xE0)
                if (track_master_volume) {
                    *track_master_volume = fx_param - 0xA0;
                }
            }
            break;

        case 7: // Set Speed
            if (perf_speed) *perf_speed = fx_param;
            if (perf_wait) *perf_wait = fx_param;
            break;

        default:
            break;
    }
}
