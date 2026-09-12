#!/usr/bin/env python3
"""Run the group-scope DATE 2x2 DSS experiments without changing C++ semantics."""

from __future__ import annotations

import argparse
import csv
import datetime as dt
import json
import math
import shlex
import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[3]
DEFAULT_MANIFEST = ROOT / "experiments/date_submission_2x2.json"
ANALYZER = Path(__file__).with_name("analyze.py")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--profile", choices=("smoke", "confirm"), required=True)
    parser.add_argument("--manifest", type=Path, default=DEFAULT_MANIFEST)
    parser.add_argument("--run-id")
    parser.add_argument("--output-dir", type=Path)
    parser.add_argument("--no-build", action="store_true")
    parser.add_argument("--skip-tests", action="store_true")
    return parser.parse_args()


def read_json(path: Path) -> dict:
    with path.open(encoding="utf-8") as source:
        return json.load(source)


def write_json(path: Path, value: object) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(value, indent=2, sort_keys=True) + "\n",
                    encoding="utf-8")


def git_metadata() -> dict[str, object]:
    def query(*args: str) -> str:
        result = subprocess.run(["git", *args], cwd=ROOT, text=True,
                                stdout=subprocess.PIPE,
                                stderr=subprocess.DEVNULL, check=False)
        return result.stdout.strip()
    return {
        "revision": query("rev-parse", "HEAD") or "unknown",
        "status_short": query("status", "--short").splitlines(),
    }


def run_logged(command: list[str], directory: Path, metadata: dict) -> None:
    if directory.exists():
        raise RuntimeError(f"point directory already exists: {directory}")
    directory.mkdir(parents=True)
    (directory / "command.txt").write_text(shlex.join(command) + "\n",
                                             encoding="utf-8")
    write_json(directory / "point.json", metadata)
    print(f"[{metadata['kind']}] {metadata['point_id']}", flush=True)
    with (directory / "run.log").open("w", encoding="utf-8") as log:
        subprocess.run(command, cwd=ROOT, stdout=log,
                       stderr=subprocess.STDOUT, check=True)


def largest_remainder(total: int, weights: list[float]) -> list[int]:
    exact = [total * weight / sum(weights) for weight in weights]
    result = [math.floor(value) for value in exact]
    order = sorted(range(len(weights)),
                   key=lambda index: (-(exact[index] - result[index]), index))
    for index in order[:total - sum(result)]:
        result[index] += 1
    return result


def target_row(row: dict[str, str]) -> tuple[str, int] | None:
    policy = row["policy"]
    shared_rows = int(row["shared_rows"])
    shared_columns = int(row["shared_columns"])
    if policy == "no_sharing" and shared_rows == shared_columns == 0:
        return ("no_sharing", int(row["fault_count"]))
    if policy == "directional" and shared_rows == shared_columns == 1:
        return ("directional_m1", int(row["fault_count"]))
    if policy == "directional" and shared_rows == shared_columns == 2:
        return ("directional_m2", int(row["fault_count"]))
    return None


def configured_spares(manifest: dict) -> list[int]:
    """Return symmetric Rs=Cs points, preserving v1 single-spare manifests."""
    values = manifest.get("spare_counts", [manifest["fixed"]["rs"]])
    if not isinstance(values, list) or not values:
        raise RuntimeError("spare_counts must be a non-empty list")
    spares = [int(value) for value in values]
    if any(value <= 0 for value in spares) or len(set(spares)) != len(spares):
        raise RuntimeError("spare_counts must contain distinct positive integers")
    if "spare_counts" not in manifest and manifest["fixed"]["rs"] != manifest["fixed"]["cs"]:
        raise RuntimeError("DATE 2x2 runner requires symmetric Rs=Cs")
    return spares


def select_representative(output_root: Path, candidates: list[int], spare: int) -> int:
    rates: dict[tuple[str, int], list[float]] = {}
    for path in sorted((output_root / "raw/repair_rate").rglob("summary.csv")):
        with path.open(newline="", encoding="utf-8") as source:
            for row in csv.DictReader(source):
                if int(row["Rs"]) != spare or int(row["Cs"]) != spare:
                    continue
                key = target_row(row)
                if key is not None:
                    rates.setdefault(key, []).append(float(row["repair_rate"]))
    usable = [faults for faults in candidates
              if ("no_sharing", faults) in rates and
              ("directional_m2", faults) in rates]
    if not usable:
        raise RuntimeError("no Figure 1 candidate exists for imbalance selection")
    def score(faults: int) -> tuple[float, float, int]:
        baseline = sum(rates[("no_sharing", faults)]) / len(rates[("no_sharing", faults)])
        proposed = sum(rates[("directional_m2", faults)]) / len(rates[("directional_m2", faults)])
        return (proposed - baseline, 1.0 - baseline, -faults)
    return max(usable, key=score)


