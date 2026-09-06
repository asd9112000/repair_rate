#!/usr/bin/env python3
"""Run the paired moderate-fault repair study without mixing architecture scopes."""

from __future__ import annotations

import argparse
import csv
import json
import shlex
import subprocess
import sys
from collections import defaultdict
from pathlib import Path


ROOT = Path(__file__).resolve().parents[3]
DEFAULT_MANIFEST = ROOT / "experiments" / "moderate_repair_study.json"


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--profile", choices=("smoke", "screen", "confirm"),
                        required=True)
    parser.add_argument("--manifest", type=Path, default=DEFAULT_MANIFEST)
    parser.add_argument("--output-dir", type=Path)
    parser.add_argument("--selection-from", type=Path,
                        help="screen selected_policies.json; required by confirm")
    parser.add_argument("--no-build", action="store_true")
    return parser.parse_args()


def read_json(path: Path) -> dict:
    with path.open(encoding="utf-8") as source:
        return json.load(source)


def write_json(path: Path, value: object) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(value, indent=2, sort_keys=True) + "\n",
                    encoding="utf-8")


def git_metadata() -> dict[str, str]:
    def query(*args: str) -> str:
        result = subprocess.run(
            ["git", *args], cwd=ROOT, text=True, stdout=subprocess.PIPE,
            stderr=subprocess.DEVNULL, check=False)
        return result.stdout.strip()
    return {
        "revision": query("rev-parse", "HEAD") or "unknown",
        "status_short": query("status", "--short"),
    }


def run_command(command: list[str], point_dir: Path, metadata: dict) -> None:
    point_dir.mkdir(parents=True, exist_ok=True)
    (point_dir / "command.txt").write_text(
        shlex.join(command) + "\n", encoding="utf-8")
    write_json(point_dir / "point.json", metadata)
    print(f"[{metadata['kind']}] {metadata['point_id']}", flush=True)
    with (point_dir / "run.log").open("w", encoding="utf-8") as log:
        subprocess.run(command, cwd=ROOT, stdout=log,
                       stderr=subprocess.STDOUT, check=True)


def policy_args(policy: dict, shared_lines: int, max_borrows: int) -> list[str]:
    lines = int(policy.get("shared_lines", shared_lines))
    result = ["--topology", policy["topology"]]
    if policy["topology"] != "none":
        result.extend(["--shared-lines", str(lines), "--local-first",
                       "--max-borrows", str(max_borrows)])
    return result


def select_policies(output_root: Path, policies: list[dict], spares: list[int]) -> dict:
    by_spare_policy: dict[tuple[int, str], list[dict[str, str]]] = defaultdict(list)
    for point in sorted((output_root / "group").glob("*/point.json")):
        metadata = read_json(point)
        with (point.parent / "summary.csv").open(newline="", encoding="utf-8") as source:
            row = next(csv.DictReader(source))
        by_spare_policy[(int(metadata["rs"]), metadata["policy_id"])].append(row)

    result: dict[str, dict] = {}
    policy_order = {policy["id"]: index for index, policy in enumerate(policies)}
    for spare in spares:
        ranked = []
        for policy in policies:
            if policy["id"] == "no_sharing":
                continue
            rows = by_spare_policy[(spare, policy["id"])]
            if not rows:
                raise RuntimeError(f"missing group results for Rs=Cs={spare}, {policy['id']}")
            mean_rate = sum(float(row["repair_rate"]) for row in rows) / len(rows)
            mean_borrow = sum(float(row["average_borrow_count"]) for row in rows) / len(rows)
            ranked.append({
                "policy_id": policy["id"],
                "mean_repair_rate": mean_rate,
                "mean_borrow_count": mean_borrow,
            })
        ranked.sort(key=lambda row: (-row["mean_repair_rate"],
                                    row["mean_borrow_count"],
                                    policy_order[row["policy_id"]]))
        result[str(spare)] = {
            "device_policy": ranked[0]["policy_id"],
            "ranked_sharing_policies": ranked,
        }
    write_json(output_root / "selected_policies.json", result)
    return result


