# RGSID - Regroove SID Synthesizer

Commodore 64 SID Chip (MOS 6581/8580) Emulation

## Structure

The RGSID plugin follows the standard Regroove plugin pattern:

1. **synth/synth_sid.c/h** - Core SID chip emulation ✅ CREATED
2. **RGSID_SynthPlugin.cpp** - DPF plugin wrapper ✅ CREATED
3. **RGSID_SynthUI.cpp** - DearImGui UI ✅ CREATED
4. **Makefile** - Build configuration ✅ CREATED

## Features

### 3 Voices (like real SID)
- Triangle, Sawtooth, Pulse, Noise waveforms
- Waveforms can be combined (except noise)
- Per-voice ADSR envelopes with authentic SID curves
- Pulse width modulation (PWM)
- Ring modulation (voice modulates next voice)
- Hard sync (voice resets next voice phase)

### Filter
- Multimode: LP/BP/HP
- Resonance control
- Cutoff control
- Per-voice filter routing

### MIDI Routing
- MIDI Channel 1 → Voice 1
- MIDI Channel 2 → Voice 2
- MIDI Channel 3 → Voice 3
- MIDI Channel 10-16 → Voice 1 (omni fallback)

## UI Layout (DearImGui)

```
┌─────────────────────────────────────┐
│  RGSID - SID Synthesizer            │
├─────────────────────────────────────┤
│  VOICE 1                            │
│  [✓] Triangle [✓] Sawtooth          │
│  [✓] Pulse    [ ] Noise             │
│  PW: [====|====]  50%               │
│  A: [==|========]  D: [====|====]   │
│  S: [======|====]  R: [===|=====]   │
│  [✓] Ring Mod  [✓] Sync             │
├─────────────────────────────────────┤
│  VOICE 2                            │
│  (same controls)                    │
├─────────────────────────────────────┤
│  VOICE 3                            │
│  (same controls)                    │
├─────────────────────────────────────┤
│  FILTER                             │
│  Mode: [ LP ] [ BP ] [ HP ] [OFF]   │
│  Cutoff:    [======|====]           │
│  Resonance: [==|========]           │
│  [✓] V1  [✓] V2  [✓] V3            │
├─────────────────────────────────────┤
│  MASTER                             │
│  Volume: [=======|===]  70%         │
└─────────────────────────────────────┘
```

## File Summary

### Created Files ✅
- `/mnt/c/Users/gbraad/Projects/rfx/synth/synth_sid.h` - SID engine header
- `/mnt/c/Users/gbraad/Projects/rfx/synth/synth_sid.c` - SID engine implementation
- `/mnt/c/Users/gbraad/Projects/rfx/plugins/RGSID_Synth/DistrhoPluginInfo.h` - Plugin metadata
- `/mnt/c/Users/gbraad/Projects/rfx/plugins/RGSID_Synth/RGSID_SynthPlugin.cpp` - Main plugin logic
- `/mnt/c/Users/gbraad/Projects/rfx/plugins/RGSID_Synth/RGSID_SynthUI.cpp` - ImGui UI implementation
- `/mnt/c/Users/gbraad/Projects/rfx/plugins/RGSID_Synth/Makefile` - Build configuration

## Next Steps

1. Test build: `make` or `WINDOWS=true make` for Windows VST3
2. Test MIDI routing (Ch1→V1, Ch2→V2, Ch3→V3)
3. Verify waveform combination works correctly
4. Test filter modes and voice routing
5. Verify authentic SID sound character
