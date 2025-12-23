# RFX Ring Modulator

Ring modulator effect with internal sine wave carrier oscillator.

## Parameters

- **Frequency** (0-1): Carrier frequency from 20 Hz to 5 kHz
- **Mix** (0-1): Dry/wet mix control

## Features

- Internal sine wave carrier oscillator
- Frequency range: 20 Hz - 5 kHz
- Smooth parameter interpolation
- VST3 and LV2 support

## Building

```bash
make
# For Windows:
make WINDOWS=true
```

## Usage

Ring modulation creates metallic, bell-like timbres by multiplying the input signal with a carrier oscillator. Lower frequencies create tremolo-like effects, while higher frequencies produce harmonic and inharmonic overtones.
