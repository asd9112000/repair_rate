#!/usr/bin/env python3

## =========== running command ==============
# python3 scripts/legacy/sharedline/plot_repair_rates.py
# python3 scripts/legacy/sharedline/plot_repair_rates.py path/to/repairRates.csv --output-dir path/to/figures
## ==========================================



"""Create publication-ready repair-rate figures and summary statistics."""

from __future__ import annotations

import argparse
import math
from pathlib import Path

import matplotlib

matplotlib.use("Agg")

import matplotlib.pyplot as plt
from matplotlib.colors import TwoSlopeNorm
import numpy as np
import pandas as pd
import seaborn as sns


PROJECT_ROOT = Path(__file__).resolve().parents[3]
DEFAULT_CSV_FILE = PROJECT_ROOT / "reports/legacy/sharedline/repairRates.csv"
REQUIRED_COLUMNS = (
    "FaultNum",
    "SpareLine",
    "RepairRate",
    "RepairRate_RECAM",
)
OUTPUT_DPI = 300
# A fixed, symmetric scale makes improvement plots comparable across runs.
# Values outside this interval remain annotated but use the end color.
IMPROVEMENT_COLOR_LIMIT = 0.10
# Relative improvement is displayed as a percentage. A fixed +/-100% scale
# keeps different experiment runs directly comparable.
RELATIVE_IMPROVEMENT_COLOR_LIMIT = 100.0


def configure_plot_style() -> None:
    """Apply a restrained style suitable for papers."""
    sns.set_theme(
        context="paper",
        style="whitegrid",
        font="DejaVu Sans",
        font_scale=1.15,
        rc={
            "axes.edgecolor": "0.25",
            "axes.linewidth": 0.8,
            "grid.color": "0.86",
            "grid.linewidth": 0.6,
            "legend.frameon": False,
            "pdf.fonttype": 42,
            "ps.fonttype": 42,
        },
    )


def load_data(csv_file: Path) -> pd.DataFrame:
    """Read, validate, sort, and enrich the repair-rate data."""
    if not csv_file.is_file():
        raise FileNotFoundError(f"CSV file not found: {csv_file}")

    df = pd.read_csv(csv_file)
    missing = [column for column in REQUIRED_COLUMNS if column not in df.columns]
    if missing:
        raise ValueError(f"Missing required CSV columns: {', '.join(missing)}")

    df = df.loc[:, REQUIRED_COLUMNS].copy()
    for column in REQUIRED_COLUMNS:
        converted = pd.to_numeric(df[column], errors="coerce")
        invalid_rows = df.index[converted.isna()].tolist()
        if invalid_rows:
            rows = ", ".join(str(row + 2) for row in invalid_rows[:10])
            suffix = " ..." if len(invalid_rows) > 10 else ""
            raise ValueError(
                f"Column '{column}' contains non-numeric or missing values at "
                f"CSV line(s): {rows}{suffix}. The file may contain results "
                "appended by multiple analysis runs."
            )
        df[column] = converted

    for column in ("FaultNum", "SpareLine"):
        non_integer = ~np.isclose(df[column], np.round(df[column]))
        if non_integer.any():
            raise ValueError(f"Column '{column}' must contain integer values.")
        df[column] = df[column].astype(int)

    for column in ("RepairRate", "RepairRate_RECAM"):
        outside_range = ~df[column].between(0.0, 1.0, inclusive="both")
        if outside_range.any():
            raise ValueError(f"Column '{column}' must contain values from 0 to 1.")

    duplicates = df.duplicated(subset=["FaultNum", "SpareLine"], keep=False)
    if duplicates.any():
        pairs = (
            df.loc[duplicates, ["FaultNum", "SpareLine"]]
            .drop_duplicates()
            .itertuples(index=False, name=None)
        )
        preview = ", ".join(f"({fault}, {spare})" for fault, spare in list(pairs)[:10])
        raise ValueError(
            "Duplicate (FaultNum, SpareLine) combinations found: " + preview
        )

    if df.empty:
        raise ValueError("The CSV file contains no data rows.")

    df = df.sort_values(["FaultNum", "SpareLine"]).reset_index(drop=True)
    df["RepairRateDiff"] = df["RepairRate"] - df["RepairRate_RECAM"]
    relative_diff = pd.Series(np.nan, index=df.index, dtype=float)
    valid_baseline = df["RepairRate_RECAM"] > 0.0
    relative_diff.loc[valid_baseline] = (
        100.0
        * df.loc[valid_baseline, "RepairRateDiff"]
        / df.loc[valid_baseline, "RepairRate_RECAM"]
    )
    df["RepairRateRelativeDiffPct"] = relative_diff
    return df


