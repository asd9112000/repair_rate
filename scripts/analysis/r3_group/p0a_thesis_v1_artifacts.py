#!/usr/bin/env python3
"""Create read-only Thesis-V1 P0-A tables and figures from frozen R3 aggregates.

This deliberately consumes only ``aggregate/r3_repair_rate_summary.csv``.  It
never reads, writes, or regenerates a raw policy sidecar or a fault corpus.
"""

from __future__ import annotations

import argparse
import csv
import json
import math
import os
import sys
from pathlib import Path

os.environ.setdefault("MPLCONFIGDIR", "/tmp/p0a-thesis-matplotlib")
import matplotlib.pyplot as plt
from matplotlib.colors import Normalize

ROOT = Path(__file__).resolve().parents[3]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))
from scripts.paper_style import apply_paper_style, save_paper_figure, style_axis


POLICY_LADDERS = {
    "directional": {
        "label": "Directional m1", "layout": "2x2",
        "local_first": "directional_m1_local_first",
        "early": "directional_m1_early", "global": "directional_m1_global",
        "global_class": "GLOBAL",
    },
    "pairwise_row": {
        "label": "Pairwise-row m1", "layout": "2x2",
        "local_first": "pairwise_row_m1_local_first",
        "early": "pairwise_row_m1_early", "global": "pairwise_row_m1_global",
        "global_class": "GLOBAL",
    },
    "single_hop": {
        "label": "Single-hop m1", "layout": "1x4",
        "local_first": "single_hop_m1_local_first",
        "early": "single_hop_m1_early", "global": "single_hop_m1_global",
        "global_class": "GLOBAL",
    },
    "two_pairwise": {
        "label": "Two-pairwise m1", "layout": "1x4",
        "local_first": "two_pairwise_m1_local_first",
        "early": "two_pairwise_m1_early", "global": "two_pairwise_m1_pair_global",
        "global_class": "PAIR_GLOBAL",
    },
}
COLORS = {"LOCAL": "#333333", "LOCAL_FIRST": "#0072B2", "EARLY": "#009E73",
          "GLOBAL": "#D55E00", "PAIR_GLOBAL": "#CC79A7"}
MARKERS = {"LOCAL": "o", "LOCAL_FIRST": "s", "EARLY": "D", "GLOBAL": "X",
           "PAIR_GLOBAL": "P"}


def arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--input-root", type=Path, required=True)
    parser.add_argument("--output-root", type=Path, required=True)
    return parser.parse_args()


def read_summary(path: Path) -> dict[tuple[int, int, str], dict[str, str]]:
    with path.open(newline="", encoding="utf-8") as source:
        rows = list(csv.DictReader(source))
    required = {"RS", "CS", "F_GROUP", "policy", "groups", "repair_rate"}
    if not rows or not required <= set(rows[0]):
        raise ValueError(f"summary missing required columns: {path}")
    result: dict[tuple[int, int, str], dict[str, str]] = {}
    for row in rows:
        if row["RS"] != row["CS"]:
            raise ValueError("P0-A quick dataset is expected to have RS=CS points")
        key = (int(row["RS"]), int(row["F_GROUP"]), row["policy"])
        if key in result:
            raise ValueError(f"duplicate summary key: {key}")
        result[key] = row
    return result


