#!/usr/bin/env python3
"""Measure tanpura pitch on a Sa x jawari grid.

Run after building build-host/render and build-host/tstring_dump:
  python3 tools/reference/pitch_grid.py

The normal-steel profiles cover a lower Sa, Pa, Sa and the upper preview note.
The short-steel profiles cover the upper Sa..Ma strings; kharaj is measured
separately because it uses bronze. WAV profiles use the isolated string's
spectrum from 1.0 to 4.9 s and report residual offsets with the renderer's
current compensation, not replacement table values. The preview profile uses
the raw nut force from tstring_dump and reports the uncompensated contact shift.
"""
import math
import os
import subprocess
import tempfile
import wave

import numpy as np

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
FS = 24_000_000 / 544
STRING_FS = FS / 2
SA = [-1800, -1500, -1200, -1000, -800, -500, -300, 0, 700]
JAWARI = [0, 25, 50, 75, 100]

PROFILES = [
    ("normal_sa_below_octave", 0, "Sa", -1200),
    ("normal_pa", 0, "Pa", -500),
    ("normal_sa", 1, None, 0),
    ("normal_preview_ma", None, None, 500),
    ("upper_sa", 0, "Sa^", 0),
    ("upper_ma", 0, "Ma^", 500),
    ("kharaj", 3, None, -1200),
]


def target_hz(cents):
    return 440.0 * 2.0 ** ((cents - 900.0) / 1200.0)


def pitch_offset(samples, fs, target):
    start, stop = int(fs), int(4.9 * fs)
    samples = samples[start:stop].astype(np.float64)
    n = len(samples)
    nfft = 1 << int(math.ceil(math.log2(n * 8)))
    spectrum = np.abs(np.fft.rfft(samples * np.hanning(n), n=nfft))
    freqs = np.fft.rfftfreq(nfft, 1.0 / fs)
    band = np.where((freqs >= target * 0.92) & (freqs <= target * 1.08))[0]
    i = band[np.argmax(spectrum[band])]
    y = np.log(np.maximum(spectrum[i - 1:i + 2], 1e-30))
    den = y[0] - 2.0 * y[1] + y[2]
    delta = 0.5 * (y[0] - y[2]) / den if den else 0.0
    peak_hz = (i + delta) * fs / nfft
    return 1200.0 * math.log2(peak_hz / target)


def wav_left(path):
    with wave.open(path, "rb") as wav:
        raw = np.frombuffer(wav.readframes(wav.getnframes()), dtype=np.uint8).reshape(-1, 6)
    value = raw[:, 0].astype(np.int32) | (raw[:, 1].astype(np.int32) << 8) | (raw[:, 2].astype(np.int32) << 16)
    return (value ^ (1 << 23)) - (1 << 23)


def render_profile(sa, jawari, string, swara, relative, directory):
    path = os.path.join(directory, f"render_{sa}_{jawari}.wav")
    events = [
        f"0:sa={sa}", "0:tan.reg=0", f"0:tan.jawari={jawari}", "0:tan2.on=0", "0:tan.human=0",
        "0:tan.vol0=0", "0:tan.vol1=0", "0:tan.vol2=0", "0:tan.vol3=0",
    ]
    events.append(f"0:tan.vol{string}=100")
    if swara:
        events.append(f"0:tan.first={swara}")
    events.append("0:tan.on=1")
    subprocess.run([os.path.join(ROOT, "build-host", "render"), path, "6", *events],
                   check=True, capture_output=True)
    return pitch_offset(wav_left(path), FS, target_hz(sa + relative))


def render_preview_profile(sa, jawari, directory):
    t = min(max((sa + 1500.0) / 1200.0, 0.0), 1.83)
    length = 1.1 - 0.25 * t
    radius = 0.16 - 0.03 * t
    high_sa = min(max(sa / 700.0, 0.0), 1.0)
    target = target_hz(sa + 500)
    path = os.path.join(directory, f"preview_{sa}_{jawari}.f32")
    subprocess.run([
        os.path.join(ROOT, "build-host", "tstring_dump"),
        f"{target:.9f}", f"{length:.9f}", f"{radius:.9f}", "steel",
        f"{(4.0 + 4.0 * jawari / 100.0) * 1e-3:.6f}", f"{0.35 * length:.9f}", "0.02",
        "0.4", "0.8", "96", f"{8000.0 * (1.0 + high_sa):.1f}", "6",
        f"{STRING_FS:.3f}", path,
    ], check=True, capture_output=True)
    force = np.fromfile(path, dtype="<f4").reshape(-1, 3)[:, 1]
    return pitch_offset(force, STRING_FS, target)


def main():
    with tempfile.TemporaryDirectory(prefix="tan-pitch-grid-") as directory:
        for name, string, swara, relative in PROFILES:
            measure = "uncompensated contact shift" if name == "normal_preview_ma" else "residual pitch offset"
            print(f"\n{name} ({measure} in cents), Sa rows={SA}, jawari columns={JAWARI}:")
            for sa in SA:
                row = []
                for jawari in JAWARI:
                    if name == "normal_preview_ma":
                        value = render_preview_profile(sa, jawari, directory)
                    else:
                        value = render_profile(sa, jawari, string, swara, relative, directory)
                    row.append(value)
                print(f"{sa:5}: " + " ".join(f"{value:6.1f}" for value in row))


if __name__ == "__main__":
    main()