def save_figure(fig: plt.Figure, output_dir: Path, stem: str) -> None:
    """Save one figure as a high-resolution PNG and vector PDF."""
    output_dir.mkdir(parents=True, exist_ok=True)
    fig.savefig(output_dir / f"{stem}.png", dpi=OUTPUT_DPI, bbox_inches="tight")
    fig.savefig(output_dir / f"{stem}.pdf", bbox_inches="tight")
    plt.close(fig)


def plot_repair_rate_comparison(
    df: pd.DataFrame, output_dir: Path, architecture_label: str
) -> None:
    """Plot the selected architecture and RECAM in one panel per FaultNum."""
    fault_nums = sorted(df["FaultNum"].unique())
    spare_lines = sorted(df["SpareLine"].unique())
    # Keep the common six-panel case at 2 x 3 while avoiding a sparse 2 x 3
    # canvas when exactly four FaultNum values are present.
    ncols = 2 if len(fault_nums) == 4 else min(3, len(fault_nums))
    nrows = math.ceil(len(fault_nums) / ncols)

    fig, axes = plt.subplots(
        nrows,
        ncols,
        figsize=(4.0 * ncols, 3.1 * nrows),
        sharex=True,
        sharey=True,
        constrained_layout=True,
        squeeze=False,
    )

    for ax, fault_num in zip(axes.flat, fault_nums):
        group = df[df["FaultNum"] == fault_num]
        ax.plot(
            group["SpareLine"],
            group["RepairRate"],
            color="#1f4e79",
            linestyle="-",
            marker="o",
            markersize=4.5,
            linewidth=1.6,
            label=architecture_label,
        )
        ax.plot(
            group["SpareLine"],
            group["RepairRate_RECAM"],
            color="#8c4a3c",
            linestyle="--",
            marker="s",
            markersize=4.2,
            linewidth=1.5,
            label="RECAM",
        )
        ax.set_title(f"FaultNum = {fault_num}")
        ax.set_xlim(min(spare_lines) - 0.15, max(spare_lines) + 0.15)
        ax.set_ylim(0.0, 1.0)
        ax.set_xticks(spare_lines)
        ax.set_yticks(np.linspace(0.0, 1.0, 6))
        ax.grid(axis="x", visible=False)
        ax.grid(axis="y", visible=True)

    for ax in axes.flat[len(fault_nums) :]:
        ax.set_visible(False)

    fig.supxlabel("Number of Spare Lines")
    fig.supylabel("Repair Rate")
    handles, labels = axes.flat[0].get_legend_handles_labels()
    fig.legend(
        handles,
        labels,
        loc="outside upper center",
        ncols=2,
    )
    save_figure(fig, output_dir, "repair_rate_comparison")


def plot_absolute_improvement_heatmap(
    df: pd.DataFrame, output_dir: Path, architecture_label: str
) -> None:
    """Plot the architecture-minus-RECAM repair-rate difference."""
    data = df.pivot(index="FaultNum", columns="SpareLine", values="RepairRateDiff")
    data = data.sort_index().sort_index(axis=1)
    norm = TwoSlopeNorm(
        vmin=-IMPROVEMENT_COLOR_LIMIT,
        vcenter=0.0,
        vmax=IMPROVEMENT_COLOR_LIMIT,
    )

    fig, ax = plt.subplots(
        figsize=(9.2, max(5.6, 0.58 * len(data))),
        constrained_layout=True,
    )
    sns.heatmap(
        data,
        ax=ax,
        annot=True,
        fmt=".2f",
        cmap="vlag",
        norm=norm,
        linewidths=0.5,
        linecolor="white",
        cbar_kws={
            "label": (
                "Absolute Repair Rate Difference "
                f"({architecture_label} - RECAM)"
            ),
            "extend": "both",
        },
    )
    ax.set_title(
        f"Absolute Repair Rate Improvement of {architecture_label} over RECAM",
        pad=10,
    )
    ax.set_xlabel("Number of Spare Lines")
    ax.set_ylabel("FaultNum")
    ax.tick_params(axis="both", rotation=0)
    save_figure(fig, output_dir, "repair_rate_improvement_heatmap")


