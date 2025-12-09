# RegrooveFX Plugin

DJ-style multi-effects processor built with DPF (DISTRHO Plugin Framework).

## Features

- **Distortion**: Analog-style drive with mix control
- **Filter**: Resonant low-pass filter with cutoff and resonance
- **3-Band EQ**: Low/Mid/High frequency control
- **Compressor**: Threshold, ratio, attack, release, and makeup gain
- **Phaser**: Rate, depth, and feedback controls
- **Reverb**: Room size, damping, and wet/dry mix
- **Delay**: Time, feedback, and wet/dry mix

All effects feature:
- Individual enable/disable switches
- Normalized 0.0-1.0 parameters for easy MIDI mapping
- Real-time automation support
- Low CPU usage

## Architecture

This plugin wraps the `regroove_effects` C library, providing:

1. **VST3/LV2 Plugin**: For use in DAWs (Bitwig, Mixxx, etc.)
2. **Standalone App**: JACK-based standalone application
3. **Shared Library**: Core effects can be linked directly in other applications

The same ImGui UI is used in both the plugin and can be reused in your standalone apps (regroove, samplecrate, rescratch).

## Building

### Prerequisites

**Linux:**
```bash
sudo apt-get install build-essential pkg-config libx11-dev libgl1-mesa-dev
```

**macOS:**
```bash
xcode-select --install
```

**Windows:**
- Visual Studio 2019 or later
- CMake 3.15+

### Option 1: Using Make (Recommended for DPF)

DPF uses Makefiles natively, which is the simplest approach:

```bash
cd plugins/RegrooveFX

# This will automatically fetch DPF if not present
make

# Built plugins will be in:
# - bin/RegrooveFX.lv2/
# - bin/RegrooveFX.vst3/
# - bin/RegrooveFX-vst2.so
# - bin/RegrooveFX-jack (standalone)
```

### Option 2: Using CMake

```bash
# From repository root
mkdir build && cd build
cmake ..
make

# Install (optional)
sudo make install
```

### Manual DPF Setup

If you want to use DPF as a git submodule:

```bash
# From repository root
git submodule add https://github.com/DISTRHO/DPF.git dpf
git submodule update --init --recursive

# Then build as normal
cd plugins/RegrooveFX
make
```

## Installation

### Linux (LV2)

```bash
# Copy to system LV2 directory
sudo cp -r bin/RegrooveFX.lv2 /usr/lib/lv2/

# Or user directory
mkdir -p ~/.lv2
cp -r bin/RegrooveFX.lv2 ~/.lv2/
```

### Linux (VST3)

```bash
mkdir -p ~/.vst3
cp -r bin/RegrooveFX.vst3 ~/.vst3/
```

### macOS

```bash
# VST3
cp -r bin/RegrooveFX.vst3 ~/Library/Audio/Plug-Ins/VST3/

# AU (if built)
cp -r bin/RegrooveFX.component ~/Library/Audio/Plug-Ins/Components/
```

### Windows

Copy `RegrooveFX.vst3` to:
- `C:\Program Files\Common Files\VST3\`

## Using in DAWs

### Bitwig Studio

1. Scan for new plugins (Settings → Locations → VST3)
2. Add "RegrooveFX" from the FX browser
3. Parameters are automatable and MIDI-mappable

### Mixxx

1. Use the LV2 plugin (Mixxx has good LV2 support)
2. Load in effect chain
3. Map MIDI controller to effect parameters

## Using in Your Applications

### Option A: Link the Plugin Library

Your standalone apps can load the VST3/LV2 plugin using a plugin host library:

```cpp
// Using JUCE, DPF, or similar plugin host
auto plugin = loadPlugin("RegrooveFX.vst3");
```

### Option B: Link the Core Library Directly (Recommended)

This is more efficient and what you likely want for regroove/samplecrate/rescratch:

```cmake
# In your CMakeLists.txt
add_subdirectory(path/to/rfx)
target_link_libraries(your_app PRIVATE regroove_effects)
```

```cpp
// In your app
#include "regroove_effects.h"

RegrooveEffects* fx = regroove_effects_create();

// In your audio callback
regroove_effects_process(fx, audioBuffer, numFrames, sampleRate);

// Your existing ImGui UI code can be used directly!
// See plugins/RegrooveFX/RegrooveFXUI.cpp for the plugin's UI
// which is based on the regroove FX panel
```

### Reusing the ImGui UI

The UI code in `RegrooveFXUI.cpp` is designed to be easily extracted and used in your standalone apps:

```cpp
// From RegrooveFXUI.cpp, you can copy the render functions:
void renderEffectSection(...) { /* vertical sliders, enable buttons */ }
void renderEQSection(...)      { /* 3-band EQ UI */ }
// etc.

// Use in your app's ImGui rendering loop
if (ImGui::Begin("FX")) {
    renderEffectSection("DISTORTION", ...);
    renderEQSection(...);
    // etc.
}
ImGui::End();
```

## Parameter Reference

All parameters are normalized to `0.0 - 1.0` range for easy automation:

| Effect      | Parameter | Range | Description |
|-------------|-----------|-------|-------------|
| Distortion  | Drive     | 0-1   | Amount of distortion |
| Distortion  | Mix       | 0-1   | Dry/wet blend |
| Filter      | Cutoff    | 0-1   | Frequency cutoff (normalized) |
| Filter      | Resonance | 0-1   | Filter resonance/Q |
| EQ          | Low       | 0-1   | 100Hz boost/cut |
| EQ          | Mid       | 0-1   | 1kHz boost/cut |
| EQ          | High      | 0-1   | 10kHz boost/cut |
| Compressor  | Threshold | 0-1   | Compression threshold |
| Compressor  | Ratio     | 0-1   | Maps to 1:1 → 10:1 |
| Phaser      | Rate      | 0-1   | LFO speed |
| Phaser      | Depth     | 0-1   | Modulation depth |
| Delay       | Time      | 0-1   | Maps to 0-1000ms |
| Delay       | Feedback  | 0-1   | Delay feedback amount |

## Development

### Project Structure

```
rfx/
├── regroove_effects.h          # Core effects library header
├── regroove_effects.c          # Core effects implementation
├── CMakeLists.txt              # Root build configuration
└── plugins/
    └── RegrooveFX/
        ├── DistrhoPluginInfo.h  # Plugin metadata & parameters
        ├── RegrooveFXPlugin.cpp # DSP processing (wraps regroove_effects)
        ├── RegrooveFXUI.cpp     # ImGui UI implementation
        ├── Makefile             # DPF native build (recommended)
        └── CMakeLists.txt       # CMake build (alternative)
```

### Adding New Effects

1. Add parameter enum to `DistrhoPluginInfo.h`
2. Add getters/setters in `RegrooveFXPlugin.cpp`
3. Add UI controls in `RegrooveFXUI.cpp`
4. Update core library: `regroove_effects.h` and `regroove_effects.c`

## License

ISC License - See main repository for details

## Credits

- Built with [DPF (DISTRHO Plugin Framework)](https://github.com/DISTRHO/DPF)
- Uses [Dear ImGui](https://github.com/ocornut/imgui) for UI
- Core effects from the Regroove project