def main() -> int:
    args = parse_args()
    manifest_path = args.manifest.resolve()
    manifest = read_json(manifest_path)
    profile = manifest["profiles"][args.profile]
    output_root = (args.output_dir or
                   ROOT / "reports" / "studies" / "moderate_repair" /
                   args.profile).resolve()
    if output_root.exists():
        raise SystemExit(f"output directory already exists: {output_root}")
    output_root.mkdir(parents=True)

    binaries = {
        "group": ROOT / "build/bin/DynamicSpareSharing",
        "sram": ROOT / "build/bin/DynamicSpareSharing_SRAM_RECAM",
        "device": ROOT / "build/bin/HierarchicalRECAM",
    }
    if not args.no_build:
        subprocess.run(
            ["make", "dynamic_sharing_b", "dynamic_sram_recam_b",
             "hierarchical_recam_b"], cwd=ROOT, check=True)
    missing = [str(path) for path in binaries.values() if not path.is_file()]
    if missing:
        raise SystemExit("missing simulator binaries: " + ", ".join(missing))

    resolved = dict(manifest)
    resolved.update({"active_profile": args.profile,
                     "active_profile_config": profile,
                     "manifest_path": str(manifest_path),
                     "git": git_metadata()})
    write_json(output_root / "run_config.json", resolved)

    policies = manifest["group_policies"]
    policy_by_id = {policy["id"]: policy for policy in policies}
    prior_selection = None
    if profile["group_policy_mode"] == "selected":
        if args.selection_from is None:
            raise SystemExit("confirm profile requires --selection-from selected_policies.json")
        prior_selection = read_json(args.selection_from.resolve())

    common_group = [
        "--fault-model", manifest["fault_model"],
        "--spatial", manifest["spatial_model"],
        "--memory-rows", str(manifest["memory_rows"]),
        "--memory-columns", str(manifest["memory_columns"]),
        "--paper-cam-reuse", "--data-width", str(manifest["word_bits"]),
        "--row-address-width", str(manifest["row_address_bits"]),
        "--column-address-width", str(manifest["column_address_bits"]),
        "--summary-only",
    ]
    for spare in manifest["spares"]:
        if prior_selection is None:
            active_policies = policies
        else:
            ranked = prior_selection[str(spare)]["ranked_sharing_policies"]
            active_ids = ["no_sharing"] + [row["policy_id"] for row in ranked[:2]]
            active_policies = [policy_by_id[policy_id] for policy_id in active_ids]
        for faults in manifest["fault_counts"]:
            for seed in profile["seeds"]:
                for policy in active_policies:
                    point_id = f"r{spare}_c{spare}_f{faults}_s{seed}_{policy['id']}"
                    point_dir = output_root / "group" / point_id
                    command = [
                        str(binaries["group"]), str(spare), str(spare),
                        "--fault-count", str(faults),
                        "--runs", str(profile["group_runs"]),
                        "--seed", str(seed), *common_group,
                        *policy_args(policy, manifest["shared_lines"],
                                     manifest["max_borrows"]),
                        "--output-dir", str(point_dir),
                    ]
                    run_command(command, point_dir, {
                        "kind": "group", "point_id": point_id,
                        "rs": spare, "cs": spare, "fault_count": faults,
                        "seed": seed, "runs": profile["group_runs"],
                        "policy_id": policy["id"],
                    })

    selection = prior_selection or select_policies(
        output_root, policies, manifest["spares"])
    if prior_selection is not None:
        write_json(output_root / "selected_policies.json", selection)

    for spare in manifest["spares"]:
        selected_id = selection[str(spare)]["device_policy"]
        selected = policy_by_id[selected_id]
        for seed in profile["seeds"]:
            for matrix, policy_list in (
                (manifest["sram_matrix_policy"], manifest["sram_policies"]),
                ("shadow", [manifest["sram_shadow_policy"]]),
            ):
                point_id = f"r{spare}_c{spare}_f{manifest['sram_fault_count']}_s{seed}_{selected_id}_{matrix}"
                point_dir = output_root / "sram" / point_id
                command = [
                    str(binaries["sram"]), str(spare), str(spare),
                    "--fault-count", str(manifest["sram_fault_count"]),
                    "--runs", str(profile["group_runs"]), "--seed", str(seed),
                    "--fault-model", manifest["fault_model"],
                    "--spatial", manifest["spatial_model"],
                    "--memory-rows", str(manifest["memory_rows"]),
                    "--memory-columns", str(manifest["memory_columns"]),
                    "--word-bits", str(manifest["word_bits"]),
                    "--paper-cam-reuse", "--policies", ",".join(policy_list),
                    "--matrix-policy", matrix,
                    *policy_args(selected, manifest["shared_lines"],
                                 manifest["max_borrows"]),
                    "--output-dir", str(point_dir),
                ]
                run_command(command, point_dir, {
                    "kind": "sram", "point_id": point_id,
                    "rs": spare, "cs": spare,
                    "fault_count": manifest["sram_fault_count"],
                    "seed": seed, "runs": profile["group_runs"],
                    "policy_id": selected_id, "matrix_policy": matrix,
                })

    for spare in manifest["spares"]:
        selected_id = selection[str(spare)]["device_policy"]
        selected = policy_by_id[selected_id]
        for faults in manifest["fault_counts"]:
            for seed in profile["seeds"]:
                point_id = f"r{spare}_c{spare}_f{faults}_s{seed}_{selected_id}"
                point_dir = output_root / "device" / point_id
                shared = int(selected.get("shared_lines", manifest["shared_lines"]))
                command = [
                    str(binaries["device"]), str(spare), str(spare),
                    "--canonical-four", "--groups", str(profile["device_groups"]),
                    "--fault-count", str(faults), "--seed", str(seed),
                    "--fault-model", manifest["fault_model"],
                    "--spatial", manifest["spatial_model"],
                    "--memory-rows", str(manifest["memory_rows"]),
                    "--memory-columns", str(manifest["memory_columns"]),
                    "--data-word-bits", str(manifest["word_bits"]),
                    "--topology", selected["topology"],
                    "--shared-rows", str(shared),
                    "--shared-columns", str(shared),
                    "--max-borrows", str(manifest["max_borrows"]),
                    "--sram-policy", manifest["device_sram_policy"],
                    "--sram-matrix-policy", manifest["sram_matrix_policy"],
                    "--write-fault-corpus", "--output-dir", str(point_dir),
                ]
                run_command(command, point_dir, {
                    "kind": "device", "point_id": point_id,
                    "rs": spare, "cs": spare, "fault_count": faults,
                    "seed": seed, "groups": profile["device_groups"],
                    "policy_id": selected_id,
                    "sram_policy": manifest["device_sram_policy"],
                    "matrix_policy": manifest["sram_matrix_policy"],
                })

    subprocess.run([
        sys.executable, str(ROOT / "scripts/device/moderate_repair_study/collect.py"),
        str(output_root)], cwd=ROOT, check=True)
    subprocess.run([
        sys.executable, str(ROOT / "scripts/device/moderate_repair_study/plot.py"),
        str(output_root / "combined"), "--output-dir",
        str(output_root / "plots")], cwd=ROOT, check=True)
    print(f"Completed {args.profile} profile: {output_root}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
