#!/usr/bin/env python3
"""Plot fault-count × spare-line sweeps from DynamicSpareSharing summary.csv."""

from __future__ import annotations

import argparse
import math
import os
import tempfile
from pathlib import Path
from typing import Callable, Sequence

os.environ.setdefault(
    "MPLCONFIGDIR", str(Path(tempfile.gettempdir()) / "recam-matplotlib-cache")
)

import matplotlib

matplotlib.use("Agg")

import matplotlib.pyplot as plt
from matplotlib.colors import Normalize, TwoSlopeNorm
import numpy as np
import pandas as pd


PROJECT_ROOT = Path(__file__).resolve().parents[3]
DEFAULT_SUMMARY = PROJECT_ROOT / "reports" / "dynamic_spare_sharing" / "summary.csv"
OUTPUT_DPI = 300
RELATIVE_IMPROVEMENT_LIMIT = 100.0

CONFIG_ORDER = ["Base", "D1", "D2", "P1", "P2", "G1"]
CONFIG_LABELS = {
    "Base": "No Sharing",
    "D1": "Directional m=1",
    "D2": "Directional m=2",
    "P1": "Pairwise m=1",
    "P2": "Pairwise m=2",
    "N1": "Neighbor m=1",
    "N2": "Neighbor m=2",
    "G1": "Global Pool m=1",
}
CONFIG_COLORS = {
    "Base": "#7F7F7F",
    "D1": "#4C78A8",
    "D2": "#9ECAE9",
    "P1": "#F58518",
    "P2": "#FFBF79",
    "N1": "#B279A2",
    "N2": "#D6B5D0",
    "G1": "#54A24B",
}
CONFIG_MARKERS = {
    "Base": "o",
    "D1": "s",
    "D2": "D",
    "P1": "^",
    "P2": "v",
    "N1": "X",
    "N2": "*",
    "G1": "P",
}
FAULT_LABELS = {
    "uniform": "Uniform",
    "moderate_imbalance": "Moderate Imbalance",
    "strong_imbalance": "Strong Imbalance",
    "hotspot": "Hotspot",
}

REQUIRED_COLUMNS = [
    "seed",
    "fault_count",
    "fault_model",
    "fault_spatial_model",
    "layout",
    "policy",
    "storage_mode",
    "Rs",
    "Cs",
    "shared_rows",
    "shared_columns",
    "global_local_rows",
    "global_local_columns",
    "global_rows",
    "global_columns",
    "runs",
    "repair_rate",
    "baseline_repair_rate",
    "sharing_gain",
]
NUMERIC_COLUMNS = [
    "seed",
    "fault_count",
    "Rs",
    "Cs",
    "shared_rows",
    "shared_columns",
    "global_local_rows",
    "global_local_columns",
    "global_rows",
    "global_columns",
    "runs",
    "repair_rate",
    "baseline_repair_rate",
    "sharing_gain",
]


def configure_plot_style() -> None:
    plt.rcParams.update(
        {
            "font.family": "DejaVu Sans",
            "font.size": 10,
            "axes.titlesize": 10.5,
            "axes.labelsize": 10,
            "axes.edgecolor": "0.25",
            "axes.linewidth": 0.8,
            "grid.color": "0.86",
            "grid.linewidth": 0.6,
            "legend.frameon": False,
            "legend.fontsize": 8.5,
            "pdf.fonttype": 42,
            "ps.fonttype": 42,
            "savefig.facecolor": "white",
        }
    )


def load_data(path: Path) -> pd.DataFrame:
    if not path.is_file():
        raise FileNotFoundError(f"Dynamic summary.csv not found: {path}")
    data = pd.read_csv(path)
    if data.empty:
        raise ValueError(f"Dynamic summary.csv has no data rows: {path}")
    missing = [column for column in REQUIRED_COLUMNS if column not in data.columns]
    if missing:
        fault_hint = (
            " Re-run DynamicSpareSharing with --repair-rate-sweep; older "
            "summary.csv files do not contain fault_count."
            if "fault_count" in missing
            else ""
        )
        raise ValueError(
            "Missing required summary columns: " + ", ".join(missing) + fault_hint
        )
    data = data.copy()
    for column in NUMERIC_COLUMNS:
        converted = pd.to_numeric(data[column], errors="coerce")
        invalid = converted.isna()
        if invalid.any():
            lines = ", ".join(str(index + 2) for index in data.index[invalid][:10])
            raise ValueError(
                f"Column '{column}' has missing/non-numeric values at CSV line(s): {lines}"
            )
        data[column] = converted
    return data


