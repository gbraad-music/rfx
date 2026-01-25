# RGAHX_Drum - AHX Drum Synthesizer


A WASM-based drum synthesizer using AHX synthesis for one-shot drum sounds.


## Preset Mapping

| MIDI Note | Drum Sound | Preset File |
|-----------|------------|-------------|
| 36 (C1)   | Bass Drum  | data/ahx/kick.ahxp |
| 38 (D1)   | Snare      | data/ahx/snare.ahxp |


## Building

```bash
cd plugins/RGAHX_Drum
./build.sh
```


## Adding New Drum Presets

1. Create AHX preset file (`.ahxp`) using the AHX preset tool

2. Place in `data/ahx/`

3. Generate C header:
```bash
./tools/ahx_preset_to_c data/ahx/hihat.ahxp plugins/RGAHX_Drum/preset_hihat.h
```

4. Include the header in `rgahxdrum.c`:
```c
#include "preset_hihat.h"
```

5. Add mapping in `rgahxdrum.c`:
```c
   static const struct {
       uint8_t midi_note;
       AhxInstrumentParams* params;
   } preset_map[] = {
       {36, &preset_kick_params},       // Kick
       {38, &preset_snare_params},      // Snare
       {42, &preset_hihat_params},      // Hi-hat
   };
```

6. Rebuild

