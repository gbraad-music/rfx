# Regroove Effects for Korg logue SDK

Individual effects from the Regroove library ported to Korg's logue series hardware synthesizers (minilogue xd, prologue, NTS-1).

## Available Effects

Each effect is a separate unit that can be loaded onto your logue hardware:

### Modulation Effects (modfx)
- **RGDistortion** - Analog-style distortion with drive and mix controls
- **RGFilter** - DJ-style resonant low-pass filter

### Delay Effects (delfx)
- **RGDelay** - Stereo delay with feedback and mix

## Hardware Compatibility

| Effect | minilogue xd | prologue | NTS-1 |
|--------|-------------|----------|-------|
| RGDistortion | ✓ | ✓ | ✓ |
| RGFilter | ✓ | ✓ | ✓ |
| RGDelay | ✓ | ✓ | ✓ |

## Building

### Prerequisites

1. Install the [logue SDK](https://github.com/korginc/logue-sdk):
   ```bash
   git clone https://github.com/korginc/logue-sdk.git
   cd logue-sdk
   # Follow installation instructions for your platform
   ```

2. Set the logue SDK path:
   ```bash
   export TOOLSDIR=/path/to/logue-sdk
   ```

### Build Individual Effects

```bash
# Build distortion
cd distortion
make

# Build filter
cd ../filter
make

# Build delay
cd ../delay
make
```

Output files will be `.ntkdigunit` files that can be loaded onto your hardware.

### Build All Effects

```bash
# From logue-sdk-effects directory
for effect in distortion filter delay; do
    cd $effect
    make
    cd ..
done
```

## Installation

### Using logue-cli (Recommended)

```bash
# Install logue-cli
npm install -g @kokoromi/logue-cli

# Load effect onto NTS-1 via USB
logue-cli load -u distortion/regroove_distortion.ntkdigunit

# Or for minilogue xd / prologue
logue-cli load -u filter/regroove_filter.ntkdigunit
```

### Using Korg Librarian

1. Build the effect (creates `.ntkdigunit` file)
2. Open Korg Librarian software
3. Connect your logue hardware via USB
4. Drag and drop the `.ntkdigunit` file to the appropriate slot
5. Sync to hardware

### Manual Installation (NTS-1)

1. Copy `.ntkdigunit` files to an SD card
2. Insert SD card into NTS-1
3. Select the effect from the unit's menu

## Effect Parameters

### RGDistortion (modfx)
| Parameter | Range | Description |
|-----------|-------|-------------|
| Drive | 0-100 | Amount of distortion/saturation |
| Mix | 0-100 | Dry/wet blend (0=dry, 100=wet) |

### RGFilter (modfx)
| Parameter | Range | Description |
|-----------|-------|-------------|
| Cutoff | 0-100 | Filter frequency (20Hz-20kHz, exponential) |
| Resonance | 0-100 | Filter resonance/Q factor |

### RGDelay (delfx)
| Parameter | Range | Description |
|-----------|-------|-------------|
| Time | 0-100 | Delay time (10ms-1000ms) |
| Feedback | 0-100 | Delay feedback amount |
| Mix | 0-100 | Dry/wet blend |

## Usage on Hardware

### minilogue xd / prologue
1. Press **SHIFT** + **SELECT/WRITE** to enter effect selection
2. Navigate to the installed effect
3. Use the knobs to adjust parameters
4. Parameters can be automated via sequencer or LFO

### NTS-1
1. Press **TYPE** button to select effect type (modfx or delfx)
2. Turn encoder to select the effect
3. Press **EDIT** to adjust parameters
4. Use knobs A/B for parameter control

## Architecture

Each logue effect is a **lightweight extraction** of the corresponding effect from the main `regroove_effects` library:

```
regroove_effects (full library)
├── All effects in one
├── Shared state
└── Used in: regroove, samplecrate, DPF plugin

logue-sdk-effects (individual units)
├── distortion/   → Extracts only distortion code + state
├── filter/       → Extracts only filter code + state
└── delay/        → Extracts only delay code + state
```

### Memory Optimization

The logue hardware has limited memory (especially NTS-1). Each effect includes:
- **Only the DSP code needed** for that effect
- **Minimal state variables** (no unused effect state)
- **Optimized for 48kHz** processing
- **SDRAM usage** for large buffers (delay)

### Performance

- All effects run at **48kHz sample rate**
- Optimized for **real-time performance** on ARM Cortex-M7
- No heap allocation (stack only)
- Cycle-counted to stay within CPU budget

## Adding More Effects

To port another effect from the main library:

1. **Create directory**:
   ```bash
   mkdir -p logue-sdk-effects/neweffect
   ```

2. **Extract DSP code** from `regroove_effects.c`:
   - Copy only the processing function for your effect
   - Extract minimal state struct
   - Adapt to logue API (`FX_INIT`, `FX_PROCESS`, `FX_PARAM`)

3. **Create manifest.json**:
   ```json
   {
     "header": {
       "platform": "nutekt-digital",
       "module": "modfx",  // or "delfx" for delay/reverb
       "name": "RGNewEffect",
       "num_param": 2,
       "params": [
         ["Param1", 0, 1023, 512],
         ["Param2", 0, 1023, 512]
       ]
     }
   }
   ```

4. **Create project.mk**:
   ```make
   PROJECT = regroove_neweffect
   PROJECT_TYPE = modfx
   PLATFORMDIR = $(TOOLSDIR)/logue-sdk/platform/nutekt-digital
   UCSRC = regroove_neweffect.cpp
   include $(PLATFORMDIR)/modfx.mk
   ```

5. **Build and test**

## Parameter Mapping

logue parameters are 10-bit (0-1023). The SDK provides `param_val_to_f32()` to convert to 0.0-1.0 float, matching the main library's normalized parameter system.

## Troubleshooting

### "logue-sdk not found"
```bash
export TOOLSDIR=/path/to/logue-sdk
```

### Build fails with "undefined reference"
- Ensure you're using the correct `PROJECT_TYPE` (modfx vs delfx)
- Check that all state variables are defined

### Effect loads but no sound
- Check parameter initialization in `FX_INIT()`
- Verify mix parameter is > 0
- Test with default parameters from manifest.json

### Crackling/artifacts
- Reduce complexity (logue CPU is limited)
- Check for buffer overruns
- Ensure sample rate is 48kHz

## Resources

- [logue SDK Documentation](https://korginc.github.io/logue-sdk/)
- [logue SDK GitHub](https://github.com/korginc/logue-sdk)
- [User Effect Examples](https://github.com/korginc/logue-sdk/tree/master/platform/nutekt-digital/demos)
- [Community Forums](https://www.korgforums.com/)

## License

ISC License - Same as main Regroove Effects library
