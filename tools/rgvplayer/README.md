# RGVPlayer - RegrooveController Test Tool

A simple terminal UI player that demonstrates all features of the `RegrooveController` layer.

## Features

- **Row-Precise Loop Control**: Set loops with row-level accuracy
- **Loop Arming**: "Play-to-loop" functionality - arm the loop and it activates when the loop point is reached
- **Command Queuing**: Queue jumps to execute on pattern boundaries (smooth transitions)
- **Pattern Mode**: Lock to a single pattern for live performance
- **Channel Mute/Solo**: Control individual channels

## Supported Formats

- **MOD**: ProTracker modules (4-channel)
- **MMD**: OctaMED/MED modules (4-64 channels)
- **AHX**: Abyss' Highest eXperience / HivelyTracker (4-channel synthesized)

## Building

```bash
cd tools/rgvplayer
make
```

Requirements:
- SDL2 development library (`libsdl2-dev` on Ubuntu/Debian)
- GCC or compatible C compiler

## Usage

```bash
./rgvplayer <file.mod|file.mmd|file.ahx>
```

## Controls

### Playback
- **SPACE**: Start/Stop playback
- **N/P**: Queue Next/Previous order (executes on pattern boundary)
- **R**: Retrigger current pattern

### Loop Control
- **L**: Set loop from current position to end
- **A**: Arm loop (play-to-loop - activates when reaching loop start)
- **T**: Trigger loop immediately
- **F**: Disable loop

### Pattern Mode
- **S**: Toggle pattern mode (loops current pattern indefinitely)

### Channel Control
- **1-8**: Toggle channel mute
- **M**: Mute all channels
- **U**: Unmute all channels

### Other
- **Q/ESC**: Quit

## Example Workflow

### DJ-Style Loop Playback

1. Load a module: `./rgvplayer mytrack.mod`
2. Let it play through the intro
3. Press **L** when you reach the chorus to set the loop point
4. Press **A** to arm the loop
5. When playback reaches the loop start, it automatically starts looping
6. Press **N** to queue jump to the next section when ready
7. The jump executes on the next pattern boundary for a smooth transition

### Live Channel Mixing

1. Load a module with multiple channels
2. Press **1** to mute the bass channel
3. Press **2** to mute the drums
4. Press **3** to solo the lead melody
5. Press **U** to unmute all and reset

### Pattern Mode (Live Performance)

1. Press **S** to enable pattern mode
2. The current pattern loops indefinitely
3. Press **R** to retrigger the pattern from the start
4. Press **S** again to disable and return to normal playback

## Architecture

RGVPlayer demonstrates the layered architecture:

```
┌──────────────────────────────┐
│  RGVPlayer (this tool)       │ ← User interface
└──────────┬───────────────────┘
           │ Uses
           ↓
┌──────────────────────────────┐
│  RegrooveController          │ ← Advanced features
│  - Loop control              │
│  - Command queue             │
│  - Pattern mode              │
└──────────┬───────────────────┘
           │ Wraps
           ↓
┌──────────────────────────────┐
│  PatternSequencer            │ ← Core timing
└──────────┬───────────────────┘
           │ Callbacks
           ↓
┌──────────────────────────────┐
│  ModPlayer / MedPlayer       │ ← Format playback
└──────────────────────────────┘
```

## Console Output

The tool provides real-time feedback:

```
[LOOP] Armed (play-to-loop)
[LOOP] Triggered at 4:00
[LOOP] State: ARMED -> ACTIVE
[QUEUED] Next order
[CMD] Executed: JUMP
[CHANNEL 1] Mute toggled
```

## Tips

- Use loop arming (**A**) for smooth loop entry - perfect for DJ mixing
- Queue commands (**N**/**P**) execute on pattern boundaries for glitch-free transitions
- Pattern mode (**S**) is great for isolating a drum break or bass loop
- The display updates in real-time showing position, loop state, and channel mutes

## Troubleshooting

**No sound:**
- Check that SDL2 audio is working on your system
- Try adjusting your system volume

**File not loading:**
- Ensure the file is a valid MOD or MMD format
- Check file permissions

**Keyboard not responding:**
- Terminal must support raw mode
- Try running in a different terminal emulator

## License

Part of the Regroove FX project. See main project LICENSE.
