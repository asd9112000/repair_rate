"""Shared DSS figure conventions for IEEE/DATE papers and 16:9 presentations.

Call :func:`apply_paper_style` or :func:`apply_ppt_style` before creating a
figure.  The functions configure only visual defaults; plotters remain
responsible for meaningful labels and captions.  Paper figures omit in-figure
titles unless a compact multi-panel identifier is necessary.
"""

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
from typing import Optional

import matplotlib as mpl
import matplotlib.pyplot as plt


# IEEE / DATE paper dimensions (inches).
SINGLE_COLUMN_WIDTH = 3.45
SINGLE_COLUMN_HEIGHT = 2.35
COMPACT_SINGLE_COLUMN_HEIGHT = 2.00
DOUBLE_COLUMN_WIDTH = 7.10
DOUBLE_COLUMN_HEIGHT = 3.60
SINGLE_COLUMN_HEATMAP_HEIGHT = SINGLE_COLUMN_HEIGHT
DOUBLE_COLUMN_HEATMAP_HEIGHT = DOUBLE_COLUMN_HEIGHT

# Typography (points).
BASE_FONT_SIZE = 8
AXIS_LABEL_SIZE = 8
TITLE_SIZE = 8
TICK_LABEL_SIZE = 7
LEGEND_SIZE = 7
ANNOTATION_SIZE = 7
COLORBAR_TICK_SIZE = TICK_LABEL_SIZE
COLORBAR_LABEL_SIZE = AXIS_LABEL_SIZE

# Heatmaps use the same paper typography unless a plotter documents an
# exception.  Aliases keep existing callers concise.
HEATMAP_AXIS_LABEL_SIZE = AXIS_LABEL_SIZE
HEATMAP_TICK_LABEL_SIZE = TICK_LABEL_SIZE
HEATMAP_ANNOTATION_SIZE = ANNOTATION_SIZE
HEATMAP_COLORBAR_TICK_SIZE = COLORBAR_TICK_SIZE
HEATMAP_COLORBAR_LABEL_SIZE = COLORBAR_LABEL_SIZE

# Strokes and output.
LINE_WIDTH = 1.3
MARKER_SIZE = 4.5
AXIS_LINE_WIDTH = 0.8
GRID_LINE_WIDTH = 0.5
RASTER_DPI = 600
SAVE_PAD_INCHES = 0.02

# 16:9 presentation dimensions (inches) and typography (points).
PPT_SLIDE_SIZE = (13.333, 7.5)
PPT_NORMAL_FIGURE_SIZE = (8.0, 4.5)
PPT_WIDE_FIGURE_SIZE = (10.0, 4.5)
PPT_HALF_SLIDE_FIGURE_SIZE = (5.0, 3.2)
PPT_BASE_FONT_SIZE = 18
PPT_AXIS_LABEL_SIZE = 20
PPT_TICK_LABEL_SIZE = 17
PPT_LEGEND_SIZE = 17
PPT_ANNOTATION_SIZE = 18
PPT_TITLE_SIZE = 22
PPT_LINE_WIDTH = 2.2
PPT_MARKER_SIZE = 8.0
PPT_AXIS_LINE_WIDTH = 1.2
PPT_RASTER_DPI = 600


@dataclass(frozen=True)
class FigureGeometry:
    """Fixed canvas and normalized axes bounds for a reusable figure layout."""

    width: float
    height: float
    left: float
    right: float
    bottom: float
    top: float
    wspace: float = 0.0
    hspace: float = 0.0


