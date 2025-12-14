Regroove Effect Plugins
=======================


This directory contains individual LV2/VST3 plugin wrappers for each effect in the RFX suite.


## Plugins

Each effect is available as a separate plugin, allowing DAWs and DJ software like Mixxx to enable/disable them independently without needing an "Enable".

### **RFX Distortion**
  - Drive (0-1)
  - Mix (0-1)

### **RFX Filter**
  - Cutoff (0-1, default: 0.8)
  - Resonance (0-1, default: 0.3)

### **RFX EQ**
  - Low (0-1, default: 0.5)
  - Mid (0-1, default: 0.5)
  - High (0-1, default: 0.5)

### **RFX Compressor**
  - Threshold (0-1, default: 0.4)
  - Ratio (0-1, default: 0.4)
  - Attack (0-1, default: 0.05)
  - Release (0-1, default: 0.5)
  - Makeup (0-1, default: 0.65)

### **RFX Delay** 
  - Time (0-1, default: 0.5)
  - Feedback (0-1, default: 0.4)
  - Mix (0-1, default: 0.3)


## Building

### Build All Plugins
```bash
cd plugins
make
```

### Build Individual Plugin
```bash
cd plugins/RFX_Filter
make lv2_sep
```

### Generate LV2 TTL Files
After building, generate the LV2 metadata files:
```bash
cd bin/RFX_Filter.lv2
../../dpf/utils/lv2_ttl_generator ./RFX_Filter_dsp.so
```

Or generate for all at once:
```bash
for plugin in RFX_Distortion RFX_Filter RFX_EQ RFX_Compressor RFX_Delay; do
    cd bin/${plugin}.lv2
    ../../dpf/utils/lv2_ttl_generator ./${plugin}_dsp.so
    cd ../..
done
```


## Installation

### Linux
Copy the `.lv2` bundles to your LV2 directory:
```bash
cp -r bin/RFX_*.lv2 ~/.lv2/
```

### macOS
```bash
cp -r bin/RFX_*.lv2 ~/Library/Audio/Plug-Ins/LV2/
```

## Usage in Mixxx

1. Install the LV2 bundles to your system's LV2 directory
2. Restart Mixxx
3. Open Preferences > Effects
4. The individual RFX effects will appear in the effects list
5. Add them to effect chains as needed
6. Enable/disable using Mixxx's built-in effect enable/disable controls


## Combined Plugin

The combined `RegrooveFX` plugin is still available in `plugins/RegrooveFX/` if you prefer all effects in one plugin with enable/disable parameters.
