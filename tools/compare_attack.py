#!/usr/bin/env python3
import wave
import struct
import numpy as np
try:
    import matplotlib
    matplotlib.use('Agg')
    import matplotlib.pyplot as plt
    HAS_MATPLOTLIB = True
except ImportError:
    HAS_MATPLOTLIB = False

# Load real 909 attack
try:
    with wave.open('/mnt/e/Samples/r909attack.wav', 'rb') as w:
        r909_frames = w.getnframes()
        r909_rate = w.getframerate()
        r909_data = w.readframes(r909_frames)
        r909 = np.array(struct.unpack(f'{r909_frames}h', r909_data), dtype=np.float32) / 32768.0
    has_real = True
except:
    print("⚠ Real attack sample not found")
    has_real = False

# Load new synth
with wave.open('RG909_BD_BassDrum.wav', 'rb') as w:
    new_frames = w.getnframes()
    new_rate = w.getframerate()
    new_data = w.readframes(new_frames)
    new = np.array(struct.unpack(f'{new_frames}h', new_data), dtype=np.float32) / 32768.0

if has_real:
    # Compare first 6ms
    samples_6ms = int(0.006 * new_rate)
    r909_6ms = int(0.006 * r909_rate)

    print("\n=== ATTACK SPIKE COMPARISON ===")
    print("\nTime   | Real 909  | NEW Synth | Diff")
    print("-------|-----------|-----------|------")
    for i in range(0, min(samples_6ms, r909_6ms, len(new)), max(1, samples_6ms//20)):
        t_ms = i / new_rate * 1000
        r_idx = int(i * r909_rate / new_rate)
        if r_idx < len(r909):
            diff = new[i] - r909[r_idx]
            print(f"{t_ms:5.2f}ms | {r909[r_idx]:+.4f}    | {new[i]:+.4f}    | {diff:+.4f}")

    # Key points
    r909_2ms_idx = int(0.002 * r909_rate)
    new_2ms_idx = int(0.002 * new_rate)
    r909_4ms_idx = int(0.004 * r909_rate)
    new_4ms_idx = int(0.004 * new_rate)

    print(f"\nKey measurements:")
    print(f"  At 2ms:  Real={r909[r909_2ms_idx]:+.3f}, Synth={new[new_2ms_idx]:+.3f}")
    print(f"  At 4ms:  Real={r909[r909_4ms_idx]:+.3f}, Synth={new[new_4ms_idx]:+.3f}")

    # Find peaks
    r909_peak = np.max(r909)
    new_peak = np.max(new[:samples_6ms])
    r909_trough = np.min(r909)
    new_trough = np.min(new[:samples_6ms])

    print(f"  Peak:    Real={r909_peak:+.3f}, Synth={new_peak:+.3f}")
    print(f"  Trough:  Real={r909_trough:+.3f}, Synth={new_trough:+.3f}")

    # Plot if matplotlib available
    if HAS_MATPLOTLIB:
        fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(14, 8))

        r909_time = np.arange(len(r909)) / r909_rate * 1000
        new_time = np.arange(samples_6ms) / new_rate * 1000

        ax1.plot(r909_time, r909, 'b-', linewidth=3, label='Real 909', alpha=0.8)
        ax1.plot(new_time, new[:samples_6ms], 'r-', linewidth=2, label='NEW Synth', alpha=0.8)
        ax1.axhline(y=0, color='k', linestyle='--', alpha=0.3)
        ax1.axvline(x=1.9, color='g', linestyle=':', label='Phase 1→2')
        ax1.axvline(x=3.8, color='orange', linestyle=':', label='Phase 2→3')
        ax1.axvline(x=5.5, color='purple', linestyle=':', label='Phase 3 end')
        ax1.grid(True, alpha=0.3)
        ax1.set_xlabel('Time (ms)')
        ax1.set_ylabel('Amplitude')
        ax1.set_title('Attack Spike Comparison (0-6ms)')
        ax1.legend()

        # Closeup on spike shape
        closeup_samples = min(240, len(r909), len(new))
        ax2.plot(r909_time[:closeup_samples], r909[:closeup_samples], 'b-', linewidth=3, marker='o', markersize=4, label='Real 909')
        ax2.plot(new_time[:closeup_samples], new[:closeup_samples], 'r-', linewidth=2, marker='s', markersize=3, label='NEW Synth')
        ax2.axhline(y=0, color='k', linestyle='--', alpha=0.3)
        ax2.grid(True, alpha=0.3)
        ax2.set_xlabel('Time (ms)')
        ax2.set_ylabel('Amplitude')
        ax2.set_title('Spike Detail (first 5ms)')
        ax2.legend()
        ax2.set_xlim(0, 5)

        plt.tight_layout()
        plt.savefig('spike_comparison.png', dpi=150)
        print("\n✓ Saved spike_comparison.png")