def apply_paper_style() -> None:
    """Apply the repository-wide publication style before creating figures."""
    mpl.rcParams.update({
        # Nimbus Roman is the installed Times-compatible publication font.
        "font.family": "Nimbus Roman",
        "font.serif": ["Nimbus Roman"],
        # Keep math labels (for example, $\\sigma$ and R_s) in the same face.
        "mathtext.fontset": "custom",
        "mathtext.rm": "Nimbus Roman",
        "mathtext.it": "Nimbus Roman:italic",
        "mathtext.bf": "Nimbus Roman:bold",
        "mathtext.cal": "Nimbus Roman:italic",
        "mathtext.sf": "Nimbus Roman",
        "mathtext.tt": "Nimbus Roman",
        "font.size": BASE_FONT_SIZE,
        "axes.labelsize": AXIS_LABEL_SIZE,
        "axes.titlesize": TITLE_SIZE,
        "xtick.labelsize": TICK_LABEL_SIZE,
        "ytick.labelsize": TICK_LABEL_SIZE,
        "legend.fontsize": LEGEND_SIZE,
        "lines.linewidth": LINE_WIDTH,
        "lines.markersize": MARKER_SIZE,
        "axes.linewidth": AXIS_LINE_WIDTH,
        "xtick.major.width": AXIS_LINE_WIDTH,
        "ytick.major.width": AXIS_LINE_WIDTH,
        "xtick.minor.width": 0.6,
        "ytick.minor.width": 0.6,
        "xtick.major.size": 3.0,
        "ytick.major.size": 3.0,
        "xtick.minor.size": 2.0,
        "ytick.minor.size": 2.0,
        "legend.frameon": False,
        "legend.handlelength": 1.8,
        "legend.borderaxespad": 0.3,
        "legend.labelspacing": 0.3,
        "legend.columnspacing": 0.8,
        "grid.linewidth": GRID_LINE_WIDTH,
        "grid.alpha": 0.30,
        "figure.figsize": (SINGLE_COLUMN_WIDTH, SINGLE_COLUMN_HEIGHT),
        "savefig.dpi": RASTER_DPI,
        "savefig.bbox": "tight",
        "savefig.pad_inches": SAVE_PAD_INCHES,
        "savefig.transparent": False,
        # Type 42 preserves TrueType glyphs in PDF/PS instead of Type 3.
        "pdf.fonttype": 42,
        "ps.fonttype": 42,
        "svg.fonttype": "none",
        "axes.unicode_minus": True,
    })


def apply_ppt_style() -> None:
    """Apply the 16:9 DSS presentation style before creating figures."""
    mpl.rcParams.update({
        "font.family": "Nimbus Roman",
        "font.serif": ["Nimbus Roman"],
        "mathtext.fontset": "custom",
        "mathtext.rm": "Nimbus Roman",
        "mathtext.it": "Nimbus Roman:italic",
        "mathtext.bf": "Nimbus Roman:bold",
        "mathtext.cal": "Nimbus Roman:italic",
        "mathtext.sf": "Nimbus Roman",
        "mathtext.tt": "Nimbus Roman",
        "font.size": PPT_BASE_FONT_SIZE,
        "axes.labelsize": PPT_AXIS_LABEL_SIZE,
        "axes.titlesize": PPT_TITLE_SIZE,
        "xtick.labelsize": PPT_TICK_LABEL_SIZE,
        "ytick.labelsize": PPT_TICK_LABEL_SIZE,
        "legend.fontsize": PPT_LEGEND_SIZE,
        "lines.linewidth": PPT_LINE_WIDTH,
        "lines.markersize": PPT_MARKER_SIZE,
        "axes.linewidth": PPT_AXIS_LINE_WIDTH,
        "xtick.major.width": PPT_AXIS_LINE_WIDTH,
        "ytick.major.width": PPT_AXIS_LINE_WIDTH,
        "legend.frameon": False,
        "figure.figsize": PPT_NORMAL_FIGURE_SIZE,
        "savefig.dpi": PPT_RASTER_DPI,
        "savefig.bbox": "tight",
        "savefig.pad_inches": SAVE_PAD_INCHES,
        "savefig.transparent": False,
        "pdf.fonttype": 42,
        "ps.fonttype": 42,
        "svg.fonttype": "none",
        "axes.unicode_minus": True,
    })


def single_column_figure(height: float = SINGLE_COLUMN_HEIGHT, **subplots_kwargs):
    """Create a standard 3.45-inch-wide paper figure."""
    return plt.subplots(figsize=(SINGLE_COLUMN_WIDTH, height), **subplots_kwargs)


def double_column_figure(height: float = DOUBLE_COLUMN_HEIGHT, **subplots_kwargs):
    """Create a standard 7.10-inch-wide paper figure."""
    return plt.subplots(figsize=(DOUBLE_COLUMN_WIDTH, height), **subplots_kwargs)


def figure_with_fixed_margins(geometry: FigureGeometry, **subplots_kwargs):
    """Create a figure whose axes rectangle remains stable across datasets.

    Callers reserve space for legends, labels, or annotations through the
    geometry rather than relying on content-dependent tight_layout().
    """
    fig, axes = plt.subplots(figsize=(geometry.width, geometry.height), **subplots_kwargs)
    fig.subplots_adjust(left=geometry.left, right=geometry.right,
                        bottom=geometry.bottom, top=geometry.top,
                        wspace=geometry.wspace, hspace=geometry.hspace)
    return fig, axes


def compact_single_column_figure(**subplots_kwargs):
    """Create a compact 3.45 × 2.00-inch paper figure."""
    return single_column_figure(COMPACT_SINGLE_COLUMN_HEIGHT, **subplots_kwargs)


