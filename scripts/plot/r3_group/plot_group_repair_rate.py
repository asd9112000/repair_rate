#!/usr/bin/env python3
"""Plot R3 repair/failure rates from derived analysis CSVs only."""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))
from scripts.plot.r3_group.common import (STYLE, add_legend, make_single_panel_figure,
                                          make_two_panel_figure, read_csv,
                                          reserve_dense_legend_space,
                                          save_three_formats, style_panels,
                                          style_single_panel)

BASELINE_RECAM = ("local_no_sharing",)
DIRECTIONAL_V2 = ("directional_m1_local_first", "directional_m1_early",
                  "directional_m1_global")
TOPOLOGY_2_2 = DIRECTIONAL_V2 + ("pairwise_row_m1_local_first", "pairwise_row_m1_early", "pairwise_row_m1_global")
TOPOLOGY_1_4 = ("two_pairwise_m1_local_first", "two_pairwise_m1_early", "two_pairwise_m1_pair_global", "single_hop_m1_local_first", "single_hop_m1_early", "single_hop_m1_global")
POLICY_EARLY = ("directional_m1_early", "two_pairwise_m1_early", "single_hop_m1_early")
POLICY_GLOBAL = ("directional_m1_global", "two_pairwise_m1_pair_global", "single_hop_m1_global")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--analysis-root", required=True, type=Path)
    parser.add_argument("--show-ci", action="store_true")
    return parser.parse_args()


def draw_curves(axis, rows, policies, metric, rs, show_ci):
    for policy in policies:
        selected = sorted((row for row in rows if int(row["RS"]) == rs and row["policy"] == policy),
                          key=lambda row: int(row["F_GROUP"]))
        if not selected:
            continue
        label, color, marker, line = STYLE[policy]
        x = [int(row["F_GROUP"]) for row in selected]
        y = [100.0 * float(row[metric]) for row in selected]
        axis.plot(x, y, label=label, color=color, marker=marker, linestyle=line)
        if show_ci and metric == "repair_rate":
            low = [100.0 * float(row["wilson95_low"]) for row in selected]
            high = [100.0 * float(row["wilson95_high"]) for row in selected]
            axis.fill_between(x, low, high, color=color, alpha=0.13)


def draw(rows, policies, metric, ylabel, output, show_ci, comparison):
    fig, axes = make_two_panel_figure()
    if comparison == "all":
        reserve_dense_legend_space(fig)
    for axis, rs in zip(axes, (2, 3)):
        draw_curves(axis, rows, policies, metric, rs, show_ci)
    style_panels(axes, ylabel)
    add_legend(fig, axes[0], dense=(comparison == "all"))
    save_three_formats(fig, output.parent, output.name)
    for rs in (2, 3):
        panel_fig, panel_axis = make_single_panel_figure()
        if comparison == "all":
            reserve_dense_legend_space(panel_fig)
        draw_curves(panel_axis, rows, policies, metric, rs, show_ci)
        style_single_panel(panel_axis, ylabel)
        add_legend(panel_fig, panel_axis, dense=(comparison == "all"))
        save_three_formats(panel_fig, output.parent, f"{output.name}_rs{rs}")


def main() -> int:
    args = parse_args()
    root = args.analysis_root.resolve()
    rows = read_csv(root / "data" / "repair_rate_summary.csv")
    figures = root / "figures"
    draw(rows, BASELINE_RECAM + TOPOLOGY_2_2 + TOPOLOGY_1_4, "repair_rate", "Repair rate (%)", figures / "repair_rate/" /"all_repair_rate/" / "fig_r3_repair_rate", args.show_ci, "all")
    draw(rows, BASELINE_RECAM + TOPOLOGY_2_2, "repair_rate", "Repair rate (%)", figures / "repair_rate/" /"topology_2_2_repair_rate"/ "topology_2_2_repair_rate", args.show_ci, "topology_2_2")
    draw(rows, BASELINE_RECAM + TOPOLOGY_1_4, "repair_rate", "Repair rate (%)", figures / "repair_rate/" /"topology_1_4_repair_rate"/ "topology_1_4_repair_rate", args.show_ci, "topology_1_4")
    draw(rows, BASELINE_RECAM + POLICY_EARLY, "repair_rate", "Repair rate (%)", figures / "repair_rate/" /"policy_early_repair_rate"/ "policy_early_repair_rate", args.show_ci, "policy_early")
    draw(rows, BASELINE_RECAM + POLICY_GLOBAL, "repair_rate", "Repair rate (%)", figures / "repair_rate/" /"policy_global_repair_rate"/ "policy_global_repair_rate", args.show_ci, "policy_global")

    print(f"Wrote repair/failure figures to {figures}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
