"""Shared stable visual identity for R3 derived-data figures."""

from __future__ import annotations

import csv
import os
import sys
from collections import defaultdict
from pathlib import Path
from typing import Iterable

os.environ.setdefault("MPLCONFIGDIR", "/tmp/r3-group-analysis-matplotlib")
import matplotlib as mpl
ROOT = Path(__file__).resolve().parents[3]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))
from scripts.analysis.r3_group.common import POLICIES
from scripts.paper_style import (ANNOTATION_SIZE, AXIS_LABEL_SIZE, AXIS_LINE_WIDTH,
                                 BASE_FONT_SIZE, GRID_LINE_WIDTH, LEGEND_SIZE,
                                 LINE_WIDTH, MARKER_SIZE, TICK_LABEL_SIZE,
                                 FigureGeometry, apply_paper_style,
                                 figure_with_fixed_margins, save_paper_figure,
                                 style_axis)


# High-contrast, colorblind-safe hues with stable policy identity. Policy keys
# come from the analysis contract; compact labels are a display-only choice for
# the frozen single-column figure canvas.
_VISUALS = {
    "local_no_sharing": ("#333333", "o", "-"),
    "directional_m1_local_first": ("#0072B2", "s", "-"),
    "directional_m1_early": ("#009E73", "D", "-."),
    "directional_m1_global": ("#D55E00", "X", "--"),
    "directional_m1_group_global": ("#8C564B", "X", ":"),
    "pairwise_row_m1_local_first": ("#E69F00", "^", "-"),
    "pairwise_row_m1_early": ("#F0A000", "v", "-."),
    "pairwise_row_m1_global": ("#B8860B", "X", "--"),
    "two_pairwise_m1_local_first": ("#56B4E9", "v", "-"),
    "two_pairwise_m1_early": ("#4FA3D1", "P", "-."),
    "two_pairwise_m1_pair_global": ("#CC79A7", "D", "--"),
    "single_hop_m1_local_first": ("#6A5ACD", "P", "-"),
    "single_hop_m1_early": ("#5B4BB7", "D", "-."),
    "single_hop_m1_global": ("#E7298A", "X", "--"),
}
COMPACT_LEGEND_LABELS = {
    "local_no_sharing": "LOCAL",
    "directional_m1_local_first": "Dir. V2 LOCAL-FIRST",
    "directional_m1_early": "Dir. V2 EARLY",
    "directional_m1_global": "Dir. V2 GLOBAL",
    "directional_m1_group_global": "Dir. generic GROUP_GLOBAL (legacy)",
    "pairwise_row_m1_local_first": "PairRow-LOCAL",
    "pairwise_row_m1_early": "PairRow-EARLY",
    "pairwise_row_m1_global": "PairRow-GLOBAL",
    "two_pairwise_m1_local_first": "2Pair-LOCAL",
    "two_pairwise_m1_early": "2Pair-EARLY",
    "two_pairwise_m1_pair_global": "2Pair-GLOBAL",
    "single_hop_m1_local_first": "1Hop-LOCAL",
    "single_hop_m1_early": "1Hop-EARLY",
    "single_hop_m1_global": "1Hop-GLOBAL",
}
STYLE = {policy: (COMPACT_LEGEND_LABELS[policy], *visual)
         for policy, visual in _VISUALS.items()}
COMMON_LOAD_TICKS = (8, 12, 16, 20, 24, 28, 32)

# The R3 review figures deliberately use a two-point type increase and a
# two-level visual-weight increase without changing the repository-wide paper
# defaults used by unrelated figure families.
R3_FONT_INCREMENT = 2
R3_WEIGHT_LEVEL_INCREMENT = 2

# R3 figures reserve space inside a fixed canvas for a figure-level legend.
# The axes rectangle is therefore independent of ylabel or legend text length.
R3_SINGLE_PANEL_GEOMETRY = FigureGeometry(
    width=3.45, height=3.20, left=0.20, right=0.97, bottom=0.19, top=0.72)
R3_TWO_PANEL_GEOMETRY = FigureGeometry(
    width=7.10, height=3.20, left=0.10, right=0.98, bottom=0.19, top=0.72,
    wspace=0.18)
DENSE_LEGEND_AXES_BOTTOM = 0.38
DENSE_LEGEND_AXES_TOP = 0.92


def read_csv(path: Path) -> list[dict[str, str]]:
    with path.open(newline="", encoding="utf-8") as source:
        return list(csv.DictReader(source))


def grouped(rows: Iterable[dict[str, str]], *keys: str):
    result = defaultdict(list)
    for row in rows:
        result[tuple(row[key] for key in keys)].append(row)
    return result


def panel_label(axis, rs: int, label: str) -> None:
    # This is a panel identifier, not a subplot title.  Keeping it above the
    # data rectangle avoids obscuring saturated repair-rate curves.
    axis.text(0.02, 1.02, f"{label} $R_s=C_s={rs}$", transform=axis.transAxes,
              va="bottom", ha="left", clip_on=False)


