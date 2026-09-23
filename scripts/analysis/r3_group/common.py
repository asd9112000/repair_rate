#!/usr/bin/env python3
"""Shared authoritative raw/corpus loader for R3 group analysis.

This module deliberately reads per-point corpus and policy sidecars rather
than runner aggregate files.  Root manifests remain provenance only because a
resume over selected point subsets can leave their point inventory incomplete.
"""

from __future__ import annotations

import csv
import json
import math
import re
from dataclasses import dataclass, field
from pathlib import Path
from statistics import mean
from typing import Any, Iterable


PROJECT_ROOT = Path(__file__).resolve().parents[3]
POINT_RE = re.compile(r"^n(?P<rs>\d+)_f(?P<faults>\d+)$")

POLICIES: dict[str, dict[str, str | int]] = {
    "local_no_sharing": {"label": "LOCAL", "architecture_family": "LOCAL", "layout": "2x2", "topology": "none", "resource": "LOCAL", "solution": "LOCAL", "plot_order": 0},
    "directional_m1_local_first": {"label": "Directional V2 LOCAL-FIRST", "topology": "directional", "solution": "LOCAL_FIRST"},
    "directional_m1_early": {"label": "Directional V2 EARLY", "topology": "directional", "solution": "EARLY"},
    "directional_m1_global": {"label": "Directional V2 GLOBAL", "topology": "directional", "solution": "GLOBAL"},
    # Retained only to analyze historical corpus results.  This policy is not
    # a Matrix V2 paper member and must never be displayed as V2 GLOBAL.
    "directional_m1_group_global": {"label": "Directional generic GROUP_GLOBAL (legacy)", "topology": "directional", "solution": "GENERIC_GROUP_GLOBAL_LEGACY"},
    "pairwise_row_m1_local_first": {"label": "Pairwise-row LOCAL-FIRST", "topology": "edge", "solution": "LOCAL_FIRST"},
    "pairwise_row_m1_early": {"label": "Pairwise-row EARLY", "topology": "edge", "solution": "EARLY"},
    "pairwise_row_m1_global": {"label": "Pairwise-row GLOBAL", "topology": "edge", "solution": "GLOBAL"},
    "two_pairwise_m1_local_first": {"label": "Two-Pairwise LOCAL-FIRST", "topology": "pair", "solution": "LOCAL_FIRST"},
    "two_pairwise_m1_early": {"label": "Two-Pairwise EARLY", "topology": "pair", "solution": "EARLY"},
    "two_pairwise_m1_pair_global": {"label": "Two-Pairwise Pair-GLOBAL", "topology": "pair", "solution": "PAIR_GLOBAL"},
    "single_hop_m1_local_first": {"label": "Single-Hop LOCAL-FIRST", "topology": "neighbor", "solution": "LOCAL_FIRST"},
    "single_hop_m1_early": {"label": "Single-Hop EARLY", "topology": "neighbor", "solution": "EARLY"},
    "single_hop_m1_global": {"label": "Single-Hop GLOBAL", "topology": "neighbor", "solution": "GLOBAL"},
    # DATE2026 six-case static sweep IDs are result-sidecar IDs, not display labels.
    "g2x2_rc_early": {"label": "G2X2 RC EARLY", "architecture_family": "SIXCASE_STATIC", "layout": "2x2", "topology": "directional", "resource": "RC", "solution": "EARLY", "plot_order": 1},
    "g2x2_rc_group": {"label": "G2X2 RC GROUP", "architecture_family": "SIXCASE_STATIC", "layout": "2x2", "topology": "directional", "resource": "RC", "solution": "GROUP", "plot_order": 2},
    "g2x2_r_early": {"label": "G2X2 R EARLY", "architecture_family": "SIXCASE_STATIC", "layout": "2x2", "topology": "directional", "resource": "R", "solution": "EARLY", "plot_order": 3},
    "g2x2_r_group": {"label": "G2X2 R GROUP", "architecture_family": "SIXCASE_STATIC", "layout": "2x2", "topology": "directional", "resource": "R", "solution": "GROUP", "plot_order": 4},
    "l1x4_r_early": {"label": "L1X4 R EARLY", "architecture_family": "SIXCASE_STATIC", "layout": "1x4", "topology": "neighbor", "resource": "R", "solution": "EARLY", "plot_order": 5},
    "l1x4_r_group": {"label": "L1X4 R GROUP", "architecture_family": "SIXCASE_STATIC", "layout": "1x4", "topology": "neighbor", "resource": "R", "solution": "GROUP", "plot_order": 6},
}
for _policy_order, _policy_metadata in enumerate(POLICIES.values()):
    _policy_metadata.setdefault("architecture_family", "R3")
    _policy_metadata.setdefault("layout", "1x4" if _policy_metadata["topology"] in ("pair", "neighbor") else "2x2")
    _policy_metadata.setdefault("resource", "R")
    _policy_metadata.setdefault("plot_order", _policy_order)

