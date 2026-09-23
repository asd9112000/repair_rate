#!/usr/bin/env python3
"""Derive R3 rank-quintile fault-imbalance summaries from paired corpora."""

from __future__ import annotations

import argparse
import math
import sys
from collections import defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))
from scripts.analysis.r3_group.common import (POLICIES, load_dataset, write_analysis_manifest,
                                                write_csv, write_gate_artifacts)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--input-root", required=True, type=Path)
    parser.add_argument("--output-root", required=True, type=Path)
    parser.add_argument("--policy-set", choices=("legacy", "sixcase_static"), default="legacy")
    return parser.parse_args()


def imbalance(counts: tuple[int, int, int, int]) -> tuple[float, int]:
    average = sum(counts) / 4.0
    stddev = math.sqrt(sum((value - average) ** 2 for value in counts) / 4.0)
    return stddev, max(counts) - min(counts)


def main() -> int:
    args = parse_args()
    points, quality, metadata = load_dataset(args.input_root, args.policy_set)
    output_root = args.output_root.resolve()
    write_gate_artifacts(output_root, points, quality, metadata)
    rows = []
    for point in points:
        ranked = sorted((imbalance(counts)[0], group_id, imbalance(counts)[1], counts)
                        for group_id, counts in point.faults.items())
        bins: dict[int, list[tuple[float, int, int, tuple[int, int, int, int]]]] = defaultdict(list)
        for rank, entry in enumerate(ranked):
            bins[min(5, rank * 5 // point.groups + 1)].append(entry)
        policy_success = {policy: [int(row["repair_success"]) for row in result]
                          for policy, result in point.results.items()}
        for bin_id, entries in sorted(bins.items()):
            group_ids = [entry[1] for entry in entries]
            for policy, outcomes in policy_success.items():
                successes = sum(outcomes[group_id] for group_id in group_ids)
                rows.append({
                    "dataset_class": metadata["dataset_class"], "RS": point.rs, "CS": point.cs,
                    "F_GROUP": point.fault_count, "seed": point.seed, "corpus_id": point.corpus_id,
                    "policy": policy, "policy_label": POLICIES[policy]["label"],
                    "topology": POLICIES[policy]["topology"], "imbalance_bin": bin_id,
                    "method": "rank_quintile_v1", "groups": len(entries), "successes": successes,
                    "failures": len(entries) - successes, "repair_rate": successes / len(entries),
                    "fault_stddev_across_SA_mean": sum(entry[0] for entry in entries) / len(entries),
                    "fault_max_minus_min_mean": sum(entry[2] for entry in entries) / len(entries),
                })
    baseline = {(row["RS"], row["CS"], row["F_GROUP"], row["imbalance_bin"]): row["repair_rate"]
                for row in rows if row["policy"] == "local_no_sharing"}
    for row in rows:
        local = baseline.get((row["RS"], row["CS"], row["F_GROUP"], row["imbalance_bin"]))
        row["gain_vs_local_percentage_points"] = math.nan if local is None else 100.0 * (row["repair_rate"] - local)
    fields = ["dataset_class", "RS", "CS", "F_GROUP", "seed", "corpus_id", "policy", "policy_label",
              "topology", "imbalance_bin", "method", "groups", "successes", "failures", "repair_rate",
              "fault_stddev_across_SA_mean", "fault_max_minus_min_mean", "gain_vs_local_percentage_points"]
    write_csv(output_root / "data" / "imbalance_summary.csv", rows, fields)
    write_analysis_manifest(output_root, metadata, points, quality)
    print(f"Wrote {len(rows)} imbalance rows to {output_root}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
