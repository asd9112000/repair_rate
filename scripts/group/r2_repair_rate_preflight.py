#!/usr/bin/env python3
"""Controlled, non-formal R2 group-level repair-rate preflight.

This runner owns the R2 corpus contract.  It generates one fixed-total
multinomial physical corpus for each (RS/CS, F_GROUP) point, materializes that
corpus as a simplified-fault replay file, then replays the identical physical
faults through each legal policy.  It intentionally stops after preflight
artifacts; it is not a formal DATE sweep runner.
"""

from __future__ import annotations

import argparse
import csv
import json
import math
import statistics
import subprocess
import sys
from collections import defaultdict
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
SCHEMA = "dss_r2_group_preflight_v1"
SEED = 20260920
SAMPLES = 1000
POINTS = {
    2: (8, 12, 16, 20),
    3: (12, 18, 24, 30),
}


def policies(n: int) -> list[dict[str, object]]:
    """Legal R2 policy matrix; no topology identity is collapsed."""
    result: list[dict[str, object]] = [
        {"id": "local_no_sharing", "layout": "2x2", "topology": "none",
         "share_row": 0, "share_col": 0, "solution": "legacy"},
    ]
    for m in (1, 2):
        result.extend([
            {"id": f"directional_m{m}_early", "layout": "2x2",
             "topology": "directional", "share_row": m, "share_col": m,
             "solution": "early"},
            {"id": f"pairwise_row_m{m}_early", "layout": "2x2",
             "topology": "edge", "share_row": m, "share_col": 0,
             "solution": "early"},
            {"id": f"two_pairwise_m{m}_early", "layout": "1x4",
             "topology": "pair", "share_row": m, "share_col": 0,
             "solution": "one_by_four_two_pairwise_early_v1"},
            {"id": f"two_pairwise_m{m}_pair_global", "layout": "1x4",
             "topology": "pair", "share_row": m, "share_col": 0,
             "solution": "one_by_four_two_pairwise_pair_global_v1"},
            {"id": f"single_hop_m{m}_early", "layout": "1x4",
             "topology": "neighbor", "share_row": m, "share_col": 0,
             "solution": "one_by_four_single_hop_early_v1"},
            {"id": f"single_hop_m{m}_global", "layout": "1x4",
             "topology": "neighbor", "share_row": m, "share_col": 0,
             "solution": "one_by_four_single_hop_global_v1"},
        ])
    # The canonical group-greedy contract is only validated at m=1.  The
    # generic group-global selector is run at the same 2x2 directional point
    # for a valid, directly paired C1/C2/C3 comparison.
    result.extend([
        {"id": "directional_m1_group_greedy", "layout": "2x2",
         "topology": "directional", "share_row": 1, "share_col": 1,
         "solution": "group_greedy_rtl_canonical"},
        {"id": "directional_m1_group_global", "layout": "2x2",
         "topology": "directional", "share_row": 1, "share_col": 1,
         "solution": "group_global"},
    ])
    assert all(int(policy["share_row"]) <= n for policy in result)
    return result


def invoke(command: list[str], log: Path) -> None:
    with log.open("w", encoding="utf-8") as stream:
        completed = subprocess.run(command, cwd=ROOT, stdout=stream,
                                   stderr=subprocess.STDOUT, text=True)
    if completed.returncode:
        raise RuntimeError(f"command failed ({completed.returncode}): {' '.join(command)}")


def read_csv(path: Path) -> list[dict[str, str]]:
    with path.open(newline="", encoding="utf-8") as source:
        return list(csv.DictReader(source))