def save_three_formats(fig, figure_root: Path, stem: str) -> None:
    """Write R3 preview PNG plus vector exports in format subdirectories."""
    figure_root.mkdir(parents=True, exist_ok=True)
    pdf_root = figure_root / "pdf"
    svg_root = figure_root / "svg"
    pdf_root.mkdir(exist_ok=True)
    svg_root.mkdir(exist_ok=True)
    save_paper_figure(fig, pdf_root / f"{stem}.pdf", tight=False)
    save_paper_figure(fig, svg_root / f"{stem}.svg", tight=False)
    save_paper_figure(fig, figure_root / f"{stem}.png", tight=False, close=True)


def style_panels(axes, ylabel: str) -> None:
    for index, axis in enumerate(axes):
        style_axis(axis, grid=True, grid_axis="y")
        axis.set_xlabel("$F_{GROUP}$")
        axis.set_xticks(COMMON_LOAD_TICKS)
        axis.set_xlim(COMMON_LOAD_TICKS[0] - 1, COMMON_LOAD_TICKS[-1] + 1)
        panel_label(axis, 2 if index == 0 else 3, "(a)" if index == 0 else "(b)")
    axes[0].set_ylabel(ylabel)


def style_single_panel(axis, ylabel: str) -> None:
    """Style an RS-specific export without adding a redundant panel label."""
    style_axis(axis, grid=True, grid_axis="y")
    axis.set_xlabel("$F_{GROUP}$")
    axis.set_ylabel(ylabel)
    axis.set_xticks(COMMON_LOAD_TICKS)
    axis.set_xlim(COMMON_LOAD_TICKS[0] - 1, COMMON_LOAD_TICKS[-1] + 1)


def legend_within_canvas(fig, legend, *, tolerance_pixels: float = 0.5) -> bool:
    """Return whether a rendered legend stays inside the fixed figure canvas."""
    fig.canvas.draw()
    legend_box = legend.get_window_extent(fig.canvas.get_renderer())
    canvas_box = fig.bbox
    return (legend_box.x0 >= canvas_box.x0 - tolerance_pixels and
            legend_box.y0 >= canvas_box.y0 - tolerance_pixels and
            legend_box.x1 <= canvas_box.x1 + tolerance_pixels and
            legend_box.y1 <= canvas_box.y1 + tolerance_pixels)


def reserve_dense_legend_space(fig) -> None:
    """Move axes upward to reserve lower-canvas space without resizing it."""
    fig.subplots_adjust(bottom=DENSE_LEGEND_AXES_BOTTOM, top=DENSE_LEGEND_AXES_TOP)


def legend_below_axes(fig, legend, axis, *, gap_pixels: float = 2.0) -> bool:
    """Return whether a dense legend is separated from the plotted axes."""
    fig.canvas.draw()
    legend_box = legend.get_window_extent(fig.canvas.get_renderer())
    axis_box = axis.get_window_extent(fig.canvas.get_renderer())
    return legend_box.y1 <= axis_box.y0 - gap_pixels


def add_legend(fig, axis, *, ncol: int = 2, dense: bool = False):
    """Add a title-free legend and reject overflow outside the fixed canvas."""
    handles, labels = axis.get_legend_handles_labels()
    if dense:
        # Two columns are the default for compact labels.  Callers with
        # descriptive comparison labels may request one column; three columns
        # are never used without a separate fixed-canvas proof.
        legend = fig.legend(handles, labels, loc="lower center", ncol=ncol,
                            bbox_to_anchor=(0.5, 0.015), frameon=False,
                            fontsize=LEGEND_SIZE + R3_FONT_INCREMENT - 2)
    else:
        legend = fig.legend(handles, labels, loc="upper center", ncol=ncol,
                            bbox_to_anchor=(0.5, 0.98), frameon=False)
    if not legend_within_canvas(fig, legend):
        raise RuntimeError("legend exceeds the fixed R3 figure canvas")
    if dense and not legend_below_axes(fig, legend, axis):
        raise RuntimeError("dense legend overlaps the plotted axes")
    return legend


def apply_r3_plot_style() -> None:
    """Apply the R3-only readability uplift on top of paper_style."""
    apply_paper_style()
    mpl.rcParams.update({
        "font.size": BASE_FONT_SIZE + R3_FONT_INCREMENT,
        "axes.labelsize": AXIS_LABEL_SIZE + R3_FONT_INCREMENT,
        "xtick.labelsize": TICK_LABEL_SIZE + R3_FONT_INCREMENT,
        "ytick.labelsize": TICK_LABEL_SIZE + R3_FONT_INCREMENT,
        "legend.fontsize": LEGEND_SIZE + R3_FONT_INCREMENT,
        "lines.linewidth": LINE_WIDTH + 1.0,
        "lines.markersize": MARKER_SIZE + R3_WEIGHT_LEVEL_INCREMENT,
        "axes.linewidth": AXIS_LINE_WIDTH + 0.4,
        "xtick.major.width": AXIS_LINE_WIDTH + 0.4,
        "ytick.major.width": AXIS_LINE_WIDTH + 0.4,
        "grid.linewidth": GRID_LINE_WIDTH + 0.2,
    })


def make_two_panel_figure():
    apply_r3_plot_style()
    return figure_with_fixed_margins(
        R3_TWO_PANEL_GEOMETRY, nrows=1, ncols=2, sharey=True)


def make_single_panel_figure():
    apply_r3_plot_style()
    return figure_with_fixed_margins(R3_SINGLE_PANEL_GEOMETRY)