def plot_relative_improvement_heatmap(
    df: pd.DataFrame, output_dir: Path, architecture_label: str
) -> None:
    """Plot improvement normalized by the RECAM repair rate, in percent."""
    data = df.pivot(
        index="FaultNum",
        columns="SpareLine",
        values="RepairRateRelativeDiffPct",
    )
    data = data.sort_index().sort_index(axis=1)
    norm = TwoSlopeNorm(
        vmin=-RELATIVE_IMPROVEMENT_COLOR_LIMIT,
        vcenter=0.0,
        vmax=RELATIVE_IMPROVEMENT_COLOR_LIMIT,
    )
    annotation_labels = np.full(data.shape, "", dtype=object)
    valid_values = data.notna().to_numpy()
    annotation_labels[valid_values] = [
        f"{value:.1f}%" for value in data.to_numpy()[valid_values]
    ]

    fig, ax = plt.subplots(
        figsize=(7.2, max(4.0, 0.45 * len(data))),
        constrained_layout=True,
    )
    # Use the neutral midpoint as the background for undefined cells. The
    # explicit N/A annotation below preserves their meaning and also avoids
    # all-NaN warnings when every RECAM baseline in a small sweep is zero.
    sns.heatmap(
        data.fillna(0.0),
        ax=ax,
        annot=annotation_labels,
        fmt="",
        cmap="vlag",
        norm=norm,
        linewidths=0.5,
        linecolor="white",
        cbar_kws={
            "label": "Relative Improvement vs. RECAM (%)",
            "extend": "both",
        },
    )

    # Normalization is undefined when the RECAM repair rate is zero, so add
    # an explicit N/A annotation to avoid ambiguity.
    for row_index, column_index in zip(*np.where(data.isna().to_numpy())):
        ax.text(
            column_index + 0.5,
            row_index + 0.5,
            "N/A",
            ha="center",
            va="center",
            color="0.35",
            fontsize=8,
        )

    ax.set_title(
        f"Relative Repair Rate Improvement of {architecture_label} over RECAM",
        pad=10,
    )
    ax.set_xlabel("Number of Spare Lines")
    ax.set_ylabel("FaultNum")
    ax.tick_params(axis="both", rotation=0)
    save_figure(fig, output_dir, "repair_rate_relative_improvement_heatmap")


def plot_repair_rate_heatmaps(
    df: pd.DataFrame, output_dir: Path, architecture_label: str
) -> None:
    """Plot architecture and RECAM heatmaps with one shared 0-to-1 scale."""
    sharedline = df.pivot(index="FaultNum", columns="SpareLine", values="RepairRate")
    recam = df.pivot(index="FaultNum", columns="SpareLine", values="RepairRate_RECAM")
    sharedline = sharedline.sort_index().sort_index(axis=1)
    recam = recam.reindex(index=sharedline.index, columns=sharedline.columns)

    height = max(4.0, 0.45 * len(sharedline))
    fig = plt.figure(figsize=(12.0, height), constrained_layout=True)
    grid_spec = fig.add_gridspec(1, 3, width_ratios=(1.0, 1.0, 0.045))
    axes = np.array(
        [
            fig.add_subplot(grid_spec[0, 0]),
            fig.add_subplot(grid_spec[0, 1]),
        ]
    )
    axes[1].sharex(axes[0])
    axes[1].sharey(axes[0])
    cbar_ax = fig.add_subplot(grid_spec[0, 2])

    heatmap_options = dict(
        annot=True,
        fmt=".2f",
        cmap="Blues",
        vmin=0.0,
        vmax=1.0,
        linewidths=0.5,
        linecolor="white",
    )
    sns.heatmap(sharedline, ax=axes[0], cbar=False, **heatmap_options)
    sns.heatmap(
        recam,
        ax=axes[1],
        cbar=True,
        cbar_ax=cbar_ax,
        cbar_kws={"label": "Repair Rate"},
        **heatmap_options,
    )

    axes[0].set_title(f"(a) {architecture_label}", pad=10)
    axes[1].set_title("(b) RECAM", pad=10)
    for ax in axes:
        ax.set_xlabel("Number of Spare Lines")
        ax.tick_params(axis="both", rotation=0)
    axes[0].set_ylabel("FaultNum")
    axes[1].set_ylabel("")

    save_figure(fig, output_dir, "repair_rate_heatmaps")