def write_csv(path: Path, rows: list[dict[str, object]], fields: list[str]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", newline="", encoding="utf-8") as target:
        writer = csv.DictWriter(target, fieldnames=fields, extrasaction="raise")
        writer.writeheader()
        writer.writerows(rows)


def derive_ladder_table(summary: dict[tuple[int, int, str], dict[str, str]]) -> list[dict[str, object]]:
    rows: list[dict[str, object]] = []
    for topology, spec in POLICY_LADDERS.items():
        # A topology is reported only at its frozen supported N/F points.  In
        # particular directional V2 was run at N=2,3, while the other P0-A
        # topology ladders also have N=4 results.
        points = sorted({key[:2] for key in summary
                         if key[2] == "local_no_sharing" and
                         all((key[0], key[1], policy) in summary for policy in
                             (spec["local_first"], spec["early"], spec["global"]))})
        for n, fault_count in points:
            policies = ("local_no_sharing", spec["local_first"], spec["early"], spec["global"])
            missing = [policy for policy in policies if (n, fault_count, policy) not in summary]
            if missing:
                raise ValueError(f"missing canonical policy at N={n}, F={fault_count}: {missing}")
            selected = [summary[(n, fault_count, policy)] for policy in policies]
            groups = {row["groups"] for row in selected}
            if len(groups) != 1:
                raise ValueError(f"same-corpus group count mismatch at N={n}, F={fault_count}")
            local, local_first, early, global_ = (float(row["repair_rate"]) for row in selected)
            rows.append({
                "topology_id": topology, "topology": spec["label"], "layout": spec["layout"],
                "N": n, "RS": n, "CS": n, "F_GROUP": fault_count, "groups": int(next(iter(groups))),
                "canonical_local_id": "local_no_sharing",
                "canonical_local_first_id": spec["local_first"],
                "canonical_early_id": spec["early"], "canonical_global_id": spec["global"],
                "global_solution_class": spec["global_class"],
                "LOCAL_repair_rate_percent": 100.0 * local,
                "LOCAL_FIRST_repair_rate_percent": 100.0 * local_first,
                "EARLY_repair_rate_percent": 100.0 * early,
                "GLOBAL_repair_rate_percent": 100.0 * global_,
                "DELTA_PRIORITY_pp": 100.0 * (early - local_first),
                "DELTA_SEARCH_pp": 100.0 * (global_ - early),
                "DELTA_TOTAL_pp": 100.0 * (global_ - local_first),
            })
    return rows


def select(rows: list[dict[str, object]], topology: str, n: int) -> list[dict[str, object]]:
    return sorted((row for row in rows if row["topology_id"] == topology and row["N"] == n),
                  key=lambda row: int(row["F_GROUP"]))


def save_all(fig, output: Path) -> None:
    output.parent.mkdir(parents=True, exist_ok=True)
    for suffix in (".png", ".pdf", ".svg"):
        save_paper_figure(fig, output.with_suffix(suffix), tight=False, close=False)
    plt.close(fig)


def plot_directional_ladder(rows: list[dict[str, object]], output: Path) -> None:
    apply_paper_style()
    supported_n = sorted({int(row["N"]) for row in rows if row["topology_id"] == "directional"})
    fig, axes = plt.subplots(1, len(supported_n), figsize=(7.10, 2.55), sharey=True)
    for axis, n in zip(axes, supported_n):
        selected = select(rows, "directional", n)
        x = [int(row["F_GROUP"]) for row in selected]
        for label, field in (("LOCAL", "LOCAL_repair_rate_percent"),
                             ("LOCAL_FIRST", "LOCAL_FIRST_repair_rate_percent"),
                             ("EARLY", "EARLY_repair_rate_percent"),
                             ("GLOBAL", "GLOBAL_repair_rate_percent")):
            axis.plot(x, [float(row[field]) for row in selected], label=label,
                      color=COLORS[label], marker=MARKERS[label])
        style_axis(axis, grid=True, grid_axis="y")
        axis.set_title(f"$R_s=C_s={n}$")
        axis.set_xlabel("$F_{GROUP}$")
        axis.set_xticks(x)
        axis.set_xlim(min(x) - 1, max(x) + 1)
    axes[0].set_ylabel("Repair rate (%)")
    handles, labels = axes[0].get_legend_handles_labels()
    fig.legend(handles, labels, loc="lower center", ncol=4, bbox_to_anchor=(0.5, -0.03))
    fig.subplots_adjust(left=0.09, right=0.99, bottom=0.26, top=0.88, wspace=0.13)
    save_all(fig, output)


def plot_topology_early(rows: list[dict[str, object]], output: Path) -> None:
    apply_paper_style()
    fig, axes = plt.subplots(1, 3, figsize=(7.10, 2.55), sharey=True)
    topology_lines = (
        ("directional", "2x2: Directional", "#0072B2", "D"),
        ("pairwise_row", "2x2: Pairwise-row", "#D55E00", "s"),
        ("single_hop", "1x4: Single-hop", "#009E73", "^") ,
        ("two_pairwise", "1x4: Two-pairwise", "#CC79A7", "v"),
    )
    for axis, n in zip(axes, (2, 3, 4)):
        for topology, label, color, marker in topology_lines:
            selected = select(rows, topology, n)
            if not selected:
                continue
            axis.plot([int(item["F_GROUP"]) for item in selected],
                      [float(item["EARLY_repair_rate_percent"]) for item in selected],
                      label=label, color=color, marker=marker)
        style_axis(axis, grid=True, grid_axis="y")
        axis.set_title(f"$R_s=C_s={n}$")
        axis.set_xlabel("$F_{GROUP}$")
        axis.set_xticks([8, 12, 16, 20, 24, 28, 32])
        axis.set_xlim(7, 33)
    axes[0].set_ylabel("EARLY repair rate (%)")
    handles, labels = axes[0].get_legend_handles_labels()
    fig.legend(handles, labels, loc="lower center", ncol=2,
               bbox_to_anchor=(0.5, -0.03))
    fig.text(0.5, 0.995, "Within-layout EARLY comparison; no cross-layout ranking", ha="center", va="top")
    fig.subplots_adjust(left=0.09, right=0.99, bottom=0.26, top=0.86, wspace=0.13)
    save_all(fig, output)


def plot_gain_decomposition(rows: list[dict[str, object]], output: Path) -> None:
    apply_paper_style()
    topology_order = ("directional", "pairwise_row", "single_hop", "two_pairwise")
    labels = [POLICY_LADDERS[key]["label"] for key in topology_order]
    faults = [8, 12, 16, 20, 24, 28, 32]
    values = {metric: [] for metric in ("DELTA_PRIORITY_pp", "DELTA_SEARCH_pp")}
    for metric in values:
        for n in (2, 3, 4):
            matrix = []
            for topology in topology_order:
                indexed = {int(row["F_GROUP"]): float(row[metric]) for row in select(rows, topology, n)}
                matrix.append([indexed.get(fault, math.nan) for fault in faults])
            values[metric].append(matrix)
    maxima = {metric: max(value for matrices in (values[metric],) for matrix in matrices
                         for line in matrix for value in line if math.isfinite(value))
              for metric in values}
    fig, axes = plt.subplots(2, 3, figsize=(7.10, 4.25), constrained_layout=False)
    images = []
    for metric_index, metric in enumerate(("DELTA_PRIORITY_pp", "DELTA_SEARCH_pp")):
        for column, n in enumerate((2, 3, 4)):
            axis = axes[metric_index][column]
            image = axis.imshow(values[metric][column], aspect="auto", cmap="YlGnBu",
                                norm=Normalize(vmin=0.0, vmax=maxima[metric]))
            images.append(image)
            for row_index, line in enumerate(values[metric][column]):
                for col_index, value in enumerate(line):
                    axis.text(col_index, row_index, "n/a" if not math.isfinite(value) else f"{value:.1f}",
                              ha="center", va="center", fontsize=6)
            if metric_index == 0:
                axis.set_title(f"$R_s=C_s={n}$")
            if column == 0:
                axis.set_yticks(range(len(labels)), labels)
            else:
                axis.set_yticks(range(len(labels)), [])
            if metric_index == 1:
                axis.set_xticks(range(len(faults)), faults)
                axis.set_xlabel("$F_{GROUP}$")
            else:
                axis.set_xticks(range(len(faults)), [])
            for spine in axis.spines.values():
                spine.set_linewidth(0.7)
        # Each cell is annotated in pp; omitting a colorbar keeps the two
        # independent scales from intruding into the N=4 panel.
        axes[metric_index][0].set_ylabel("$\\Delta$ priority (pp)" if metric_index == 0
                                         else "$\\Delta$ search (pp)")
    fig.text(0.5, 0.99, "Gain decomposition (GLOBAL means PAIR_GLOBAL for two-pairwise)", ha="center", va="top")
    fig.subplots_adjust(left=0.15, right=0.95, bottom=0.12, top=0.91, wspace=0.18, hspace=0.25)
    save_all(fig, output)


def witness_rows() -> list[dict[str, object]]:
    base = "tmp/r3_normalized_1k_witness_replay_seeded_20260918"
    return [
        {"transition": "LOCAL_FIRST->EARLY", "topology_id": "directional", "N": 2, "F_GROUP": 12, "group_id": 738, "faults_A_B_C_D": "2/1/6/3", "thesis_mechanism": "release-aware A choice preserves a legal C configuration", "trace_root": base},
        {"transition": "LOCAL_FIRST->EARLY", "topology_id": "pairwise_row", "N": 2, "F_GROUP": 16, "group_id": 475, "faults_A_B_C_D": "2/3/3/8", "thesis_mechanism": "EARLY B choice preserves one row for D", "trace_root": base},
        {"transition": "LOCAL_FIRST->EARLY", "topology_id": "single_hop", "N": 2, "F_GROUP": 12, "group_id": 718, "faults_A_B_C_D": "2/2/8/0", "thesis_mechanism": "release-aware ranking retains row capacity for C", "trace_root": base},
        {"transition": "LOCAL_FIRST->EARLY", "topology_id": "two_pairwise", "N": 2, "F_GROUP": 12, "group_id": 49, "faults_A_B_C_D": "3/7/0/2", "thesis_mechanism": "EARLY A choice makes a later B candidate legal", "trace_root": base},
        {"transition": "EARLY->GLOBAL", "topology_id": "directional", "N": 2, "F_GROUP": 16, "group_id": 172, "faults_A_B_C_D": "5/0/7/4", "thesis_mechanism": "joint V2 tuple avoids irreversible sequential A commitment", "trace_root": base},
        {"transition": "EARLY->GLOBAL", "topology_id": "pairwise_row", "N": 2, "F_GROUP": 12, "group_id": 738, "faults_A_B_C_D": "2/1/6/3", "thesis_mechanism": "joint generic search finds a complete legal tuple", "trace_root": base},
    ]


def main() -> int:
    args = arguments()
    input_root, output_root = args.input_root.resolve(), args.output_root.resolve()
    summary = read_summary(input_root / "aggregate" / "r3_repair_rate_summary.csv")
    rows = derive_ladder_table(summary)
    table_fields = list(rows[0])
    write_csv(output_root / "tables" / "p0a_group_policy_ladder.csv", rows, table_fields)
    baseline_rows = []
    for (n, fault_count, policy), item in sorted(summary.items()):
        if policy != "local_no_sharing":
            continue
        baseline_rows.append({"N": n, "RS": n, "CS": n, "F_GROUP": fault_count,
                              "groups": int(item["groups"]),
                              "canonical_policy_id": "local_no_sharing",
                              "solution_class": "LOCAL",
                              "repair_rate_percent": 100.0 * float(item["repair_rate"])})
    write_csv(output_root / "tables" / "p0a_local_no_sharing.csv", baseline_rows,
              list(baseline_rows[0]))
    for topology in POLICY_LADDERS:
        subset = [row for row in rows if row["topology_id"] == topology]
        write_csv(output_root / "tables" / f"p0a_{topology}_policy_ladder.csv", subset, table_fields)
    witnesses = witness_rows()
    write_csv(output_root / "tables" / "p0a_canonical_witness_summary.csv", witnesses, list(witnesses[0]))
    maxima = {}
    for topology in POLICY_LADDERS:
        subset = [row for row in rows if row["topology_id"] == topology]
        maxima[topology] = {metric: max(float(row[metric]) for row in subset)
                             for metric in ("DELTA_PRIORITY_pp", "DELTA_SEARCH_pp", "DELTA_TOTAL_pp")}
    provenance = {
        "input": str(input_root / "aggregate" / "r3_repair_rate_summary.csv"),
        "input_rows": len(summary), "ladder_rows": len(rows),
        "baseline_rows": len(baseline_rows), "witness_rows": len(witnesses),
        "raw_sidecars_written": False, "sweep_rerun": False, "maxima_pp": maxima,
        "two_pairwise_reference_class": "PAIR_GLOBAL",
        "scope": "normalized 1k quick-sweep observations only",
    }
    (output_root / "manifest").mkdir(parents=True, exist_ok=True)
    (output_root / "manifest" / "p0a_artifact_manifest.json").write_text(
        json.dumps(provenance, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    figure_root = output_root / "figures"
    plot_directional_ladder(rows, figure_root / "figure_a_directional_policy_ladder")
    plot_topology_early(rows, figure_root / "figure_b_within_layout_early_topology_comparison")
    plot_gain_decomposition(rows, figure_root / "figure_c_policy_gain_decomposition")
    print(json.dumps(provenance, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