def _config_key(row: pd.Series) -> str | None:
    if row["policy"] == "no_sharing":
        return "Base"
    layout = str(row["layout"])
    if layout == "2x2" and row["shared_rows"] != row["shared_columns"]:
        return None
    if layout == "1x4" and row["shared_columns"] != 0:
        return None
    shared = int(row["shared_rows"])
    prefix = (
        {
            "pair": "P",
            "neighbor": "N",
            "global_pool": "G",
        }
        if layout == "1x4"
        else {
            "directional": "D",
            "pairwise_edge": "P",
            "global_pool": "G",
        }
    ).get(str(row["policy"]))
    key = f"{prefix}{shared}" if prefix else None
    return key if key in CONFIG_ORDER else None


def prepare_data(
    data: pd.DataFrame,
    fault_model: str | None,
    storage_mode: str | None,
) -> tuple[pd.DataFrame, list[str]]:
    global CONFIG_ORDER
    warnings: list[str] = []
    selected = data.copy()
    if fault_model is not None:
        selected = selected[selected["fault_model"] == fault_model].copy()
    if storage_mode is not None:
        selected = selected[
            selected["storage_mode"].astype(str).str.lower()
            == storage_mode.lower()
        ].copy()
    if selected.empty:
        raise ValueError("No summary rows match the requested fault/storage filter")

    models = sorted(selected["fault_model"].astype(str).unique())
    storage_modes = sorted(selected["storage_mode"].astype(str).unique())
    if len(models) != 1:
        raise ValueError(
            "The sweep plot requires one fault_model; select one with "
            f"--fault-model. Available: {', '.join(models)}"
        )
    if len(storage_modes) != 1:
        raise ValueError(
            "The sweep plot requires one storage_mode; select one with "
            f"--storage-mode. Available: {', '.join(storage_modes)}"
        )

    layouts = sorted(selected["layout"].astype(str).unique())
    if len(layouts) != 1:
        raise ValueError(
            "The sweep plot requires one layout; separate 2x2 and 1x4 "
            f"results. Available: {', '.join(layouts)}"
        )
    if layouts[0] == "1x4":
        CONFIG_ORDER = ["Base", "P1", "P2", "N1", "N2", "G1"]
        CONFIG_LABELS["P1"] = "Pair m=1"
        CONFIG_LABELS["P2"] = "Pair m=2"
    elif layouts[0] == "2x2":
        CONFIG_ORDER = ["Base", "D1", "D2", "P1", "P2", "G1"]
        CONFIG_LABELS["P1"] = "Pairwise Edge m=1"
        CONFIG_LABELS["P2"] = "Pairwise Edge m=2"
    else:
        raise ValueError(f"Unsupported group layout: {layouts[0]}")

    if (selected["Rs"] != selected["Cs"]).any():
        raise ValueError("Repair-rate sweep plots require symmetric Rs=Cs rows")
    for column in ("fault_count", "Rs", "Cs", "runs"):
        if not np.allclose(selected[column], np.round(selected[column])):
            raise ValueError(f"Column '{column}' must contain integer values")
        selected[column] = selected[column].astype(int)
    if (selected["fault_count"] <= 0).any() or (selected["Rs"] <= 0).any():
        raise ValueError("fault_count and spare-line values must be positive")
    for column in ("repair_rate", "baseline_repair_rate"):
        if (~selected[column].between(0.0, 1.0, inclusive="both")).any():
            raise ValueError(f"Column '{column}' contains values outside [0,1]")
    gain_error = (
        selected["sharing_gain"]
        - (selected["repair_rate"] - selected["baseline_repair_rate"])
    ).abs()
    if (gain_error > 1.0e-7).any():
        raise ValueError("sharing_gain does not match repair_rate - baseline_repair_rate")

    selected["configuration"] = selected.apply(_config_key, axis=1)
    unsupported = selected["configuration"].isna()
    if unsupported.any():
        warnings.append(
            f"Excluded {int(unsupported.sum())} rows that are not representative "
            f"{('/'.join(CONFIG_ORDER))} configurations."
        )
        selected = selected[~unsupported].copy()
    selected["configuration_label"] = selected["configuration"].map(CONFIG_LABELS)
    selected["configuration_order"] = selected["configuration"].map(
        {key: index for index, key in enumerate(CONFIG_ORDER)}
    )
    selected["spare_line"] = selected["Rs"].astype(int)
    selected["repair_rate_percent"] = selected["repair_rate"] * 100.0
    selected["global_partition"] = selected.apply(
        lambda row: (
            f"local={int(row['global_local_rows'])}R/"
            f"{int(row['global_local_columns'])}C; "
            f"global={int(row['global_rows'])}R/"
            f"{int(row['global_columns'])}C"
            if row["policy"] == "global_pool"
            else ""
        ),
        axis=1,
    )

    duplicate = selected.duplicated(
        ["fault_count", "spare_line", "configuration"], keep=False
    )
    if duplicate.any():
        preview = selected.loc[
            duplicate, ["fault_count", "spare_line", "configuration"]
        ].drop_duplicates()
        raise ValueError(
            "Duplicate sweep cells found: "
            + ", ".join(
                f"f={row.fault_count}/s={row.spare_line}/{row.configuration}"
                for row in preview.head(10).itertuples(index=False)
            )
        )

    baseline = selected[selected["configuration"] == "Base"]
    expected_cells = {
        (fault, spare)
        for fault in selected["fault_count"].unique()
        for spare in selected["spare_line"].unique()
    }
    baseline_cells = set(zip(baseline["fault_count"], baseline["spare_line"]))
    missing_baseline = sorted(expected_cells - baseline_cells)
    if missing_baseline:
        raise ValueError(
            "Missing No Sharing baseline cells: "
            + ", ".join(f"f={fault}/s={spare}" for fault, spare in missing_baseline)
        )

    baseline_lookup = baseline[
        ["fault_count", "spare_line", "repair_rate"]
    ].rename(columns={"repair_rate": "baseline_lookup"})
    selected = selected.merge(
        baseline_lookup, on=["fault_count", "spare_line"], how="left"
    )
    if not np.allclose(
        selected["baseline_repair_rate"], selected["baseline_lookup"], atol=1.0e-7
    ):
        raise ValueError("Policy baseline_repair_rate differs from No Sharing row")
    selected["repair_gain_pp"] = 100.0 * (
        selected["repair_rate"] - selected["baseline_lookup"]
    )
    selected["relative_improvement_percent"] = np.where(
        selected["baseline_lookup"] > 0.0,
        100.0
        * (selected["repair_rate"] - selected["baseline_lookup"])
        / selected["baseline_lookup"],
        np.nan,
    )

    for key in CONFIG_ORDER:
        cells = set(
            zip(
                selected.loc[selected["configuration"] == key, "fault_count"],
                selected.loc[selected["configuration"] == key, "spare_line"],
            )
        )
        missing = sorted(expected_cells - cells)
        if missing:
            warnings.append(
                f"{CONFIG_LABELS[key]} is missing {len(missing)} sweep cells; "
                "plots show these cells as N/A."
            )

    selected = selected.sort_values(
        ["fault_count", "spare_line", "configuration_order"]
    ).reset_index(drop=True)
    return selected, warnings


