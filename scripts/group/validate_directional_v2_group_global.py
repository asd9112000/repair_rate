#!/usr/bin/env python3
"""Validate the directional V2 group-global oracle on an existing corpus.

This script intentionally does not alter R3's canonical Matrix V2.  It
materializes the already checked quick corpus, runs only the new V2 oracle in
an explicit temporary root, and verifies GREEDY containment by group_id.
"""

from __future__ import annotations

import argparse
import csv
import json
import subprocess
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
POINTS = ((2, 8), (2, 12), (2, 16), (2, 20), (2, 24), (2, 28), (2, 32),
          (3, 8), (3, 12), (3, 16), (3, 20), (3, 24), (3, 28), (3, 32))
POLICY_ID = "directional_m1_v2_group_global"
SOLUTION = "directional_v2_group_global"


def read_csv(path: Path) -> list[dict[str, str]]:
    with path.open(newline="", encoding="utf-8") as source:
        return list(csv.DictReader(source))


def write_csv(path: Path, rows: list[dict[str, object]], fields: list[str]) -> None:
    with path.open("w", newline="", encoding="utf-8") as target:
        writer = csv.DictWriter(target, fieldnames=fields)
        writer.writeheader()
        writer.writerows(rows)


def materialize(rows: list[dict[str, str]], destination: Path) -> None:
    with destination.open("w", encoding="utf-8") as target:
        for expected, row in enumerate(rows):
            if int(row["group_id"]) != expected:
                raise RuntimeError(f"corpus group IDs are not contiguous at {destination}")
            buckets = row["sa_local_faults"].split("|")
            if len(buckets) != 4:
                raise RuntimeError(f"invalid four-SA corpus row at {destination}")
            for bucket in buckets:
                for fault in ([] if not bucket else bucket.split(";")):
                    target.write(fault.replace(":", " ") + "\n")


def run(command: list[str], log: Path) -> None:
    with log.open("w", encoding="utf-8") as output:
        output.write("command=" + " ".join(command) + "\n")
        completed = subprocess.run(command, cwd=ROOT, text=True,
                                   stdout=output, stderr=subprocess.STDOUT)
    if completed.returncode:
        raise RuntimeError(f"simulation failed; inspect {log}")


def result_map(path: Path) -> dict[int, int]:
    rows = read_csv(path)
    result = {int(row["group_id"]): int(row["repair_success"]) for row in rows}
    if sorted(result) != list(range(len(rows))):
        raise RuntimeError(f"noncontiguous result IDs: {path}")
    return result


def complete_v2_sidecar(path: Path, samples: int) -> bool:
    if not path.is_file():
        return False
    rows = read_csv(path)
    return (len(rows) == samples and
            all(int(row["group_id"]) == index for index, row in enumerate(rows)) and
            all(row["policy_id"] == SOLUTION and
                row["solution_policy"] == SOLUTION for row in rows))