# Legacy generic GROUP_GLOBAL is ingested when present for descriptive
# cross-contract analysis, but it is never mandatory for Matrix V2 gates.
OPTIONAL_LEGACY_POLICIES = {"directional_m1_group_global"}
SIXCASE_STATIC_CASE_POLICIES = ("g2x2_rc_early", "g2x2_rc_group", "g2x2_r_early", "g2x2_r_group", "l1x4_r_early", "l1x4_r_group")
SIXCASE_STATIC_POLICIES = ("local_no_sharing",) + SIXCASE_STATIC_CASE_POLICIES
LEGACY_POLICIES = tuple(policy for policy in POLICIES if policy not in set(SIXCASE_STATIC_POLICIES))
POLICY_SETS = {"legacy": LEGACY_POLICIES, "sixcase_static": SIXCASE_STATIC_POLICIES}

COMPARISONS = (
    ("local_vs_directional_local_first", "local_no_sharing", "directional_m1_local_first", "PAIRED_TOPOLOGY_COMPARISON"),
    ("directional_local_first_vs_early", "directional_m1_local_first", "directional_m1_early", "SAME_V2_UNIVERSE"),
    ("directional_early_vs_global", "directional_m1_early", "directional_m1_global", "SAME_V2_UNIVERSE"),
    ("edge_local_first_vs_early", "pairwise_row_m1_local_first", "pairwise_row_m1_early", "SAME_UNIVERSE"),
    ("edge_early_vs_global", "pairwise_row_m1_early", "pairwise_row_m1_global", "SAME_UNIVERSE"),
    ("early_vs_generic_global_legacy", "directional_m1_early", "directional_m1_group_global", "EMPIRICAL_CROSS_CONTRACT"),
    ("single_hop_early_vs_global", "single_hop_m1_early", "single_hop_m1_global", "SAME_UNIVERSE"),
    ("single_hop_local_first_vs_early", "single_hop_m1_local_first", "single_hop_m1_early", "SAME_UNIVERSE"),
    ("two_pairwise_local_first_vs_early", "two_pairwise_m1_local_first", "two_pairwise_m1_early", "SAME_UNIVERSE"),
    ("two_pairwise_early_vs_pair_global", "two_pairwise_m1_early", "two_pairwise_m1_pair_global", "SAME_UNIVERSE"),
    ("two_pairwise_vs_single_hop", "two_pairwise_m1_early", "single_hop_m1_early", "TOPOLOGY_COMPARISON"),
    ("g2x2_rc_group_vs_g2x2_r_group", "g2x2_rc_group", "g2x2_r_group", "RESOURCE_TYPE_EFFECT"),
    ("g2x2_rc_early_vs_g2x2_r_early", "g2x2_rc_early", "g2x2_r_early", "RESOURCE_TYPE_EFFECT"),
    ("g2x2_r_group_vs_l1x4_r_group", "g2x2_r_group", "l1x4_r_group", "TOPOLOGY_EFFECT"),
    ("g2x2_r_early_vs_l1x4_r_early", "g2x2_r_early", "l1x4_r_early", "TOPOLOGY_EFFECT"),
    ("g2x2_rc_early_vs_g2x2_rc_group", "g2x2_rc_early", "g2x2_rc_group", "SOLUTION_POLICY_EFFECT"),
    ("g2x2_r_early_vs_g2x2_r_group", "g2x2_r_early", "g2x2_r_group", "SOLUTION_POLICY_EFFECT"),
    ("l1x4_r_early_vs_l1x4_r_group", "l1x4_r_early", "l1x4_r_group", "SOLUTION_POLICY_EFFECT"),
)