def save_figure(fig: plt.Figure, output_dir: Path, stem: str) -> None:
    output_dir.mkdir(parents=True, exist_ok=True)
    fig.savefig(output_dir / f"{stem}.png", dpi=OUTPUT_DPI, bbox_inches="tight")
    fig.savefig(output_dir / f"{stem}.pdf", bbox_inches="tight")
    plt.close(fig)


def save_plot_data(data: pd.DataFrame, data_dir: Path, stem: str) -> None:
    data_dir.mkdir(parents=True, exist_ok=True)
    data.to_csv(data_dir / f"{stem}.csv", index=False, float_format="%.10g")


def _metadata_title(data: pd.DataFrame) -> str:
    model = str(data["fault_model"].iloc[0])
    model_label = FAULT_LABELS.get(model, model.replace("_", " ").title())
    storage = str(data["storage_mode"].iloc[0]).upper()
    return f"{model_label}, {storage} analysis"


def plot_repair_rate_comparison(
    data: pd.DataFrame, output_dir: Path, data_dir: Path
) -> None:
    stem = "repair_rate_comparison"
    save_plot_data(data, data_dir, stem)
    fault_counts = sorted(data["fault_count"].unique())
    spare_lines = sorted(data["spare_line"].unique())
    ncols = min(3, len(fault_counts))
    nrows = math.ceil(len(fault_counts) / ncols)
    figure_height = 3.15 * nrows + 1.25
    fig, axes = plt.subplots(
        nrows,
        ncols,
        figsize=(4.1 * ncols, figure_height),
        sharex=True,
        sharey=True,
        squeeze=False,
    )
    for ax, fault_count in zip(axes.flat, fault_counts):
        group = data[data["fault_count"] == fault_count]
        for key in CONFIG_ORDER:
            policy = group[group["configuration"] == key].sort_values("spare_line")
            if policy.empty:
                continue
            ax.plot(
                policy["spare_line"],
                policy["repair_rate_percent"],
                color=CONFIG_COLORS[key],
                marker=CONFIG_MARKERS[key],
                linewidth=1.5,
                markersize=4.2,
                label=CONFIG_LABELS[key],
            )
        ax.set_title(f"Fault Number = {fault_count}")
        ax.set_xticks(spare_lines)
        ax.set_ylim(0.0, 100.0)
        ax.grid(axis="y")
        ax.set_axisbelow(True)
    for ax in axes.flat[len(fault_counts) :]:
        ax.set_visible(False)
    fig.supxlabel(
        "Number of Spare Lines (Rs = Cs)", y=0.65 / figure_height
    )
    fig.supylabel("Repair Rate (%)", x=0.025)
    handles, labels = axes.flat[0].get_legend_handles_labels()
    fig.legend(
        handles,
        labels,
        loc="lower center",
        bbox_to_anchor=(0.5, 0.005),
        ncols=3,
    )
    fig.suptitle(
        f"Dynamic Spare Sharing Repair Rate — {_metadata_title(data)}", y=0.97
    )
    fig.subplots_adjust(
        left=0.07,
        right=0.99,
        bottom=1.15 / figure_height,
        top=1.0 - 0.75 / figure_height,
        hspace=0.34,
        wspace=0.04,
    )
    save_figure(fig, output_dir, stem)


