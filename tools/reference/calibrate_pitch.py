#!/usr/bin/env python3
"""Calibrate the tanpura's contact pitch against its rendered fundamental.

Build first: make build-host/pitch_probe
Measure representative configurations: python3 tools/reference/calibrate_pitch.py
Regenerate the firmware tables:
  python3 tools/reference/calibrate_pitch.py --generate --jobs 4
  python3 tools/reference/calibrate_pitch.py --generate --upper --jobs 4

The probe renders the same string, body and output filters used by the firmware.
The pitch is the power-weighted centroid within 80 cents of the requested
fundamental, averaged over the whole 6 seconds after a pluck. This avoids choosing
an arbitrary peak when the jawari splits the fundamental into nearby components,
and it does not depend on a window that falls in the decaying tail.
"""
import argparse
from concurrent.futures import FIRST_COMPLETED, ProcessPoolExecutor, wait
import hashlib
import json
import math
import os
from pathlib import Path
import subprocess
import tempfile

import numpy as np

ROOT = Path(__file__).resolve().parents[2]
PROBE = ROOT / "build-host" / "pitch_probe"
STANDARD_SA = (-1900, -1800, -1700, -1600, -1500, -1400, -1300, -1200, -1100, -1000, -900, -800, -700,
               -600, -500, -400, -300, -200, -100, 0, 100, 200, 300, 400, 500, 600, 700)
LARGE_SA = (0, 100, 200, 300, 400, 500, 600, 700)
JAWARI = (75, 100)
SHIFTS = (-200, 0, 200)
PREVIEW_NOTES = tuple(range(-3100, 701, 100))  # FX + SEL plucks F2..E3 one octave down, from -3100 cents
UPPER_SWARAS = tuple(range(11, 18))
TABLES = (
    ("PITCH_FIRST", "first", tuple(range(18))),
    ("PITCH_STEEL", "sa", SHIFTS),
    ("PITCH_KHARAJ", "kharaj", SHIFTS),
    ("PITCH_PREVIEW", "preview", PREVIEW_NOTES),
)
UPPER_TABLES = (
    ("PITCH_FIRST_FLAT", "first_flat", UPPER_SWARAS),
    ("PITCH_FIRST_SHARP", "first_sharp", UPPER_SWARAS),
)
MEASURE_SECONDS = 6.0
TARGET_CENTS = 0.1
FAIL_CENTS = 0.31
PREVIEW_FAIL_CENTS = 0.5  # FX plucks: the 6 s centroid mixes transients, see DECISIONS.md
MAX_PROBES = 6
SCAN_STEPS = 10


def fail_limit(kind):
    return PREVIEW_FAIL_CENTS if kind == "preview" else FAIL_CENTS


def within_limit(residual, kind):
    # Compared at 0.01 cent, the resolution at which residuals are reported.
    return round(abs(residual), 2) <= fail_limit(kind)


def render_pitch(sa, reg, jawari, kind, selection, offset=0, fine=0, a4=4400, correction="auto"):
    args = [
        str(PROBE), str(sa), str(reg), str(jawari), kind, str(selection), str(fine), str(a4),
        str(correction), str(MEASURE_SECONDS), str(offset),
    ]
    result = subprocess.run(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=False)
    if result.returncode:
        raise RuntimeError(f"{' '.join(args)} failed: {result.stderr.decode(errors='replace').strip()}")
    data = np.frombuffer(result.stdout, dtype=np.float32)
    if len(data) < 4:
        raise RuntimeError(f"{' '.join(args)} returned incomplete audio")
    target, applied, fs = (float(value) for value in data[:3])
    samples = data[3::2].astype(np.float64)
    fs *= 0.5
    if not (math.isfinite(target) and target > 0 and math.isfinite(applied) and len(samples) > 0):
        raise RuntimeError(f"{' '.join(args)} returned invalid metadata")
    n = len(samples)
    nfft = 1 << (4 * n - 1).bit_length()
    spectrum = np.abs(np.fft.rfft(samples * np.hanning(n), n=nfft)) ** 2
    freqs = np.fft.rfftfreq(nfft, 1.0 / fs)
    band = (freqs >= target * 2.0 ** (-80.0 / 1200.0)) & (freqs <= target * 2.0 ** (80.0 / 1200.0))
    power = spectrum[band]
    total = float(np.sum(power))
    if not math.isfinite(total) or total <= 0.0:
        raise RuntimeError(f"{' '.join(args)} returned silence or non-finite audio near {target:.3f} Hz")
    centre = float(np.sum(freqs[band] * power) / total)
    return applied, 1200.0 * math.log2(centre / target)