FIGURE_TRACEABILITY = {
    "fig_r3_directional_repair_rate": {
        "source_csv": "data/repair_rate_summary.csv",
        "policies": ["local_no_sharing", "directional_m1_local_first", "directional_m1_early", "directional_m1_global"],
        "filters": {"metric": "repair_rate", "topology": ["none", "directional"]},
    },
    "fig_r3_topology_repair_rate": {
        "source_csv": "data/repair_rate_summary.csv",
        "policies": ["two_pairwise_m1_local_first", "two_pairwise_m1_early", "two_pairwise_m1_pair_global", "single_hop_m1_local_first", "single_hop_m1_early", "single_hop_m1_global"],
        "filters": {"metric": "repair_rate", "layout": "1x4"},
    },
    "fig_r3_directional_failure_rate": {
        "source_csv": "data/failure_rate_summary.csv",
        "policies": ["local_no_sharing", "directional_m1_local_first", "directional_m1_early", "directional_m1_global"],
        "filters": {"metric": "failure_rate", "topology": ["none", "directional"]},
    },
    "fig_r3_topology_failure_rate": {
        "source_csv": "data/failure_rate_summary.csv",
        "policies": ["two_pairwise_m1_local_first", "two_pairwise_m1_early", "two_pairwise_m1_pair_global", "single_hop_m1_local_first", "single_hop_m1_early", "single_hop_m1_global"],
        "filters": {"metric": "failure_rate", "layout": "1x4"},
    },
    "fig_r3_directional_fault_imbalance": {
        "source_csv": "data/imbalance_summary.csv",
        "policies": ["local_no_sharing", "directional_m1_local_first", "directional_m1_early", "directional_m1_global"],
        "filters": {"comparison": "directional", "imbalance": "rank_quintile"},
    },
    "fig_r3_topology_fault_imbalance": {
        "source_csv": "data/imbalance_summary.csv",
        "policies": ["two_pairwise_m1_local_first", "two_pairwise_m1_early", "two_pairwise_m1_pair_global", "single_hop_m1_local_first", "single_hop_m1_early", "single_hop_m1_global"],
        "filters": {"comparison": "1x4", "imbalance": "rank_quintile"},
    },
    "fig_r3_paired_outcomes": {
        "source_csv": "data/paired_outcomes.csv",
        "policies": [],
        "filters": {"metric": "left_fail_right_pass_rate"},
    },
    "fig_r3_repair_rate_gain": {
        "source_csv": "data/repair_rate_summary.csv",
        "policies": ["directional_m1_local_first", "directional_m1_early", "directional_m1_global"],
        "filters": {"metric": "gain_vs_local_percentage_points", "comparison": "directional_v2"},
    },
    "fig_r3_search_complexity": {
        "source_csv": "data/search_complexity.csv",
        "policies": ["directional_m1_global", "directional_m1_group_global", "single_hop_m1_global"],
        "filters": {"metric": "search_nodes", "metric_status": "SUPPORTED"},
    },
}

CORPUS_FIELDS = {"corpus_id", "seed", "Rs", "Cs", "group_fault_count", "group_id", "sa_local_faults"}
RESULT_FIELDS = {"corpus_id", "group_id", "repair_success", "total_physical_group", "global_search_nodes_visited", "global_complete_assignments_checked", "global_candidate_count_A", "global_candidate_count_B", "global_candidate_count_C", "global_candidate_count_D"}


@dataclass
class PointData:
    name: str
    rs: int
    cs: int
    fault_count: int
    seed: int
    corpus_id: str
    faults: dict[int, tuple[int, int, int, int]]
    results: dict[str, list[dict[str, str]]] = field(default_factory=dict)
    source_sidecars: dict[str, str] = field(default_factory=dict)

    @property
    def groups(self) -> int:
        return len(self.faults)


def read_csv(path: Path) -> tuple[list[dict[str, str]], set[str]]:
    with path.open(newline="", encoding="utf-8") as source:
        reader = csv.DictReader(source)
        fields = set(reader.fieldnames or [])
        return list(reader), fields


