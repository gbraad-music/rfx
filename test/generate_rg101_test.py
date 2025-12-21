#!/usr/bin/env python3
"""
Generate test MIDI files for RG101 Synthesizer
Tests: oscillator mix, portamento, filter sweeps, PWM
"""

from midiutil import MIDIFile
import os

def create_bass_line():
    """Classic bass line pattern - tests basic functionality"""
    midi = MIDIFile(1)
    track = 0
    channel = 0
    time = 0
    tempo = 120
    midi.addTempo(track, time, tempo)
    midi.addTrackName(track, time, "RG101 Bass Line")

    # Bass line pattern (C minor pentatonic)
    # note, duration, velocity
    pattern = [
        (36, 0.5, 100),  # C1
        (36, 0.5, 80),
        (43, 0.5, 100),  # G1
        (41, 0.5, 90),   # F1
        (38, 0.5, 100),  # D1
        (36, 0.5, 80),
        (39, 0.5, 100),  # Eb1
        (41, 0.5, 90),   # F1
    ]

    time = 0
    for note, duration, velocity in pattern * 4:  # Repeat 4 times
        midi.addNote(track, channel, note, time, duration, velocity)
        time += duration

    return midi

def create_portamento_test():
    """Test portamento/glide with overlapping notes"""
    midi = MIDIFile(1)
    track = 0
    channel = 0
    time = 0
    tempo = 100
    midi.addTempo(track, time, tempo)
    midi.addTrackName(track, time, "RG101 Portamento Test")

    # Overlapping notes for portamento
    notes = [36, 40, 43, 48, 43, 40, 36, 31]

    time = 0
    for i, note in enumerate(notes):
        # Note on
        midi.addNote(track, channel, note, time, 1.0, 100)

        # If not last note, add note off slightly before next note
        # This creates overlap which triggers portamento
        if i < len(notes) - 1:
            time += 0.8  # Overlap = 0.2 beats
        else:
            time += 1.0

    return midi

def create_range_test():
    """Test different octaves/note ranges"""
    midi = MIDIFile(1)
    track = 0
    channel = 0
    time = 0
    tempo = 120
    midi.addTempo(track, time, tempo)
    midi.addTrackName(track, time, "RG101 Range Test")

    # Full chromatic sweep from C1 to C4
    for midi_note in range(24, 61):  # C1 to C4
        note_names = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B']
        note_name = note_names[midi_note % 12]
        octave = (midi_note // 12) - 1

        duration = 0.25
        velocity = 100

        midi.addNote(track, channel, midi_note, time, duration, velocity)
        time += duration

    return midi

def create_filter_sweep():
    """Long notes for testing filter modulation"""
    midi = MIDIFile(1)
    track = 0
    channel = 0
    time = 0
    tempo = 60
    midi.addTempo(track, time, tempo)
    midi.addTrackName(track, time, "RG101 Filter Sweep")

    # Long sustained notes at different pitches
    notes = [36, 40, 43, 48, 52, 55, 60]

    time = 0
    for note in notes:
        # Long note (4 beats) to hear filter envelope and LFO
        midi.addNote(track, channel, note, time, 4.0, 100)
        time += 4.5  # Small gap between notes

    return midi

def create_velocity_test():
    """Test velocity response across different velocities"""
    midi = MIDIFile(1)
    track = 0
    channel = 0
    time = 0
    tempo = 120
    midi.addTempo(track, time, tempo)
    midi.addTrackName(track, time, "RG101 Velocity Test")

    # Same note at different velocities
    note = 48  # C3
    velocities = [30, 50, 70, 90, 110, 127]

    time = 0
    for velocity in velocities:
        midi.addNote(track, channel, note, time, 0.5, velocity)
        time += 0.75

    # Different notes at different velocities
    notes = [36, 40, 43, 48, 52, 55]
    time += 1.0

    for note, velocity in zip(notes, velocities):
        midi.addNote(track, channel, note, time, 0.5, velocity)
        time += 0.75

    return midi

def create_pwm_demo():
    """Sustained notes for PWM demonstration"""
    midi = MIDIFile(1)
    track = 0
    channel = 0
    time = 0
    tempo = 80
    midi.addTempo(track, time, tempo)
    midi.addTrackName(track, time, "RG101 PWM Demo")

    # Very long notes to hear PWM modulation
    notes = [36, 43, 48, 55]

    time = 0
    for note in notes:
        # Very long note (8 beats) to hear PWM sweep
        midi.addNote(track, channel, note, time, 8.0, 100)
        time += 8.5

    return midi

def create_sub_oscillator_test():
    """Low notes to test sub-oscillator"""
    midi = MIDIFile(1)
    track = 0
    channel = 0
    time = 0
    tempo = 100
    midi.addTempo(track, time, tempo)
    midi.addTrackName(track, time, "RG101 Sub Oscillator Test")

    # Very low notes where sub oscillator is most effective
    notes = [24, 26, 28, 29, 31, 33, 35, 36]  # C0 to C1

    time = 0
    for note in notes:
        midi.addNote(track, channel, note, time, 1.0, 100)
        time += 1.25

    return midi

def main():
    """Generate all test MIDI files"""
    output_dir = os.path.dirname(os.path.abspath(__file__))

    tests = {
        'rg101_bass_line.mid': create_bass_line(),
        'rg101_portamento.mid': create_portamento_test(),
        'rg101_range.mid': create_range_test(),
        'rg101_filter_sweep.mid': create_filter_sweep(),
        'rg101_velocity.mid': create_velocity_test(),
        'rg101_pwm_demo.mid': create_pwm_demo(),
        'rg101_sub_osc.mid': create_sub_oscillator_test(),
    }

    print("Generating RG101 test MIDI files...")
    for filename, midi in tests.items():
        filepath = os.path.join(output_dir, filename)
        with open(filepath, 'wb') as f:
            midi.writeFile(f)
        print(f"  Created: {filename}")

    print(f"\nGenerated {len(tests)} test files in {output_dir}")
    print("\nTest descriptions:")
    print("  - rg101_bass_line.mid: Classic bass line pattern")
    print("  - rg101_portamento.mid: Overlapping notes for glide/slide")
    print("  - rg101_range.mid: Full chromatic sweep C1-C4")
    print("  - rg101_filter_sweep.mid: Long notes for filter envelope/LFO")
    print("  - rg101_velocity.mid: Velocity response test")
    print("  - rg101_pwm_demo.mid: Long notes for PWM modulation")
    print("  - rg101_sub_osc.mid: Low notes for sub-oscillator test")

if __name__ == '__main__':
    main()
