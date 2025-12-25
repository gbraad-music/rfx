#!/usr/bin/env python3
import struct
import numpy as np
try:
    import matplotlib
    matplotlib.use('Agg')
    import matplotlib.pyplot as plt
    HAS_MATPLOTLIB = True
except ImportError:
    HAS_MATPLOTLIB = False

# Read raw float32 wavetables
def read_f32_raw(filename):
    with open(filename, 'rb') as f:
        data = f.read()
    num_samples = len(data) // 4
    samples = struct.unpack(f'<{num_samples}f', data)
    return np.array(samples)

attack = read_f32_raw('/tmp/tr-909-main/resources/bassdrum-attack.raw')

print("=== ATTACK WAVEFORM ANALYSIS ===\n")

# Peak analysis
peak_idx = np.argmax(np.abs(attack))
peak_val = attack[peak_idx]
print(f"Peak: {peak_val:.6f} at sample {peak_idx} ({peak_idx/44.1:.2f}ms)")

# Envelope analysis
envelope = np.abs(attack)
print(f"\nEnvelope characteristics:")
print(f"  Start: {envelope[0]:.6f}")
print(f"  Peak: {np.max(envelope):.6f}")
print(f"  End: {envelope[-1]:.6f}")

# Find where it crosses thresholds
threshold_50 = np.max(envelope) * 0.5
threshold_10 = np.max(envelope) * 0.1

for i, val in enumerate(envelope):
    if val >= threshold_50:
        print(f"  50% threshold at sample {i} ({i/44.1:.2f}ms)")
        break

for i in range(len(envelope)-1, -1, -1):
    if envelope[i] >= threshold_50:
        print(f"  50% decay at sample {i} ({i/44.1:.2f}ms)")
        break

# RMS over time (10-sample windows)
print(f"\nRMS by segment (10-sample windows):")
for i in range(0, len(attack), 10):
    segment = attack[i:min(i+10, len(attack))]
    rms = np.sqrt(np.mean(segment**2))
    print(f"  {i:3d}-{i+10:3d} ({i/44.1:5.2f}ms): RMS={rms:.6f}")

# Show every 10th sample
print(f"\nEvery 10th sample:")
for i in range(0, len(attack), 10):
    print(f"  [{i:3d}] ({i/44.1:5.2f}ms) {attack[i]:8.6f}")

if HAS_MATPLOTLIB:
    plt.figure(figsize=(16, 8))

    # Plot 1: Full attack waveform
    plt.subplot(2, 1, 1)
    time_ms = np.arange(len(attack)) / 44.1
    plt.plot(time_ms, attack, linewidth=2, color='#cc0000', label='Attack waveform')
    plt.plot(time_ms, envelope, linewidth=1, color='#0066cc', linestyle='--', alpha=0.7, label='Envelope (abs)')
    plt.axhline(y=peak_val, color='g', linestyle=':', linewidth=1, alpha=0.5, label=f'Peak: {peak_val:.3f}')
    plt.title('andremichelle bassdrum-attack.raw', fontsize=14, fontweight='bold')
    plt.xlabel('Time (ms)', fontsize=12)
    plt.ylabel('Amplitude', fontsize=12)
    plt.grid(True, alpha=0.3)
    plt.legend()

    # Plot 2: Zoomed first 50 samples
    plt.subplot(2, 1, 2)
    zoom_samples = 50
    plt.plot(time_ms[:zoom_samples], attack[:zoom_samples], linewidth=2, color='#cc0000', marker='o', markersize=3)
    plt.title('First 50 samples (zoomed)', fontsize=14, fontweight='bold')
    plt.xlabel('Time (ms)', fontsize=12)
    plt.ylabel('Amplitude', fontsize=12)
    plt.grid(True, alpha=0.3)

    plt.tight_layout()
    plt.savefig('attack_envelope_analysis.png', dpi=150, bbox_inches='tight')
    print(f"\n✓ Saved plot: attack_envelope_analysis.png")
