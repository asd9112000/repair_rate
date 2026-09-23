#!/usr/bin/env python3
"""Build authoritative R3 repair/failure summaries from raw policy sidecars."""

from __future__ import annotations

import argparse
import math
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))
from scripts.analysis.r3_group.common import (POLICIES, ensure_output_layout, load_dataset,
                                                wilson, write_csv, write_gate_artifacts)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--input-root", required=True, type=Path)
    parser.add_argument("--output-root", required=True, type=Path)
    parser.add_argument("--policy-set", choices=("legacy", "sixcase_static"), default="legacy")
    return parser.parse_args()


def layout_for(policy: str) -> str:
    return str(POLICIES[policy].get("layout", "1x4" if policy.startswith(("two_pairwise", "single_hop")) else "2x2"))


def main() -> int:
    args = parse_args()
    points, quality, metadata = load_dataset(args.input_root, args.policy_set)
    output_root = args.output_root.resolve()
    ensure_output_layout(output_root)
    write_gate_artifacts(output_root, points, quality, metadata)
    rows = []
    for point in points:
        for policy, result_rows in point.results.items():
            successes = sum(int(row["repair_success"]) for row in result_rows)
            failures = point.groups - successes
            low, high = wilson(successes, point.groups)
            rows.append({
                "dataset_class": metadata["dataset_class"], "RS": point.rs, "CS": point.cs,
                "F_GROUP": point.fault_count, "seed": point.seed, "corpus_id": point.corpus_id,
                "policy": policy, "policy_label": POLICIES[policy]["label"],
                "source_sidecar": point.source_sidecars[policy],
                "layout": layout_for(policy), "topology": POLICIES[policy]["topology"],
                "solution_class": POLICIES[policy]["solution"], "groups": point.groups,
                "successes": successes, "failures": failures,
                "repair_rate": successes / point.groups, "failure_rate": failures / point.groups,
                "wilson95_low": low, "wilson95_high": high,
            })
    baseline = {(row["RS"], row["CS"], row["F_GROUP"]): row
                for row in rows if row["policy"] == "local_no_sharing"}
    for row in rows:
        local = baseline.get((row["RS"], row["CS"], row["F_GROUP"]))
        if local is None:
            row["absolute_gain_vs_local"] = math.nan
            row["gain_vs_local_percentage_points"] = math.nan
            row["failure_reduction_vs_local"] = math.nan
        else:
            row["absolute_gain_vs_local"] = row["repair_rate"] - local["repair_rate"]
            row["gain_vs_local_percentage_points"] = 100.0 * row["absolute_gain_vs_local"]
            row["failure_reduction_vs_local"] = (
                math.nan if local["failure_rate"] == 0 else
                (local["failure_rate"] - row["failure_rate"]) / local["failure_rate"])
    fields = ["dataset_class", "RS", "CS", "F_GROUP", "seed", "corpus_id", "policy", "policy_label", "source_sidecar",
              "layout", "topology", "solution_class", "groups", "successes", "failures", "repair_rate",
              "failure_rate", "wilson95_low", "wilson95_high", "absolute_gain_vs_local",
              "gain_vs_local_percentage_points", "failure_reduction_vs_local"]
    write_csv(output_root / "data" / "repair_rate_summary.csv", rows, fields)
    failure_rows = [{key: row[key] for key in fields} for row in rows]
    write_csv(output_root / "data" / "failure_rate_summary.csv", failure_rows, fields)
    write_csv(output_root / "tables" / "repair_rate_table.csv", rows, fields)
    gain_fields = ["dataset_class", "RS", "CS", "F_GROUP", "policy", "policy_label", "source_sidecar", "groups",
                   "repair_rate", "failure_rate", "absolute_gain_vs_local",
                   "gain_vs_local_percentage_points", "failure_reduction_vs_local",
                   "wilson95_low", "wilson95_high"]
    gain_rows = [{field: row[field] for field in gain_fields} for row in rows]
    write_csv(output_root / "tables" / "policy_gain_table.csv", gain_rows, gain_fields)
    directional = {(row["RS"], row["CS"], row["F_GROUP"], row["policy"]): row
                   for row in rows}
    ladder_gains = []
    for point_key in sorted({key[:3] for key in directional}):
        early = directional.get((*point_key, "directional_m1_early"))
        greedy = directional.get((*point_key, "directional_m1_early"))
        global_ = directional.get((*point_key, "directional_m1_global"))
        if not (early and greedy and global_):
            continue
        ladder_gains.append({
            "dataset_class": early["dataset_class"], "RS": point_key[0],
            "CS": point_key[1], "F_GROUP": point_key[2], "groups": early["groups"],
            "GAIN_EARLY_TO_GREEDY": greedy["repair_rate"] - early["repair_rate"],
            "GAIN_GREEDY_TO_GLOBAL": global_["repair_rate"] - greedy["repair_rate"],
            "GAIN_EARLY_TO_GLOBAL": global_["repair_rate"] - early["repair_rate"],
        })
    ladder_gain_fields = ["dataset_class", "RS", "CS", "F_GROUP", "groups",
                          "GAIN_EARLY_TO_GREEDY", "GAIN_GREEDY_TO_GLOBAL",
                          "GAIN_EARLY_TO_GLOBAL"]
    write_csv(output_root / "data" / "directional_v2_policy_gains.csv",
              ladder_gains, ladder_gain_fields)
    print(f"Wrote {len(rows)} repair/failure rows for {len(points)} points to {output_root}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
