#!/usr/bin/env python3
"""
RG303 Test MIDI Generator
Creates test patterns to showcase the RG303 bass synthesizer
Requires: pip install mido
"""

import mido
from mido import Message, MidiFile, MidiTrack

def create_classic_acid_pattern():
    """Create a classic TB-303 style acid bass pattern"""
    mid = MidiFile()
    track = MidiTrack()
    mid.tracks.append(track)

    # Set tempo (120 BPM)
    track.append(mido.MetaMessage('set_tempo', tempo=mido.bpm2tempo(120)))
    track.append(mido.MetaMessage('track_name', name='RG303 Classic Acid'))

    # Ticks per beat (480 is standard)
    ticks_per_beat = mid.ticks_per_beat

    # 16th note duration
    sixteenth = ticks_per_beat // 4

    # Classic acid pattern with accents
    # Format: (note, velocity, duration_in_16ths)
    pattern = [
        # Bar 1 - Classic acid line
        (36, 100, 1),   # C2 - ACCENT
        (0, 0, 1),      # Rest
        (36, 70, 1),    # C2 - normal
        (0, 0, 1),      # Rest
        (39, 110, 2),   # D#2 - ACCENT (longer)
        (41, 80, 1),    # F2 - normal
        (0, 0, 1),      # Rest
        (41, 70, 1),    # F2 - normal
        (0, 0, 1),      # Rest
        (43, 120, 2),   # G2 - STRONG ACCENT
        (0, 0, 2),      # Rest

        # Bar 2 - Variation
        (36, 100, 1),   # C2 - ACCENT
        (0, 0, 1),      # Rest
        (43, 70, 1),    # G2 - normal
        (0, 0, 1),      # Rest
        (39, 100, 1),   # D#2 - ACCENT
        (0, 0, 1),      # Rest
        (36, 80, 2),    # C2 - normal (longer)
        (0, 0, 4),      # Rest
        (36, 70, 2),    # C2 - normal
        (0, 0, 2),      # Rest
    ]

    time = 0
    for note, velocity, duration in pattern:
        if note > 0:  # Note on
            track.append(Message('note_on', note=note, velocity=velocity, time=time))
            track.append(Message('note_off', note=note, velocity=0, time=sixteenth * duration))
            time = 0
        else:  # Rest
            time += sixteenth * duration

    mid.save('rg303_test_classic_acid.mid')
    print("Created: rg303_test_classic_acid.mid")
    print("  - Classic TB-303 acid pattern")
    print("  - Contains accented notes (high velocity)")
    print("  - 2 bars, 120 BPM")

def create_accent_test():
    """Create a pattern specifically to test accent response"""
    mid = MidiFile()
    track = MidiTrack()
    mid.tracks.append(track)

    track.append(mido.MetaMessage('set_tempo', tempo=mido.bpm2tempo(110)))
    track.append(mido.MetaMessage('track_name', name='RG303 Accent Test'))

    ticks_per_beat = mid.ticks_per_beat
    quarter = ticks_per_beat

    # Same note, different velocities
    velocities = [40, 60, 80, 100, 120, 127]
    note = 36  # C2

    time = 0
    for vel in velocities:
        track.append(Message('note_on', note=note, velocity=vel, time=time))
        track.append(Message('note_off', note=note, velocity=0, time=quarter))
        time = quarter // 2  # Rest between notes

    mid.save('rg303_test_accent.mid')
    print("Created: rg303_test_accent.mid")
    print("  - Same note (C2) with increasing velocity")
    print("  - Velocities: 40, 60, 80, 100, 120, 127")
    print("  - Set Accent knob to 50-70% to hear difference")