def write_csv(path: Path, rows: list[dict[str, object]], fields: list[str]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", newline="", encoding="utf-8") as target:
        writer = csv.DictWriter(target, fieldnames=fields, extrasaction="raise")
        writer.writeheader()
        writer.writerows(rows)


def materialize_replay(corpus_rows: list[dict[str, str]], destination: Path) -> dict[str, tuple[int, int, int, int]]:
    counts: dict[str, tuple[int, int, int, int]] = {}
    with destination.open("w", encoding="utf-8") as target:
        for row in corpus_rows:
            per_sa = row["sa_local_faults"].split("|")
            if len(per_sa) != 4:
                raise RuntimeError("paired corpus does not contain four SA fault lists")
            sa_counts = []
            for entries in per_sa:
                faults = [] if not entries else entries.split(";")
                sa_counts.append(len(faults))
                for encoded in faults:
                    target.write(encoded.replace(":", " ") + "\n")
            counts[row["group_id"]] = tuple(sa_counts)  # type: ignore[assignment]
    return counts


def percentile(values: list[float], fraction: float) -> float:
    if not values:
        return 0.0
    ordered = sorted(values)
    return ordered[max(0, math.ceil(fraction * len(ordered)) - 1)]


def integer(value: str) -> int:
    if value in ("", "NA", "-"):
        return 0
    return int(value)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output-root", type=Path,
                        default=ROOT / "results/date2026/repair_rate/r2_preflight")
    parser.add_argument("--samples", type=int, default=SAMPLES)
    parser.add_argument("--seed", type=int, default=SEED)
    parser.add_argument("--binary", type=Path,
                        default=ROOT / "build/bin/DynamicSpareSharing")
    args = parser.parse_args()
    output_root = args.output_root.resolve()
    binary = args.binary.resolve()
    if args.samples <= 0:
        raise SystemExit("--samples must be positive")
    if output_root.exists():
        raise SystemExit(f"R2 output root must be fresh: {output_root}")
    if not binary.is_file():
        raise SystemExit(f"DynamicSpareSharing binary not found: {binary}")

    for name in ("raw", "summary", "paired", "imbalance", "logs"):
        (output_root / name).mkdir(parents=True)
    manifest = {
        "schema_version": SCHEMA, "experiment_id": "date2026_repair_rate_r2_preflight",
        "simulation_level": "group", "non_formal": True,
        "fault_distribution_mode": "multinomial_uniform",
        "fault_contract": "Multinomial(F_GROUP; 0.25,0.25,0.25,0.25)",
        "seed": args.seed, "sample_count": args.samples, "points": POINTS,
        "policy_matrix": {str(n): policies(n) for n in POINTS},
        "cam_policy": {
            "2x2": "GENERIC_CPP_CAPACITY (not RTL-calibrated)",
            "1x4": "NOT_PROVEN; capacity and occupancy fields emitted as NA",
        },
    }
    (output_root / "manifest.json").write_text(
        json.dumps(manifest, indent=2, sort_keys=True) + "\n", encoding="utf-8")

    all_records: list[dict[str, object]] = []
    commands: list[str] = []
    for n, faults in POINTS.items():
        for f_group in faults:
            point_id = f"n{n}_f{f_group}"
            point_root = output_root / "raw" / point_id
            bootstrap = point_root / "local_no_sharing"
            bootstrap.mkdir(parents=True)
            source_command = [
                str(binary), str(n), str(n), "--fault-model", "multinomial_uniform",
                "--fault-count", str(f_group), "--runs", str(args.samples),
                "--seed", str(args.seed), "--spatial", "mixed", "--layout", "2x2",
                "--topology", "none", "--shared-rows", "0", "--shared-columns", "0",
                "--solution-take", "legacy", "--paper-cam-reuse",
                "--hybrid-cam-entry-width-bits", "20", "--summary-only",
                "--output-dir", str(bootstrap),
            ]
            commands.append(" ".join(source_command))
            invoke(source_command, output_root / "logs" / f"{point_id}_local_no_sharing.log")
            corpus_rows = read_csv(bootstrap / "paired_corpus_v1.csv")
            if len(corpus_rows) != args.samples:
                raise RuntimeError(f"{point_id}: source corpus sample count mismatch")
            corpus_id = corpus_rows[0]["corpus_id"]
            if any(row["corpus_id"] != corpus_id for row in corpus_rows):
                raise RuntimeError(f"{point_id}: source corpus has multiple IDs")
            replay_file = point_root / "corpus.simplified.faults"
            counts_by_group = materialize_replay(corpus_rows, replay_file)

            for policy in policies(n):
                policy_id = str(policy["id"])
                run_dir = point_root / policy_id
                if policy_id != "local_no_sharing":
                    run_dir.mkdir(parents=True)
                    command = [
                        str(binary), str(n), str(n), "--simplified-fault-file",
                        str(replay_file), "--fault-model", "multinomial_uniform",
                        "--fault-count", str(f_group), "--runs", str(args.samples),
                        "--seed", str(args.seed), "--spatial", "mixed", "--layout",
                        str(policy["layout"]), "--topology", str(policy["topology"]),
                        "--shared-rows", str(policy["share_row"]), "--shared-columns",
                        str(policy["share_col"]), "--solution-take", str(policy["solution"]),
                        "--paper-cam-reuse", "--hybrid-cam-entry-width-bits", "20",
                        "--summary-only", "--output-dir", str(run_dir),
                    ]
                    commands.append(" ".join(command))
                    invoke(command, output_root / "logs" / f"{point_id}_{policy_id}.log")

                result_rows = read_csv(run_dir / "paired_policy_results_v1.csv")
                if len(result_rows) != args.samples:
                    raise RuntimeError(f"{point_id}/{policy_id}: incomplete policy result")
                if any(row["corpus_id"] != corpus_id for row in result_rows):
                    raise RuntimeError(f"{point_id}/{policy_id}: policy did not replay source corpus")
                for row in result_rows:
                    counts = counts_by_group[row["group_id"]]
                    if sum(counts) != f_group:
                        raise RuntimeError(f"{point_id}: fixed total fault contract failed")
                    mean = sum(counts) / 4.0
                    stddev = statistics.pstdev(counts)
                    record: dict[str, object] = {
                        "schema_version": SCHEMA, "experiment_id": manifest["experiment_id"],
                        "corpus_id": corpus_id, "simulator_revision": row["simulator_revision"],
                        "simulation_level": "group", "topology": row["sharing_policy"],
                        "sharing_policy": row["sharing_policy"], "solution_policy": row["solution_policy"],
                        "policy": policy_id, "config_contract_version": row["config_contract_version"],
                        "RS": n, "CS": n, "share_row": policy["share_row"],
                        "share_col": policy["share_col"], "m": policy["share_row"],
                        "F_GROUP": f_group, "fault_distribution_mode": "multinomial_uniform",
                        "seed": args.seed, "sample_count": args.samples, "group_id": row["group_id"],
                        "fault_A": counts[0], "fault_B": counts[1], "fault_C": counts[2], "fault_D": counts[3],
                        "fault_mean": mean, "fault_stddev": stddev, "fault_min": min(counts),
                        "fault_max": max(counts), "fault_range": max(counts) - min(counts),
                        "repairable": row["repair_success"],
                    }
                    for field in (
                        "private_rows_per_SA", "private_columns_per_SA", "shareable_rows_per_SA",
                        "total_rows_group", "total_columns_group", "total_physical_group",
                        "local_row_used", "local_column_used", "own_shareable_row_used",
                        "borrowed_shareable_row_used", "remaining_shareable_rows",
                        "cam_accounting_status", "address_cam_used_entries",
                        "address_cam_capacity_entries", "address_cam_entry_bits",
                        "address_cam_capacity_bits", "hybrid_cam_used_entries",
                        "hybrid_cam_capacity_entries", "hybrid_cam_entry_bits",
                        "hybrid_cam_capacity_bits", "total_cam_capacity_bits",
                    ):
                        record[field] = row[field]
                    record["borrow_events"] = integer(row["borrowed_rows"]) + integer(row["borrowed_columns"])
                    record["resource_match"] = (
                        "RESOURCE_MATCHED" if integer(row["total_physical_group"]) == 4 * (2 * n)
                        else "RESOURCE_UNMATCHED")
                    all_records.append(record)

    (output_root / "logs" / "commands.txt").write_text(
        "\n".join(commands) + "\n", encoding="utf-8")
    fields = list(all_records[0])
    write_csv(output_root / "raw" / "r2_group_records_v1.csv", all_records, fields)

    summary_rows: list[dict[str, object]] = []
    grouped: dict[tuple[object, ...], list[dict[str, object]]] = defaultdict(list)
    key_fields = ("policy", "topology", "sharing_policy", "solution_policy", "RS", "CS",
                  "share_row", "share_col", "m", "F_GROUP", "corpus_id")
    for record in all_records:
        grouped[tuple(record[field] for field in key_fields)].append(record)
    for key, records in sorted(grouped.items()):
        repairable = [integer(str(row["repairable"])) for row in records]
        stddevs = [float(row["fault_stddev"]) for row in records]
        borrowed = [integer(str(row["borrowed_shareable_row_used"])) for row in records]
        if len(records) != args.samples:
            raise RuntimeError(f"incomplete aggregate group for {key}")
        first = records[0]
        summary = {field: first[field] for field in key_fields}
        summary.update({
            "groups_simulated": len(records), "groups_repairable": sum(repairable),
            "groups_failed": len(records) - sum(repairable),
            "group_repair_rate": sum(repairable) / len(records),
            "mean_fault_stddev": statistics.mean(stddevs),
            "p50_fault_stddev": percentile(stddevs, 0.50),
            "p95_fault_stddev": percentile(stddevs, 0.95),
            "borrow_mean": statistics.mean(borrowed), "borrow_p95": percentile(borrowed, 0.95),
            "borrow_max": max(borrowed), "cam_accounting_status": first["cam_accounting_status"],
            "resource_match": first["resource_match"],
        })
        summary_rows.append(summary)
    write_csv(output_root / "summary" / "r2_summary.csv", summary_rows, list(summary_rows[0]))

    imbalance = [{"fault_stddev": row["fault_stddev"], "policy": row["policy"],
                  "repairable": row["repairable"], "RS": row["RS"], "F_GROUP": row["F_GROUP"]}
                 for row in all_records]
    write_csv(output_root / "imbalance" / "fault_stddev_policy_repairable.csv", imbalance,
              list(imbalance[0]))

    by_point_policy: dict[tuple[object, ...], dict[str, dict[str, object]]] = defaultdict(dict)
    for row in all_records:
        by_point_policy[(row["RS"], row["F_GROUP"], row["group_id"])][str(row["policy"])] = row
    paired_rows: list[dict[str, object]] = []
    for (n, f_group, group_id), values in sorted(by_point_policy.items()):
        early = values["directional_m1_early"]
        greedy = values["directional_m1_group_greedy"]
        global_choice = values["directional_m1_group_global"]
        pair_early = values["two_pairwise_m1_early"]
        pair_global = values["two_pairwise_m1_pair_global"]
        hop_early = values["single_hop_m1_early"]
        hop_global = values["single_hop_m1_global"]
        paired_rows.append({
            "RS": n, "CS": n, "F_GROUP": f_group, "group_id": group_id,
            "corpus_id": early["corpus_id"], "EARLY_PASS": early["repairable"],
            "GROUP_GREEDY_PASS": greedy["repairable"], "GROUP_GLOBAL_PASS": global_choice["repairable"],
            "EARLY_FAIL_GREEDY_PASS": int(not integer(str(early["repairable"])) and integer(str(greedy["repairable"]))),
            "EARLY_FAIL_GLOBAL_PASS": int(not integer(str(early["repairable"])) and integer(str(global_choice["repairable"]))),
            "GREEDY_FAIL_GLOBAL_PASS": int(not integer(str(greedy["repairable"])) and integer(str(global_choice["repairable"]))),
            "ALL_PASS": int(all(integer(str(item["repairable"])) for item in (early, greedy, global_choice))),
            "ALL_FAIL": int(not any(integer(str(item["repairable"])) for item in (early, greedy, global_choice))),
            "TWO_PAIRWISE_EARLY_FAIL_PAIR_GLOBAL_PASS": int(not integer(str(pair_early["repairable"])) and integer(str(pair_global["repairable"]))),
            "SINGLE_HOP_EARLY_FAIL_GLOBAL_PASS": int(not integer(str(hop_early["repairable"])) and integer(str(hop_global["repairable"]))),
            "TWO_PAIRWISE_FAIL_SINGLE_HOP_PASS": int(not integer(str(pair_global["repairable"])) and integer(str(hop_global["repairable"]))),
            "TWO_PAIRWISE_PASS_SINGLE_HOP_PASS": int(integer(str(pair_global["repairable"])) and integer(str(hop_global["repairable"]))),
        })
    write_csv(output_root / "paired" / "r2_paired_outcomes_v1.csv", paired_rows,
              list(paired_rows[0]))
    print(f"R2 preflight complete: {len(all_records)} raw policy-group records in {output_root}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
