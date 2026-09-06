#!/usr/bin/env python3

from __future__ import annotations

import csv
import subprocess
import sys
import tempfile
from pathlib import Path


def main() -> int:
    if len(sys.argv) != 3:
        raise SystemExit("usage: canonical_sweep_test.py SIMULATOR SWEEP_SCRIPT")
    simulator = Path(sys.argv[1]).resolve()
    sweep = Path(sys.argv[2]).resolve()
    with tempfile.TemporaryDirectory() as temporary:
        output = Path(temporary) / "sweep"
        subprocess.run([
            sys.executable, str(sweep), "--simulator", str(simulator),
            "--output-dir", str(output), "--rs", "2", "--cs", "2",
            "--fault-counts", "4", "--word-bits", "16",
            "--seeds", "20260820", "--sram-policies", "chunked:2",
            "--groups", "2", "--memory-rows", "16",
            "--memory-columns", "64", "--topology", "edge",
            "--shared-rows", "1", "--shared-columns", "0",
            "--max-borrows", "1",
        ], check=True)
        with (output / "canonical_four_sweep.csv").open(
                newline="", encoding="utf-8") as source:
            rows = list(csv.DictReader(source))
        expected = {
            "B0_CAM_NO_SHARING", "B1_CAM_SHARING",
            "B2_SRAM_NO_SHARING", "B3_SRAM_SHARING",
        }
        actual = {row["configuration_id"] for row in rows}
        if len(rows) != 4 or actual != expected:
            raise RuntimeError("sweep did not emit exactly B0/B1/B2/B3")
        by_id = {row["configuration_id"]: row for row in rows}
        if (by_id["B0_CAM_NO_SHARING"]["repairable_groups"] !=
                by_id["B2_SRAM_NO_SHARING"]["repairable_groups"] or
                by_id["B1_CAM_SHARING"]["repairable_groups"] !=
                by_id["B3_SRAM_SHARING"]["repairable_groups"]):
            raise RuntimeError("sweep CAM/SRAM logical equivalence failed")
    print("Canonical-four sweep tests passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