def main() -> int:
    args = parse_args()
    manifest_path = args.manifest.resolve()
    manifest = read_json(manifest_path)
    profile = manifest["profiles"][args.profile]
    timestamp = dt.datetime.now(dt.timezone.utc).strftime("%Y%m%dT%H%M%SZ")
    run_id = args.run_id or f"{timestamp}_{args.profile}"
    output_root = (args.output_dir or ROOT / "reports/group/date_submission_2x2" / run_id).resolve()
    if output_root.exists():
        raise SystemExit(f"output directory already exists: {output_root}")
    output_root.mkdir(parents=True)
    (output_root / "logs").mkdir()

    repair = ROOT / manifest["simulators"]["repair"]
    sram = ROOT / manifest["simulators"]["sram_recam"]
    validation_commands = []
    if not args.no_build:
        validation_commands.append(["make", "all"])
    if not args.skip_tests:
        validation_commands.append([
            "make", "test_dynamic_spare_sharing_policy",
            "test_dynamic_repair_rate_sweep", "test_dynamic_remap_output",
            "test_sram_recam_model", "test_sram_recam_dynamic_equivalence",
            "test_sram_fault_collection_fifo", "test_fault_address_bist",
            "test_recam_common_models", "test_canonical_experiment",
            "test_canonical_sweep", "test_layout_2x2_regression",
        ])
    with (output_root / "logs/validation.log").open("w", encoding="utf-8") as log:
        for command in validation_commands:
            subprocess.run(command, cwd=ROOT, stdout=log,
                           stderr=subprocess.STDOUT, check=True)
    for binary in (repair, sram):
        if not binary.is_file():
            raise SystemExit(f"missing simulator binary: {binary}")

    resolved = dict(manifest)
    resolved.update({
        "active_profile": args.profile,
        "active_profile_config": profile,
        "manifest_path": str(manifest_path),
        "output_root": str(output_root),
        "run_id": run_id,
        "started_at_utc": timestamp,
        "git": git_metadata(),
    })
    write_json(output_root / "run_config.json", resolved)

    fixed = manifest["fixed"]
    spares = configured_spares(manifest)
    resolved["spare_counts"] = spares
    common_repair = [
        "--layout", "2x2", "--fault-model", fixed["fault_model"],
        "--spatial", fixed["spatial_model"],
        "--memory-rows", str(fixed["memory_rows"]),
        "--memory-columns", str(fixed["memory_columns"]),
        "--paper-cam-reuse", "--local-first",
        "--max-borrows", str(fixed["max_borrows"]),
        "--solution-take", fixed["solution_take"], "--summary-only",
        "--data-width", str(fixed["data_width_bits"]),
        "--row-address-width", str(fixed["row_address_width_bits"]),
        "--column-address-width", str(fixed["column_address_width_bits"]),
    ]
    for spare in spares:
        spare_id = f"rs{spare}_cs{spare}"
        for faults in manifest["fault_counts_per_group"]:
            for seed in profile["seeds"]:
                point_id = f"{spare_id}_f{faults}_s{seed}"
                point_dir = output_root / "raw/repair_rate" / spare_id / f"f{faults}_s{seed}"
                command = [
                    str(repair), str(spare), str(spare),
                    "--repair-rate-sweep", "--fault-min", str(faults),
                    "--fault-max", str(faults), "--fault-step", "1",
                    "--spare-min", str(spare), "--spare-max", str(spare),
                    "--runs", str(profile["runs_per_seed"]), "--seed", str(seed),
                    *common_repair, "--output-dir", str(point_dir),
                ]
                run_logged(command, point_dir, {
                    "kind": "repair_rate", "point_id": point_id,
                    "scope": "group", "rs": spare, "cs": spare,
                    "fault_count_per_group": faults, "seed": seed,
                    "runs": profile["runs_per_seed"],
                    "paired_policies": [policy["id"] for policy in manifest["policies"]],
                })

    representatives = {
        f"rs{spare}_cs{spare}": select_representative(
            output_root, manifest["imbalance"]["representative_candidates"], spare)
        for spare in spares
    }
    # Keep the v1 field for existing tooling while recording every spare point.
    resolved["selected_representative_fault_counts"] = representatives
    if len(spares) == 1:
        resolved["selected_representative_fault_count"] = representatives[f"rs{spares[0]}_cs{spares[0]}"]
    write_json(output_root / "run_config.json", resolved)

    for spare in spares:
        spare_id = f"rs{spare}_cs{spare}"
        representative = representatives[spare_id]
        for distribution in manifest["imbalance"]["profiles"]:
            counts = largest_remainder(representative, distribution["weights"])
            for seed in profile["seeds"]:
                corpus_id = f"{distribution['id']}_f{representative}_s{seed}"
                for policy in manifest["policies"]:
                    point_id = f"{spare_id}_{corpus_id}_{policy['id']}"
                    point_dir = output_root / "raw/imbalance" / spare_id / f"{corpus_id}_{policy['id']}"
                    policy_args = ["--topology", policy["topology"]]
                    if policy["shared_lines"]:
                        policy_args += ["--shared-lines", str(policy["shared_lines"])]
                    command = [
                        str(repair), str(spare), str(spare),
                        "--fault-model", "user", "--fault-counts",
                        ",".join(map(str, counts)), "--spatial", fixed["spatial_model"],
                        "--memory-rows", str(fixed["memory_rows"]),
                        "--memory-columns", str(fixed["memory_columns"]),
                        "--runs", str(profile["runs_per_seed"]), "--seed", str(seed),
                        "--paper-cam-reuse", "--local-first", "--max-borrows",
                        str(fixed["max_borrows"]), "--solution-take",
                        fixed["solution_take"], "--summary-only", *policy_args,
                        "--output-dir", str(point_dir),
                    ]
                    run_logged(command, point_dir, {
                        "kind": "imbalance", "point_id": point_id, "scope": "group",
                        "rs": spare, "cs": spare,
                        "corpus_id": corpus_id, "distribution_id": distribution["id"],
                        "distribution_label": distribution["label"],
                        "fault_counts_abcd": counts, "fault_count_per_group": representative,
                        "policy_id": policy["id"], "seed": seed,
                        "runs": profile["runs_per_seed"],
                    })

    hardware = manifest["hardware_model"]
    for spare in spares:
        spare_id = f"rs{spare}_cs{spare}"
        representative = representatives[spare_id]
        for seed in profile["seeds"]:
            point_id = f"{spare_id}_f{representative}_s{seed}_directional_m2"
            point_dir = output_root / "raw/hardware_model" / spare_id / f"f{representative}_s{seed}_directional_m2"
            command = [
            str(sram), str(spare), str(spare),
            "--runs", str(profile["runs_per_seed"]), "--fault-count", str(representative),
            "--seed", str(seed), "--fault-model", fixed["fault_model"],
            "--spatial", fixed["spatial_model"],
            "--memory-rows", str(fixed["memory_rows"]),
            "--memory-columns", str(fixed["memory_columns"]),
            "--topology", hardware["topology"], "--shared-lines",
            str(hardware["shared_lines"]), "--local-first", "--max-borrows",
            str(fixed["max_borrows"]), "--solution-take", fixed["solution_take"],
            "--paper-cam-reuse", "--policies", ",".join(hardware["search_policies"]),
            "--matrix-policy", hardware["matrix_policy"], "--word-bits",
            str(hardware["word_bits"]), "--runtime-requests",
            str(hardware["runtime_requests"]), "--runtime-hit-rate",
            str(hardware["runtime_hit_rate"]), "--output-dir", str(point_dir),
        ]
            run_logged(command, point_dir, {
            "kind": "hardware_model", "point_id": point_id, "scope": "group",
            "rs": spare, "cs": spare,
            "fault_count_per_group": representative, "seed": seed,
            "runs": profile["runs_per_seed"], "policy_id": "directional_m2",
            "physical_area_available": False,
            })

    subprocess.run([sys.executable, str(ANALYZER), str(output_root)],
                   cwd=ROOT, check=True)
    print(f"Completed DATE 2x2 {args.profile} run: {output_root}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