def single_column_heatmap(height: float = SINGLE_COLUMN_HEATMAP_HEIGHT,
                          **subplots_kwargs):
    """Create a compact single-column heatmap canvas."""
    return single_column_figure(height, **subplots_kwargs)


def double_column_heatmap(height: float = DOUBLE_COLUMN_HEATMAP_HEIGHT,
                          **subplots_kwargs):
    """Create a double-column heatmap canvas."""
    return double_column_figure(height, **subplots_kwargs)


def ppt_normal_figure(**subplots_kwargs):
    """Create an 8.0 × 4.5-inch 16:9 presentation figure."""
    return plt.subplots(figsize=PPT_NORMAL_FIGURE_SIZE, **subplots_kwargs)


def ppt_wide_figure(**subplots_kwargs):
    """Create a 10.0 × 4.5-inch wide 16:9 presentation figure."""
    return plt.subplots(figsize=PPT_WIDE_FIGURE_SIZE, **subplots_kwargs)


def ppt_half_slide_figure(**subplots_kwargs):
    """Create a 5.0 × 3.2-inch half-slide presentation figure."""
    return plt.subplots(figsize=PPT_HALF_SLIDE_FIGURE_SIZE, **subplots_kwargs)


def style_axis(
    ax,
    *,
    grid: bool = True,
    grid_axis: str = "both",
    remove_top_right_spines: bool = False,
) -> None:
    """Apply shared axis treatment without changing data semantics."""
    if grid:
        ax.grid(True, axis=grid_axis, linewidth=GRID_LINE_WIDTH, alpha=0.30)
    if remove_top_right_spines:
        ax.spines["top"].set_visible(False)
        ax.spines["right"].set_visible(False)


def style_heatmap_axis(ax) -> None:
    """Apply standard paper axis and tick sizes to a heatmap."""
    ax.xaxis.label.set_size(HEATMAP_AXIS_LABEL_SIZE)
    ax.yaxis.label.set_size(HEATMAP_AXIS_LABEL_SIZE)
    ax.tick_params(axis="both", labelsize=HEATMAP_TICK_LABEL_SIZE)


def annotate_heatmap_cell(ax, x, y, text: str, *, fontsize: int = HEATMAP_ANNOTATION_SIZE,
                          **kwargs):
    """Add a consistently sized, enlarged centered heatmap annotation."""
    defaults = {"ha": "center", "va": "center", "fontsize": fontsize}
    defaults.update(kwargs)
    return ax.text(x, y, text, **defaults)


def style_colorbar(cbar, *, label: Optional[str] = None) -> None:
    """Apply the standard colorbar typography."""
    cbar.ax.tick_params(labelsize=COLORBAR_TICK_SIZE)
    if label is not None:
        cbar.set_label(label, fontsize=COLORBAR_LABEL_SIZE)


def style_heatmap_colorbar(cbar, *, label: Optional[str] = None) -> None:
    """Apply standard paper typography to a heatmap colorbar."""
    cbar.ax.tick_params(labelsize=HEATMAP_COLORBAR_TICK_SIZE)
    if label is not None:
        cbar.set_label(label, fontsize=HEATMAP_COLORBAR_LABEL_SIZE)


def save_paper_figure(
    fig,
    filename: str | Path,
    *,
    dpi: int = RASTER_DPI,
    transparent: bool = False,
    tight: bool = True,
    close: bool = False,
) -> Path:
    """Save one figure with optional tight bounds and optional closing."""
    path = Path(filename)
    path.parent.mkdir(parents=True, exist_ok=True)
    if tight:
        fig.savefig(path, dpi=dpi, bbox_inches="tight",
                    pad_inches=SAVE_PAD_INCHES, transparent=transparent)
    else:
        # fig.savefig(..., bbox_inches=None) still consults savefig.bbox.
        # Temporarily clear that global default so fixed-geometry callers keep
        # their exact canvas dimensions.
        with mpl.rc_context({"savefig.bbox": None, "savefig.pad_inches": 0.0}):
            fig.savefig(path, dpi=dpi, transparent=transparent)
    if close:
        plt.close(fig)
    return path


def save_ppt_figure(
    fig,
    filename: str | Path,
    *,
    dpi: int = PPT_RASTER_DPI,
    transparent: bool = False,
    close: bool = False,
) -> Path:
    """Save a presentation figure with the shared vector/raster settings."""
    path = Path(filename)
    path.parent.mkdir(parents=True, exist_ok=True)
    fig.savefig(path, dpi=dpi, bbox_inches="tight", pad_inches=SAVE_PAD_INCHES,
                transparent=transparent)
    if close:
        plt.close(fig)
    return path