def plot_repair_rate_vs_fault_count_by_spare(
    data: pd.DataFrame, output_dir: Path, data_dir: Path
) -> list[str]:
    figure_dir = output_dir / "repair_rate_vs_fault_count_by_spare"
    figure_data_dir = data_dir / "repair_rate_vs_fault_count_by_spare"
    fault_counts = sorted(data["fault_count"].unique())
    generated: list[str] = []

    for spare_line in sorted(data["spare_line"].unique()):
        group = data[data["spare_line"] == spare_line].copy()
        group = group.sort_values(["fault_count", "configuration_order"])
        stem = f"repair_rate_vs_fault_count_spare_{spare_line}"
        save_plot_data(group, figure_data_dir, stem)

        fig, ax = plt.subplots(figsize=(8.6, 4.8), constrained_layout=True)
        for key in CONFIG_ORDER:
            policy = group[group["configuration"] == key].sort_values(
                "fault_count"
            )
            if policy.empty:
                continue
            ax.plot(
                policy["fault_count"],
                policy["repair_rate_percent"],
                color=CONFIG_COLORS[key],
                marker=CONFIG_MARKERS[key],
                linewidth=1.6,
                markersize=4.5,
                label=CONFIG_LABELS[key],
            )
        ax.set_xlabel("Total Fault Count across A/B/C/D")
        ax.set_ylabel("Repair Rate (%)")
        ax.set_title(
            f"Repair Rate vs. Fault Count — Spare Lines Rs=Cs={spare_line}\n"
            f"{_metadata_title(group)}"
        )
        ax.set_xticks(fault_counts)
        if len(fault_counts) > 10:
            ax.tick_params(axis="x", labelrotation=45)
        ax.set_ylim(0.0, 100.0)
        ax.grid(axis="y")
        ax.set_axisbelow(True)
        ax.legend(
            ncols=min(3, len(CONFIG_ORDER)),
            loc="upper center",
            bbox_to_anchor=(0.5, -0.18),
        )
        save_figure(fig, figure_dir, stem)
        generated.append(stem)

    return generated


