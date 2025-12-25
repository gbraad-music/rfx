#!/bin/bash
# Bass drum analysis script - compares rendered output to real TR-909 samples

gcc -o render_drums render_drums.c rg909_drum_synth.c synth_resonator.c synth_noise.c synth_filter.c synth_envelope.c synth_voice_manager.c -lm -O2 && \
./render_drums && \
echo "" && \
echo "=== WAVEFORM SHAPE ANALYSIS ===" && \
python3 check_drums.py && \
echo "" && \
python3 << 'EOF'
import wave
import numpy as np

with wave.open('RG909_BD_BassDrum.wav', 'rb') as wav:
    sr = wav.getframerate()
    frames = wav.readframes(wav.getnframes())
    rendered = np.frombuffer(frames, dtype=np.int16).astype(np.float32) / 32768.0

with wave.open('/mnt/e/Samples/TR909/909 Kick_long.wav', 'rb') as wav:
    frames = wav.readframes(wav.getnframes())
    real = np.frombuffer(frames, dtype=np.int16).astype(np.float32) / 32768.0

rendered_peak_idx = np.argmax(np.abs(rendered))
real_peak_idx = np.argmax(np.abs(real))

samples_500ms = min(len(rendered), len(real), int(0.5 * sr))
rendered_rms = np.sqrt(np.mean(rendered[:samples_500ms]**2))
real_rms = np.sqrt(np.mean(real[:samples_500ms]**2))

rendered_peak_time = rendered_peak_idx / sr * 1000
real_peak_time = real_peak_idx / sr * 1000

print(f"\n=== TR-909 BASS DRUM ANALYSIS ===")
print(f"Rendered: RMS={rendered_rms:.3f}, peak={rendered_peak_time:.1f}ms, max={np.max(np.abs(rendered)):.3f}")
print(f"Real 909: RMS={real_rms:.3f}, peak={real_peak_time:.1f}ms, max={np.max(np.abs(real)):.3f}")
print(f"\nRMS match: {rendered_rms/real_rms:.2f}x ({'✓ GOOD' if abs(rendered_rms/real_rms - 1.0) < 0.10 else '⚠ ADJUST'})")
print(f"Peak timing: {'✓ GOOD' if abs(rendered_peak_time - real_peak_time) < 2.5 else '⚠ OFF'} ({abs(rendered_peak_time - real_peak_time):.1f}ms diff)")
print(f"\nWaveform: Sine + 2nd/3rd harmonics for analog warmth")

# Triangle character check
cycle_start = int(0.020 * sr)
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