def minimum_spares_for_target(
    df: pd.DataFrame, rate_column: str, target: float
) -> pd.Series:
    """Return the first (minimum) SpareLine meeting target for each FaultNum."""
    qualifying = df[df[rate_column] >= target]
    return qualifying.groupby("FaultNum")["SpareLine"].min()


def print_statistics(
    df: pd.DataFrame, architecture_label: str, target: float = 0.95
) -> None:
    """Print aggregate improvement and resource-threshold comparisons."""
    diff = df["RepairRateDiff"]
    tolerance = 1e-12
    maximum = diff.max()
    best = df[np.isclose(diff, maximum, atol=tolerance, rtol=0.0)]

    print(f"\n{architecture_label} vs. RECAM summary")
    print("-" * 34)
    print(f"Maximum improvement: {maximum:.4f}")
    print(f"Mean improvement:    {diff.mean():.4f}")
    print(f"Median improvement:  {diff.median():.4f}")
    print(
        f"Number of cases {architecture_label} > RECAM: "
        f"{int((diff > tolerance).sum())}"
    )
    print(
        f"Number of cases {architecture_label} = RECAM: "
        f"{int((diff.abs() <= tolerance).sum())}"
    )
    print(
        f"Number of cases {architecture_label} < RECAM: "
        f"{int((diff < -tolerance).sum())}"
    )
    print("Maximum-improvement combination(s):")
    for row in best.itertuples(index=False):
        print(
            f"  FaultNum={row.FaultNum}, SpareLine={row.SpareLine} "
            f"(improvement={row.RepairRateDiff:.4f})"
        )

    fault_nums = sorted(df["FaultNum"].unique())
    sharedline_min = minimum_spares_for_target(df, "RepairRate", target)
    recam_min = minimum_spares_for_target(df, "RepairRate_RECAM", target)
    threshold_table = pd.DataFrame(
        {
            "FaultNum": fault_nums,
            architecture_label: [
                sharedline_min.get(fault, pd.NA) for fault in fault_nums
            ],
            "RECAM": [recam_min.get(fault, pd.NA) for fault in fault_nums],
        }
    )
    for column in (architecture_label, "RECAM"):
        threshold_table[column] = threshold_table[column].map(
            lambda value: "N/A" if pd.isna(value) else str(int(value))
        )

    print(f"\nMinimum SpareLine required for Repair Rate >= {target:.2f}")
    print("-" * 54)
    print(threshold_table.to_string(index=False))


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Plot one architecture and RECAM repair-rate results."
    )
    parser.add_argument(
        "csv_file",
        nargs="?",
        type=Path,
        default=DEFAULT_CSV_FILE,
        help=f"input CSV (default: {DEFAULT_CSV_FILE})",
    )
    parser.add_argument(
        "--output-dir",
        type=Path,
        default=None,
        help="figure output directory (default: directory containing the CSV)",
    )
    parser.add_argument(
        "--architecture-label",
        default="SharedLine",
        help="label used for the non-RECAM architecture (default: SharedLine)",
    )
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    csv_file = args.csv_file.expanduser().resolve()
    output_dir = (
        args.output_dir.expanduser().resolve()
        if args.output_dir is not None
        else csv_file.parent
    )

    configure_plot_style()
    df = load_data(csv_file)
    architecture_label = args.architecture_label.strip()
    if not architecture_label:
        raise ValueError("--architecture-label cannot be empty.")

    print_statistics(df, architecture_label)
    plot_repair_rate_comparison(df, output_dir, architecture_label)
    plot_absolute_improvement_heatmap(df, output_dir, architecture_label)
    plot_relative_improvement_heatmap(df, output_dir, architecture_label)
    plot_repair_rate_heatmaps(df, output_dir, architecture_label)
    print(f"\nFigures written to: {output_dir}")


if __name__ == "__main__":
    main()
