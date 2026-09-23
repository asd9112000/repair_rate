#!/usr/bin/env python3
"""Build paired R3 policy outcomes from identical corpus/group keys."""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))
from scripts.analysis.r3_group.common import (COMPARISONS, POLICIES, load_dataset,
                                                write_analysis_manifest, write_csv, write_gate_artifacts)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--input-root", required=True, type=Path)
    parser.add_argument("--output-root", required=True, type=Path)
    parser.add_argument("--policy-set", choices=("legacy", "sixcase_static"), default="legacy")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    points, quality, metadata = load_dataset(args.input_root, args.policy_set)
    output_root = args.output_root.resolve()
    write_gate_artifacts(output_root, points, quality, metadata)
    rows = []
    ladder_rows = []
    for point in points:
        for comparison, left, right, comparison_class in COMPARISONS:
            if left not in point.results or right not in point.results:
                continue
            left_values = [int(row["repair_success"]) for row in point.results[left]]
            right_values = [int(row["repair_success"]) for row in point.results[right]]
            both = left_only = right_only = neither = 0
            for left_value, right_value in zip(left_values, right_values):
                if (left_value, right_value) == (1, 1): both += 1
                elif (left_value, right_value) == (1, 0): left_only += 1
                elif (left_value, right_value) == (0, 1): right_only += 1
                else: neither += 1
            left_failures = right_only + neither
            rows.append({
                "dataset_class": metadata["dataset_class"], "RS": point.rs, "CS": point.cs,
                "F_GROUP": point.fault_count, "seed": point.seed, "corpus_id": point.corpus_id,
                "comparison": comparison, "comparison_class": comparison_class,
                "dominance_applicability": "APPLICABLE" if comparison_class in ("SAME_V2_UNIVERSE", "SAME_UNIVERSE") else "NOT_APPLICABLE",
                "left_policy": left, "left_policy_label": POLICIES[left]["label"],
                "right_policy": right, "right_policy_label": POLICIES[right]["label"],
                "groups": point.groups, "A_PASS_B_PASS": both, "A_PASS_B_FAIL": left_only,
                "A_FAIL_B_PASS": right_only, "A_FAIL_B_FAIL": neither,
                "BOTH_PASS": both, "A_ONLY": left_only, "B_ONLY": right_only, "BOTH_FAIL": neither,
                "left_fail_right_pass_rate": right_only / point.groups,
                "left_fail_right_pass_rate_among_left_failures": (
                    right_only / left_failures if left_failures else 0.0),
            })
        ladder = ("directional_m1_local_first", "directional_m1_early",
                  "directional_m1_global")
        if all(policy in point.results for policy in ladder):
            for group_id, outcomes in enumerate(zip(*(point.results[p] for p in ladder))):
                early, greedy, global_ = (int(row["repair_success"]) for row in outcomes)
                pattern = (early, greedy, global_)
                # These are exhaustive, mutually exclusive categories.  The
                # "relative" labels state the stage at which a success first
                # appears; they do not claim that EARLY is contained by GREEDY.
                category = {
                    (1, 1, 1): "ALL_PASS",
                    (0, 0, 0): "ALL_FAIL",
                    (1, 0, 0): "EARLY_ONLY",
                    (0, 1, 0): "GREEDY_ONLY",
                    (0, 0, 1): "GLOBAL_ONLY",
                    (1, 1, 0): "EARLY_AND_GREEDY",
                    (1, 0, 1): "EARLY_AND_GLOBAL",
                    (0, 1, 1): "GREEDY_ONLY_RELATIVE_TO_EARLY",
                }[pattern]
                ladder_rows.append({
                    "dataset_class": metadata["dataset_class"], "RS": point.rs,
                    "CS": point.cs, "F_GROUP": point.fault_count,
                    "seed": point.seed, "corpus_id": point.corpus_id,
                    "group_id": group_id, "early_pass": early,
                    "greedy_pass": greedy, "v2_global_pass": global_,
                    "stage_outcome_class": category,
                })
    fields = ["dataset_class", "RS", "CS", "F_GROUP", "seed", "corpus_id", "comparison",
              "comparison_class", "dominance_applicability", "left_policy", "left_policy_label",
              "right_policy", "right_policy_label", "groups", "A_PASS_B_PASS", "A_PASS_B_FAIL", "A_FAIL_B_PASS", "A_FAIL_B_FAIL",
              "BOTH_PASS", "A_ONLY", "B_ONLY", "BOTH_FAIL", "left_fail_right_pass_rate",
              "left_fail_right_pass_rate_among_left_failures"]
    write_csv(output_root / "data" / "paired_outcomes.csv", rows, fields)
    ladder_fields = ["dataset_class", "RS", "CS", "F_GROUP", "seed", "corpus_id",
                     "group_id", "early_pass", "greedy_pass", "v2_global_pass",
                     "stage_outcome_class"]
    write_csv(output_root / "data" / "directional_v2_ladder_outcomes.csv",
              ladder_rows, ladder_fields)
    contract_rows = [{
        "comparison": comparison, "left_policy": left,
        "right_policy": right, "comparison_class": comparison_class,
        "dominance_applicability": "APPLICABLE" if comparison_class == "SAME_V2_UNIVERSE" else "NOT_APPLICABLE",
        "interpretation": ("paired descriptive comparison only; no dominance inference across candidate contracts"
                           if comparison_class == "EMPIRICAL_CROSS_CONTRACT"
                           else ("same frozen directional-V2 candidate and ledger contract; paired stage gain"
                                 if comparison_class == "SAME_V2_UNIVERSE"
                                 else "paired descriptive comparison only")),
    } for comparison, left, right, comparison_class in COMPARISONS]
    contract_fields = ["comparison", "left_policy", "right_policy", "comparison_class",
                       "dominance_applicability", "interpretation"]
    write_csv(output_root / "data" / "policy_comparison_contract.csv", contract_rows, contract_fields)
    write_csv(output_root / "tables" / "counterexample_table.csv", rows, fields)
    write_analysis_manifest(output_root, metadata, points, quality)
    print(f"Wrote {len(rows)} paired-comparison rows to {output_root}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