def _matrix(
    data: pd.DataFrame, key: str, value_column: str
) -> pd.DataFrame:
    fault_counts = sorted(data["fault_count"].unique())
    spare_lines = sorted(data["spare_line"].unique())
    group = data[data["configuration"] == key]
    return group.pivot(
        index="fault_count", columns="spare_line", values=value_column
    ).reindex(index=fault_counts, columns=spare_lines)


def _draw_heatmap(
    ax: plt.Axes,
    matrix: pd.DataFrame,
    cmap_name: str,
    norm: Normalize,
    formatter: Callable[[float], str],
) -> matplotlib.image.AxesImage:
    cmap = plt.get_cmap(cmap_name).copy()
    cmap.set_bad("#EEEEEE")
    values = np.ma.masked_invalid(matrix.to_numpy(dtype=float))
    image = ax.imshow(values, aspect="auto", cmap=cmap, norm=norm)
    ax.set_xticks(np.arange(len(matrix.columns)), matrix.columns)
    ax.set_yticks(np.arange(len(matrix.index)), matrix.index)
    for row in range(values.shape[0]):
        for column in range(values.shape[1]):
            value = matrix.iloc[row, column]
            label = "N/A" if pd.isna(value) else formatter(float(value))
            text_color = "0.18"
            if not pd.isna(value):
                red, green, blue, _ = image.cmap(image.norm(float(value)))
                luminance = 0.2126 * red + 0.7152 * green + 0.0722 * blue
                if luminance < 0.48:
                    text_color = "white"
            ax.text(
                column,
                row,
                label,
                ha="center",
                va="center",
                fontsize=7.2,
                color=text_color,
            )
    return image


def _heatmap_panels(
    data: pd.DataFrame,
    keys: Sequence[str],
    value_column: str,
    cmap: str,
    norm: Normalize,
    formatter: Callable[[float], str],
    colorbar_label: str,
    title: str,
) -> plt.Figure:
    ncols = 3
    nrows = math.ceil(len(keys) / ncols)
    fig, axes = plt.subplots(
        nrows,
        ncols,
        figsize=(4.0 * ncols, 3.4 * nrows),
        constrained_layout=True,
        squeeze=False,
    )
    image = None
    for ax, key in zip(axes.flat, keys):
        image = _draw_heatmap(
            ax, _matrix(data, key, value_column), cmap, norm, formatter
        )
        ax.set_title(CONFIG_LABELS[key])
        ax.set_xlabel("Spare Lines (Rs = Cs)")
        ax.set_ylabel("Fault Number")
    for ax in axes.flat[len(keys) :]:
        ax.set_visible(False)
    if image is not None:
        visible_axes = [ax for ax in axes.flat if ax.get_visible()]
        fig.colorbar(image, ax=visible_axes, shrink=0.88, label=colorbar_label)
    fig.suptitle(f"{title} — {_metadata_title(data)}")
    return fig


