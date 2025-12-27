#!/bin/bash
# Snare drum analysis script - compares rendered output to real TR-909 samples

cd "$(dirname "$0")/.." || exit 1

gcc -o tools/render_drums tools/render_drums.c synth/rg909_drum_synth.c synth/synth_resonator.c synth/synth_noise.c synth/synth_filter.c synth/synth_envelope.c synth/synth_voice_manager.c -Isynth -lm -O2 && \
./tools/render_drums && \
echo "" && \
echo "=== WAVEFORM SHAPE ANALYSIS ===" && \
python3 tools/check_drums.py && \
python3 tools/compare_attack.py && \
echo "" && \
python3 tools/compare_snare_text.py && \
echo "" && \
python3 << 'EOF'
import wave
import numpy as np

with wave.open('RG909_SD_Snare.wav', 'rb') as wav:
    sr = wav.getframerate()
    frames = wav.readframes(wav.getnframes())
    rendered = np.frombuffer(frames, dtype=np.int16).astype(np.float32) / 32768.0

with wave.open('/mnt/e/Samples/TR909/909 Snare2_3.wav', 'rb') as wav:
    frames = wav.readframes(wav.getnframes())
    real = np.frombuffer(frames, dtype=np.int16).astype(np.float32) / 32768.0

# Find attack transient (first sample > 0.05 threshold) for alignment
threshold = 0.05
rendered_start = np.argmax(np.abs(rendered) > threshold)
real_start = np.argmax(np.abs(real) > threshold)

# Find peaks RELATIVE to transient (skip first 3ms to ignore initial spike)
skip_spike = int(0.003 * sr)
rendered_peak_idx = rendered_start + skip_spike + np.argmax(np.abs(rendered[rendered_start+skip_spike:rendered_start+int(0.1*sr)]))
real_peak_idx = real_start + np.argmax(np.abs(real[real_start:real_start+int(0.1*sr)]))

# Analyze 500ms from transient start
samples_500ms = min(len(rendered) - rendered_start, len(real) - real_start, int(0.5 * sr))
rendered_rms = np.sqrt(np.mean(rendered[rendered_start:rendered_start+samples_500ms]**2))
real_rms = np.sqrt(np.mean(real[real_start:real_start+samples_500ms]**2))

rendered_peak_time = (rendered_peak_idx - rendered_start) / sr * 1000
real_peak_time = (real_peak_idx - real_start) / sr * 1000

print(f"\n=== TR-909 SNARE DRUM ANALYSIS ===")
print(f"Rendered: RMS={rendered_rms:.3f}, peak={rendered_peak_time:.1f}ms, max={np.max(np.abs(rendered)):.3f}")
print(f"Real 909: RMS={real_rms:.3f}, peak={real_peak_time:.1f}ms, max={np.max(np.abs(real)):.3f}")
print(f"\nRMS match: {rendered_rms/real_rms:.2f}x ({'✓ GOOD' if abs(rendered_rms/real_rms - 1.0) < 0.10 else '⚠ ADJUST'})")
print(f"Peak timing: {'✓ GOOD' if abs(rendered_peak_time - real_peak_time) < 2.5 else '⚠ OFF'} ({abs(rendered_peak_time - real_peak_time):.1f}ms diff)")
print(f"\nWaveform: Twin-T resonators + filtered noise")

# Triangle character check (from aligned start)
cycle_start = rendered_start + int(0.020 * sr)
cycle_len = int(sr / 80)
waveform = rendered[cycle_start:cycle_start+cycle_len]
peak_idx = np.argmax(waveform)

if peak_idx > 4:
    peak_region = waveform[peak_idx-4:peak_idx+5]
    diffs = np.diff(peak_region)
    pos_slope = diffs[:4]
    neg_slope = diffs[5:]

    slope_consistency = np.std(pos_slope) < 0.001 and np.std(neg_slope) < 0.001
    sharp_flip = abs(diffs[4]) < abs(diffs[3]) * 0.3

    print(f"\nTriangle character: {'✓ PRESERVED' if slope_consistency and sharp_flip else '✗ ROUNDED'}")
    if not slope_consistency or not sharp_flip:
        print(f"  Slopes: {diffs}")

print(f"\n{'='*40}")
EOF
