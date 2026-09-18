#!/usr/bin/env python3
"""Plot repair-rate gains in percentage points relative to LOCAL."""

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

# These memberships deliberately mirror the repair-rate comparison views.  The
# LOCAL baseline is omitted from gain curves because it is identically zero.
TOPOLOGY_2_2 = ("directional_m1_local_first", "directional_m1_early",
                 "directional_m1_global", "pairwise_row_m1_local_first",
                 "pairwise_row_m1_early", "pairwise_row_m1_global")
TOPOLOGY_1_4 = ("two_pairwise_m1_local_first", "two_pairwise_m1_early", "two_pairwise_m1_pair_global",
                "single_hop_m1_local_first", "single_hop_m1_early", "single_hop_m1_global")
POLICY_EARLY = ("directional_m1_early", "two_pairwise_m1_early", "single_hop_m1_early")
POLICY_GLOBAL = ("directional_m1_global", "two_pairwise_m1_pair_global",
                 "single_hop_m1_global")
COMPARISONS = (
    ("all", TOPOLOGY_2_2 + TOPOLOGY_1_4, "all_repair_rate_gain", "fig_r3_repair_rate_gain"),
    ("topology_2_2", TOPOLOGY_2_2, "topology_2_2_repair_rate_gain", "topology_2_2_repair_rate_gain"),
    ("topology_1_4", TOPOLOGY_1_4, "topology_1_4_repair_rate_gain", "topology_1_4_repair_rate_gain"),
    ("policy_early", POLICY_EARLY, "policy_early_repair_rate_gain", "policy_early_repair_rate_gain"),
    ("policy_global", POLICY_GLOBAL, "policy_global_repair_rate_gain", "policy_global_repair_rate_gain"),
)
GAIN_METRIC = "gain_vs_local_percentage_points"
GAIN_YLABEL = "Repair-rate gain vs LOCAL (pp)"


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--analysis-root", required=True, type=Path)
    return parser.parse_args()


def draw_curves(axis, rows, policies, rs: int) -> None:
    for policy in policies:
        selected = sorted((row for row in rows if int(row["RS"]) == rs and row["policy"] == policy),
                          key=lambda row: int(row["F_GROUP"]))
        if not selected:
            continue
        label, color, marker, line = STYLE[policy]
        axis.plot([int(row["F_GROUP"]) for row in selected],
                  [float(row[GAIN_METRIC]) for row in selected],
                  label=label, color=color, marker=marker, linestyle=line)
    axis.axhline(0.0, color="#777777", linewidth=1.0, linestyle=":", zorder=0)


def draw(rows, comparison: str, policies, output: Path) -> None:
    # Every view may contain three long V2 labels; reserve the same bottom
    # legend region rather than allowing labels to extend beyond the canvas.
    dense = True
    fig, axes = make_two_panel_figure()
    if dense:
        reserve_dense_legend_space(fig)
    for axis, rs in zip(axes, (2, 3)):
        draw_curves(axis, rows, policies, rs)
    style_panels(axes, GAIN_YLABEL)
    add_legend(fig, axes[0], dense=dense)
    save_three_formats(fig, output.parent, output.name)

    for rs in (2, 3):
        panel_fig, panel_axis = make_single_panel_figure()
        if dense:
            reserve_dense_legend_space(panel_fig)
        draw_curves(panel_axis, rows, policies, rs)
        style_single_panel(panel_axis, GAIN_YLABEL)
        add_legend(panel_fig, panel_axis, dense=dense)
        save_three_formats(panel_fig, output.parent, f"{output.name}_rs{rs}")


def main() -> int:
    args = parse_args()
    root = args.analysis_root.resolve()
    rows = read_csv(root / "data" / "repair_rate_summary.csv")
    figures = root / "figures" / "repair_rate_gain_vs_local"
    for comparison, policies, directory, stem in COMPARISONS:
        draw(rows, comparison, policies, figures / directory / stem)
    print(f"Wrote repair-rate-gain figures to {figures}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
