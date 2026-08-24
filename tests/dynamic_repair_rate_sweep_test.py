#!/usr/bin/env python3
"""Integration regression for the native DynamicSpareSharing repair sweep."""

from __future__ import annotations

import csv
from pathlib import Path
import subprocess
import sys
import tempfile


EXPECTED_CONFIGS = {
    ("no_sharing", 0),
    ("directional", 1),
    ("directional", 2),
    ("pairwise_edge", 1),
    ("pairwise_edge", 2),
    ("global_pool", 1),
}


def run_sweep(executable: Path, output_dir: Path) -> bytes:
    command = [
        str(executable),
        "2",
        "2",
        "--repair-rate-sweep",
        "--fault-min",
        "8",
        "--fault-max",
        "12",
        "--fault-step",
        "4",
        "--spare-min",
        "2",
        "--spare-max",
        "2",
        "--fault-model",
        "moderate",
        "--spatial",
        "mixed",
        "--runs",
        "4",
        "--seed",
        "77123",
        "--local-first",
        "--hybrid-cam-entry-width-bits",
        "64",
        "--summary-only",
        "--output-dir",
        str(output_dir),
    ]
    completed = subprocess.run(
        command,
        check=False,
        capture_output=True,
        text=True,
    )
    if completed.returncode != 0:
        raise AssertionError(
            "repair-rate sweep failed:\n"
            + completed.stdout
            + "\n"
            + completed.stderr
        )
    summary = output_dir / "summary.csv"
    if not summary.is_file():
        raise AssertionError("repair-rate sweep did not create summary.csv")
    if (output_dir / "attempts.csv").exists() or (output_dir / "runs.csv").exists():
        raise AssertionError("--summary-only emitted attempts.csv or runs.csv")
    return summary.read_bytes()


def validate_summary(contents: bytes) -> None:
    text = contents.decode("utf-8")
    rows = list(csv.DictReader(text.splitlines()))
    if len(rows) != 12:
        raise AssertionError(
            f"expected 12 representative configurations, got {len(rows)}"
        )
    if "fault_count" not in rows[0]:
        raise AssertionError("summary.csv does not expose fault_count")
    fault_counts = {int(row["fault_count"]) for row in rows}
    if fault_counts != {8, 12}:
        raise AssertionError(f"fault step was not respected: {fault_counts}")
    if {int(row["Rs"]) for row in rows} != {2} or {
        int(row["Cs"]) for row in rows
    } != {2}:
        raise AssertionError("summary.csv spare-line sweep coordinates are wrong")
    for fault_count in fault_counts:
        configurations = {
            (row["policy"], int(row["shared_rows"]))
            for row in rows
            if int(row["fault_count"]) == fault_count
        }
        if configurations != EXPECTED_CONFIGS:
            raise AssertionError(
                "representative configuration set changed for "
                f"fault_count={fault_count}: {configurations}"
            )
    baseline_rates = {
        int(row["fault_count"]): float(row["repair_rate"])
        for row in rows
        if row["policy"] == "no_sharing"
    }
    for row in rows:
        baseline_rate = baseline_rates[int(row["fault_count"])]
        if abs(float(row["baseline_repair_rate"]) - baseline_rate) > 1.0e-12:
            raise AssertionError("a sharing policy did not reuse the matching baseline")


def main() -> None:
    if len(sys.argv) != 2:
        raise SystemExit("usage: dynamic_repair_rate_sweep_test.py EXECUTABLE")
    executable = Path(sys.argv[1]).resolve()
    if not executable.is_file():
        raise AssertionError(f"simulator not found: {executable}")
    with tempfile.TemporaryDirectory(prefix="dynamic-sweep-first-") as first:
        first_contents = run_sweep(executable, Path(first))
    with tempfile.TemporaryDirectory(prefix="dynamic-sweep-second-") as second:
        second_contents = run_sweep(executable, Path(second))
    validate_summary(first_contents)
    if first_contents != second_contents:
        raise AssertionError("repair-rate sweep summary is not deterministic")
    print("Dynamic repair-rate sweep integration test passed")


if __name__ == "__main__":
    main()
