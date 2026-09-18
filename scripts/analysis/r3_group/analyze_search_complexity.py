#!/usr/bin/env python3
"""Aggregate C++ algorithmic GLOBAL search complexity from raw sidecars."""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))
from scripts.analysis.r3_group.common import (POLICIES, average, load_dataset, percentile,
                                                write_analysis_manifest, write_csv, write_gate_artifacts)

GLOBAL_POLICIES = ("directional_m1_global", "directional_m1_group_global",
                   "two_pairwise_m1_pair_global", "single_hop_m1_global")
METRICS = {
    "search_nodes": "global_search_nodes_visited",
    "complete_tuples": "global_complete_assignments_checked",
    "candidate_demand_count": None,
}


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--input-root", required=True, type=Path)
    parser.add_argument("--output-root", required=True, type=Path)
    return parser.parse_args()


def values(rows, metric: str) -> list[float]:
    if metric == "candidate_demand_count":
        return [sum(float(row[f"global_candidate_count_{name}"]) for name in "ABCD") for row in rows]
    return [float(row[METRICS[metric]]) for row in rows]


def main() -> int:
    args = parse_args()
    points, quality, metadata = load_dataset(args.input_root)
    output_root = args.output_root.resolve()
    write_gate_artifacts(output_root, points, quality, metadata)
    rows = []
    for point in points:
        for policy in GLOBAL_POLICIES:
            if policy not in point.results:
                continue
            for metric in METRICS:
                # Pair-GLOBAL is not evaluated by the generic GLOBAL DFS.
                # Its generic-global CSV columns are fixed zero sentinels, not
                # a measured zero-cost search, so report the metric as
                # unsupported instead of putting misleading zeroes on a plot.
                unsupported = policy == "two_pairwise_m1_pair_global"
                metric_values = [] if unsupported else values(point.results[policy], metric)
                rows.append({
                    "dataset_class": metadata["dataset_class"], "RS": point.rs, "CS": point.cs,
                    "F_GROUP": point.fault_count, "seed": point.seed, "corpus_id": point.corpus_id,
                    "policy": policy, "policy_label": POLICIES[policy]["label"],
                    "topology": POLICIES[policy]["topology"], "metric": metric,
                    "complexity_label": "C++ algorithmic search complexity", "groups": len(metric_values),
                    "metric_status": "UNSUPPORTED" if unsupported else "SUPPORTED",
                    "metric_detail": ("Pair-GLOBAL uses a distinct solver; generic GLOBAL CSV search columns are zero sentinels"
                                      if unsupported else "raw CSV metric"),
                    "mean": average(map(str, metric_values)), "median": percentile(metric_values, 0.5),
                    "p95": percentile(metric_values, 0.95), "p99": percentile(metric_values, 0.99),
                    "max": max(metric_values) if metric_values else float("nan"),
                })
    fields = ["dataset_class", "RS", "CS", "F_GROUP", "seed", "corpus_id", "policy", "policy_label",
              "topology", "metric", "complexity_label", "groups", "metric_status", "metric_detail",
              "mean", "median", "p95", "p99", "max"]
    write_csv(output_root / "data" / "search_complexity.csv", rows, fields)
    write_analysis_manifest(output_root, metadata, points, quality)
    print(f"Wrote {len(rows)} search-complexity rows to {output_root}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
