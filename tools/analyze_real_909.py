#!/usr/bin/env python3
import wave
import numpy as np

# Read real TR-909
with wave.open('/mnt/e/Samples/TR909/909 Kick_long.wav', 'rb') as wav:
    sr = wav.getframerate()
    frames = wav.readframes(wav.getnframes())
    real = np.frombuffer(frames, dtype=np.int16).astype(np.float32) / 32768.0

# Read rendered
with wave.open('RG909_BD_BassDrum.wav', 'rb') as wav:
    frames = wav.readframes(wav.getnframes())
    rendered = np.frombuffer(frames, dtype=np.int16).astype(np.float32) / 32768.0

print("=== DETAILED TR-909 ANALYSIS ===\n")

# Peak analysis
real_peak_idx = np.argmax(np.abs(real))
rendered_peak_idx = np.argmax(np.abs(rendered))

real_peak_time = real_peak_idx / sr * 1000
rendered_peak_time = rendered_peak_idx / sr * 1000

print(f"Peak timing:")
print(f"  Real:     {real_peak_time:.1f}ms (sample {real_peak_idx})")
print(f"  Rendered: {rendered_peak_time:.1f}ms (sample {rendered_peak_idx})")
print(f"  Difference: {abs(real_peak_time - rendered_peak_time):.1f}ms")

# Decay analysis - measure time to decay to various levels
real_peak_val = np.abs(real[real_peak_idx])
rendered_peak_val = np.abs(rendered[rendered_peak_idx])

print(f"\nPeak amplitude:")
print(f"  Real:     {real_peak_val:.3f}")
print(f"  Rendered: {rendered_peak_val:.3f}")

# Find decay times
def find_decay_time(signal, peak_idx, peak_val, threshold_db):
    threshold = peak_val * (10 ** (threshold_db / 20))
    for i in range(peak_idx, len(signal)):
        if np.abs(signal[i]) < threshold:
            return (i - peak_idx) / sr * 1000
    return None

print(f"\nDecay times (from peak):")
for db in [-6, -12, -20, -40]:
    real_decay = find_decay_time(real, real_peak_idx, real_peak_val, db)
    rendered_decay = find_decay_time(rendered, rendered_peak_idx, rendered_peak_val, db)
    print(f"  {db:3d}dB: Real={real_decay:6.1f}ms, Rendered={rendered_decay:6.1f}ms")

# Attack analysis - measure rise time
def find_attack_start(signal, peak_idx, peak_val, threshold_db=-40):
    threshold = peak_val * (10 ** (threshold_db / 20))
    for i in range(peak_idx, -1, -1):
        if np.abs(signal[i]) < threshold:
            return i
    return 0

real_attack_start = find_attack_start(real, real_peak_idx, real_peak_val)
rendered_attack_start = find_attack_start(rendered, rendered_peak_idx, rendered_peak_val)

real_attack_time = (real_peak_idx - real_attack_start) / sr * 1000
rendered_attack_time = (rendered_peak_idx - rendered_attack_start) / sr * 1000

print(f"\nAttack time (to peak):")
print(f"  Real:     {real_attack_time:.2f}ms")
print(f"  Rendered: {rendered_attack_time:.2f}ms")

# RMS comparison over different windows
print(f"\nRMS levels:")
for window_ms in [50, 100, 200, 500]:
    window_samples = int(window_ms / 1000 * sr)
    real_rms = np.sqrt(np.mean(real[:window_samples]**2))
    rendered_rms = np.sqrt(np.mean(rendered[:window_samples]**2))
    print(f"  {window_ms:3d}ms: Real={real_rms:.3f}, Rendered={rendered_rms:.3f}, Ratio={rendered_rms/real_rms:.2f}x")

# Check for presence of attack click
print(f"\nAttack transient (first 0.5ms):")
first_samples = int(0.0005 * sr)
real_attack_max = np.max(np.abs(real[:first_samples]))
rendered_attack_max = np.max(np.abs(rendered[:first_samples]))
print(f"  Real:     {real_attack_max:.3f}")
print(f"  Rendered: {rendered_attack_max:.3f}")
