#!/usr/bin/env python3
"""
Generate test MIDI files for RG20 Synthesizer (MS-20 style)
Tests: dual VCO, ring modulation, aggressive filters, portamento
"""

from midiutil import MIDIFile
import os

def create_bass_sequence():
    """Aggressive bass sequence - tests dual VCO detuning"""
    midi = MIDIFile(1)
    track = 0
    channel = 0
    time = 0
    tempo = 130
    midi.addTempo(track, time, tempo)
    midi.addTrackName(track, time, "RG20 Bass Sequence")

    # Aggressive bass pattern
    pattern = [
        (28, 0.25, 127),  # E0
        (28, 0.25, 100),
        (31, 0.5, 120),   # G0
        (28, 0.5, 110),
        (33, 0.25, 127),  # A0
        (35, 0.25, 120),  # B0
        (31, 0.5, 110),
        (28, 0.5, 127),
    ]

    time = 0
    for note, duration, velocity in pattern * 8:
        midi.addNote(track, channel, note, time, duration, velocity)
        time += duration

    return midi

def create_filter_sweep_test():
    """Long notes for testing aggressive filter resonance"""
    midi = MIDIFile(1)
    track = 0
    channel = 0
    time = 0
    tempo = 60
    midi.addTempo(track, time, tempo)
    midi.addTrackName(track, time, "RG20 Filter Sweep")

    # Low notes to test filter self-oscillation
    notes = [28, 31, 33, 36, 40, 43, 48]

    time = 0
    for note in notes:
        # Very long notes (6 beats) to hear filter sweep and self-oscillation
        midi.addNote(track, channel, note, time, 6.0, 120)
        time += 6.5

    return midi

def create_ring_mod_test():
    """Intervals to demonstrate ring modulation"""
    midi = MIDIFile(1)
    track = 0
    channel = 0
    time = 0
    tempo = 100
    midi.addTempo(track, time, tempo)
    midi.addTrackName(track, time, "RG20 Ring Mod Test")

    # Different intervals - ring mod creates interesting harmonics
    # Play each note twice (once for each VCO tuning)
    notes = [
        36,  # C2
        40,  # E2 (major third)
        43,  # G2 (fifth)
        48,  # C3 (octave)
        52,  # E3
        55,  # G3
        60,  # C4
    ]

    time = 0
    for note in notes:
        # Longer notes to hear ring mod harmonics
        midi.addNote(track, channel, note, time, 2.0, 110)
        time += 2.5

    return midi

def create_portamento_test():
    """Overlapping notes for portamento test"""
    midi = MIDIFile(1)
    track = 0
    channel = 0
    time = 0
    tempo = 80
    midi.addTempo(track, time, tempo)
    midi.addTrackName(track, time, "RG20 Portamento Test")

    # Big intervallic jumps with overlap
    notes = [28, 40, 31, 43, 33, 48, 36, 52, 40, 55, 43, 60]

    time = 0
    for i, note in enumerate(notes):
        midi.addNote(track, channel, note, time, 1.2, 100)
        if i < len(notes) - 1:
            time += 0.9  # Overlap for portamento
        else:
            time += 1.2

    return midi

def create_dual_vco_detuning():
    """Test dual VCO detuning effects"""
    midi = MIDIFile(1)
    track = 0
    channel = 0
    time = 0
    tempo = 90
    midi.addTempo(track, time, tempo)
    midi.addTrackName(track, time, "RG20 Dual VCO Detuning")

    # Sustained notes at different octaves
    notes = [24, 28, 31, 36, 40, 43, 48, 52]

    time = 0
    for note in notes:
        # Long sustained notes (4 beats) to hear VCO beating/detuning
        midi.addNote(track, channel, note, time, 4.0, 100)
        time += 4.5

    return midi

def create_aggressive_sequence():
    """Fast aggressive sequence - classic MS-20 style"""
    midi = MIDIFile(1)
    track = 0
    channel = 0
    time = 0
    tempo = 140
    midi.addTempo(track, time, tempo)
    midi.addTrackName(track, time, "RG20 Aggressive Sequence")

    # Fast chromatic runs and jumps
    pattern = []
    # Chromatic run up
    for note in range(28, 40):
        pattern.append((note, 0.125, 110))

    # Big jumps
    pattern.extend([
        (48, 0.25, 127),
        (28, 0.25, 120),
        (55, 0.25, 127),
        (31, 0.25, 120),
        (60, 0.25, 127),
        (33, 0.25, 120),
    ])

    time = 0
    for note, duration, velocity in pattern * 4:
        midi.addNote(track, channel, note, time, duration, velocity)
        time += duration

    return midi

def create_range_test():
    """Full range chromatic sweep"""
    midi = MIDIFile(1)
    track = 0
    channel = 0
    time = 0
    tempo = 120
    midi.addTempo(track, time, tempo)
    midi.addTrackName(track, time, "RG20 Range Test")

    # Full chromatic sweep from C1 to C4
    for midi_note in range(24, 61):
        note_names = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B']
        note_name = note_names[midi_note % 12]
        octave = (midi_note // 12) - 1

        duration = 0.25
        velocity = 100

        midi.addNote(track, channel, midi_note, time, duration, velocity)
        time += duration

    return midi

def create_noise_test():
    """Low notes to test noise + oscillator mix"""
    midi = MIDIFile(1)
    track = 0
    channel = 0
    time = 0
    tempo = 100
    midi.addTempo(track, time, tempo)
    midi.addTrackName(track, time, "RG20 Noise Test")

    # Very low notes where noise adds character
    notes = [24, 26, 28, 29, 31, 33, 35, 36, 38, 40]

    time = 0
    for note in notes:
        midi.addNote(track, channel, note, time, 1.0, 110)
        time += 1.25

    return midi

def main():
    """Generate all test MIDI files"""
    output_dir = os.path.dirname(os.path.abspath(__file__))

    tests = {
        'rg20_bass_sequence.mid': create_bass_sequence(),
        'rg20_filter_sweep.mid': create_filter_sweep_test(),
        'rg20_ring_mod.mid': create_ring_mod_test(),
        'rg20_portamento.mid': create_portamento_test(),
        'rg20_dual_vco.mid': create_dual_vco_detuning(),
        'rg20_aggressive.mid': create_aggressive_sequence(),
        'rg20_range.mid': create_range_test(),
        'rg20_noise.mid': create_noise_test(),
    }

    print("Generating RG20 test MIDI files...")
    for filename, midi in tests.items():
        filepath = os.path.join(output_dir, filename)
        with open(filepath, 'wb') as f:
            midi.writeFile(f)
        print(f"  Created: {filename}")

    print(f"\nGenerated {len(tests)} test files in {output_dir}")
    print("\nTest descriptions:")
    print("  - rg20_bass_sequence.mid: Aggressive bass pattern (dual VCO detuning)")
    print("  - rg20_filter_sweep.mid: Long notes for filter self-oscillation test")
    print("  - rg20_ring_mod.mid: Harmonic intervals for ring modulation")
    print("  - rg20_portamento.mid: Overlapping notes with big jumps")
    print("  - rg20_dual_vco.mid: Sustained notes for VCO beating/detuning")
    print("  - rg20_aggressive.mid: Fast chromatic runs (classic MS-20 style)")
    print("  - rg20_range.mid: Full chromatic sweep C1-C4")
    print("  - rg20_noise.mid: Low notes with noise mix")

if __name__ == '__main__':
    main()