def calibrate(job):
    table, row, sa_index, jawari_index, sa, reg, jawari, kind, selection, offset = job
    measured = {}

    def error_at(correction):
        if correction not in measured:
            _, measured[correction] = render_pitch(sa, reg, jawari, kind, selection, offset, correction=correction)
        return measured[correction]

    def best():
        return min(measured, key=lambda value: abs(measured[value]))

    correction = 0.0
    for _ in range(MAX_PROBES):
        error = error_at(correction)
        if abs(error) <= TARGET_CENTS:
            break
        if len(measured) == 1:
            proposal = correction + error
        else:
            (c1, e1), (c2, e2) = sorted(measured.items(), key=lambda item: abs(item[1]))[:2]
            slope = (e2 - e1) / (c2 - c1)
            if not -1.5 < slope < -0.5:
                slope = -1.0
            proposal = c2 - e2 / slope
        correction = round(proposal, 1)
        if correction in measured:
            break
    # The measured centroid can jump by several tenths of a cent over 0.1 cent of correction
    # (a mode enters or leaves the truncated series), so the secant may not reach a zero.
    # Scan the 0.1-cent grid around the best estimate and keep the smallest error.
    centre = best()
    for step in range(1, SCAN_STEPS + 1):
        if abs(measured[best()]) <= TARGET_CENTS:
            break
        error_at(round(centre + 0.1 * step, 1))
        error_at(round(centre - 0.1 * step, 1))
    chosen = best()
    return table, row, sa_index, jawari_index, chosen, measured[chosen]


def calibration_jobs(tables=TABLES):
    for suffix, sa_values, reg in (("", STANDARD_SA, 0), ("_LARGE", LARGE_SA, 1)):
        for name, kind, selections in tables:
            for row, value in enumerate(selections):
                for sa_index, sa in enumerate(sa_values):
                    for jawari_index, jawari in enumerate(JAWARI):
                        probe_kind = "first" if kind in ("first_flat", "first_sharp") else kind
                        selection = value if probe_kind in ("first", "preview") else 0
                        offset = (-200 if kind == "first_flat" else 200 if kind == "first_sharp" else
                                  value if kind in ("sa", "kharaj") else 0)
                        yield (name + suffix, row, sa_index, jawari_index, sa, reg, jawari,
                               probe_kind, selection, offset)


def save_cache(path, cache):
    with tempfile.TemporaryDirectory(prefix="pitch-cache-", dir=path.parent) as directory:
        temporary = Path(directory) / path.name
        temporary.write_text(json.dumps(cache, sort_keys=True), encoding="utf-8")
        os.replace(temporary, path)