def point_command(binary: Path, n: int, f_group: int, samples: int,
                  seed: int, replay: Path, output: Path) -> list[str]:
    return [str(binary), str(n), str(n),
            "--simplified-fault-file", str(replay),
            "--fault-model", "multinomial_uniform", "--fault-count", str(f_group),
            "--runs", str(samples), "--seed", str(seed), "--spatial", "mixed",
            "--layout", "2x2", "--topology", "directional",
            "--shared-rows", "1", "--shared-columns", "1",
            "--solution-take", SOLUTION, "--paper-cam-reuse",
            "--hybrid-cam-entry-width-bits", "20", "--summary-only",
            "--output-dir", str(output)]


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--source-root", type=Path,
                        default=Path("tmp/repair_rate_simple"))
    parser.add_argument("--output-root", type=Path, required=True)
    parser.add_argument("--binary", type=Path,
                        default=Path("build/bin/DynamicSpareSharing"))
    parser.add_argument("--resume", action="store_true",
                        help="Reuse complete V2 policy sidecars in this validation root")
    args = parser.parse_args()

    source_root = (ROOT / args.source_root).resolve()
    output_root = (ROOT / args.output_root).resolve()
    binary = (ROOT / args.binary).resolve()
    if output_root.exists() and not args.resume:
        raise RuntimeError(f"output root must be fresh: {output_root}")
    if not binary.is_file():
        raise RuntimeError(f"missing DynamicSpareSharing binary: {binary}")
    output_root.mkdir(parents=True, exist_ok=args.resume)
    (output_root / "corpus").mkdir(exist_ok=args.resume)
    (output_root / "raw").mkdir(exist_ok=args.resume)
    (output_root / "logs").mkdir(exist_ok=args.resume)

    summary: list[dict[str, object]] = []
    first_suboptimal: tuple[int, int, int, dict[str, str]] | None = None
    totals = {"both_pass": 0, "greedy_only": 0, "v2_global_only": 0, "both_fail": 0}
    scaling: dict[int, dict[str, int]] = {}

    for n, f_group in POINTS:
        point = f"n{n}_f{f_group}"
        corpus_rows = read_csv(source_root / "corpus" / point / "paired_corpus_v1.csv")
        greedy = result_map(source_root / "raw" / point /
                            "directional_m1_group_greedy" /
                            "paired_policy_results_v1.csv")
        replay = output_root / "corpus" / f"{point}.simplified_faults"
        if not replay.exists():
            materialize(corpus_rows, replay)
        point_output = output_root / "raw" / point / POLICY_ID
        result_path = point_output / "paired_policy_results_v1.csv"
        if result_path.exists() and not complete_v2_sidecar(
                result_path, len(corpus_rows)):
            raise RuntimeError(f"{point}: incomplete or wrong-policy V2 sidecar: {result_path}")
        if not result_path.exists():
            command = point_command(binary, n, f_group, len(corpus_rows),
                                    int(corpus_rows[0]["seed"]), replay, point_output)
            point_output.parent.mkdir(parents=True, exist_ok=True)
            run(command, output_root / "logs" / f"{point}_{POLICY_ID}.log")
        v2_rows = read_csv(point_output / "paired_policy_results_v1.csv")
        v2_global = {int(row["group_id"]): int(row["repair_success"]) for row in v2_rows}
        if set(greedy) != set(v2_global):
            raise RuntimeError(f"{point}: GREEDY/V2 GLOBAL row universe differs")

        counts = {"both_pass": 0, "greedy_only": 0,
                  "v2_global_only": 0, "both_fail": 0}
        for group_id in sorted(greedy):
            pair = (greedy[group_id], v2_global[group_id])
            if pair == (1, 1):
                counts["both_pass"] += 1
            elif pair == (1, 0):
                counts["greedy_only"] += 1
            elif pair == (0, 1):
                counts["v2_global_only"] += 1
                if first_suboptimal is None:
                    first_suboptimal = (n, f_group, group_id, corpus_rows[group_id])
            else:
                counts["both_fail"] += 1
        if counts["greedy_only"]:
            raise RuntimeError(
                f"{point}: V2 GLOBAL containment violation; "
                f"first group={next(i for i in sorted(greedy) if greedy[i] and not v2_global[i])}")
        for key in totals:
            totals[key] += counts[key]

        point_scale = scaling.setdefault(n, {"max_local_candidates_per_sa": 0,
                                               "max_raw_tuple_space": 0,
                                               "max_dfs_nodes": 0,
                                               "max_complete_tuples_checked": 0})
        for row in v2_rows:
            local = max(int(row[f"global_candidate_count_{sa}"])
                        for sa in ("A", "B", "C", "D"))
            point_scale["max_local_candidates_per_sa"] = max(
                point_scale["max_local_candidates_per_sa"], local)
            point_scale["max_raw_tuple_space"] = max(
                point_scale["max_raw_tuple_space"], int(row["global_raw_cartesian_product_size"]))
            point_scale["max_dfs_nodes"] = max(
                point_scale["max_dfs_nodes"], int(row["global_search_nodes_visited"]))
            point_scale["max_complete_tuples_checked"] = max(
                point_scale["max_complete_tuples_checked"], int(row["global_complete_assignments_checked"]))
        summary.append({"point": point, "N": n, "F_GROUP": f_group,
                        "groups": len(corpus_rows), **counts,
                        **{f"max_{key}": value for key, value in point_scale.items()}})
        print(f"validated {point}: {counts}", flush=True)

    write_csv(output_root / "v2_global_paired_summary.csv", summary,
              list(summary[0].keys()))
    manifest = {"classification": "QUICK_SWEEP / DEVELOPMENT / NON-FORMAL",
                "source_root": str(source_root), "policy_id": POLICY_ID,
                "solution_take": SOLUTION, "points": [f"n{n}_f{f}" for n, f in POINTS],
                "totals": totals, "scaling": scaling,
                "first_true_greedy_suboptimal_case": (
                    {"N": first_suboptimal[0], "F_GROUP": first_suboptimal[1],
                     "group_id": first_suboptimal[2],
                     "faults": first_suboptimal[3]["sa_local_faults"]}
                    if first_suboptimal else None),
                "formal_100k_data_touched": False,
                "historical_sweep_rewritten": False,
                "generic_global_semantics_changed": False}
    with (output_root / "validation_manifest.json").open("w", encoding="utf-8") as target:
        json.dump(manifest, target, indent=2)
        target.write("\n")
    print(json.dumps(manifest, indent=2), flush=True)


if __name__ == "__main__":
    main()
