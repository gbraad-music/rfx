/*
 * Regroove Delay for logue SDK
 *
 * Stereo delay with feedback and mix controls
 * Compatible with: minilogue xd, prologue, NTS-1
 */

#include "userfx.h"

// Delay buffer size (1 second at 48kHz)
#define MAX_DELAY_SAMPLES 48000

typedef struct {
    float time;        // 0.0 - 1.0 (maps to 0-1000ms)
    float feedback;    // 0.0 - 1.0
    float mix;         // 0.0 - 1.0 (dry/wet)

    // Delay buffers (stereo)
    float buffer_l[MAX_DELAY_SAMPLES];
    float buffer_r[MAX_DELAY_SAMPLES];
    int write_pos;
} RegrooveDelay;

static __sdram RegrooveDelay fx_state; // Use SDRAM for large buffers

void FX_INIT(uint32_t platform, uint32_t api)
{
    fx_state.time = 0.5f;      // 500ms default
    fx_state.feedback = 0.4f;  // Moderate feedback
    fx_state.mix = 0.3f;       // 30% wet

    fx_state.write_pos = 0;

    // Clear buffers
    for (int i = 0; i < MAX_DELAY_SAMPLES; i++) {
        fx_state.buffer_l[i] = 0.0f;
        fx_state.buffer_r[i] = 0.0f;
    }
}

void FX_PROCESS(float *xn, uint32_t frames)
{
    const float time = fx_state.time;
    const float feedback = fx_state.feedback;
    const float mix = fx_state.mix;

    // Map time to delay samples (10ms - 1000ms)
    const int min_delay = 480;   // 10ms
    const int max_delay = 48000; // 1000ms
    const int delay_samples = min_delay + (int)(time * (max_delay - min_delay));

    float * __restrict x = xn;
    const float * x_e = x + (frames << 1);

    int write_pos = fx_state.write_pos;

    for (; x != x_e; ) {
        // Calculate read position
        int read_pos = write_pos - delay_samples;
        if (read_pos < 0) read_pos += MAX_DELAY_SAMPLES;

        // Left channel
        float dry_l = *x;
        float delayed_l = fx_state.buffer_l[read_pos];
        float output_l = dry_l + mix * (delayed_l - dry_l);
        fx_state.buffer_l[write_pos] = dry_l + delayed_l * feedback;
        *x++ = output_l;

        // Right channel
        float dry_r = *x;
        float delayed_r = fx_state.buffer_r[read_pos];
        float output_r = dry_r + mix * (delayed_r - dry_r);
        fx_state.buffer_r[write_pos] = dry_r + delayed_r * feedback;
        *x++ = output_r;

        // Advance write position
        write_pos++;
        if (write_pos >= MAX_DELAY_SAMPLES) write_pos = 0;
    }

    fx_state.write_pos = write_pos;
}

void FX_PARAM(uint8_t index, int32_t value)
{
    const float valf = param_val_to_f32(value);

    switch (index) {
    case 0: // Time
        fx_state.time = valf;
        break;
    case 1: // Feedback
        fx_state.feedback = valf;
        break;
    case 2: // Mix
        fx_state.mix = valf;
        break;
    default:
        break;
    }
}
