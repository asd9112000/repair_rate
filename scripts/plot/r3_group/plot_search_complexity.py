#!/usr/bin/env python3
"""Plot derived C++ algorithmic GLOBAL search-node complexity."""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))
from scripts.plot.r3_group.common import (STYLE, add_legend, make_single_panel_figure,
                                          make_two_panel_figure, read_csv,
                                          save_three_formats, style_panels,
                                          style_single_panel)

POLICIES = ("directional_m1_global", "single_hop_m1_global")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--analysis-root", required=True, type=Path)
    return parser.parse_args()


def draw_curves(axis, rows, rs):
    for policy in POLICIES:
        selected = sorted((row for row in rows if int(row["RS"]) == rs and row["policy"] == policy),
                          key=lambda row: int(row["F_GROUP"]))
        if selected:
            label, color, marker, line = STYLE[policy]
            axis.plot([int(row["F_GROUP"]) for row in selected], [float(row["median"]) for row in selected],
                      label=label, color=color, marker=marker, linestyle=line)
    # A legal no-search case has zero visited nodes.  Symlog retains that
    # measured zero instead of silently dropping it on a logarithmic axis.
    axis.set_yscale("symlog", linthresh=1.0)


def main() -> int:
    args = parse_args()
    root = args.analysis_root.resolve()
    rows = [row for row in read_csv(root / "data" / "search_complexity.csv")
            if row["metric"] == "search_nodes" and row["metric_status"] == "SUPPORTED"]
    output = root / "figures" / "fig_r3_search_complexity"
    fig, axes = make_two_panel_figure()
    for axis, rs in zip(axes, (2, 3)):
        draw_curves(axis, rows, rs)
    style_panels(axes, "Median search nodes\n(C++ algorithmic)")
    add_legend(fig, axes[0])
    save_three_formats(fig, output.parent, output.name)
    for rs in (2, 3):
        panel_fig, panel_axis = make_single_panel_figure()
        draw_curves(panel_axis, rows, rs)
        style_single_panel(panel_axis, "Median search nodes\n(C++ algorithmic)")
        add_legend(panel_fig, panel_axis)
        save_three_formats(panel_fig, output.parent, f"{output.name}_rs{rs}")
    print(f"Wrote search-complexity figure to {root / 'figures'}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
