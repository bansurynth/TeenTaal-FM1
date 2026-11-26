#!/usr/bin/env python3
"""Check tanpura pitches between the calibration grid points (Sa interpolated), with the two jawari values (75, 100).

Run after building the probe: python3 tools/reference/verify_pitch.py --all-notes --jobs 4
Reports all cases beyond the limit (0.25 cents, 0.5 for FX previews) and fails if any exceeds it.
"""
import argparse
from concurrent.futures import ThreadPoolExecutor
import itertools
import random

from calibrate_pitch import render_pitch

LIMIT_CENTS = 0.25
PREVIEW_LIMIT_CENTS = 0.5  # FX plucks: the 6 s centroid mixes transients, see DECISIONS.md
JAWARI_VALUES = (75, 100)


def limit(kind):
    return PREVIEW_LIMIT_CENTS if kind == "preview" else LIMIT_CENTS


def exceeds(item):
    return round(item[0], 2) > limit(item[2][0])


def cases():
    rng = random.Random(20261120)
    for kind, count in (("first", 500), ("sa", 200), ("kharaj", 200), ("preview", 200)):
        for _ in range(count):
            sa, reg, jawari = rng.randint(-1900, 700), rng.randrange(2), rng.choice(JAWARI_VALUES)
            selection = rng.randrange(18) if kind == "first" else rng.randrange(-31, 8) * 100 if kind == "preview" else 0
            fine = 0 if kind == "preview" else rng.randint(-50, 50)
            a4 = rng.choice((4150, 4320, 4400, 4460, 4660, rng.randint(4150, 4660)))
            yield kind, sa, reg, jawari, selection, fine, a4

    sas = (-1900, -1800, -1200, -500, 0, 350, 500, 600, 700)
    tuning = ((-50, 4150), (-50, 4400), (-43, 4460), (0, 4400), (50, 4400), (50, 4660))
    for sa, reg, jawari, swara, (fine, a4) in itertools.product(
            sas, (0, 1), JAWARI_VALUES, (0, 10, 11, 12, 16, 17), tuning):
        yield "first", sa, reg, jawari, swara, fine, a4


def measure(case):
    kind, sa, reg, jawari, selection, fine, a4 = case
    _, error = render_pitch(sa, reg, jawari, kind, selection, fine=fine, a4=a4)
    return abs(error), error, case


def all_note_cases():
    for sa, reg, jawari in itertools.product(range(-1900, 701, 100), (0, 1), JAWARI_VALUES):
        for swara in range(18):
            yield "first", sa, reg, jawari, swara, 0, 4400
        yield "sa", sa, reg, jawari, 0, 0, 4400
        yield "kharaj", sa, reg, jawari, 0, 0, 4400
    for sa, reg, jawari, note in itertools.product(range(-1900, 701, 100), (0, 1), JAWARI_VALUES,
                                                   range(-3100, 701, 100)):
        yield "preview", sa, reg, jawari, note, 0, 4400


def report(results):
    for kind in ("first", "sa", "kharaj", "preview"):
        group = [item for item in results if item[2][0] == kind]
        if group:
            print(f"{kind:8s} {len(group):4d} cases; max {max(item[0] for item in group):.2f} cents; "
                  f"{sum(exceeds(item) for item in group)} above {limit(kind)} cents")
    outliers = sorted((item for item in results if exceeds(item)), reverse=True)
    for _, error, case in outliers[:40]:
        print(f"  {error:+.2f} cents: {case}")
    if len(outliers) > 40:
        print(f"  ... {len(outliers) - 40} more")


def breakdown(results, key, label):
    worst = {}
    for item in results:
        value = key(item[2])
        if value is not None:
            worst[value] = max(worst.get(value, 0.0), item[0])
    print(f"worst by {label}: " + " ".join(f"{value}={worst[value]:.2f}" for value in sorted(worst)))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--jobs", type=int, default=4, help="parallel probe processes (default: 4)")
    parser.add_argument("--all-notes", action="store_true",
                        help="also check all 27 Sa values (F2..G4), both registers, both jawari values and all swaras")
    args = parser.parse_args()
    if args.jobs < 1:
        parser.error("--jobs must be positive")
    grid = []
    with ThreadPoolExecutor(max_workers=args.jobs) as pool:
        results = list(pool.map(measure, cases()))
        if args.all_notes:
            grid = list(pool.map(measure, all_note_cases()))
    report(results)
    if grid:
        print("semitone grid:")
        report(grid)
        breakdown(grid, lambda case: case[1], "Sa")
        breakdown(grid, lambda case: case[4] if case[0] == "first" else None, "swara")
    return int(any(exceeds(item) for item in results + grid))


if __name__ == "__main__":
    raise SystemExit(main())