def write_csv(path: Path, rows: list[dict[str, Any]], fields: Iterable[str]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", newline="", encoding="utf-8") as target:
        writer = csv.DictWriter(target, fieldnames=list(fields), extrasaction="raise")
        writer.writeheader()
        writer.writerows(rows)


def parse_sa_fault_counts(value: str) -> tuple[int, int, int, int]:
    buckets = value.split("|")
    if len(buckets) != 4:
        raise ValueError("expected four subarray fault buckets")
    return tuple(0 if not bucket else len(bucket.split(";")) for bucket in buckets)  # type: ignore[return-value]


def wilson(successes: int, groups: int) -> tuple[float, float]:
    if groups <= 0:
        return math.nan, math.nan
    z = 1.959963984540054
    rate = successes / groups
    denominator = 1.0 + z * z / groups
    center = (rate + z * z / (2.0 * groups)) / denominator
    margin = z * math.sqrt(rate * (1.0 - rate) / groups + z * z / (4.0 * groups * groups)) / denominator
    return max(0.0, center - margin), min(1.0, center + margin)


def percentile(values: list[float], probability: float) -> float:
    if not values:
        return math.nan
    ordered = sorted(values)
    index = (len(ordered) - 1) * probability
    lower, upper = math.floor(index), math.ceil(index)
    if lower == upper:
        return ordered[lower]
    return ordered[lower] + (ordered[upper] - ordered[lower]) * (index - lower)


def point_sort_key(point: PointData) -> tuple[int, int]:
    return point.rs, point.fault_count


def _quality(row: list[dict[str, Any]], point: str, policy: str, status: str, detail: str) -> None:
    row.append({"point": point, "policy": policy, "status": status, "detail": detail})


def policy_ids(policy_set: str) -> tuple[str, ...]:
    try:
        return POLICY_SETS[policy_set]
    except KeyError as error:
        raise ValueError(f"unknown policy set: {policy_set}") from error


def load_dataset(input_root: Path, policy_set: str = "legacy") -> tuple[list[PointData], list[dict[str, Any]], dict[str, Any]]:
    input_root = input_root.resolve()
    corpus_root = input_root / "corpus"
    raw_root = input_root / "raw"
    quality: list[dict[str, Any]] = []
    points: list[PointData] = []
    if not corpus_root.is_dir():
        raise RuntimeError(f"missing authoritative corpus directory: {corpus_root}")
    for corpus_dir in sorted(path for path in corpus_root.iterdir() if path.is_dir()):
        match = POINT_RE.fullmatch(corpus_dir.name)
        if match is None:
            _quality(quality, corpus_dir.name, "*", "WARNING", "ignored corpus directory: point name is not n<RS>_f<F_GROUP>")
            continue
        corpus_path = corpus_dir / "paired_corpus_v1.csv"
        if not corpus_path.is_file():
            _quality(quality, corpus_dir.name, "*", "ERROR", "missing paired_corpus_v1.csv")
            continue
        expected_rs, expected_f = int(match["rs"]), int(match["faults"])
        try:
            rows, fields = read_csv(corpus_path)
            if not CORPUS_FIELDS <= fields:
                raise ValueError(f"missing corpus fields: {sorted(CORPUS_FIELDS - fields)}")
            if not rows:
                raise ValueError("empty corpus")
            corpus_id = rows[0]["corpus_id"]
            seed = int(rows[0]["seed"])
            faults: dict[int, tuple[int, int, int, int]] = {}
            rs = cs = expected_rs
            for index, item in enumerate(rows):
                group_id = int(item["group_id"])
                rs, cs = int(item["Rs"]), int(item["Cs"])
                counts = parse_sa_fault_counts(item["sa_local_faults"])
                if group_id != index or group_id in faults:
                    raise ValueError("noncontiguous or duplicate corpus group_id")
                if rs != expected_rs or cs != expected_rs:
                    raise ValueError("point directory and corpus RS/CS differ")
                if int(item["group_fault_count"]) != expected_f or sum(counts) != expected_f:
                    raise ValueError("fixed-total corpus contract failed")
                if item["corpus_id"] != corpus_id or int(item["seed"]) != seed:
                    raise ValueError("corpus ID or seed changes within point")
                faults[group_id] = counts
            point = PointData(corpus_dir.name, rs, cs, expected_f, seed, corpus_id, faults)
            _quality(quality, point.name, "*", "PASS", "corpus schema, fixed-total, seed, and contiguous group IDs verified")
        except (OSError, ValueError, KeyError, csv.Error) as error:
            _quality(quality, corpus_dir.name, "*", "ERROR", f"invalid corpus: {error}")
            continue
        for policy in policy_ids(policy_set):
            result_path = raw_root / point.name / policy / "paired_policy_results_v1.csv"
            if not result_path.is_file():
                if policy not in OPTIONAL_LEGACY_POLICIES:
                    _quality(quality, point.name, policy, "MISSING", "missing policy sidecar")
                continue
            try:
                rows, fields = read_csv(result_path)
                if not RESULT_FIELDS <= fields:
                    raise ValueError(f"missing result fields: {sorted(RESULT_FIELDS - fields)}")
                if len(rows) != point.groups:
                    raise ValueError(f"result rows={len(rows)}, corpus groups={point.groups}")
                for index, item in enumerate(rows):
                    if int(item["group_id"]) != index:
                        raise ValueError("noncontiguous or duplicate result group_id")
                    if item["corpus_id"] != point.corpus_id:
                        raise ValueError("result sidecar does not replay point corpus")
                    if int(item["repair_success"]) not in (0, 1):
                        raise ValueError("repair_success is not binary")
                    if int(item["total_physical_group"]) != 8 * point.rs:
                        raise ValueError("physical spare budget mismatch")
                point.results[policy] = rows
                point.source_sidecars[policy] = str(result_path)
                _quality(quality, point.name, policy, "PASS", "schema, same-corpus replay, group IDs, and physical budget verified")
            except (OSError, ValueError, KeyError, csv.Error) as error:
                _quality(quality, point.name, policy, "ERROR", f"invalid policy sidecar: {error}")
        points.append(point)

    manifest_path = input_root / "manifest" / "r3_manifest.json"
    manifest: dict[str, Any] = {}
    if manifest_path.is_file():
        try:
            manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
        except json.JSONDecodeError:
            _quality(quality, "*", "*", "WARNING", "root manifest is not valid JSON; ignored as provenance")
    discovered = {(point.rs, point.cs, point.fault_count) for point in points}
    manifest_points = {(int(rs), int(rs), int(fault))
                       for rs, values in manifest.get("points", {}).items()
                       for fault in values}
    if manifest and manifest_points and manifest_points != discovered:
        _quality(quality, "*", "*", "WARNING", "root manifest point inventory differs from discovered corpus inventory; raw/corpus retained as source of truth")
    groups = sorted({point.groups for point in points})
    if manifest.get("formal") is False:
        dataset_class = "QUICK_SWEEP / DEVELOPMENT / NON-FORMAL"
    elif manifest.get("formal") is True:
        dataset_class = "FORMAL_CANDIDATE / MANIFEST_PROVENANCE"
    else:
        dataset_class = "UNCLASSIFIED / MANIFEST_UNAVAILABLE"
    metadata = {
        "input_root": str(input_root),
        "dataset_class": dataset_class,
        "observed_groups_per_policy_point": groups,
        "manifest_present": bool(manifest),
        "manifest_formal": manifest.get("formal"),
        "discovered_points": len(points),
        "policy_set": policy_set,
    }
    return sorted(points, key=point_sort_key), quality, metadata


def inventory_rows(points: list[PointData], metadata: dict[str, Any]) -> list[dict[str, Any]]:
    rows = []
    for point in points:
        required = [policy for policy in policy_ids(str(metadata["policy_set"])) if policy not in OPTIONAL_LEGACY_POLICIES]
        complete = sum(policy in point.results for policy in required)
        rows.append({
            "dataset_class": metadata["dataset_class"], "point": point.name,
            "RS": point.rs, "CS": point.cs, "F_GROUP": point.fault_count,
            "seed": point.seed, "corpus_id": point.corpus_id, "groups": point.groups,
            "complete_policies": complete, "expected_policies": len(required),
            "point_status": "COMPLETE" if complete == len(required) else "INCOMPLETE",
        })
    return rows


def ensure_output_layout(output_root: Path) -> None:
    for name in ("data", "tables", "figures"):
        (output_root / name).mkdir(parents=True, exist_ok=True)


def write_analysis_manifest(output_root: Path, metadata: dict[str, Any], points: list[PointData], quality: list[dict[str, Any]]) -> None:
    ensure_output_layout(output_root)
    payload = {
        "schema_version": "r3_group_analysis_v1",
        **metadata,
        "source_of_truth": "corpus/<point>/paired_corpus_v1.csv + raw/<point>/<policy>/paired_policy_results_v1.csv",
        "root_manifest_role": "provenance_only",
        "policy_legend_mapping": {policy: value["label"] for policy, value in POLICIES.items()},
        "point_inventory": inventory_rows(points, metadata),
        "quality_rows": len(quality),
        "figure_traceability": FIGURE_TRACEABILITY,
    }
    (output_root / "analysis_manifest.json").write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")


def write_gate_artifacts(output_root: Path, points: list[PointData], quality: list[dict[str, Any]], metadata: dict[str, Any]) -> None:
    write_csv(output_root / "data" / "source_point_inventory.csv", inventory_rows(points, metadata),
              ["dataset_class", "point", "RS", "CS", "F_GROUP", "seed", "corpus_id", "groups", "complete_policies", "expected_policies", "point_status"])
    write_csv(output_root / "data" / "data_quality_report.csv", quality,
              ["point", "policy", "status", "detail"])
    write_analysis_manifest(output_root, metadata, points, quality)


def numeric(values: Iterable[str]) -> list[float]:
    result = []
    for value in values:
        if value not in ("", "-", "NA", "nan"):
            result.append(float(value))
    return result


def average(values: Iterable[str]) -> float:
    data = numeric(values)
    return mean(data) if data else math.nan
