#!/usr/bin/env python3
import wave
import numpy as np
try:
    import matplotlib
    matplotlib.use('Agg')
    import matplotlib.pyplot as plt
    HAS_MATPLOTLIB = True
except ImportError:
    HAS_MATPLOTLIB = False

# Read rendered bass drum
with wave.open('RG909_BD_BassDrum.wav', 'rb') as wav:
    sr = wav.getframerate()
    frames = wav.readframes(wav.getnframes())
    rendered = np.frombuffer(frames, dtype=np.int16).astype(np.float32) / 32768.0

# Read real TR-909 sample
try:
    with wave.open('/mnt/e/Samples/TR909/909 Kick_long.wav', 'rb') as wav:
        frames = wav.readframes(wav.getnframes())
        real = np.frombuffer(frames, dtype=np.int16).astype(np.float32) / 32768.0
    has_real = True
except:
    has_real = False
    print("⚠ Real TR-909 sample not found")

# Look at a few cycles around 20ms to see waveform shape
start_sample = int(0.020 * sr)  # 20ms
duration_samples = int(sr / 50)  # ~20ms window (50Hz = one cycle at that frequency)

waveform_rendered = rendered[start_sample:start_sample+duration_samples]
if has_real:
    waveform_real = real[start_sample:start_sample+duration_samples]

# Plot if matplotlib is available
if HAS_MATPLOTLIB:
    if has_real:
        # Create 3-panel comparison
        plt.figure(figsize=(16, 10))

        # Plot 1: Rendered waveform detail
        plt.subplot(3, 1, 1)
        plt.plot(waveform_rendered, linewidth=2, color='#0066cc', label='Rendered')
        plt.title('Rendered Bass Drum Waveform at 20ms', fontsize=14, fontweight='bold')
        plt.ylabel('Amplitude', fontsize=12)
        plt.grid(True, alpha=0.3)
        plt.axhline(y=0, color='k', linestyle='-', linewidth=0.5)
        plt.legend()

        # Plot 2: Real TR-909 waveform detail
        plt.subplot(3, 1, 2)
        plt.plot(waveform_real, linewidth=2, color='#cc0000', label='Real TR-909')
        plt.title('Real TR-909 Waveform at 20ms', fontsize=14, fontweight='bold')
        plt.ylabel('Amplitude', fontsize=12)
        plt.grid(True, alpha=0.3)
        plt.axhline(y=0, color='k', linestyle='-', linewidth=0.5)
        plt.legend()

        # Plot 3: Full envelope comparison (ALIGNED BY TRANSIENT)
        plt.subplot(3, 1, 3)

        # Find attack transient (first sample > 0.05 threshold)
        threshold = 0.05
        rendered_start = np.argmax(np.abs(rendered) > threshold)
        real_start = np.argmax(np.abs(real) > threshold)

        # Align both waveforms at their transients
        envelope_samples = min(int(0.5 * sr), len(rendered) - rendered_start, len(real) - real_start)
        time_ms = np.arange(envelope_samples) / sr * 1000  # Convert to milliseconds

        plt.plot(time_ms, rendered[rendered_start:rendered_start+envelope_samples], linewidth=1, color='#0066cc', alpha=0.7, label='Rendered')
        plt.plot(time_ms, real[real_start:real_start+envelope_samples], linewidth=1, color='#cc0000', alpha=0.7, label='Real TR-909')
        plt.title(f'Full Envelope Comparison (aligned at transient: rendered@{rendered_start}, real@{real_start})', fontsize=14, fontweight='bold')
        plt.xlabel('Time (ms)', fontsize=12)
        plt.ylabel('Amplitude', fontsize=12)
        plt.grid(True, alpha=0.3)
        plt.axhline(y=0, color='k', linestyle='-', linewidth=0.5)
        # Add vertical lines at key intervals
        for ms in [50, 100, 150, 200, 250, 300, 350, 400, 450]:
            plt.axvline(x=ms, color='gray', linestyle=':', linewidth=0.5, alpha=0.3)
        plt.legend()

        plt.tight_layout()
        plt.savefig('waveform_comparison.png', dpi=150, bbox_inches='tight')
        print("✓ Saved comparison plot: waveform_comparison.png")
    else:
        # Fallback to single plot
        plt.figure(figsize=(14, 8))

        plt.subplot(2, 1, 1)
        plt.plot(waveform_rendered, linewidth=2, color='#0066cc')
        plt.title('Bass Drum Waveform at 20ms', fontsize=14, fontweight='bold')
        plt.ylabel('Amplitude', fontsize=12)
        plt.grid(True, alpha=0.3)
        plt.axhline(y=0, color='k', linestyle='-', linewidth=0.5)

        plt.subplot(2, 1, 2)
        envelope_samples = int(0.5 * sr)
        plt.plot(rendered[:envelope_samples], linewidth=1, color='#cc6600')
        plt.title('Full Bass Drum Envelope', fontsize=14, fontweight='bold')
        plt.xlabel('Sample', fontsize=12)
        plt.ylabel('Amplitude', fontsize=12)
        plt.grid(True, alpha=0.3)
        plt.axhline(y=0, color='k', linestyle='-', linewidth=0.5)

        plt.tight_layout()
        plt.savefig('waveform_analysis.png', dpi=150, bbox_inches='tight')
        print("✓ Saved waveform plot: waveform_analysis.png")
else:
    print("(matplotlib not available - skipping plot)")

# Analyze the shape
peak_idx = np.argmax(np.abs(waveform_rendered))
if peak_idx > 10 and peak_idx < len(waveform_rendered)-10:
    region = waveform_rendered[peak_idx-10:peak_idx+10]
    print(f"\nWaveform values around peak:")
    for i, v in enumerate(region):
        marker = " <-- PEAK" if i == 10 else ""
        print(f"  {v:.6f}{marker}")

    # Check if slopes are linear (triangle) or curved (sine)
    diffs = np.diff(region)
    print(f"\nSlope changes (should be constant for triangle):")
    print(f"  {diffs}")
    print(f"  Slope std dev: {np.std(diffs):.6f} (low = linear/triangle, high = curved/sine)")
