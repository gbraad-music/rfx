#!/usr/bin/env python3
"""
Generate Acid Techno Pattern for RG303 Testing
Josh Wink style - demonstrates slide and accent
"""

import mido
from mido import Message, MidiFile, MidiTrack

TICKS_PER_BEAT = 480
SIXTEENTH = TICKS_PER_BEAT // 4

def create_acid_techno_pattern():
    """Acid techno with slides and accents - Josh Wink style"""
    mid = MidiFile()
    track = MidiTrack()
    mid.tracks.append(track)

    track.append(Message('program_change', program=0, time=0))

    # Pattern: 2 bars of aggressive acid
    # Accent = high velocity (100+)
    # Normal = medium velocity (70-85)
    # Slide = overlapping note_off/note_on (note_off AFTER next note_on)

    pattern = []

    # Bar 1 - Ascending pattern with slides
    # Beat 1
    pattern.append((0, 36, 100, 'on'))           # C2 - ACCENT (start)
    pattern.append((SIXTEENTH*2, 36, 0, 'off'))  # C2 off

    pattern.append((0, 43, 75, 'on'))            # G2 - slide from C2
    pattern.append((SIXTEENTH, 43, 0, 'off'))    # G2 off quickly (short note)

    # Beat 1.75 - Slide into next note
    pattern.append((SIXTEENTH, 48, 100, 'on'))   # C3 - ACCENT
    # DON'T release yet - this will slide!

    # Beat 2 - Slide continues
    pattern.append((SIXTEENTH*2, 50, 85, 'on'))  # D3 - slide from C3
    pattern.append((0, 48, 0, 'off'))            # NOW release C3 (creates slide)
    pattern.append((SIXTEENTH*2, 50, 0, 'off'))  # D3 off

    # Beat 2.5
    pattern.append((0, 43, 70, 'on'))            # G2
    pattern.append((SIXTEENTH, 43, 0, 'off'))

    # Beat 3 - Accent with slide
    pattern.append((SIXTEENTH, 48, 100, 'on'))   # C3 - ACCENT
    pattern.append((SIXTEENTH*2, 55, 90, 'on'))  # G3 - slide from C3
    pattern.append((0, 48, 0, 'off'))            # Release C3 (slide)
    pattern.append((SIXTEENTH, 55, 0, 'off'))    # G3 off

    # Beat 3.75
    pattern.append((SIXTEENTH, 43, 75, 'on'))    # G2
    pattern.append((SIXTEENTH*2, 43, 0, 'off'))

    # Beat 4 - Descending slide
    pattern.append((0, 50, 85, 'on'))            # D3
    pattern.append((SIXTEENTH*2, 48, 80, 'on'))  # C3 - slide down from D3
    pattern.append((0, 50, 0, 'off'))            # Release D3 (slide)
    pattern.append((SIXTEENTH, 48, 0, 'off'))    # C3 off

    pattern.append((SIXTEENTH, 36, 70, 'on'))    # C2
    pattern.append((SIXTEENTH, 36, 0, 'off'))

    # Bar 2 - More aggressive with faster slides
    # Beat 1
    pattern.append((0, 36, 100, 'on'))           # C2 - ACCENT
    pattern.append((SIXTEENTH, 36, 0, 'off'))

    # Rapid slide sequence
    pattern.append((SIXTEENTH, 43, 75, 'on'))    # G2
    pattern.append((SIXTEENTH, 45, 80, 'on'))    # A2 - slide
    pattern.append((0, 43, 0, 'off'))
    pattern.append((SIXTEENTH, 45, 0, 'off'))

    # Beat 2
    pattern.append((0, 48, 100, 'on'))           # C3 - ACCENT
    pattern.append((SIXTEENTH, 48, 0, 'off'))

    pattern.append((SIXTEENTH, 50, 75, 'on'))    # D3
    pattern.append((SIXTEENTH, 50, 0, 'off'))

    # Beat 3 - Triplet feel slide
    pattern.append((SIXTEENTH, 55, 100, 'on'))   # G3 - ACCENT
    pattern.append((SIXTEENTH, 53, 85, 'on'))    # F3 - slide down
    pattern.append((0, 55, 0, 'off'))
    pattern.append((SIXTEENTH, 48, 80, 'on'))    # C3 - continue slide
    pattern.append((0, 53, 0, 'off'))
    pattern.append((SIXTEENTH, 48, 0, 'off'))

    # Beat 4 - Return home
    pattern.append((SIXTEENTH, 43, 75, 'on'))    # G2
    pattern.append((SIXTEENTH*2, 36, 100, 'on')) # C2 - ACCENT slide down
    pattern.append((0, 43, 0, 'off'))
    pattern.append((SIXTEENTH*2, 36, 0, 'off'))

    # Write pattern to track
    for delta_time, note, velocity, msg_type in pattern:
        if msg_type == 'on':
            track.append(Message('note_on', note=note, velocity=velocity, time=delta_time))
        else:  # 'off'
            track.append(Message('note_off', note=note, velocity=0, time=delta_time))

    mid.save('rg303_acid_techno.mid')
    print("Created rg303_acid_techno.mid")
    print("  - Uses overlapping notes for SLIDE/PORTAMENTO")
    print("  - High velocity (100+) = ACCENT")
    print("  - Pattern inspired by acid techno style")

