# Modular Effects Architecture

This directory contains individual, self-contained effect modules that can be used:
1. **Directly** in applications
2. **Combined** via the `regroove_effects` wrapper (legacy API)
3. **Wrapped** for logue SDK (minimal adapter code)
4. **Wrapped** for DPF plugins (VST3/LV2)

## Benefits

### Before (Monolithic)
```
regroove_effects.c (12KB, all effects)
├── Used directly → applications
├── Extracted manually → logue (duplicated code)
└── Wrapped entirely → DPF plugin
```

**Problems:**
- logue wrappers duplicate DSP code
- Hard to maintain consistency
- Can't use individual effects easily

### After (Modular)
```
effects/
├── fx_distortion.c  (2KB)
├── fx_filter.c      (1.5KB)
├── fx_delay.c       (2KB)
└── ...

These modules are used by:
├── regroove_effects.c (wrapper, legacy API)
├── logue wrappers (thin adapters, <100 lines)
├── DPF plugin (can use modules OR wrapper)
└── applications (can use modules OR wrapper)
```

**Benefits:**
- ✅ Single source of truth for DSP code
- ✅ logue wrappers are trivial (just parameter mapping)
- ✅ Easy to test individual effects
- ✅ Flexible deployment (use what you need)
- ✅ Common interface pattern

## Common Interface Pattern

Each effect module follows this pattern:

### Header Structure
```c
typedef struct FXEffectName FXEffectName;

// Lifecycle
FXEffectName* fx_effectname_create(void);
void fx_effectname_destroy(FXEffectName* fx);
void fx_effectname_reset(FXEffectName* fx);

// Processing (multiple formats supported)
void fx_effectname_process_f32(FXEffectName* fx, float* buffer, int frames, int sample_rate);
void fx_effectname_process_i16(FXEffectName* fx, int16_t* buffer, int frames, int sample_rate);
void fx_effectname_process_frame(FXEffectName* fx, float* left, float* right, int sample_rate);

// Parameters (normalized 0.0-1.0)
void fx_effectname_set_enabled(FXEffectName* fx, int enabled);
void fx_effectname_set_param1(FXEffectName* fx, float value);
// ...

int fx_effectname_get_enabled(FXEffectName* fx);
float fx_effectname_get_param1(FXEffectName* fx);
// ...
```

### Why Three Process Functions?

1. **`process_f32()`** - For float buffers (VST3/LV2, modern apps)
2. **`process_i16()`** - For int16 buffers (legacy compatibility)
3. **`process_frame()`** - For sample-by-sample processing (logue, embedded, optimized)

The `process_frame()` function is key for embedded use - it processes one stereo frame at a time, allowing the logue wrapper to be extremely simple.

## Usage Examples

### Direct Use (Application)

```c
#include "effects/fx_distortion.h"

FXDistortion* dist = fx_distortion_create();
fx_distortion_set_enabled(dist, 1);
fx_distortion_set_drive(dist, 0.7f);
fx_distortion_set_mix(dist, 0.5f);

// In audio callback (float buffer)
fx_distortion_process_f32(dist, audio_buffer, num_frames, sample_rate);

fx_distortion_destroy(dist);
```

### Via Legacy Wrapper

```c
#include "regroove_effects.h"

RegrooveEffects* fx = regroove_effects_create();
regroove_effects_set_distortion_enabled(fx, 1);
regroove_effects_set_distortion_drive(fx, 0.7f);

// Process all enabled effects
regroove_effects_process(fx, audio_buffer, num_frames, sample_rate);

regroove_effects_destroy(fx);
```

### logue Wrapper (Minimal)

```cpp
#include "userfx.h"
extern "C" {
    #include "../../effects/fx_distortion.h"
}

static FXDistortion* fx = NULL;

void FX_INIT(uint32_t platform, uint32_t api) {
    fx = fx_distortion_create();
    fx_distortion_set_enabled(fx, 1);
}

void FX_PROCESS(float *xn, uint32_t frames) {
    for (uint32_t i = 0; i < frames; i++) {
        fx_distortion_process_frame(fx, &xn[i*2], &xn[i*2+1], 48000);
    }
}

void FX_PARAM(uint8_t index, int32_t value) {
    float valf = param_val_to_f32(value);
    if (index == 0) fx_distortion_set_drive(fx, valf);
    // ...
}
```