def generate(output, jobs, cache_path, upper=False):
    specs = UPPER_TABLES if upper else TABLES
    tables = {}
    for suffix, sa_values in (("", STANDARD_SA), ("_LARGE", LARGE_SA)):
        for name, _, selections in specs:
            tables[name + suffix] = np.empty((len(selections), len(sa_values), len(JAWARI)), dtype=np.int16)
    work = list(calibration_jobs(specs))
    signature = hashlib.sha256(PROBE.read_bytes()).hexdigest()
    cache = {"version": 2, "probe": signature, "numpy": np.__version__, "results": {}}
    if cache_path:
        cache_path.parent.mkdir(parents=True, exist_ok=True)
        if cache_path.exists():
            saved = json.loads(cache_path.read_text(encoding="utf-8"))
            if any(saved.get(key) != cache[key] for key in ("version", "probe", "numpy")):
                raise RuntimeError(f"calibration cache does not match the probe or NumPy: {cache_path}")
            cache = saved
    done = 0
    worst = 0.0
    remaining = []
    failed = []
    for job in work:
        key = "|".join(str(value) for value in job)
        if key in cache["results"] and within_limit(cache["results"][key][1], job[7]):
            value, residual = cache["results"][key]
            table, row, sa_index, jawari_index = job[:4]
            tables[table][row, sa_index, jawari_index] = int(round(10.0 * value))
            worst = max(worst, abs(residual))
            done += 1
        else:
            remaining.append(job)
    print(f"Calibrating {len(work)} string configurations with {jobs} workers ({done} cached)", flush=True)
    with ProcessPoolExecutor(max_workers=jobs) as pool:
        pending = {}
        todo = iter(remaining)
        for _ in range(min(2 * jobs, len(remaining))):
            job = next(todo)
            pending[pool.submit(calibrate, job)] = job
        while pending:
            finished, _ = wait(pending, return_when=FIRST_COMPLETED)
            for future in finished:
                job = pending.pop(future)
                table, row, sa_index, jawari_index, value, residual = future.result()
                worst = max(worst, abs(residual))
                if not within_limit(residual, job[7]):
                    failed.append((job, residual))
                quantized = int(round(10.0 * value))
                if not -32768 <= quantized <= 32767:
                    raise RuntimeError(f"pitch correction outside int16 range: {table} {value}")
                tables[table][row, sa_index, jawari_index] = quantized
                cache["results"]["|".join(str(item) for item in job)] = [value, residual]
                done += 1
                if cache_path and done % 100 == 0:
                    save_cache(cache_path, cache)
                if done % 250 == 0 or done == len(work):
                    print(f"  {done}/{len(work)}", flush=True)
                next_job = next(todo, None)
                if next_job is not None:
                    pending[pool.submit(calibrate, next_job)] = next_job
    if cache_path:
        save_cache(cache_path, cache)
    if failed:
        for job, residual in failed[:20]:
            print(f"  not converged: {job} residual {residual:+.2f} cents", flush=True)
        raise RuntimeError(f"{len(failed)} configurations exceed {FAIL_CENTS} cents; tables not written")

    lines = [
        "/* Generated by tools/reference/calibrate_pitch.py --generate.",
        " * Audible fundamental, power-weighted average over 0-6 s after a pluck. Values are 0.1 cent.",
        " * Standard rows use the effective Sa; LARGE rows use the player's Sa with the lower register.",
        " * Columns are jawari 75, 100.",
    ]
    if upper:
        lines += [
            " * Fixed string geometry, pitch shifted by -200 or +200 cents.",
            " * Regenerate with --generate --upper. */",
            "#pragma once",
            '#include "pitch_coefs.h"',
            f"#define PITCH_FIRST_SHIFT_START {UPPER_SWARAS[0]}",
            f"#define PITCH_FIRST_SHIFT_N {len(UPPER_SWARAS)}",
            "",
        ]
    else:
        lines += [
            " */",
            "#pragma once",
            "#include <stdint.h>",
            "",
            f"#define PITCH_STANDARD_SA_N {len(STANDARD_SA)}",
            f"#define PITCH_LARGE_SA_N {len(LARGE_SA)}",
            f"#define PITCH_JAWARI_N {len(JAWARI)}",
            f"#define PITCH_FIRST_N {len(TABLES[0][2])}",
            f"#define PITCH_SHIFT_N {len(SHIFTS)}",
            f"#define PITCH_PREVIEW_N {len(PREVIEW_NOTES)}",
            "static const float PITCH_STANDARD_SA[PITCH_STANDARD_SA_N] = {" +
            ", ".join(f"{value}.0f" for value in STANDARD_SA) + "};",
            "static const float PITCH_LARGE_SA[PITCH_LARGE_SA_N] = {" +
            ", ".join(f"{value}.0f" for value in LARGE_SA) + "};",
            "",
        ]
    for suffix, sa_values in (("", STANDARD_SA), ("_LARGE", LARGE_SA)):
        for name, _, selections in specs:
            table = tables[name + suffix]
            sa_count = "PITCH_LARGE_SA_N" if suffix else "PITCH_STANDARD_SA_N"
            row_count = ("PITCH_FIRST_SHIFT_N" if upper else "PITCH_FIRST_N" if name == "PITCH_FIRST" else
                         "PITCH_PREVIEW_N" if name == "PITCH_PREVIEW" else "PITCH_SHIFT_N")
            lines.append(f"static const int16_t {name + suffix}[{row_count}][{sa_count}][PITCH_JAWARI_N] = {{")
            for row, value in enumerate(selections):
                lines.append(f"    {{ /* {value:+d} */")
                for sa_index, _ in enumerate(sa_values):
                    values = ", ".join(f"{int(value):4d}" for value in table[row, sa_index])
                    lines.append(f"        {{{values}}},")
                lines.append("    },")
            lines.extend(("};", ""))
    output.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="pitch-tables-", dir=output.parent) as directory:
        temporary = Path(directory) / output.name
        temporary.write_text("\n".join(lines), encoding="utf-8")
        os.replace(temporary, output)
    print(f"Wrote {output}; largest grid-point residual {worst:.2f} cents")


