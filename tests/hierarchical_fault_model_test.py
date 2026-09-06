#!/usr/bin/env python3
"""Regression for deterministic moderate/mixed hierarchical fault input."""

from __future__ import annotations

import csv
import subprocess
import sys
import tempfile
from collections import Counter
from pathlib import Path


def corpus_rows(path: Path) -> tuple[list[str], list[tuple[int, ...]]]:
    headers: list[str] = []
    rows: list[tuple[int, ...]] = []
    for line in path.read_text(encoding="utf-8").splitlines():
        if line.startswith("#"):
            headers.append(line)
        elif line.strip():
            fields = tuple(int(field) for field in line.split())
            if len(fields) != 7:
                raise AssertionError(f"invalid corpus row: {line}")
            rows.append(fields)
    return headers, rows


def canonical_rows(path: Path) -> dict[str, dict[str, str]]:
    with path.open(newline="", encoding="utf-8") as source:
        return {row["configuration_id"]: row for row in csv.DictReader(source)}


def main() -> int:
    simulator = Path(sys.argv[1] if len(sys.argv) > 1
                     else "build/bin/HierarchicalRECAM").resolve()
    if not simulator.is_file():
        raise SystemExit(f"missing simulator: {simulator}")
    with tempfile.TemporaryDirectory() as temporary:
        root = Path(temporary)
        generated = root / "generated"
        repeated = root / "repeated"
        replayed = root / "replayed"
        common = [
            str(simulator), "2", "2", "--canonical-four", "--groups", "2",
            "--fault-count", "20", "--seed", "20260820",
            "--fault-model", "moderate", "--spatial", "mixed",
            "--topology", "edge", "--shared-rows", "1",
            "--shared-columns", "1", "--max-borrows", "3",
            "--sram-policy", "chunked:2", "--write-fault-corpus",
        ]
        subprocess.run([*common, "--output-dir", str(generated)], check=True,
                       stdout=subprocess.DEVNULL)
        subprocess.run([*common, "--output-dir", str(repeated)], check=True,
                       stdout=subprocess.DEVNULL)
        headers, rows = corpus_rows(generated / "fault_corpus.txt")
        _, repeat_rows = corpus_rows(repeated / "fault_corpus.txt")
        if rows != repeat_rows:
            raise AssertionError("same seed did not reproduce the fault corpus")
        if "# fault_model=moderate_imbalance" not in headers or \
           "# spatial_model=mixed" not in headers:
            raise AssertionError("fault corpus metadata is missing")
        counts = Counter((row[3], row[4]) for row in rows)
        expected = {(group, subarray): count
                    for group in range(2)
                    for subarray, count in enumerate((2, 4, 5, 9))}
        if counts != expected:
            raise AssertionError(f"moderate 2:4:5:9 counts mismatch: {counts}")

        subprocess.run([
            str(simulator), "2", "2", "--canonical-four",
            "--simplified-fault-file", str(generated / "fault_corpus.txt"),
            "--topology", "edge", "--shared-rows", "1",
            "--shared-columns", "1", "--max-borrows", "3",
            "--sram-policy", "chunked:2", "--write-fault-corpus",
            "--output-dir", str(replayed),
        ], check=True, stdout=subprocess.DEVNULL)
        _, replay_rows = corpus_rows(replayed / "fault_corpus.txt")
        if rows != replay_rows:
            raise AssertionError("saved fault corpus did not replay exactly")

        summary = canonical_rows(generated / "canonical_four_summary.csv")
        for left, right in (("B0_CAM_NO_SHARING", "B2_SRAM_NO_SHARING"),
                            ("B1_CAM_SHARING", "B3_SRAM_SHARING")):
            for field in ("repairable_groups", "repair_rate",
                          "global_reuse_entries_reserved",
                          "global_reuse_overflow_count"):
                if summary[left][field] != summary[right][field]:
                    raise AssertionError(f"{left}/{right} mismatch in {field}")
    print("Hierarchical moderate fault-model tests passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
