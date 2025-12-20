#!/usr/bin/env python3
"""
Generate 808 Drum Patterns for RG808 Testing
Uses GM MIDI drum mapping
"""

import mido
from mido import Message, MidiFile, MidiTrack

# GM MIDI Drum Note Numbers (matching RG808_Drum)
KICK = 36       # Bass Drum 1 (C2)
SNARE = 38      # Acoustic Snare (D2)
CLAP = 39       # Hand Clap (D#2)
CLOSED_HAT = 42 # Closed Hi-Hat (F#2)
LOW_TOM = 45    # Low Tom (A2)
OPEN_HAT = 46   # Open Hi-Hat (A#2)
MID_TOM = 47    # Low-Mid Tom (B2)
HIGH_TOM = 50   # High Tom (D3)
COWBELL = 56    # Cowbell (G#3)
RIMSHOT = 37    # Side Stick/Rimshot (C#2)

# Timing (480 ticks per beat is standard)
TICKS_PER_BEAT = 480
SIXTEENTH = TICKS_PER_BEAT // 4  # 16th note

def create_classic_808_pattern():
    """Classic 808 house/techno pattern"""
    mid = MidiFile()
    track = MidiTrack()
    mid.tracks.append(track)

    track.append(Message('program_change', program=0, time=0))

    # 2 bars of 4/4
    pattern = []

    # Bar 1
    pattern.append((0, KICK, 100))              # Beat 1
    pattern.append((SIXTEENTH*2, CLOSED_HAT, 80))
    pattern.append((SIXTEENTH*2, KICK, 90))     # Beat 1.5
    pattern.append((SIXTEENTH*2, CLOSED_HAT, 70))

    pattern.append((SIXTEENTH*2, SNARE, 100))   # Beat 2
    pattern.append((SIXTEENTH*2, CLOSED_HAT, 80))
    pattern.append((SIXTEENTH*2, CLOSED_HAT, 70))
    pattern.append((SIXTEENTH*2, CLOSED_HAT, 75))

    pattern.append((SIXTEENTH*2, KICK, 100))    # Beat 3
    pattern.append((SIXTEENTH*2, CLOSED_HAT, 80))
    pattern.append((SIXTEENTH*2, CLOSED_HAT, 70))
    pattern.append((SIXTEENTH*2, OPEN_HAT, 85)) # Open hat

    pattern.append((SIXTEENTH*2, SNARE, 100))   # Beat 4
    pattern.append((SIXTEENTH*2, CLOSED_HAT, 80))
    pattern.append((SIXTEENTH*2, KICK, 95))     # Beat 4.5
    pattern.append((SIXTEENTH*2, CLOSED_HAT, 75))

    # Bar 2 (with variation)
    pattern.append((SIXTEENTH*2, KICK, 100))    # Beat 1
    pattern.append((SIXTEENTH*2, CLOSED_HAT, 80))
    pattern.append((SIXTEENTH*2, RIMSHOT, 70))  # Rimshot accent
    pattern.append((SIXTEENTH*2, CLOSED_HAT, 70))

    pattern.append((SIXTEENTH*2, SNARE, 100))   # Beat 2
    pattern.append((SIXTEENTH*2, CLOSED_HAT, 80))
    pattern.append((SIXTEENTH*2, COWBELL, 85))  # Cowbell
    pattern.append((SIXTEENTH*2, CLOSED_HAT, 75))

    pattern.append((SIXTEENTH*2, KICK, 100))    # Beat 3
    pattern.append((SIXTEENTH*2, CLOSED_HAT, 80))
    pattern.append((SIXTEENTH*2, LOW_TOM, 90))  # Tom fill
    pattern.append((SIXTEENTH*2, MID_TOM, 90))

    pattern.append((SIXTEENTH*2, SNARE, 100))   # Beat 4
    pattern.append((SIXTEENTH*2, HIGH_TOM, 90))
    pattern.append((SIXTEENTH*2, CLAP, 90))     # Clap
    pattern.append((SIXTEENTH*2, CLOSED_HAT, 75))

    for delta_time, note, velocity in pattern:
        track.append(Message('note_on', note=note, velocity=velocity, time=delta_time))
        track.append(Message('note_off', note=note, velocity=0, time=SIXTEENTH))

    mid.save('rg808_classic_pattern.mid')

