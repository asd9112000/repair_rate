#!/usr/bin/env python3
"""Plot paired improvement rates from derived paired-outcome data."""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))
from scripts.plot.r3_group.common import (add_legend, make_single_panel_figure,
                                          make_two_panel_figure, read_csv,
                                          reserve_dense_legend_space,
                                          save_three_formats, style_panels,
                                          style_single_panel)

COMPARISON_STYLE = {
    "g2x2_rc_group_vs_g2x2_r_group": ("G2X2 RC GROUP vs R GROUP", "#D55E00", "X", "--"),
    "g2x2_rc_early_vs_g2x2_r_early": ("G2X2 RC EARLY vs R EARLY", "#0072B2", "s", "-"),
    "g2x2_r_group_vs_l1x4_r_group": ("L1X4 R GROUP vs G2X2 R GROUP", "#6A5ACD", "v", "--"),
    "g2x2_r_early_vs_l1x4_r_early": ("L1X4 R EARLY vs G2X2 R EARLY", "#009E73", "D", "-."),
    "g2x2_rc_early_vs_g2x2_rc_group": ("G2X2 RC GROUP over EARLY", "#CC79A7", "P", "--"),
    "g2x2_r_early_vs_g2x2_r_group": ("G2X2 R GROUP over EARLY", "#56B4E9", "^", "--"),
    "l1x4_r_early_vs_l1x4_r_group": ("L1X4 R GROUP over EARLY", "#E7298A", "X", "--"),
    "local_vs_directional_early": ("Directional EARLY over LOCAL", "#0072B2", "s", "-"),
    "directional_early_vs_greedy": ("V2 GROUP_GREEDY over V2 EARLY", "#009E73", "D", "-."),
    "directional_greedy_vs_v2_global": ("V2 GROUP_GLOBAL over V2 GROUP_GREEDY", "#D55E00", "X", "--"),
    "directional_early_vs_v2_global": ("V2 GROUP_GLOBAL over V2 EARLY", "#D55E00", "X", "--"),
    "greedy_vs_generic_global_legacy": ("generic GROUP_GLOBAL (legacy) vs V2 GROUP_GREEDY (cross-contract)", "#8C564B", "X", ":"),
    "single_hop_early_vs_global": ("Single-Hop GLOBAL over EARLY", "#E7298A", "X", "--"),
    "two_pairwise_vs_single_hop": ("Single-Hop EARLY over Two-Pairwise EARLY", "#6A5ACD", "P", "-"),
}


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--analysis-root", required=True, type=Path)
    parser.add_argument("--policy-set", choices=("legacy", "sixcase_static"), default="legacy")
    return parser.parse_args()


def draw_curves(axis, rows, rs):
    for comparison, (label, color, marker, line) in COMPARISON_STYLE.items():
        selected = sorted((row for row in rows if int(row["RS"]) == rs and row["comparison"] == comparison),
                          key=lambda row: int(row["F_GROUP"]))
        if selected:
            axis.plot([int(row["F_GROUP"]) for row in selected],
                      [100.0 * float(row["left_fail_right_pass_rate"]) for row in selected],
                      label=label, color=color, marker=marker, linestyle=line)


def main() -> int:
    args = parse_args()
    root = args.analysis_root.resolve()
    rows = read_csv(root / "data" / "paired_outcomes.csv")
    output = root / "figures" / "fig_r3_paired_outcomes"
    if args.policy_set == "sixcase_static":
        output = root / "figures" / "sixcase_static" / "fig_sixcase_static_paired_outcomes"
    fig, axes = make_two_panel_figure()
    reserve_dense_legend_space(fig)
    for axis, rs in zip(axes, (2, 3)):
        draw_curves(axis, rows, rs)
    style_panels(axes, "Baseline-fail / comparator-pass (%)")
    add_legend(fig, axes[0], ncol=1, dense=True)
    save_three_formats(fig, output.parent, output.name)
    for rs in (2, 3):
        panel_fig, panel_axis = make_single_panel_figure()
        reserve_dense_legend_space(panel_fig)
        draw_curves(panel_axis, rows, rs)
        style_single_panel(panel_axis, "Baseline-fail / comparator-pass (%)")
        add_legend(panel_fig, panel_axis, ncol=1, dense=True)
        save_three_formats(panel_fig, output.parent, f"{output.name}_rs{rs}")
    print(f"Wrote paired-outcome figure to {root / 'figures'}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
