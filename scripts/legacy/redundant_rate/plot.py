#!/usr/bin/env python3
"""Visualize the legacy average-extra-line sweep for equal spare settings."""

from __future__ import annotations

import argparse
from pathlib import Path

import matplotlib

matplotlib.use("Agg")

import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
import seaborn as sns


PROJECT_ROOT = Path(__file__).resolve().parents[3]
DEFAULT_CSV_FILE = PROJECT_ROOT / "reports/RedundantRate/RedundantRate.csv"
REQUIRED_COLUMNS = ("Row", "Column", "Fault", "AverageExtraLines")
OUTPUT_STEM = "redundant_rate_visualization"
OUTPUT_DPI = 300


def configure_plot_style() -> None:
    """Apply a restrained, publication-oriented plotting style."""
    sns.set_theme(
        context="paper",
        style="whitegrid",
        font="DejaVu Sans",
        font_scale=1.8,
        rc={
            "axes.edgecolor": "0.25",
            "axes.linewidth": 1.2,
            "grid.color": "0.86",
            "grid.linewidth": 0.8,
            "legend.frameon": False,
            "pdf.fonttype": 42,
            "ps.fonttype": 42,
        },
    )


def load_data(csv_file: Path) -> pd.DataFrame:
    """Read and validate the RedundantRate CSV."""
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
                f"CSV line(s): {rows}{suffix}."
            )
        df[column] = converted

    for column in ("Row", "Column", "Fault"):
        non_integer = ~np.isclose(df[column], np.round(df[column]))
        if non_integer.any():
            raise ValueError(f"Column '{column}' must contain integer values.")
        df[column] = df[column].astype(int)

    if df.empty:
        raise ValueError("The CSV file contains no data rows.")

    unequal_spares = df["Row"] != df["Column"]
    if unequal_spares.any():
        rows = ", ".join(str(row + 2) for row in df.index[unequal_spares][:10])
        raise ValueError(
            "This visualization uses one spare-line variable and therefore "
            f"requires Row == Column. Unequal values occur at CSV line(s): {rows}."
        )

    if (df["AverageExtraLines"] < 0.0).any():
        raise ValueError("Column 'AverageExtraLines' cannot contain negative values.")

    df["SpareLine"] = df["Row"]
    duplicates = df.duplicated(subset=["SpareLine", "Fault"], keep=False)
    if duplicates.any():
        pairs = (
            df.loc[duplicates, ["SpareLine", "Fault"]]
            .drop_duplicates()
            .itertuples(index=False, name=None)
        )
        preview = ", ".join(
            f"(SpareLine={spare}, Fault={fault})"
            for spare, fault in list(pairs)[:10]
        )
        raise ValueError("Duplicate experiment combinations found: " + preview)

    return df.sort_values(["SpareLine", "Fault"]).reset_index(drop=True)


def plot_redundant_rate(df: pd.DataFrame, output_dir: Path) -> None:
    """Plot AverageExtraLines against Fault for each equal Rs/Cs setting."""
    fault_nums = sorted(df["Fault"].unique())
    spare_lines = sorted(df["SpareLine"].unique())
    colors = sns.color_palette("colorblind", n_colors=len(spare_lines))
    markers = ("o", "s", "^")
    line_styles = ("-", "--", "-.")

    fig, ax = plt.subplots(figsize=(7.4, 4.8), constrained_layout=True)
    for index, spare_line in enumerate(spare_lines):
        group = df[df["SpareLine"] == spare_line]
        ax.plot(
            group["Fault"],
            group["AverageExtraLines"],
            color=colors[index],
            marker=markers[index % len(markers)],
            linestyle=line_styles[index % len(line_styles)],
            markersize=8,
            linewidth=4,
            label=str(spare_line),
        )

    ax.set_title(
        "Remaining Spare Lines vs. Fault Count\n",
        # r"Equal row/column spare configuration: $R_s = C_s$",
        pad=10,
    )
    ax.set_xlabel("Number of Faults")
    ax.set_ylabel("Remaining Spare Lines")
    ax.set_xticks(fault_nums)
    ax.set_ylim(bottom=0.0)
    ax.grid(axis="x", visible=False)
    ax.grid(axis="y", visible=True)
    ax.legend(
        title=r"Spare Lines ($R_s = C_s$)",
        loc="best",
        ncols=1,
    )

    output_dir.mkdir(parents=True, exist_ok=True)
    fig.savefig(
        output_dir / f"{OUTPUT_STEM}.png",
        dpi=OUTPUT_DPI,
        bbox_inches="tight",
    )
    fig.savefig(output_dir / f"{OUTPUT_STEM}.pdf", bbox_inches="tight")
    plt.close(fig)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=(
            "Plot average extra lines using one spare-line variable when Row == Column."
        )
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
    plot_redundant_rate(df, output_dir)
    print("Row and column spare counts are equal in this data (Rs = Cs).")
    print(f"Figures written to: {output_dir}")


if __name__ == "__main__":
    main()