def create_range_test():
    """Test different octaves/note ranges - FULL SWEEP"""
    mid = MidiFile()
    track = MidiTrack()
    mid.tracks.append(track)

    track.append(mido.MetaMessage('set_tempo', tempo=mido.bpm2tempo(100)))
    track.append(mido.MetaMessage('track_name', name='RG303 Range Test'))

    ticks_per_beat = mid.ticks_per_beat
    quarter = ticks_per_beat

    # FULL chromatic sweep from C1 to C5
    # Start at MIDI note 24 (C1), end at 72 (C5)
    notes = []
    for midi_note in range(24, 73):  # C1 to C5
        note_names = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B']
        note_name = note_names[midi_note % 12]
        octave = (midi_note // 12) - 1
        notes.append((midi_note, f"{note_name}{octave}"))

    time = 0
    for note, name in notes:
        track.append(Message('note_on', note=note, velocity=100, time=time))
        track.append(Message('note_off', note=note, velocity=0, time=quarter))
        time = quarter // 4  # Short rest

    mid.save('rg303_test_range.mid')
    print("✓ Created: rg303_test_range.mid")
    print("  - FULL chromatic sweep from C1 (24) to C5 (72)")
    print("  - 49 notes total")
    print("  - C2 octave (36-47) is the 'sweet spot' for RG303")

def create_filter_sweep_pattern():
    """Pattern designed for filter cutoff automation"""
    mid = MidiFile()
    track = MidiTrack()
    mid.tracks.append(track)

    track.append(mido.MetaMessage('set_tempo', tempo=mido.bpm2tempo(128)))
    track.append(mido.MetaMessage('track_name', name='RG303 Filter Sweep'))

    ticks_per_beat = mid.ticks_per_beat
    sixteenth = ticks_per_beat // 4

    # Repeating pattern - good for filter automation
    pattern = [36, 36, 39, 41, 36, 43, 39, 36] * 4  # Repeat 4 times

    time = 0
    for note in pattern:
        track.append(Message('note_on', note=note, velocity=100, time=time))
        track.append(Message('note_off', note=note, velocity=0, time=sixteenth * 2))
        time = 0

    mid.save('rg303_test_filter_sweep.mid')
    print("✓ Created: rg303_test_filter_sweep.mid")
    print("  - Repeating pattern for filter automation")
    print("  - Try automating Cutoff from 20% to 80% over 4 bars")

def create_phuture_style():
    """Inspired by 'Acid Tracks' by Phuture"""
    mid = MidiFile()
    track = MidiTrack()
    mid.tracks.append(track)

    track.append(mido.MetaMessage('set_tempo', tempo=mido.bpm2tempo(132)))
    track.append(mido.MetaMessage('track_name', name='RG303 Phuture Style'))

    ticks_per_beat = mid.ticks_per_beat
    sixteenth = ticks_per_beat // 4

    # Phuture-ish pattern with lots of movement
    notes = [
        (36, 120, 2), (0, 0, 1), (39, 100, 1),
        (36, 80, 1), (0, 0, 1), (41, 110, 2),
        (43, 127, 2), (0, 0, 2), (39, 90, 2),
        (36, 100, 2), (0, 0, 2),

        (36, 120, 1), (39, 100, 1), (41, 90, 1), (43, 110, 1),
        (48, 127, 4), (0, 0, 4),
        (36, 100, 2), (39, 90, 2), (36, 80, 2), (34, 100, 2),
    ]

    time = 0
    for note, velocity, duration in notes:
        if note > 0:
            track.append(Message('note_on', note=note, velocity=velocity, time=time))
            track.append(Message('note_off', note=note, velocity=0, time=sixteenth * duration))
            time = 0
        else:
            time += sixteenth * duration

    mid.save('rg303_test_phuture.mid')
    print("Created: rg303_test_phuture.mid")
    print("  - Phuture 'Acid Tracks' inspired pattern")
    print("  - Settings: Res 70%, EnvMod 80%, Decay 35%")

if __name__ == "__main__":
    print("=" * 50)
    print("RG303 Test MIDI File Generator")
    print("=" * 50)
    print()

    create_classic_acid_pattern()
    print()
    create_accent_test()
    print()
    create_range_test()
    print()
    create_filter_sweep_pattern()
    print()
    create_phuture_style()

    print()
    print("=" * 50)
    print("All MIDI files created!")
    print("=" * 50)
    print()
    print("HOW TO USE:")
    print("  Import any .mid file into Reaper")
    print("  Assign to RG303 track")
    print("  For accent test: Set Accent knob to 50-70%")
    print("  For filter sweep: Automate Cutoff parameter")
    print()
    print("RECOMMENDED RG303 SETTINGS:")
    print("  Waveform: Sawtooth")
    print("  Cutoff: 45%")
    print("  Resonance: 70%")
    print("  Env Mod: 70%")
    print("  Decay: 40%")
    print("  Accent: 50%")
    print("  Volume: 70%")