def plot_repair_rate_heatmaps(
    data: pd.DataFrame, output_dir: Path, data_dir: Path
) -> None:
    stem = "repair_rate_heatmaps"
    save_plot_data(data, data_dir, stem)
    fig = _heatmap_panels(
        data,
        CONFIG_ORDER,
        "repair_rate_percent",
        "Blues",
        Normalize(vmin=0.0, vmax=100.0),
        lambda value: f"{value:.1f}",
        "Repair Rate (%)",
        "Repair Rate by Architecture",
    )
    save_figure(fig, output_dir, stem)


def plot_improvement_heatmaps(
    data: pd.DataFrame, output_dir: Path, data_dir: Path
) -> None:
    sharing_keys = CONFIG_ORDER[1:]
    sharing_rows = data[data["configuration"].isin(sharing_keys)].copy()

    stem = "repair_rate_improvement_heatmap"
    save_plot_data(sharing_rows, data_dir, stem)
    finite_gain = sharing_rows["repair_gain_pp"].dropna().abs()
    limit = max(1.0, float(np.ceil(finite_gain.max()))) if not finite_gain.empty else 1.0
    fig = _heatmap_panels(
        sharing_rows,
        sharing_keys,
        "repair_gain_pp",
        "RdBu_r",
        TwoSlopeNorm(vmin=-limit, vcenter=0.0, vmax=limit),
        lambda value: f"{value:+.1f}",
        "Repair-Rate Improvement (percentage points)",
        "Absolute Improvement over No Sharing",
    )
    save_figure(fig, output_dir, stem)

    stem = "repair_rate_relative_improvement_heatmap"
    save_plot_data(sharing_rows, data_dir, stem)
    fig = _heatmap_panels(
        sharing_rows,
        sharing_keys,
        "relative_improvement_percent",
        "RdBu_r",
        TwoSlopeNorm(
            vmin=-RELATIVE_IMPROVEMENT_LIMIT,
            vcenter=0.0,
            vmax=RELATIVE_IMPROVEMENT_LIMIT,
        ),
        lambda value: f"{value:+.0f}%",
        "Relative Improvement vs. No Sharing (%)",
        "Relative Improvement over No Sharing",
    )
    save_figure(fig, output_dir, stem)


def generate_tables(
    data: pd.DataFrame, data_dir: Path, target_rate: float
) -> tuple[pd.DataFrame, pd.DataFrame]:
    threshold_records: list[dict[str, object]] = []
    for fault_count in sorted(data["fault_count"].unique()):
        for key in CONFIG_ORDER:
            rows = data[
                (data["fault_count"] == fault_count)
                & (data["configuration"] == key)
                & (data["repair_rate"] >= target_rate)
            ]
            threshold_records.append(
                {
                    "fault_count": fault_count,
                    "configuration": key,
                    "configuration_label": CONFIG_LABELS[key],
                    "target_repair_rate": target_rate,
                    "minimum_spare_lines": (
                        int(rows["spare_line"].min()) if not rows.empty else pd.NA
                    ),
                }
            )
    thresholds = pd.DataFrame.from_records(threshold_records)
    save_plot_data(thresholds, data_dir, "minimum_spares_for_target")

    best_rows: list[pd.Series] = []
    for _, group in data.groupby(["fault_count", "spare_line"], sort=True):
        ordered = group.sort_values(
            ["repair_rate", "configuration_order"], ascending=[False, True]
        )
        best_rows.append(ordered.iloc[0])
    best = pd.DataFrame(best_rows).reset_index(drop=True)
    best = best[
        [
            "fault_count",
            "spare_line",
            "configuration",
            "configuration_label",
            "repair_rate",
            "repair_rate_percent",
            "repair_gain_pp",
        ]
    ]
    save_plot_data(best, data_dir, "best_policy_by_case")
    return thresholds, best


