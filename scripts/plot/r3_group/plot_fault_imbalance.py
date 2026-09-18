#!/usr/bin/env python3
"""Plot pooled R3 repair rate versus rank-quintile fault imbalance."""

from __future__ import annotations

import argparse
import sys
from collections import defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))
from scripts.plot.r3_group.common import (STYLE, add_legend, make_single_panel_figure,
                                          make_two_panel_figure, read_csv,
                                          reserve_dense_legend_space,
                                          save_three_formats)
from scripts.paper_style import style_axis

# TOPOLOGY = ("two_pairwise_m1_early", "two_pairwise_m1_pair_global", "single_hop_m1_early", "single_hop_m1_global")
BASELINE_RECAM = ("local_no_sharing",)
TOPOLOGY_2_2 = ("directional_m1_local_first", "directional_m1_early",
                 "directional_m1_global", "pairwise_row_m1_local_first",
                 "pairwise_row_m1_early", "pairwise_row_m1_global")
TOPOLOGY_1_4 = ("two_pairwise_m1_local_first", "two_pairwise_m1_early", "two_pairwise_m1_pair_global", "single_hop_m1_local_first", "single_hop_m1_early", "single_hop_m1_global")
POLICY_EARLY = ( "directional_m1_early", "two_pairwise_m1_early", "single_hop_m1_early")
POLICY_GLOBAL = ("directional_m1_global", "two_pairwise_m1_pair_global", "single_hop_m1_global")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--analysis-root", required=True, type=Path)
    return parser.parse_args()


def pooled(rows):
    sums = defaultdict(lambda: [0, 0])
    for row in rows:
        key = (int(row["RS"]), row["policy"], int(row["imbalance_bin"]))
        sums[key][0] += int(row["successes"])
        sums[key][1] += int(row["groups"])
    return {key: success / groups for key, (success, groups) in sums.items()}


def draw_curves(axis, rates, policies, rs):
    for policy in policies:
        selected = [(bin_id, rate) for (row_rs, row_policy, bin_id), rate in rates.items()
                    if row_rs == rs and row_policy == policy]
        if not selected:
            continue
        selected.sort()
        text, color, marker, line = STYLE[policy]
        axis.plot([item[0] for item in selected], [100.0 * item[1] for item in selected],
                  label=text, color=color, marker=marker, linestyle=line)


def style_imbalance_axis(axis, *, ylabel: bool, panel_label: str | None = None, rs: int | None = None):
    style_axis(axis, grid=True, grid_axis="y")
    axis.set_xlabel("Fault-imbalance rank quintile")
    if ylabel:
        axis.set_ylabel("Repair rate (%)")
    axis.set_xticks((1, 2, 3, 4, 5))
    if panel_label is not None and rs is not None:
        axis.text(0.02, 1.02, f"{panel_label} $R_s=C_s={rs}$", transform=axis.transAxes,
                  va="bottom", ha="left", clip_on=False)


def draw(rates, policies, output, comparison):
    fig, axes = make_two_panel_figure()
    if comparison == "all":
        reserve_dense_legend_space(fig)
    for axis, rs, label in zip(axes, (2, 3), ("(a)", "(b)")):
        draw_curves(axis, rates, policies, rs)
        style_imbalance_axis(axis, ylabel=(rs == 2), panel_label=label, rs=rs)
    add_legend(fig, axes[0], dense=(comparison == "all"))
    save_three_formats(fig, output.parent, output.name)
    for rs in (2, 3):
        panel_fig, panel_axis = make_single_panel_figure()
        if comparison == "all":
            reserve_dense_legend_space(panel_fig)
        draw_curves(panel_axis, rates, policies, rs)
        style_imbalance_axis(panel_axis, ylabel=True)
        add_legend(panel_fig, panel_axis, dense=(comparison == "all"))
        save_three_formats(panel_fig, output.parent, f"{output.name}_rs{rs}")


def main() -> int:
    args = parse_args()
    root = args.analysis_root.resolve()
    rates = pooled(read_csv(root / "data" / "imbalance_summary.csv"))
    figures = root / "figures"
    # draw(rates, DIRECTIONAL, figures / "fig_r3_directional_fault_imbalance")
    # draw(rates, TOPOLOGY, figures / "fig_r3_topology_fault_imbalance")
    draw(rates, BASELINE_RECAM + TOPOLOGY_2_2 + TOPOLOGY_1_4, figures / "imbalance/" /"all_imbalance/" / "fig_r3_imbalance", "all")
    draw(rates, BASELINE_RECAM + TOPOLOGY_2_2, figures / "imbalance/" /"topology_2_2_imbalance"/ "topology_2_2_imbalance", "topology_2_2")
    draw(rates, BASELINE_RECAM + TOPOLOGY_1_4, figures / "imbalance/" /"topology_1_4_imbalance"/ "topology_1_4_imbalance", "topology_1_4")
    draw(rates, BASELINE_RECAM + POLICY_EARLY, figures / "imbalance/" /"policy_early_imbalance"/ "policy_early_imbalance", "policy_early")
    draw(rates, BASELINE_RECAM + POLICY_GLOBAL, figures / "imbalance/" /"policy_global_imbalance"/ "policy_global_imbalance", "policy_global")




    print(f"Wrote fault-imbalance figures to {figures}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
