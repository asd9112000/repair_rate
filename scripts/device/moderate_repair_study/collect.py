#!/usr/bin/env python3
"""Merge scope-separated moderate-study outputs and enforce paired invariants."""

from __future__ import annotations

import argparse
import csv
import json
from collections import defaultdict
from pathlib import Path


META_FIELDS = [
    "study_kind", "point_id", "policy_id", "sweep_rs", "sweep_cs",
    "sweep_fault_count", "sweep_seed", "sweep_runs", "sweep_groups",
    "sweep_matrix_policy", "sweep_sram_policy", "spare_lines_per_group",
    "spare_cell_equivalent_per_group", "spare_cell_equivalent_modeled",
    "spare_cell_equivalent_reference_device",
]


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("run_root", type=Path)
    parser.add_argument("--output-dir", type=Path)
    return parser.parse_args()


def read_json(path: Path) -> dict:
    with path.open(encoding="utf-8") as source:
        return json.load(source)


def read_csv(path: Path) -> list[dict[str, str]]:
    with path.open(newline="", encoding="utf-8") as source:
        return [dict(row) for row in csv.DictReader(source)]


def cost_metadata(meta: dict, rows: int, columns: int) -> dict[str, str]:
    rs, cs = int(meta["rs"]), int(meta["cs"])
    per_group = 4 * (rs * columns + cs * rows)
    modeled_groups = int(meta.get("groups", 1))
    return {
        "study_kind": meta["kind"], "point_id": meta["point_id"],
        "policy_id": meta["policy_id"], "sweep_rs": str(rs),
        "sweep_cs": str(cs), "sweep_fault_count": str(meta["fault_count"]),
        "sweep_seed": str(meta["seed"]),
        "sweep_runs": str(meta.get("runs", "")),
        "sweep_groups": str(meta.get("groups", "")),
        "sweep_matrix_policy": str(meta.get("matrix_policy", "")),
        "sweep_sram_policy": str(meta.get("sram_policy", "")),
        "spare_lines_per_group": str(4 * (rs + cs)),
        "spare_cell_equivalent_per_group": str(per_group),
        "spare_cell_equivalent_modeled": str(per_group * modeled_groups),
        "spare_cell_equivalent_reference_device": str(
            8192 * (rs * columns + cs * rows)),
    }


def write_csv(path: Path, rows: list[dict[str, str]]) -> None:
    if not rows:
        raise RuntimeError(f"no rows collected for {path.name}")
    extras = sorted({key for row in rows for key in row}.difference(META_FIELDS))
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", newline="", encoding="utf-8") as output:
        writer = csv.DictWriter(output, fieldnames=META_FIELDS + extras,
                                extrasaction="ignore")
        writer.writeheader()
        writer.writerows(rows)


def validate_corpus(path: Path, expected_groups: int, faults_per_group: int) -> None:
    group_counts: dict[tuple[str, str, str, str], int] = defaultdict(int)
    header = []
    with path.open(encoding="utf-8") as source:
        for line in source:
            if line.startswith("#"):
                header.append(line.strip())
                continue
            fields = line.split()
            if fields:
                if len(fields) != 7:
                    raise RuntimeError(f"invalid fault corpus row in {path}: {line}")
                group_counts[tuple(fields[:4])] += 1
    if len(group_counts) != expected_groups or set(group_counts.values()) != {faults_per_group}:
        raise RuntimeError(f"fault corpus group/count mismatch: {path}")
    if "# fault_model=moderate_imbalance" not in header or \
       "# spatial_model=mixed" not in header:
        raise RuntimeError(f"fault corpus metadata mismatch: {path}")


def validate_canonical(rows: list[dict[str, str]], point_id: str) -> None:
    by_id = {row["configuration_id"]: row for row in rows}
    pairs = (("B0_CAM_NO_SHARING", "B2_SRAM_NO_SHARING"),
             ("B1_CAM_SHARING", "B3_SRAM_SHARING"))
    fields = (
        "repairable_groups", "repair_rate", "device_success",
        "global_reuse_entries_reserved", "global_reuse_overflow_count",
        "local_only_groups", "sharing_required_groups",
        "reuse_required_groups", "sharing_and_reuse_required_groups",
        "unrepairable_groups",
    )
    for left, right in pairs:
        for field in fields:
            if by_id[left][field] != by_id[right][field]:
                raise RuntimeError(
                    f"CAM/SRAM functional mismatch at {point_id}: "
                    f"{left}.{field}={by_id[left][field]} != "
                    f"{right}.{field}={by_id[right][field]}")


def main() -> int:
    args = parse_args()
    run_root = args.run_root.resolve()
    config = read_json(run_root / "run_config.json")
    rows_count = int(config["memory_rows"])
    columns_count = int(config["memory_columns"])
    collected: dict[str, list[dict[str, str]]] = defaultdict(list)
    device_group_rows: list[dict[str, str]] = []

    for metadata_path in sorted(run_root.glob("*/*/point.json")):
        meta = read_json(metadata_path)
        point_dir = metadata_path.parent
        prefix = cost_metadata(meta, rows_count, columns_count)
        if meta["kind"] == "group":
            rows = read_csv(point_dir / "summary.csv")
            if len(rows) != 1:
                raise RuntimeError(f"expected one group summary row: {point_dir}")
            if rows[0]["fault_model"] != "moderate_imbalance" or \
               rows[0]["fault_spatial_model"] != "mixed":
                raise RuntimeError(f"group fault-model mismatch: {point_dir}")
            collected["group"].append({**prefix, **rows[0]})
        elif meta["kind"] == "sram":
            for row in read_csv(point_dir / "sram_recam_metrics.csv"):
                if row["repair_result_matches_cam_baseline"] != "1":
                    raise RuntimeError(f"SRAM functional mismatch: {point_dir}")
                collected["sram"].append({**prefix, **row})
        elif meta["kind"] == "device":
            rows = read_csv(point_dir / "canonical_four_summary.csv")
            if len(rows) != 4:
                raise RuntimeError(f"expected B0-B3 rows: {point_dir}")
            validate_canonical(rows, meta["point_id"])
            validate_corpus(point_dir / "fault_corpus.txt",
                            int(meta["groups"]), int(meta["fault_count"]))
            collected["device"].extend({**prefix, **row} for row in rows)
            device_group_rows.extend(
                {**prefix, **row} for row in
                read_csv(point_dir / "canonical_four_groups.csv"))
        else:
            raise RuntimeError(f"unknown study kind: {meta['kind']}")

    output_dir = (args.output_dir or run_root / "combined").resolve()
    write_csv(output_dir / "group_summary.csv", collected["group"])
    write_csv(output_dir / "sram_summary.csv", collected["sram"])
    write_csv(output_dir / "device_summary.csv", collected["device"])
    write_csv(output_dir / "device_groups.csv", device_group_rows)
    validation = {
        "status": "passed",
        "group_rows": len(collected["group"]),
        "sram_rows": len(collected["sram"]),
        "device_rows": len(collected["device"]),
        "device_group_rows": len(device_group_rows),
        "checks": [
            "moderate/mixed metadata",
            "fault corpus group and fault counts",
            "B0=B2 and B1=B3 functional outcomes",
            "SRAM group results match CAM baseline",
        ],
    }
    (output_dir / "validation.json").write_text(
        json.dumps(validation, indent=2) + "\n", encoding="utf-8")
    print(f"Collected and validated results in {output_dir}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