**Only ~30 lines of glue code!** All DSP is in the module.

### DPF Plugin

The DPF plugin can use individual modules:

```cpp
class RegrooveFXPlugin : public Plugin {
    FXDistortion* distortion;
    FXFilter* filter;
    FXDelay* delay;
    // ...

    void run(const float** inputs, float** outputs, uint32_t frames) {
        // Copy input to output
        memcpy(outputs[0], inputs[0], frames * sizeof(float));
        memcpy(outputs[1], inputs[1], frames * sizeof(float));

        // Interleave for processing
        float buffer[frames * 2];
        for (uint32_t i = 0; i < frames; i++) {
            buffer[i*2] = outputs[0][i];
            buffer[i*2+1] = outputs[1][i];
        }

        // Process each effect
        fx_distortion_process_f32(distortion, buffer, frames, getSampleRate());
        fx_filter_process_f32(filter, buffer, frames, getSampleRate());
        fx_delay_process_f32(delay, buffer, frames, getSampleRate());

        // De-interleave
        for (uint32_t i = 0; i < frames; i++) {
            outputs[0][i] = buffer[i*2];
            outputs[1][i] = buffer[i*2+1];
        }
    }
};
```

Or use the legacy wrapper:

```cpp
class RegrooveFXPlugin : public Plugin {
    RegrooveEffects* effects;

    void run(const float** inputs, float** outputs, uint32_t frames) {
        // Convert and process...
        regroove_effects_process(effects, buffer, frames, getSampleRate());
    }
};
```

## Available Modules

| Module | Files | Parameters | Description |
|--------|-------|------------|-------------|
| **fx_distortion** | fx_distortion.{h,c} | drive, mix | Analog-style saturation |
| **fx_filter** | fx_filter.{h,c} | cutoff, resonance | Resonant low-pass |
| **fx_delay** | fx_delay.{h,c} | time, feedback, mix | Stereo delay |

More effects can be added following the same pattern.

## Adding a New Effect

1. **Create `fx_neweffect.h`** with the standard interface:
   ```c
   typedef struct FXNewEffect FXNewEffect;
   FXNewEffect* fx_neweffect_create(void);
   void fx_neweffect_process_frame(...);
   // etc.
   ```

2. **Implement `fx_neweffect.c`** with the DSP:
   ```c
   struct FXNewEffect {
       int enabled;
       float param1;
       // state variables
   };
   // Implementation...
   ```

3. **Add to legacy wrapper** (optional, for backward compatibility):
   ```c
   // In regroove_effects.c
   #include "effects/fx_neweffect.h"
   // Add to struct, init, process chain...
   ```

4. **Create logue wrapper** (if needed):
   ```cpp
   // Trivial adapter using fx_neweffect_process_frame()
   ```

## Testing

Each module can be tested independently:

```c
// test_distortion.c
#include "effects/fx_distortion.h"

void test_distortion() {
    FXDistortion* fx = fx_distortion_create();

    // Test parameter ranges
    fx_distortion_set_drive(fx, 0.5f);
    assert(fx_distortion_get_drive(fx) == 0.5f);

    // Test processing
    float buffer[2] = {1.0f, 1.0f};
    fx_distortion_set_enabled(fx, 1);
    fx_distortion_process_frame(fx, &buffer[0], &buffer[1], 48000);

    // Verify output...

    fx_distortion_destroy(fx);
}
```

## Performance

- **Stack allocation**: Modules use heap, but state is compact
- **Zero-copy**: `process_frame()` operates in-place
- **Optimized**: No unnecessary conversions or copies
- **Embedded-friendly**: Each module can be used with or without malloc() by providing buffer

## License

ISC License - Same as parent project
