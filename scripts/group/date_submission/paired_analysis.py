#!/usr/bin/env python3
"""Run and analyze per-sample paired DSS outcomes at selected fault counts."""

from __future__ import annotations

import argparse
import csv
import json
import math
import shlex
import subprocess
from pathlib import Path


ROOT = Path(__file__).resolve().parents[3]
POLICIES = {
    ("no_sharing", "0", "0"): "no_sharing",
    ("directional", "1", "1"): "directional_m1",
    ("directional", "2", "2"): "directional_m2",
}


def read_json(path: Path) -> dict:
    with path.open(encoding="utf-8") as source:
        return json.load(source)


def write_json(path: Path, value: object) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(value, indent=2, sort_keys=True) + "\n", encoding="utf-8")


def exact_mcnemar_p(left_only: int, right_only: int) -> float:
    discordant = left_only + right_only
    if discordant == 0:
        return 1.0
    tail = sum(math.comb(discordant, k) for k in range(min(left_only, right_only) + 1))
    return min(1.0, 2.0 * tail / (2 ** discordant))


def compare(outcomes: dict[str, bool], other: dict[str, bool]) -> tuple[int, int, int, int]:
    both = left = right = neither = 0
    for key in outcomes:
        pair = outcomes[key], other[key]
        if pair == (True, True): both += 1
        elif pair == (True, False): left += 1
        elif pair == (False, True): right += 1
        else: neither += 1
    return both, left, right, neither


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("run_root", type=Path)
    parser.add_argument("--fault-counts", default="16,22,24")
    args = parser.parse_args()
    run_root = args.run_root.resolve()
    config = read_json(run_root / "run_config.json")
    faults = [int(value) for value in args.fault_counts.split(",")]
    invalid = sorted(set(faults) - set(config["fault_counts_per_group"]))
    if invalid:
        raise SystemExit(f"fault counts not in primary sweep: {invalid}")
    exploration = run_root / "explorations/paired_outcomes"
    if exploration.exists():
        raise SystemExit(f"exploration directory already exists: {exploration}")
    raw = exploration / "raw"
    raw.mkdir(parents=True)
    fixed = config["fixed"]
    profile = config["active_profile_config"]
    simulator = ROOT / config["simulators"]["repair"]
    write_json(exploration / "run_config.json", {
        "scope": "group", "parent_run": str(run_root), "fault_counts": faults,
        "seeds": profile["seeds"], "runs_per_seed": profile["runs_per_seed"],
        "purpose": "exact paired contingency analysis from per-run outcomes",
    })
    for fault_count in faults:
        for seed in profile["seeds"]:
            point_id = f"f{fault_count}_s{seed}"
            point = raw / point_id
            point.mkdir()
            command = [
                str(simulator), str(fixed["rs"]), str(fixed["cs"]),
                "--repair-rate-sweep", "--fault-min", str(fault_count),
                "--fault-max", str(fault_count), "--fault-step", "1",
                "--spare-min", "2", "--spare-max", "2",
                "--runs", str(profile["runs_per_seed"]), "--seed", str(seed),
                "--layout", "2x2", "--fault-model", fixed["fault_model"],
                "--spatial", fixed["spatial_model"],
                "--memory-rows", str(fixed["memory_rows"]),
                "--memory-columns", str(fixed["memory_columns"]),
                "--paper-cam-reuse", "--local-first", "--max-borrows",
                str(fixed["max_borrows"]), "--solution-take", fixed["solution_take"],
                "--data-width", str(fixed["data_width_bits"]),
                "--row-address-width", str(fixed["row_address_width_bits"]),
                "--column-address-width", str(fixed["column_address_width_bits"]),
                "--output-dir", str(point),
            ]
            (point / "command.txt").write_text(shlex.join(command) + "\n", encoding="utf-8")
            write_json(point / "point.json", {"point_id": point_id, "scope": "group",
                                               "fault_count": fault_count, "seed": seed,
                                               "runs": profile["runs_per_seed"]})
            with (point / "run.log").open("w", encoding="utf-8") as log:
                subprocess.run(command, cwd=ROOT, stdout=log,
                               stderr=subprocess.STDOUT, check=True)
            print(f"[paired] {point_id}", flush=True)

    by_fault = {fault_count: {pid: {} for pid in POLICIES.values()} for fault_count in faults}
    corpus_signatures = {}
    for path in sorted(raw.glob("*/runs.csv")):
        with path.open(newline="", encoding="utf-8") as source:
            for row in csv.DictReader(source):
                pid = POLICIES.get((row["policy"], row["shared_rows"], row["shared_columns"]))
                if pid is None:
                    continue
                fault_count = sum(int(row[field]) for field in ("fault_A", "fault_B", "fault_C", "fault_D"))
                key = f"{row['seed']}:{row['run_index']}"
                signature = tuple(row[field] for field in ("fault_A", "fault_B", "fault_C", "fault_D"))
                previous = corpus_signatures.setdefault((fault_count, key), signature)
                if previous != signature:
                    raise RuntimeError(f"unpaired fault-count signature at {fault_count}/{key}")
                by_fault[fault_count][pid][key] = row["group_repair_success"] == "1"

    rows = []
    pairs = (("directional_m1", "no_sharing"),
             ("directional_m2", "no_sharing"),
             ("directional_m2", "directional_m1"))
    for fault_count in faults:
        expected = len(profile["seeds"]) * profile["runs_per_seed"]
        if any(len(values) != expected for values in by_fault[fault_count].values()):
            raise RuntimeError(f"incomplete paired outcomes at fault_count={fault_count}")
        for left_id, right_id in pairs:
            both, left_only, right_only, neither = compare(
                by_fault[fault_count][left_id], by_fault[fault_count][right_id])
            rows.append({
                "fault_count": fault_count, "left_policy": left_id, "right_policy": right_id,
                "paired_samples": expected, "both_success": both,
                "left_only_success": left_only, "right_only_success": right_only,
                "neither_success": neither,
                "paired_gain_pp": 100 * (left_only - right_only) / expected,
                "exact_mcnemar_two_sided_p": exact_mcnemar_p(left_only, right_only),
            })
    output = exploration / "paired_contingency.csv"
    with output.open("w", newline="", encoding="utf-8") as target:
        writer = csv.DictWriter(target, fieldnames=list(rows[0]))
        writer.writeheader()
        writer.writerows(rows)
    m2_vs_m1 = [row for row in rows if row["left_policy"] == "directional_m2" and
                row["right_policy"] == "directional_m1"]
    lines = ["# Paired-outcome exploration", "",
             "Exact McNemar tests use the discordant outcomes from identical seed/run-index corpus pairs.", ""]
    for row in m2_vs_m1:
        lines.append(f"- F={row['fault_count']}: m2-only={row['left_only_success']}, "
                     f"m1-only={row['right_only_success']}, gain={row['paired_gain_pp']:.2f} pp, "
                     f"two-sided exact p={row['exact_mcnemar_two_sided_p']:.6g}.")
    lines.append("\nThis is an exploratory multiple-comparison analysis; p-values are unadjusted and should not be presented as a pre-registered confirmatory test.")
    (exploration / "summary.md").write_text("\n".join(lines) + "\n", encoding="utf-8")
    write_json(exploration / "validation.json", {
        "complete": True, "scope": "group", "corpus_pairing_verified": True,
        "test": "two-sided exact McNemar/binomial test on discordant pairs",
        "multiple_comparison_adjustment": "none; exploratory",
    })
    print(f"Wrote paired exploration to {exploration}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
