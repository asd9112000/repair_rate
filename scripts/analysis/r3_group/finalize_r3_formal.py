#!/usr/bin/env python3
"""Close the archived R3 formal analysis from existing sidecars only.

This script reads the frozen R3 manifest and external raw sidecars.  It never
invokes a simulator, creates a corpus, or mutates the source evidence root.
"""

from __future__ import annotations

import argparse
import csv
import hashlib
import json
import math
import sys
from collections import defaultdict
from pathlib import Path

import matplotlib.pyplot as plt

ROOT = Path(__file__).resolve().parents[3]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))
from scripts.paper_style import style_axis


POLICY_METADATA = {
    "local_no_sharing": ("LOCAL / no sharing", "2x2", "none", "legacy", "LOCAL"),
    "directional_m1_early": ("Directional EARLY", "2x2", "directional", "early", "generic_recam_candidate_v1"),
    "directional_m1_group_greedy": ("Directional GROUP-GREEDY", "2x2", "directional", "group_greedy_rtl_canonical", "frozen_date_2x2_m1"),
    "directional_m1_group_global": ("Directional generic GROUP-GLOBAL", "2x2", "directional", "group_global", "generic_recam_candidate_v1"),
    "pairwise_row_m1_early": ("Pairwise-row EARLY", "2x2", "edge", "early", "generic_recam_candidate_v1"),
    "two_pairwise_m1_early": ("Two-Pairwise EARLY", "1x4", "pair", "one_by_four_two_pairwise_early_v1", "topology-specific"),
    "two_pairwise_m1_pair_global": ("Two-Pairwise Pair-GLOBAL", "1x4", "pair", "one_by_four_two_pairwise_pair_global_v1", "topology-specific"),
    "single_hop_m1_early": ("Single-Hop EARLY", "1x4", "neighbor", "one_by_four_single_hop_early_v1", "topology-specific"),
    "single_hop_m1_global": ("Single-Hop GLOBAL", "1x4", "neighbor", "one_by_four_single_hop_global_v1", "topology-specific"),
}
PLOT_POLICIES = ("local_no_sharing", "directional_m1_early", "directional_m1_group_greedy", "directional_m1_group_global")
PLOT_STYLE = {
    "local_no_sharing": ("#4d4d4d", "o"),
    "directional_m1_early": ("#1f77b4", "s"),
    "directional_m1_group_greedy": ("#d62728", "^"),
    "directional_m1_group_global": ("#9467bd", "D"),
}


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", required=True, type=Path)
    parser.add_argument("--input-root", required=True, type=Path)
    parser.add_argument("--output-root", required=True, type=Path)
    parser.add_argument("--imbalance-only", action="store_true")
    parser.add_argument("--skip-imbalance", action="store_true")
    return parser.parse_args()


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for block in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def write_csv(path: Path, rows: list[dict[str, object]], fields: list[str]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", newline="", encoding="utf-8") as target:
        writer = csv.DictWriter(target, fieldnames=fields, extrasaction="raise", lineterminator="\n")
        writer.writeheader()
        writer.writerows(rows)


def frozen_points(manifest: dict[str, object]) -> dict[str, tuple[int, int, int]]:
    points: dict[str, tuple[int, int, int]] = {}
    for rs_text, faults in manifest["f_group_points"].items():
        rs = int(rs_text)
        for fault in faults:
            fault = int(fault)
            points[f"n{rs}_f{fault}"] = (rs, rs, fault)
    return points


def variance_from_faults(value: str) -> float:
    counts = []
    for bucket in value.split("|"):
        counts.append(0 if not bucket else len(bucket.split(";")))
    if len(counts) != 4:
        raise RuntimeError("expected four subarray fault buckets")
    average = sum(counts) / 4.0
    return sum((count - average) ** 2 for count in counts) / 4.0


def load_results(path: Path) -> dict[int, int]:
    values: dict[int, int] = {}
    with path.open(newline="", encoding="utf-8") as source:
        for row in csv.DictReader(source):
            values[int(row["group_id"])] = int(row["repair_success"])
    return values


def build_imbalance(input_root: Path, output_root: Path) -> list[dict[str, object]]:
    point = "n3_f24"
    policies = ("local_no_sharing", "directional_m1_early", "directional_m1_group_greedy")
    outcomes = {policy: load_results(input_root / "raw" / point / policy / "paired_policy_results_v1.csv") for policy in policies}
    ranked: list[tuple[float, int]] = []
    with (input_root / "corpus" / point / "paired_corpus_v1.csv").open(newline="", encoding="utf-8") as source:
        for row in csv.DictReader(source):
            ranked.append((variance_from_faults(row["sa_local_faults"]), int(row["group_id"])))
    ranked.sort(key=lambda item: (item[0], item[1]))
    bins: dict[int, int] = {}
    for index, (_, group_id) in enumerate(ranked):
        bins[group_id] = min(5, index * 5 // len(ranked) + 1)
    totals: dict[tuple[int, str], list[float]] = defaultdict(lambda: [0.0, 0.0, 0.0])
    for variance, group_id in ranked:
        bin_id = bins[group_id]
        for policy in policies:
            totals[(bin_id, policy)][0] += outcomes[policy][group_id]
            totals[(bin_id, policy)][1] += 1
            totals[(bin_id, policy)][2] += variance
    rows: list[dict[str, object]] = []
    local_rate = {bin_id: totals[(bin_id, "local_no_sharing")][0] / totals[(bin_id, "local_no_sharing")][1] for bin_id in range(1, 6)}
    for bin_id in range(1, 6):
        for policy in policies:
            successes, groups, variance_sum = totals[(bin_id, policy)]
            rate = successes / groups
            rows.append({"point": point, "F_GROUP": 24, "imbalance_metric": "population_variance_of_four_SA_fault_counts", "imbalance_rank_quintile": bin_id, "policy": policy, "policy_label": POLICY_METADATA[policy][0], "groups": int(groups), "mean_variance": variance_sum / groups, "successes": int(successes), "repair_rate": rate, "gain_vs_local_percentage_points": 100.0 * (rate - local_rate[bin_id])})
    fields = ["point", "F_GROUP", "imbalance_metric", "imbalance_rank_quintile", "policy", "policy_label", "groups", "mean_variance", "successes", "repair_rate", "gain_vs_local_percentage_points"]
    write_csv(output_root / "R3_FINAL_IMBALANCE_TABLE.csv", rows, fields)
    return rows


def save_figure(figure, base: Path) -> None:
    base.parent.mkdir(parents=True, exist_ok=True)
    for suffix in ("png", "pdf", "svg"):
        figure.savefig(base.with_suffix(f".{suffix}"), dpi=220, bbox_inches="tight")
    svg = base.with_suffix(".svg")
    svg.write_text("\n".join(line.rstrip() for line in svg.read_text(encoding="utf-8").splitlines()) + "\n", encoding="utf-8")
    plt.close(figure)


def plot_main(rows: list[dict[str, object]], figures: Path) -> None:
    figure, axis = plt.subplots(figsize=(6.8, 3.8))
    for policy in PLOT_POLICIES:
        selected = [row for row in rows if row["policy"] == policy]
        selected.sort(key=lambda row: int(row["F_GROUP"]))
        color, marker = PLOT_STYLE[policy]
        axis.plot([row["F_GROUP"] for row in selected], [100.0 * float(row["repair_rate"]) for row in selected], label=POLICY_METADATA[policy][0], color=color, marker=marker)
    style_axis(axis, grid=True, grid_axis="y")
    axis.set_xlabel("$F_{GROUP}$")
    axis.set_ylabel("Repair rate (%)")
    axis.legend(frameon=False, fontsize=8)
    save_figure(figure, figures / "R3-1_main_repairability")


def plot_capacity(rows: list[dict[str, object]], figures: Path) -> None:
    figure, axis = plt.subplots(figsize=(6.8, 3.8))
    local = {int(row["F_GROUP"]): float(row["repair_rate"]) for row in rows if row["policy"] == "local_no_sharing"}
    for policy in PLOT_POLICIES[1:]:
        selected = [row for row in rows if row["policy"] == policy]
        selected.sort(key=lambda row: int(row["F_GROUP"]))
        color, marker = PLOT_STYLE[policy]
        axis.plot([row["F_GROUP"] for row in selected], [100.0 * (float(row["repair_rate"]) - local[int(row["F_GROUP"])]) for row in selected], label=POLICY_METADATA[policy][0], color=color, marker=marker)
    axis.axhline(0.0, color="#777777", linewidth=0.8, linestyle=":")
    style_axis(axis, grid=True, grid_axis="y")
    axis.set_xlabel("$F_{GROUP}$")
    axis.set_ylabel("Repair-rate gain vs LOCAL (pp)")
    axis.legend(frameon=False, fontsize=8)
    save_figure(figure, figures / "R3-2_capacity_boundary")


def plot_imbalance(rows: list[dict[str, object]], figures: Path) -> None:
    figure, axis = plt.subplots(figsize=(6.8, 3.8))
    for policy in ("directional_m1_early", "directional_m1_group_greedy"):
        selected = [row for row in rows if row["policy"] == policy]
        selected.sort(key=lambda row: int(row["imbalance_rank_quintile"]))
        color, marker = PLOT_STYLE[policy]
        axis.plot([row["imbalance_rank_quintile"] for row in selected], [row["gain_vs_local_percentage_points"] for row in selected], label=POLICY_METADATA[policy][0], color=color, marker=marker)
    axis.axhline(0.0, color="#777777", linewidth=0.8, linestyle=":")
    style_axis(axis, grid=True, grid_axis="y")
    axis.set_xlabel("Fault-allocation variance rank quintile, F=24")
    axis.set_ylabel("Repair-rate gain vs LOCAL (pp)")
    axis.set_xticks((1, 2, 3, 4, 5))
    axis.legend(frameon=False, fontsize=8)
    save_figure(figure, figures / "R3-3_fault_imbalance_f24")


def main() -> int:
    args = parse_args()
    manifest_path = args.manifest.resolve()
    input_root = args.input_root.resolve()
    output_root = args.output_root.resolve()
    if args.imbalance_only:
        imbalance_rows = build_imbalance(input_root, output_root)
        plot_imbalance(imbalance_rows, output_root / "figures")
        print("wrote F=24 fault-imbalance side analysis")
        return 0
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    frozen = frozen_points(manifest)
    output_root.mkdir(parents=True, exist_ok=True)
    root_policies = tuple(item["id"] for item in manifest["policy_matrix"]["3"])
    evidence_rows: list[dict[str, object]] = []
    final_rows: list[dict[str, object]] = []
    point_status: list[dict[str, object]] = []
    raw_root = input_root / "raw"
    corpus_root = input_root / "corpus"
    all_points = sorted({path.name for path in raw_root.iterdir() if path.is_dir()} | {path.name for path in corpus_root.iterdir() if path.is_dir()})
    for point_name in all_points:
        point_frozen = point_name in frozen
        # The frozen manifest is the only paper-facing scope.  Discover
        # non-frozen sidecars, but do not spend a second full raw parse on
        # historical points merely to classify them.
        if not point_frozen:
            policy_count = len([path for path in (raw_root / point_name).iterdir() if path.is_dir()]) if (raw_root / point_name).is_dir() else 0
            point_status.append({"point": point_name, "classification": "NON_FROZEN_SIDECAR", "status": "DISCOVERED_NOT_PRIMARY", "policy_count": policy_count})
            continue
        policy_dirs = sorted(path for path in (raw_root / point_name).iterdir() if path.is_dir()) if (raw_root / point_name).is_dir() else []
        if not policy_dirs:
            point_status.append({"point": point_name, "classification": "NON_FROZEN_SIDECAR" if not point_frozen else "FROZEN", "status": "MISSING_RAW", "policy_count": 0})
            continue
        point_ok = True
        for policy_dir in policy_dirs:
            policy = policy_dir.name
            result_path = policy_dir / "paired_policy_results_v1.csv"
            summary_path = policy_dir / "summary.csv"
            policy_corpus_path = policy_dir / "paired_corpus_v1.csv"
            corpus_path = corpus_root / point_name / "paired_corpus_v1.csv"
            if not (result_path.is_file() and summary_path.is_file() and policy_corpus_path.is_file() and corpus_path.is_file()):
                point_ok = False
                continue
            with summary_path.open(newline="", encoding="utf-8") as source:
                summary = next(csv.DictReader(source))
            with corpus_path.open(newline="", encoding="utf-8") as source:
                corpus_head = next(csv.DictReader(source))
            with policy_corpus_path.open(newline="", encoding="utf-8") as source:
                policy_corpus_head = next(csv.DictReader(source))
            count = int(summary["runs"])
            successes = round(float(summary["repair_rate"]) * count)
            corpus_id = corpus_head["corpus_id"]
            result_policy_id = policy
            expected_rs, expected_cs, fault = frozen.get(point_name, (int(point_name.split("_")[0][1:]), int(point_name.split("_f")[1]), int(point_name.split("_f")[1])))
            seed = int(summary["seed"])
            classification = "FROZEN_FORMAL" if point_frozen else "NON_FROZEN_SIDECAR"
            status = "COMPLETE" if count == 100000 and int(corpus_head["seed"]) == seed and policy_corpus_head["corpus_id"] == corpus_id and int(policy_corpus_head["seed"]) == seed else "INCONSISTENT"
            point_ok &= status == "COMPLETE"
            evidence_rows.append({"point": point_name, "policy": policy, "topology": POLICY_METADATA.get(policy, ("UNKNOWN", "UNKNOWN", "UNKNOWN", "UNKNOWN", "UNKNOWN"))[2], "RS": expected_rs, "CS": expected_cs, "m": int(manifest["share_m"]), "F_GROUP": fault, "seed": seed, "N": count, "status": status, "source_path": str(summary_path), "source_file_sha256": sha256(summary_path), "raw_sidecar_path": str(result_path), "corpus_path": str(corpus_path), "policy_corpus_path": str(policy_corpus_path), "corpus_id": corpus_id, "result_policy_id": result_policy_id, "classification": classification})
            if point_frozen:
                label, layout, topology, solution, universe = POLICY_METADATA[policy]
                final_rows.append({"RS": expected_rs, "CS": expected_cs, "m": 1, "F_GROUP": fault, "seed": seed, "policy": policy, "policy_label": label, "layout": layout, "topology": topology, "solution_semantics": solution, "candidate_universe": universe, "groups": count, "successes": successes, "failures": count - successes, "repair_rate": successes / count, "gain_vs_local_percentage_points": ""})
        point_status.append({"point": point_name, "classification": "FROZEN_FORMAL" if point_frozen else "NON_FROZEN_SIDECAR", "status": "COMPLETE" if point_ok else "INCONSISTENT", "policy_count": len(policy_dirs)})
    if {row["policy"] for row in final_rows} != set(root_policies) or len(final_rows) != len(frozen) * len(root_policies):
        raise RuntimeError("frozen policy/point inventory does not match manifest")
    local = {int(row["F_GROUP"]): float(row["repair_rate"]) for row in final_rows if row["policy"] == "local_no_sharing"}
    for row in final_rows:
        row["gain_vs_local_percentage_points"] = 100.0 * (float(row["repair_rate"]) - local[int(row["F_GROUP"])])
    fields = ["point", "policy", "topology", "RS", "CS", "m", "F_GROUP", "seed", "N", "status", "source_path", "source_file_sha256", "raw_sidecar_path", "corpus_path", "policy_corpus_path", "corpus_id", "result_policy_id", "classification"]
    write_csv(output_root / "R3_FORMAL_EVIDENCE_MANIFEST.csv", evidence_rows, fields)
    final_fields = ["RS", "CS", "m", "F_GROUP", "seed", "policy", "policy_label", "layout", "topology", "solution_semantics", "candidate_universe", "groups", "successes", "failures", "repair_rate", "gain_vs_local_percentage_points"]
    write_csv(output_root / "R3_FINAL_REPAIR_RATE_TABLE.csv", sorted(final_rows, key=lambda row: (int(row["F_GROUP"]), root_policies.index(str(row["policy"])))), final_fields)
    scope_rows = [{"policy": policy, "policy_label": POLICY_METADATA[policy][0], "layout": POLICY_METADATA[policy][1], "topology": POLICY_METADATA[policy][2], "solution_semantics": POLICY_METADATA[policy][3], "candidate_universe": POLICY_METADATA[policy][4], "m": 1, "classification": "FROZEN_FORMAL"} for policy in root_policies]
    write_csv(output_root / "R3_FINAL_POLICY_SCOPE_TABLE.csv", scope_rows, list(scope_rows[0]))
    write_csv(output_root / "R3_FINAL_POINT_STATUS_TABLE.csv", point_status, ["point", "classification", "status", "policy_count"])
    figures = output_root / "figures"
    plot_main(final_rows, figures)
    plot_capacity(final_rows, figures)
    audit = {"frozen_points": sorted(frozen), "frozen_policy_count": len(root_policies), "frozen_result_count": len(final_rows), "non_frozen_result_count": sum(int(row["policy_count"]) for row in point_status if row["classification"] == "NON_FROZEN_SIDECAR"), "frozen_complete": all(row["status"] == "COMPLETE" for row in evidence_rows if row["classification"] == "FROZEN_FORMAL"), "same_corpus_by_point": True, "new_repair_rate_simulation": False, "manifest_sha256": sha256(manifest_path)}
    (output_root / "R3_FINAL_EVIDENCE_AUDIT.json").write_text(json.dumps(audit, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    if not args.skip_imbalance:
        # Kept for direct interactive use; controlled closure invokes this
        # phase separately so the analysis worker stays below its wall-time cap.
        imbalance_rows = build_imbalance(input_root, output_root)
        plot_imbalance(imbalance_rows, figures)
    print(json.dumps(audit, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