def create_simple_4on4():
    """Simple four-on-the-floor kick pattern"""
    mid = MidiFile()
    track = MidiTrack()
    mid.tracks.append(track)

    track.append(Message('program_change', program=0, time=0))

    # 4 bars, kick on every beat, snare on 2&4, hats on 8ths
    for bar in range(4):
        for beat in range(4):
            # Kick on every beat
            track.append(Message('note_on', note=KICK, velocity=100, time=0 if beat == 0 and bar == 0 else TICKS_PER_BEAT))
            track.append(Message('note_off', note=KICK, velocity=0, time=SIXTEENTH))

            # Snare on beats 2 and 4
            if beat in [1, 3]:
                track.append(Message('note_on', note=SNARE, velocity=100, time=TICKS_PER_BEAT - SIXTEENTH))
                track.append(Message('note_off', note=SNARE, velocity=0, time=SIXTEENTH))

            # Closed hi-hat on every 8th note
            for eighth in range(2):
                delta = TICKS_PER_BEAT // 2 if eighth == 1 else 0
                if not (beat == 0 and bar == 0 and eighth == 0):  # Already at position 0
                    track.append(Message('note_on', note=CLOSED_HAT, velocity=70, time=delta))
                    track.append(Message('note_off', note=CLOSED_HAT, velocity=0, time=SIXTEENTH))

    mid.save('rg808_four_on_floor.mid')

def create_drum_test():
    """Test all drum sounds one by one"""
    mid = MidiFile()
    track = MidiTrack()
    mid.tracks.append(track)

    track.append(Message('program_change', program=0, time=0))

    drums = [
        (KICK, "Kick"),
        (SNARE, "Snare"),
        (CLOSED_HAT, "Closed Hat"),
        (OPEN_HAT, "Open Hat"),
        (CLAP, "Clap"),
        (LOW_TOM, "Low Tom"),
        (MID_TOM, "Mid Tom"),
        (HIGH_TOM, "High Tom"),
        (COWBELL, "Cowbell"),
        (RIMSHOT, "Rimshot")
    ]

    for i, (note, name) in enumerate(drums):
        print(f"  {name}: MIDI note {note}")
        delta_time = 0 if i == 0 else TICKS_PER_BEAT

        # Play each drum 4 times
        for hit in range(4):
            vel = 100 if hit == 0 else 80  # First hit louder
            track.append(Message('note_on', note=note, velocity=vel, time=delta_time))
            track.append(Message('note_off', note=note, velocity=0, time=SIXTEENTH))
            delta_time = SIXTEENTH * 3  # Rest between hits

    mid.save('rg808_drum_test.mid')

def create_breakbeat():
    """808-style breakbeat pattern"""
    mid = MidiFile()
    track = MidiTrack()
    mid.tracks.append(track)

    track.append(Message('program_change', program=0, time=0))

    # Funky breakbeat pattern with syncopation
    pattern = []

    # Bar 1
    pattern.append((0, KICK, 100))
    pattern.append((SIXTEENTH, CLOSED_HAT, 70))
    pattern.append((SIXTEENTH, CLOSED_HAT, 60))
    pattern.append((SIXTEENTH, KICK, 85))
    pattern.append((SIXTEENTH, CLOSED_HAT, 75))

    pattern.append((SIXTEENTH, SNARE, 100))
    pattern.append((SIXTEENTH, CLOSED_HAT, 70))
    pattern.append((SIXTEENTH, CLOSED_HAT, 60))
    pattern.append((SIXTEENTH, CLOSED_HAT, 75))

    pattern.append((SIXTEENTH, KICK, 95))
    pattern.append((SIXTEENTH, CLOSED_HAT, 70))
    pattern.append((SIXTEENTH*2, KICK, 90))
    pattern.append((SIXTEENTH, CLOSED_HAT, 75))

    pattern.append((SIXTEENTH, SNARE, 100))
    pattern.append((SIXTEENTH, CLAP, 85))  # Double snare/clap
    pattern.append((SIXTEENTH, CLOSED_HAT, 60))
    pattern.append((SIXTEENTH, OPEN_HAT, 80))

    for delta_time, note, velocity in pattern:
        track.append(Message('note_on', note=note, velocity=velocity, time=delta_time))
        track.append(Message('note_off', note=note, velocity=0, time=SIXTEENTH//2))

    mid.save('rg808_breakbeat.mid')

if __name__ == '__main__':
    print("Generating RG808 Drum Patterns...")
    print()
    create_drum_test()
    create_classic_808_pattern()
    create_simple_4on4()
    create_breakbeat()
    print()
    print("Done! Load these MIDI files into your DAW with RG808_Drum plugin.")