def generate_analysis_summary(
    data: pd.DataFrame,
    warnings: Sequence[str],
    output_dir: Path,
    target_rate: float,
) -> None:
    lines = [
        "Dynamic spare-sharing repair-rate sweep",
        "=========================================",
        "",
        f"Experiment: {_metadata_title(data)}",
        f"Fault range: {data['fault_count'].min()}..{data['fault_count'].max()}",
        f"Spare range: {data['spare_line'].min()}..{data['spare_line'].max()} (Rs=Cs)",
        f"Runs per configuration: {', '.join(str(value) for value in sorted(data['runs'].unique()))}",
        f"Seed(s): {', '.join(str(value) for value in sorted(data['seed'].unique()))}",
        "",
        "Configuration statistics:",
    ]
    for key in CONFIG_ORDER[1:]:
        group = data[data["configuration"] == key]
        if group.empty:
            continue
        gain = group["repair_gain_pp"]
        best = group.loc[gain.idxmax()]
        lines.append(
            f"- {CONFIG_LABELS[key]}: mean gain {gain.mean():+.3f} pp; "
            f"maximum {gain.max():+.3f} pp at fault={int(best['fault_count'])}, "
            f"spare={int(best['spare_line'])}."
        )
    lines.extend(
        [
            "",
            f"Minimum-spare table target: {target_rate * 100:.1f}% repair rate.",
            "",
            "Warnings:",
        ]
    )
    lines.extend(f"- {warning}" for warning in warnings)
    if not warnings:
        lines.append("- None")
    output_dir.mkdir(parents=True, exist_ok=True)
    (output_dir / "analysis_summary.txt").write_text(
        "\n".join(lines) + "\n", encoding="utf-8"
    )


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Plot DynamicSpareSharing fault-count × spare-line sweeps."
    )
    parser.add_argument(
        "summary_csv",
        nargs="?",
        type=Path,
        default=DEFAULT_SUMMARY,
        help=f"input summary.csv (default: {DEFAULT_SUMMARY})",
    )
    parser.add_argument(
        "--output-dir",
        type=Path,
        default=None,
        help="plot directory (default: sibling plots directory)",
    )
    parser.add_argument(
        "--plot-data-dir",
        type=Path,
        default=None,
        help="processed-data directory (default: sibling plot_data directory)",
    )
    parser.add_argument(
        "--fault-model",
        choices=sorted(FAULT_LABELS),
        default=None,
        help="select one fault model when the CSV contains multiple models",
    )
    parser.add_argument(
        "--storage-mode",
        choices=("cam", "sram"),
        default=None,
        help="select one storage mode when the CSV contains both",
    )
    parser.add_argument(
        "--target-repair-rate",
        type=float,
        default=0.95,
        help="target used by minimum-spare table (default: 0.95)",
    )
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    if not 0.0 <= args.target_repair_rate <= 1.0:
        raise ValueError("--target-repair-rate must be in [0,1]")
    summary = args.summary_csv.expanduser().resolve()
    run_root = summary.parent.parent if summary.parent.name == "raw" else summary.parent
    output_dir = (
        args.output_dir.expanduser().resolve()
        if args.output_dir is not None
        else run_root / "plots"
    )
    data_dir = (
        args.plot_data_dir.expanduser().resolve()
        if args.plot_data_dir is not None
        else run_root / "plot_data"
    )

    configure_plot_style()
    raw = load_data(summary)
    data, warnings = prepare_data(raw, args.fault_model, args.storage_mode)
    save_plot_data(data, data_dir, "repair_rate_sweep_long")
    plot_repair_rate_comparison(data, output_dir, data_dir)
    by_spare_figures = plot_repair_rate_vs_fault_count_by_spare(
        data, output_dir, data_dir
    )
    plot_repair_rate_heatmaps(data, output_dir, data_dir)
    plot_improvement_heatmaps(data, output_dir, data_dir)
    generate_tables(data, data_dir, args.target_repair_rate)
    generate_analysis_summary(data, warnings, output_dir, args.target_repair_rate)

    for warning in warnings:
        print(f"WARNING: {warning}")
    print(
        "Generated 4 original figure groups plus "
        f"{len(by_spare_figures)} fixed-spare repair-rate figures as PNG and PDF."
    )
    print(f"Figures: {output_dir}")
    print(f"Processed data: {data_dir}")


if __name__ == "__main__":
    main()
