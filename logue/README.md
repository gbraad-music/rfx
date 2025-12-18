Regroove Effects for Korg logue
===============================

Individual effects from the Regroove library ported to Korg's logue series hardware synthesizers (minilogue xd, prologue, NTS-1).

## Available Effects

Each effect is a separate unit that can be loaded onto your logue hardware:

### Modulation Effects (modfx)
  - RFXDist  
    Analog-style distortion with drive and mix controls
  - RFXFilter  
    low-pass filter with cutoff and resonance
  - RFXEQ  
    DJ-Style (kill) 3-band parametric equalizer; low, mid, high
  - RFXComp  
    RMS compressor with soft knee
  - RFXPhaser  
    Cascaded allpass phaser with LFO modulation
  - RFXReverb  
    Algorithmic reverb (room)
  - RFXWiden  
    Stereo width enhancement (mid-side processing)
  - RM1LPF  
    Model 1 inspired low-pass filter
  - RM1HPF  
    Model 1 inspired high-pass filter
  - RM1Sculpt  
    Model 1 inspired resonant filter sculpting
  - RM1Trim
    Model 1 inspired soft saturation and gain

### Delay Effects (delfx)
  - RFXDelay
    Stereo delay with feedback and mix

## Building

### Prerequisites

1. Install the [logue SDK](https://github.com/korginc/logue-sdk):
```bash
git clone https://github.com/korginc/logue-sdk.git
cd logue-sdk
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
./build-all.sh
```

Or manually:
```bash
for effect in distortion filter eq compressor delay phaser reverb stereo_widen model1_lpf model1_hpf model1_sculpt model1_trim; do
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


### RFXDist (modfx)
| Parameter | Range | Description |
|-----------|-------|-------------|
| Drive | 0-100 | Amount of distortion/saturation |
| Mix | 0-100 | Dry/wet blend (0=dry, 100=wet) |

### RFXFilter (modfx)
| Parameter | Range | Description |
|-----------|-------|-------------|
| Cutoff | 0-100 | Filter frequency (20Hz-20kHz, exponential) |
| Resonance | 0-100 | Filter resonance/Q factor |

### RFXEQ (modfx)
| Parameter | Range | Description |
|-----------|-------|-------------|
| Low | 0-100 | Low frequency gain (-12dB to +12dB @ 100Hz) |
| Mid | 0-100 | Mid frequency gain (-12dB to +12dB @ 1kHz) |
| High | 0-100 | High frequency gain (-12dB to +12dB @ 10kHz) |

### RFXComp (modfx)
| Parameter | Range | Description |
|-----------|-------|-------------|
| Threshold | 0-100 | Compression threshold |
| Ratio | 0-100 | Compression ratio (1:1 to 10:1) |
| Makeup | 0-100 | Output gain compensation |

### RFXPhaser (modfx)
| Parameter | Range | Description |
|-----------|-------|-------------|
| Rate | 0-100 | LFO rate (0.1Hz to 10Hz) |
| Depth | 0-100 | Modulation depth |
| Feedback | 0-100 | Feedback amount for resonance |

### RFXReverb (modfx)
| Parameter | Range | Description |
|-----------|-------|-------------|
| Size | 0-100 | Room size/decay time |
| Damping | 0-100 | High frequency damping |
| Mix | 0-100 | Dry/wet blend |

### RFXWiden (modfx)
| Parameter | Range | Description |
|-----------|-------|-------------|
| Width | 0-100 | Stereo width (0=mono, 50=normal, 100=wide) |

### RM1LPF/HPF (modfx)
| Parameter | Range | Description |
|-----------|-------|-------------|
| Cutoff | 0-100 | Filter cutoff frequency |
| Resonance | 0-100 | Filter resonance |

### RM1Sculpt (modfx)
| Parameter | Range | Description |
|-----------|-------|-------------|
| Frequency | 0-100 | Center frequency of resonant peak |
| Resonance | 0-100 | Resonance amount |
| Mix | 0-100 | Dry/wet blend |

### RM1Trim (modfx)
| Parameter | Range | Description |
|-----------|-------|-------------|
| Drive | 0-100 | Soft saturation amount |
| Level | 0-100 | Output level |

### RFXDelay (delfx)
| Parameter | Range | Description |
|-----------|-------|-------------|
| Time | 0-100 | Delay time (10ms-1000ms) |
| Feedback | 0-100 | Delay feedback amount |
| Mix | 0-100 | Dry/wet blend |



## Adding More Effects

To port another effect from the main library:

```bash
mkdir -p logue-sdk-effects/neweffect
```

Extract DSP code from `regroove_effects.c`:
  - Copy only the processing function for your effect
  - Extract minimal state struct
  - Adapt to logue API (`FX_INIT`, `FX_PROCESS`, `FX_PARAM`)

Create manifest.json:
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

Create project.mk:
```make
PROJECT = regroove_neweffect
PROJECT_TYPE = modfx
PLATFORMDIR = $(TOOLSDIR)/logue-sdk/platform/nutekt-digital
UCSRC = regroove_neweffect.cpp
include $(PLATFORMDIR)/modfx.mk
```

## Parameter Mapping

logue parameters are 10-bit (0-1023). The SDK provides `param_val_to_f32()` to convert to 0.0-1.0 float, matching the main library's normalized parameter system.


## Resources

- [logue SDK Documentation](https://korginc.github.io/logue-sdk/)
- [logue SDK GitHub](https://github.com/korginc/logue-sdk)


## License

ISC License - Same as main Regroove Effects library

