Regroove Effects
================

DJ-style audio effects library and VST3/LV2 plugin.

This repo contains:
  - **Modular Effects** (`effects/`)  
    Individual, self-contained effect modules
  - **Legacy Wrapper** (`regroove_effects.c/.h`)  
    Backward-compatible API wrapping all effects
  - **VST3/LV2 Plugin** (`plugins/RegrooveFX/`)  
    For use in DAWs like Bitwig, Mixxx, etc.
  - **Korg logue Effects** (`logue-sdk-effects/`)  
    Minimal wrappers for minilogue xd, prologue, NTS-1


## Features

  - **Distortion** - Analog-style saturation with drive and mix controls
  - **Filter** - Resonant low-pass filter with cutoff and resonance
  - **EQ** - DJ-style 3-band kill EQ (Low/Mid/High)
  - **Compressor** - RMS compressor with soft knee and makeup gain
  - **Delay** - Stereo delay with time, feedback, and mix


## Quick Start

### Using Individual Effects (Recommended)

```c
#include "effects/fx_distortion.h"
#include "effects/fx_filter.h"

FXDistortion* dist = fx_distortion_create();
fx_distortion_set_drive(dist, 0.7f);

// In audio callback:
fx_distortion_process_f32(dist, audioBuffer, numFrames, sampleRate);
```


### Using Legacy Wrapper (All Effects)

This is mostly provided for Regroove, Samplecrate and Rescratch

```c
#include "regroove_effects.h"

RegrooveEffects* fx = regroove_effects_create();
regroove_effects_set_distortion_drive(fx, 0.7f);

// In audio callback:
regroove_effects_process(fx, audioBuffer, numFrames, sampleRate);
```


### Building the VST3/LV2 Plugin

```bash
cd plugins/RegrooveFX
make  # Builds VST3, LV2, and standalone versions
```

### Building for Korg logue Hardware

```bash
cd logue-sdk-effects/distortion
make  # Creates .ntkdigunit file for NTS-1/minilogue xd/prologue
```