def measure_examples():
    cases = (
        ("G3 normal Sa", -500, 0, 75, "sa", 0, 0, 4400),
        ("G3 large Sa", -500, 1, 75, "sa", 0, 0, 4400),
        ("G3 as upper Ma", 200, 1, 75, "first", 17, 0, 4400),
        ("E3 upper Ma", -800, 0, 100, "first", 17, 0, 4400),
        ("F2 Pa", -1900, 0, 75, "first", 7, 0, 4400),
        ("G4 kharaj", 700, 0, 100, "kharaj", 0, 0, 4400),
        ("G3 first +37", -500, 0, 75, "first", 7, 37, 4400),
        ("G3 Sa A415", -500, 0, 75, "sa", 0, 0, 4150),
        ("G3 Sa A466", -500, 0, 75, "sa", 0, 0, 4660),
        ("G3 preview", -500, 1, 100, "preview", -500, 0, 4400),
    )
    failed = False
    for name, sa, reg, jawari, kind, selection, fine, a4 in cases:
        applied, error = render_pitch(sa, reg, jawari, kind, selection, fine=fine, a4=a4)
        print(f"{name:18s}: {error:+7.2f} cents (correction {applied:+6.2f})")
        failed |= not within_limit(error, kind)
    return int(failed)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--generate", action="store_true", help="regenerate the C calibration tables")
    parser.add_argument("--upper", action="store_true",
                        help="generate fixed-geometry fine-tuning tables for the highest first-string notes")
    parser.add_argument("--jobs", type=int, default=4, help="parallel calibration workers (default: 4)")
    parser.add_argument("--output", type=Path, help="output header (default: tanpura/pitch[_upper]_coefs.h)")
    parser.add_argument("--cache", type=Path, help="resume a calibration using this intermediate cache")
    args = parser.parse_args()
    if args.jobs < 1:
        parser.error("--jobs must be positive")
    if args.upper and not args.generate:
        parser.error("--upper requires --generate")
    if not PROBE.is_file():
        parser.error("build the calibration probe first: make build-host/pitch_probe")
    if args.generate:
        output = args.output or ROOT / "tanpura" / ("pitch_upper_coefs.h" if args.upper else "pitch_coefs.h")
        generate(output, args.jobs, args.cache, upper=args.upper)
        return 0
    return measure_examples()


if __name__ == "__main__":
    raise SystemExit(main())
