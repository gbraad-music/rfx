# Regroove Engine


A libopenmpt wrapper for playing tracker modules (MOD/XM/IT/S3M/MPTM) with advanced playback control.

## Features

  - Pattern/Order Navigation  
    Queue next/previous orders, jump to specific patterns
  - Loop Control  
    Custom loop ranges with armed/active states, pattern-level looping
  - Channel Control  
    Per-channel muting, soloing, volume, and panning
  - Audio Quality  
    Interpolation filters, stereo separation, dithering, Amiga emulation
  - Playback Modes  
    Pattern mode (loop single pattern) vs Song mode
  - Event Callbacks  
    Order/row/note change notifications (for MIDI output, UI updates)
  - Thread-Safe Commands  
    Command queue for audio-thread-safe operations


## Dependencies

- libopenmpt
- Standard C library (stdio, stdlib, string.h)


## Usage

```c
#include "rfx/engine/regroove_engine.h"

// Create engine instance
Regroove *g = regroove_create("song.xm", 48000);

// Set up callbacks (optional)
struct RegrooveCallbacks callbacks = {
    .on_order_change = my_order_callback,
    .on_row_change = my_row_callback,
    .on_note = my_note_callback,
    .userdata = my_userdata
};
regroove_set_callbacks(g, &callbacks);

// Render audio
int16_t buffer[frames * 2];  // Stereo interleaved
regroove_render_audio(g, buffer, frames);

// Control playback
regroove_pattern_mode(g, 1);              // Enable pattern mode
regroove_toggle_channel_mute(g, 0);       // Mute channel 0
regroove_set_channel_volume(g, 1, 0.5);   // 50% volume on channel 1
regroove_queue_next_order(g);             // Queue next pattern
regroove_set_pitch(g, 1.5);               // 1.5x speed

// Cleanup
regroove_destroy(g);
```


## Integration

```makefile
CFLAGS = -I/path/to/rfx
LDFLAGS = -lopenmpt
SOURCES = rfx/engine/regroove_engine.c your_app.c
```


## API Reference

See `regroove_engine.h` for complete API documentation.


### Core Functions

  - `regroove_create()` - Create engine instance
  - `regroove_destroy()` - Free resources
  - `regroove_render_audio()` - Render audio buffer
  - `regroove_set_callbacks()` - Register event callbacks


### Playback Control

  - `regroove_pattern_mode()` - Toggle pattern looping
  - `regroove_queue_next_order()` / `regroove_queue_prev_order()`
  - `regroove_jump_to_pattern()` / `regroove_jump_to_order()`
  - `regroove_retrigger_pattern()` - Restart current pattern


### Loop Control

  - `regroove_set_loop_range()` - Set custom loop range
  - `regroove_trigger_loop()` - Jump to loop start and activate
  - `regroove_play_to_loop()` - Arm loop for next boundary
  - `regroove_get_loop_state()` - Get loop state (OFF/ARMED/ACTIVE)


### Channel Control

  - `regroove_toggle_channel_mute()` - Immediate mute toggle
  - `regroove_queue_channel_mute()` - Queued mute (at pattern boundary)
  - `regroove_toggle_channel_solo()` - Solo/unsolo channel
  - `regroove_set_channel_volume()` - Set channel volume (0.0-1.0)
  - `regroove_set_channel_panning()` - Set channel pan (0.0-1.0)


### Audio Quality

  - `regroove_set_interpolation_filter()` - 0=none, 1=linear, 2=cubic, 4=FIR
  - `regroove_set_stereo_separation()` - 0-200 (0=mono, 100=default)
  - `regroove_set_dither()` - 0=none, 1=default, 2-3=shaped
  - `regroove_set_amiga_resampler()` - Enable Amiga-style resampling
  - `regroove_set_amiga_filter_type()` - 0=auto, 1=A500, 2=A1200, 3=unfiltered


### State Queries

  - `regroove_get_current_order()` / `regroove_get_current_pattern()` / `regroove_get_current_row()`
  - `regroove_get_num_channels()` / `regroove_get_num_orders()` / `regroove_get_num_patterns()`
  - `regroove_get_current_bpm()` - Module BPM (before pitch adjustment)
  - `regroove_get_effective_bpm()` - Playback BPM (after pitch adjustment)
  - `regroove_is_channel_muted()` / `regroove_has_pending_mute_changes()`
  - `regroove_get_pattern_cell()` - Get pattern data for specific cell