def create_simple_slide_test():
    """Simple slide test - just a few notes to verify slide works"""
    mid = MidiFile()
    track = MidiTrack()
    mid.tracks.append(track)

    track.append(Message('program_change', program=0, time=0))

    # Simple slide test: C2 -> G2 -> C3 (all sliding)
    track.append(Message('note_on', note=36, velocity=100, time=0))           # C2 start
    track.append(Message('note_on', note=43, velocity=90, time=TICKS_PER_BEAT)) # G2 - slide from C2
    track.append(Message('note_off', note=36, velocity=0, time=0))            # C2 off (creates slide)
    track.append(Message('note_on', note=48, velocity=95, time=TICKS_PER_BEAT)) # C3 - slide from G2
    track.append(Message('note_off', note=43, velocity=0, time=0))            # G2 off (creates slide)
    track.append(Message('note_off', note=48, velocity=0, time=TICKS_PER_BEAT)) # C3 off

    # Gap
    track.append(Message('note_on', note=36, velocity=80, time=TICKS_PER_BEAT))
    track.append(Message('note_off', note=36, velocity=0, time=SIXTEENTH*2))

    # Another slide
    track.append(Message('note_on', note=48, velocity=100, time=SIXTEENTH*2))
    track.append(Message('note_on', note=36, velocity=90, time=TICKS_PER_BEAT))  # Slide DOWN
    track.append(Message('note_off', note=48, velocity=0, time=0))
    track.append(Message('note_off', note=36, velocity=0, time=TICKS_PER_BEAT))

    mid.save('rg303_slide_test.mid')
    print("Created rg303_slide_test.mid")
    print("  - Simple slide demonstration")
    print("  - C2 -> G2 -> C3 (ascending slide)")
    print("  - C3 -> C2 (descending slide)")

def create_accent_test():
    """Test accent response at different velocities"""
    mid = MidiFile()
    track = MidiTrack()
    mid.tracks.append(track)

    track.append(Message('program_change', program=0, time=0))

    # Same note (C2) at different velocities
    velocities = [40, 60, 80, 100, 120, 127]  # Increasing accent

    for i, vel in enumerate(velocities):
        delta = 0 if i == 0 else TICKS_PER_BEAT
        track.append(Message('note_on', note=36, velocity=vel, time=delta))
        track.append(Message('note_off', note=36, velocity=0, time=SIXTEENTH*3))
        track.append(Message('note_on', note=36, velocity=vel, time=SIXTEENTH))  # Second hit same velocity
        track.append(Message('note_off', note=36, velocity=0, time=SIXTEENTH*3))

    mid.save('rg303_accent_test.mid')
    print("Created rg303_accent_test.mid")
    print("  - Tests accent at velocities: 40, 60, 80, 100, 120, 127")
    print("  - Each velocity played twice on C2")

if __name__ == '__main__':
    print("Generating RG303 Acid Techno Test MIDI Files...")
    print()
    create_acid_techno_pattern()
    create_simple_slide_test()
    create_accent_test()
    print()
    print("Done! Load these files into Reaper with RG303_Synth.")
    print()
    print("Settings for acid sound:")
    print("  - Waveform: Sawtooth")
    print("  - Cutoff: 30-50%")
    print("  - Resonance: 60-80%")
    print("  - Env Mod: 70-90%")
    print("  - Decay: 20-40%")
    print("  - Accent: 50-70%")
    print("  - Slide Time: 30-50ms")
