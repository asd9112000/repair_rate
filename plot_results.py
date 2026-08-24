#!/usr/bin/env python3
"""Generate publication-quality figures from dynamic sharing summary.csv.

The script intentionally consumes only existing summary rows.  It does not
interpolate missing sweeps or synthesize SRAM/fault-count experiments.
"""

from __future__ import annotations

import argparse
import os
import tempfile
from dataclasses import dataclass, field
from pathlib import Path
from typing import Sequence

os.environ.setdefault(
    "MPLCONFIGDIR", str(Path(tempfile.gettempdir()) / "recam-matplotlib-cache")
)

import matplotlib

matplotlib.use("Agg")

import matplotlib.pyplot as plt
import numpy as np
import pandas as pd


PROJECT_ROOT = Path(__file__).resolve().parent
DEFAULT_SUMMARY_CSV = PROJECT_ROOT / "reports" / "dynamic_spare_sharing" / "summary.csv"
DEFAULT_PLOTS_DIR = PROJECT_ROOT / "plots"
DEFAULT_PLOT_DATA_DIR = PROJECT_ROOT / "plot_data"

# Keep the suspicious local=0/global=8 configuration out of publication plots
# until its simulator behavior has been verified.  The command-line flag can
# override this for explicit diagnostic comparisons.
INCLUDE_UNVERIFIED_GLOBAL_M2 = False

OUTPUT_DPI = 300
FAULT_ORDER = [
    "uniform",
    "moderate_imbalance",
    "strong_imbalance",
    "hotspot",
]
FAULT_LABELS = {
    "uniform": "Uniform",
    "moderate_imbalance": "Moderate Imbalance",
    "strong_imbalance": "Strong Imbalance",
    "hotspot": "Hotspot",
}
FAULT_COLORS = {
    "uniform": "#4C78A8",
    "moderate_imbalance": "#F58518",
    "strong_imbalance": "#54A24B",
    "hotspot": "#E45756",
}
CONFIG_ORDER = ["Base", "D1", "D2", "P1", "P2", "G1", "G2"]
CONFIG_LABELS = {
    "Base": "RECAM / No Sharing",
    "D1": "Directional m=1",
    "D2": "Directional m=2",
    "P1": "Pairwise m=1",
    "P2": "Pairwise m=2",
    "G1": "Global Pool m=1",
    "G2": "Global Pool m=2",
}
CONFIG_COLORS = {
    "Base": "#7F7F7F",
    "D1": "#4C78A8",
    "D2": "#9ECAE9",
    "P1": "#F58518",
    "P2": "#FFBF79",
    "G1": "#54A24B",
    "G2": "#E45756",
}
CONFIG_MARKERS = {
    "Base": "o",
    "D1": "s",
    "D2": "D",
    "P1": "^",
    "P2": "v",
    "G1": "P",
    "G2": "X",
}

REQUIRED_COLUMNS = [
    "fault_model",
    "fault_spatial_model",
    "policy",
    "storage_mode",
    "data_width_bits",
    "Rs",
    "Cs",
    "shared_rows",
    "shared_columns",
    "global_local_rows",
    "global_local_columns",
    "global_rows",
    "global_columns",
    "repair_rate",
    "baseline_repair_rate",
    "sharing_gain",
    "average_spare_utilization",
    "average_borrow_count",
    "p95_borrow_count",
    "average_CAM_requirement",
    "peak_CAM_requirement",
    "average_matrix_size",
    "peak_matrix_size",
    "average_candidate_count",
    "average_analysis_attempts",
    "average_analysis_cycles",
    "p95_analysis_cycles",
    "average_hardware_cost_proxy_bits",
    "peak_hardware_cost_proxy_bits",
]

NUMERIC_COLUMNS = [
    column
    for column in REQUIRED_COLUMNS
    if column
    not in {"fault_model", "fault_spatial_model", "policy", "storage_mode"}
]

CONFIG_ID_COLUMNS = [
    "seed",
    "fault_count",
    "fault_model",
    "fault_spatial_model",
    "policy",
    "storage_mode",
    "data_width_bits",
    "row_address_width_bits",
    "column_address_width_bits",
    "hybrid_CAM_entry_width_bits",
    "Rs",
    "Cs",
    "shared_rows",
    "shared_columns",
    "global_local_rows",
    "global_local_columns",
    "global_rows",
    "global_columns",
    "local_first",
    "minimum_row_reserve",
    "minimum_column_reserve",
    "single_dimension",
    "max_group_borrows",
]


@dataclass
class PlotContext:
    plots_dir: Path
    data_dir: Path
    include_unverified_global_m2: bool
    warnings: list[str] = field(default_factory=list)
    figure_stems: list[str] = field(default_factory=list)

    def warn(self, message: str) -> None:
        if message not in self.warnings:
            self.warnings.append(message)
        print(f"WARNING: {message}")


def configure_plot_style() -> None:
    """Apply one restrained visual language to all figures."""
    plt.rcParams.update(
        {
            "font.family": "DejaVu Sans",
            "font.size": 10,
            "axes.titlesize": 11,
            "axes.labelsize": 10,
            "axes.edgecolor": "0.25",
            "axes.linewidth": 0.8,
            "axes.grid": False,
            "grid.color": "0.86",
            "grid.linewidth": 0.6,
            "legend.frameon": False,
            "legend.fontsize": 8.5,
            "xtick.labelsize": 9,
            "ytick.labelsize": 9,
            "pdf.fonttype": 42,
            "ps.fonttype": 42,
            "savefig.facecolor": "white",
        }
    )


def load_data(csv_file: Path) -> pd.DataFrame:
    if not csv_file.is_file():
        raise FileNotFoundError(f"summary.csv not found: {csv_file}")
    data = pd.read_csv(csv_file)
    if data.empty:
        raise ValueError(f"summary.csv has no data rows: {csv_file}")
    missing = [column for column in REQUIRED_COLUMNS if column not in data.columns]
    if missing:
        raise ValueError("Missing required summary columns: " + ", ".join(missing))

    data = data.copy()
    for column in NUMERIC_COLUMNS:
        converted = pd.to_numeric(data[column], errors="coerce")
        bad = converted.isna()
        if bad.any():
            lines = ", ".join(str(index + 2) for index in data.index[bad][:10])
            raise ValueError(
                f"Column '{column}' has missing/non-numeric data at CSV line(s): {lines}"
            )
        data[column] = converted
    return data


def _row_description(row: pd.Series) -> str:
    return (
        f"fault_model={row.get('fault_model')}, policy={row.get('policy')}, "
        f"shared=({row.get('shared_rows')},{row.get('shared_columns')})"
    )


def validate_data(data: pd.DataFrame, context: PlotContext) -> None:
    """Report data-quality problems without rewriting any measurements."""
    for column in ("repair_rate", "baseline_repair_rate"):
        invalid = ~data[column].between(0.0, 1.0, inclusive="both")
        for _, row in data.loc[invalid].iterrows():
            context.warn(f"{column} outside [0,1]: {_row_description(row)}")

    gain_error = (
        data["sharing_gain"]
        - (data["repair_rate"] - data["baseline_repair_rate"])
    ).abs()
    for _, row in data.loc[gain_error > 1.0e-7].iterrows():
        context.warn(f"sharing_gain mismatch: {_row_description(row)}")

    for column in ("Rs", "Cs"):
        for _, row in data.loc[data[column] <= 0].iterrows():
            context.warn(f"{column} must be positive: {_row_description(row)}")

    for _, row in data.loc[data["average_analysis_cycles"] < 0].iterrows():
        context.warn(f"negative average_analysis_cycles: {_row_description(row)}")

    cam_invalid = data["peak_CAM_requirement"] + 1.0e-9 < data[
        "average_CAM_requirement"
    ]
    for _, row in data.loc[cam_invalid].iterrows():
        context.warn(f"peak CAM below average CAM: {_row_description(row)}")

    matrix_invalid = data["peak_matrix_size"] + 1.0e-9 < data[
        "average_matrix_size"
    ]
    for _, row in data.loc[matrix_invalid].iterrows():
        context.warn(f"peak matrix below average matrix: {_row_description(row)}")

    identity = [column for column in CONFIG_ID_COLUMNS if column in data.columns]
    duplicates = data.duplicated(subset=identity, keep=False)
    if duplicates.any():
        context.warn(
            f"found {int(duplicates.sum())} rows belonging to duplicate configurations"
        )


def select_experiment(
    data: pd.DataFrame,
    rs: int,
    cs: int,
    storage_mode: str,
    context: PlotContext,
) -> pd.DataFrame:
    storage = data["storage_mode"].astype(str).str.lower()
    selected = data.loc[
        (data["Rs"] == rs) & (data["Cs"] == cs) & (storage == storage_mode.lower())
    ].copy()
    if selected.empty:
        raise ValueError(
            f"No rows match Rs={rs}, Cs={cs}, storage_mode={storage_mode}"
        )

    unknown_models = sorted(set(selected["fault_model"]) - set(FAULT_ORDER))
    if unknown_models:
        context.warn(
            "excluding unsupported fault models from current figures: "
            + ", ".join(unknown_models)
        )
        selected = selected[selected["fault_model"].isin(FAULT_ORDER)].copy()

    missing_models = [model for model in FAULT_ORDER if model not in set(selected["fault_model"])]
    if missing_models:
        context.warn("current experiment is missing fault models: " + ", ".join(missing_models))

    return selected


def _configuration_key(row: pd.Series) -> str | None:
    policy = str(row["policy"])
    shared_rows = int(row["shared_rows"])
    shared_columns = int(row["shared_columns"])
    if policy == "no_sharing":
        return "Base"
    if shared_rows != shared_columns or shared_rows not in (1, 2):
        return None
    prefix = {
        "directional": "D",
        "pairwise_edge": "P",
        "global_pool": "G",
    }.get(policy)
    return f"{prefix}{shared_rows}" if prefix else None


def prepare_labels(data: pd.DataFrame) -> pd.DataFrame:
    prepared = data.copy()
    prepared["fault_label"] = prepared["fault_model"].map(FAULT_LABELS)
    prepared["fault_order"] = prepared["fault_model"].map(
        {name: index for index, name in enumerate(FAULT_ORDER)}
    )
    prepared["configuration"] = prepared.apply(_configuration_key, axis=1)
    prepared["configuration_label"] = prepared["configuration"].map(CONFIG_LABELS)
    prepared["m"] = prepared["shared_rows"].astype(int)
    prepared["global_partition"] = prepared.apply(
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
    prepared["unverified_global_m2"] = (
        (prepared["policy"] == "global_pool")
        & (prepared["global_local_rows"] == 0)
        & (prepared["global_local_columns"] == 0)
        & (prepared["global_rows"] == 8)
        & (prepared["global_columns"] == 8)
    )
    prepared["repair_rate_percent"] = prepared["repair_rate"] * 100.0
    prepared["sharing_gain_pp"] = prepared["sharing_gain"] * 100.0
    prepared["spare_utilization_percent"] = (
        prepared["average_spare_utilization"] * 100.0
    )
    prepared["unused_spare_fraction_percent"] = (
        1.0 - prepared["average_spare_utilization"]
    ) * 100.0
    prepared["configuration_order"] = prepared["configuration"].map(
        {name: index for index, name in enumerate(CONFIG_ORDER)}
    )
    return prepared


def filter_main_configs(data: pd.DataFrame, context: PlotContext) -> pd.DataFrame:
    keys = CONFIG_ORDER if context.include_unverified_global_m2 else CONFIG_ORDER[:-1]
    main = data[data["configuration"].isin(keys)].copy()
    main = main.sort_values(["fault_order", "configuration_order"])

    duplicates = main.duplicated(
        subset=["fault_model", "configuration"], keep=False
    )
    if duplicates.any():
        details = main.loc[duplicates, ["fault_model", "configuration"]].drop_duplicates()
        pairs = ", ".join(
            f"{row.fault_model}/{row.configuration}"
            for row in details.itertuples(index=False)
        )
        raise ValueError(
            "Publication configuration selection is ambiguous; duplicate rows: " + pairs
        )

    expected = {
        (fault_model, key) for fault_model in FAULT_ORDER for key in keys
    }
    actual = set(zip(main["fault_model"], main["configuration"]))
    missing = sorted(expected - actual)
    if missing:
        context.warn(
            "missing publication configurations: "
            + ", ".join(f"{fault}/{key}" for fault, key in missing)
        )
    return main


def save_figure(
    fig: plt.Figure, directory: Path, stem: str, context: PlotContext
) -> None:
    directory.mkdir(parents=True, exist_ok=True)
    fig.savefig(directory / f"{stem}.png", dpi=OUTPUT_DPI, bbox_inches="tight")
    fig.savefig(directory / f"{stem}.pdf", bbox_inches="tight")
    plt.close(fig)
    context.figure_stems.append(stem)


def save_plot_data(data: pd.DataFrame, stem: str, context: PlotContext) -> None:
    context.data_dir.mkdir(parents=True, exist_ok=True)
    data.to_csv(context.data_dir / f"{stem}.csv", index=False, float_format="%.10g")


def _ordered_rows(data: pd.DataFrame, keys: Sequence[str]) -> pd.DataFrame:
    order = {key: index for index, key in enumerate(keys)}
    rows = data[data["configuration"].isin(keys)].copy()
    rows["plot_configuration_order"] = rows["configuration"].map(order)
    return rows.sort_values(["fault_order", "plot_configuration_order"])


def _grid_y(ax: plt.Axes) -> None:
    ax.grid(axis="y", color="0.88", linewidth=0.6)
    ax.set_axisbelow(True)


def _grouped_bars_by_fault(
    data: pd.DataFrame,
    value_column: str,
    keys: Sequence[str],
    ylabel: str,
    title: str,
    ylim: tuple[float, float] | None = None,
) -> plt.Figure:
    fig, ax = plt.subplots(figsize=(9.2, 4.8), constrained_layout=True)
    x = np.arange(len(FAULT_ORDER), dtype=float)
    width = 0.82 / max(1, len(keys))
    for index, key in enumerate(keys):
        values = []
        for fault in FAULT_ORDER:
            match = data[(data["fault_model"] == fault) & (data["configuration"] == key)]
            values.append(float(match.iloc[0][value_column]) if not match.empty else np.nan)
        offset = (index - (len(keys) - 1) / 2.0) * width
        ax.bar(
            x + offset,
            values,
            width=width * 0.94,
            label=CONFIG_LABELS[key],
            color=CONFIG_COLORS[key],
            edgecolor="white",
            linewidth=0.4,
        )
    ax.set_xticks(x, [FAULT_LABELS[fault] for fault in FAULT_ORDER])
    ax.set_xlabel("Fault Distribution")
    ax.set_ylabel(ylabel)
    ax.set_title(title)
    if ylim is not None:
        ax.set_ylim(*ylim)
    _grid_y(ax)
    ax.legend(ncols=min(3, len(keys)), loc="upper center", bbox_to_anchor=(0.5, -0.16))
    return fig


def _lines_by_fault(
    data: pd.DataFrame,
    value_column: str,
    keys: Sequence[str],
    ylabel: str,
    title: str,
    ylim: tuple[float, float] | None = None,
) -> plt.Figure:
    fig, ax = plt.subplots(figsize=(8.8, 4.8), constrained_layout=True)
    x = np.arange(len(FAULT_ORDER), dtype=float)
    for key in keys:
        values = []
        for fault in FAULT_ORDER:
            match = data[
                (data["fault_model"] == fault)
                & (data["configuration"] == key)
            ]
            values.append(
                float(match.iloc[0][value_column]) if not match.empty else np.nan
            )
        ax.plot(
            x,
            values,
            color=CONFIG_COLORS[key],
            marker=CONFIG_MARKERS.get(key, "o"),
            linewidth=1.8,
            markersize=5.5,
            label=CONFIG_LABELS[key],
        )
    ax.set_xticks(x, [FAULT_LABELS[fault] for fault in FAULT_ORDER])
    ax.set_xlabel("Fault Distribution")
    ax.set_ylabel(ylabel)
    ax.set_title(title)
    if ylim is not None:
        ax.set_ylim(*ylim)
    _grid_y(ax)
    ax.legend(ncols=min(3, len(keys)), loc="upper center", bbox_to_anchor=(0.5, -0.16))
    return fig


def plot_fig01(main: pd.DataFrame, context: PlotContext) -> None:
    keys = ["Base", "D1", "P1", "P2", "G1"]
    rows = _ordered_rows(main, keys)
    stem = "Fig01_repair_rate_vs_fault_distribution"
    save_plot_data(rows, stem, context)
    fig = _lines_by_fault(
        rows,
        "repair_rate_percent",
        keys,
        "Repair Rate (%)",
        "Repair Rate under Different Fault Distributions",
        (0.0, 100.0),
    )
    save_figure(fig, context.plots_dir / "core", stem, context)


def _policy_m_rows(data: pd.DataFrame, policy: str) -> pd.DataFrame:
    rows = data[
        (data["policy"] == policy)
        & (data["shared_rows"] == data["shared_columns"])
        & data["shared_rows"].isin([0, 1, 2])
    ].copy()
    rows["m"] = rows["shared_rows"].astype(int)
    return rows.sort_values(["fault_order", "m"])


def _plot_gain_lines(
    rows: pd.DataFrame,
    title: str,
    xlabel: str,
    categorical_labels: dict[int, str] | None = None,
) -> plt.Figure:
    fig, ax = plt.subplots(figsize=(7.3, 4.7), constrained_layout=True)
    for fault in FAULT_ORDER:
        group = rows[rows["fault_model"] == fault].sort_values("m")
        if group.empty:
            continue
        ax.plot(
            group["m"],
            group["sharing_gain_pp"],
            color=FAULT_COLORS[fault],
            marker="o",
            linewidth=1.8,
            markersize=5,
            label=FAULT_LABELS[fault],
        )
    ticks = sorted(int(value) for value in rows["m"].unique())
    ax.set_xticks(
        ticks,
        [categorical_labels.get(tick, str(tick)) for tick in ticks]
        if categorical_labels
        else [str(tick) for tick in ticks],
    )
    ax.axhline(0.0, color="0.35", linewidth=0.8)
    ax.set_xlabel(xlabel)
    ax.set_ylabel("Repair-Rate Improvement (percentage points)")
    ax.set_title(title)
    _grid_y(ax)
    ax.legend(title="Fault Distribution", ncols=2)
    return fig


def _marginal_gain_table(data: pd.DataFrame, context: PlotContext) -> pd.DataFrame:
    records: list[dict[str, object]] = []
    for policy in ("directional", "pairwise_edge", "global_pool"):
        rows = _policy_m_rows(data, policy)
        for fault in FAULT_ORDER:
            group = rows[rows["fault_model"] == fault].set_index("m")
            if not {0, 1}.issubset(group.index):
                continue
            m2_exists = 2 in group.index
            records.append(
                {
                    "fault_model": fault,
                    "fault_label": FAULT_LABELS[fault],
                    "policy": policy,
                    "repair_rate_m0": group.loc[0, "repair_rate"],
                    "repair_rate_m1": group.loc[1, "repair_rate"],
                    "repair_rate_m2": group.loc[2, "repair_rate"] if m2_exists else np.nan,
                    "gain_m1_pp": 100.0
                    * (group.loc[1, "repair_rate"] - group.loc[0, "repair_rate"]),
                    "gain_m2_incremental_pp": (
                        100.0
                        * (group.loc[2, "repair_rate"] - group.loc[1, "repair_rate"])
                        if m2_exists
                        else np.nan
                    ),
                    "m2_unverified": bool(
                        m2_exists
                        and policy == "global_pool"
                        and group.loc[2, "unverified_global_m2"]
                    ),
                    "m2_included_in_main": bool(
                        m2_exists
                        and (
                            policy != "global_pool"
                            or context.include_unverified_global_m2
                        )
                    ),
                    "global_m0_partition": (
                        group.loc[0, "global_partition"] if policy == "global_pool" else ""
                    ),
                    "global_m1_partition": (
                        group.loc[1, "global_partition"] if policy == "global_pool" else ""
                    ),
                    "global_m2_partition": (
                        group.loc[2, "global_partition"]
                        if policy == "global_pool" and m2_exists
                        else ""
                    ),
                }
            )
    return pd.DataFrame.from_records(records)


def plot_fig02(data: pd.DataFrame, context: PlotContext) -> None:
    specifications = [
        ("directional", "Fig02a_directional_gain_vs_m", "Directional"),
        ("pairwise_edge", "Fig02b_pairwise_gain_vs_m", "Pairwise"),
    ]
    for policy, stem, label in specifications:
        rows = _policy_m_rows(data, policy)
        save_plot_data(rows, stem, context)
        fig = _plot_gain_lines(
            rows,
            f"{label}: Repair Gain vs. Sharing Degree",
            "Shared Line Number m",
        )
        save_figure(fig, context.plots_dir / "core", stem, context)

    global_rows = _policy_m_rows(data, "global_pool")
    if not context.include_unverified_global_m2:
        global_rows = global_rows[~global_rows["unverified_global_m2"]].copy()
    partition_labels = {
        int(row.m): f"L{int(row.global_local_rows)}/G{int(row.global_rows)}"
        for row in global_rows.drop_duplicates("m").itertuples(index=False)
    }
    stem = "Fig02c_global_pool_gain_vs_m"
    save_plot_data(global_rows, stem, context)
    fig = _plot_gain_lines(
        global_rows,
        "Global Pool: Repair Gain vs. Local/Global Partition",
        "Row Partition (local per subarray / group-global)",
        partition_labels,
    )
    fig.axes[0].text(
        0.99,
        0.02,
        "L/G labels show row counts; column partitions are retained in plot data.",
        transform=fig.axes[0].transAxes,
        ha="right",
        va="bottom",
        fontsize=7.5,
        color="0.35",
    )
    save_figure(fig, context.plots_dir / "core", stem, context)

    marginal = _marginal_gain_table(data, context)
    save_plot_data(marginal, "Fig02_marginal_gain", context)


def pareto_analysis(
    data: pd.DataFrame,
    cost_column: str,
    benefit_column: str,
) -> pd.DataFrame:
    result = data.copy()
    optimal: list[bool] = []
    dominated_by: list[str] = []
    for index, row in result.iterrows():
        same_fault = result[result["fault_model"] == row["fault_model"]]
        valid = same_fault[[cost_column, benefit_column]].notna().all(axis=1)
        candidates = same_fault[valid]
        dominates = candidates[
            (candidates[cost_column] <= row[cost_column] + 1.0e-9)
            & (candidates[benefit_column] >= row[benefit_column] - 1.0e-9)
            & (
                (candidates[cost_column] < row[cost_column] - 1.0e-9)
                | (candidates[benefit_column] > row[benefit_column] + 1.0e-9)
            )
        ]
        names = [str(value) for value in dominates["configuration"].tolist()]
        optimal.append(len(names) == 0)
        dominated_by.append(";".join(names))
    result["pareto_optimal"] = optimal
    result["dominated_by"] = dominated_by
    return result


def _tradeoff_panels(
    rows: pd.DataFrame,
    x_column: str,
    xlabel: str,
    title: str,
    emphasize_pareto: bool = False,
) -> plt.Figure:
    fig, axes = plt.subplots(
        2, 2, figsize=(9.5, 7.2), sharex=False, sharey=True, constrained_layout=True
    )
    annotation_offsets = {
        "Base": (4, 4),
        "D1": (5, -13),
        "D2": (5, 5),
        "P1": (5, 5),
        "P2": (5, 5),
        "G1": (5, 7),
        "G2": (5, -13),
    }
    for ax, fault in zip(axes.flat, FAULT_ORDER):
        group = rows[rows["fault_model"] == fault]
        for _, row in group.iterrows():
            is_optimal = bool(row.get("pareto_optimal", False))
            ax.scatter(
                row[x_column],
                row["sharing_gain_pp"],
                s=58 if is_optimal and emphasize_pareto else 44,
                color=CONFIG_COLORS[row["configuration"]],
                edgecolor="black" if is_optimal and emphasize_pareto else "white",
                linewidth=1.1 if is_optimal and emphasize_pareto else 0.5,
                zorder=3,
            )
            ax.annotate(
                row["configuration"],
                (row[x_column], row["sharing_gain_pp"]),
                xytext=annotation_offsets[row["configuration"]],
                textcoords="offset points",
                fontsize=8,
            )
        ax.axhline(0.0, color="0.4", linewidth=0.7)
        ax.set_title(FAULT_LABELS[fault])
        ax.set_xlabel(xlabel)
        _grid_y(ax)
    fig.supylabel("Repair-Rate Improvement (percentage points)")
    fig.suptitle(title)
    fig.text(
        0.5,
        -0.015,
        "Base = No Sharing; D = Directional; P = Pairwise; G = Global Pool",
        ha="center",
        fontsize=8,
        color="0.3",
    )
    return fig


def plot_fig03(main: pd.DataFrame, context: PlotContext) -> None:
    keys = CONFIG_ORDER if context.include_unverified_global_m2 else CONFIG_ORDER[:-1]
    rows = _ordered_rows(main, keys)
    rows = pareto_analysis(
        rows, "peak_hardware_cost_proxy_bits", "sharing_gain_pp"
    )
    stem = "Fig03_repair_gain_vs_hardware_cost"
    save_plot_data(rows, stem, context)
    pareto_columns = {
        "fault_model": "fault_model",
        "configuration_label": "configuration",
        "sharing_gain_pp": "repair_gain_pp",
        "peak_hardware_cost_proxy_bits": "hardware_cost",
        "pareto_optimal": "pareto_optimal",
        "dominated_by": "dominated_by",
    }
    pareto = rows[list(pareto_columns)].rename(columns=pareto_columns)
    save_plot_data(pareto, "Fig03_pareto_analysis", context)
    fig = _tradeoff_panels(
        rows,
        "peak_hardware_cost_proxy_bits",
        "Peak Hardware Cost Proxy (bits)",
        "Repair Gain vs. Provisioned Hardware Cost",
        emphasize_pareto=True,
    )
    save_figure(fig, context.plots_dir / "core", stem, context)


def _plot_cycle_tradeoff(
    main: pd.DataFrame,
    metric: str,
    xlabel: str,
    stem: str,
    directory: Path,
    context: PlotContext,
) -> None:
    keys = CONFIG_ORDER if context.include_unverified_global_m2 else CONFIG_ORDER[:-1]
    rows = _ordered_rows(main, keys)
    save_plot_data(rows, stem, context)
    fig = _tradeoff_panels(rows, metric, xlabel, f"Repair Gain vs. {xlabel}")
    save_figure(fig, directory, stem, context)


def plot_fig04(main: pd.DataFrame, context: PlotContext) -> None:
    _plot_cycle_tradeoff(
        main,
        "average_analysis_cycles",
        "Average Analysis Cycles",
        "Fig04_repair_gain_vs_analysis_cycles",
        context.plots_dir / "core",
        context,
    )
    _plot_cycle_tradeoff(
        main,
        "p95_analysis_cycles",
        "P95 Analysis Cycles",
        "Fig04b_repair_gain_vs_p95_cycles",
        context.plots_dir / "appendix",
        context,
    )


def _plot_policy_metric_vs_m(
    data: pd.DataFrame,
    policy: str,
    metric: str,
    ylabel: str,
    title: str,
) -> tuple[pd.DataFrame, plt.Figure]:
    rows = _policy_m_rows(data, policy)
    fig, ax = plt.subplots(figsize=(7.3, 4.7), constrained_layout=True)
    for fault in FAULT_ORDER:
        group = rows[rows["fault_model"] == fault].sort_values("m")
        ax.plot(
            group["m"],
            group[metric],
            marker="o",
            linewidth=1.8,
            markersize=5,
            color=FAULT_COLORS[fault],
            label=FAULT_LABELS[fault],
        )
    ax.set_xticks([0, 1, 2])
    ax.set_xlabel("Shared Line Number m")
    ax.set_ylabel(ylabel)
    ax.set_title(title)
    ax.set_ylim(bottom=0.0)
    _grid_y(ax)
    ax.legend(title="Fault Distribution", ncols=2)
    return rows, fig


def plot_fig05_and_fig06(data: pd.DataFrame, context: PlotContext) -> None:
    specs = [
        (
            "average_candidate_count",
            "Average Candidate Solutions Evaluated",
            "Pairwise Candidate Search Space vs. Sharing Degree",
            "Fig05_candidate_count_vs_m",
            "complexity",
        ),
        (
            "average_analysis_attempts",
            "Average Analysis Attempts",
            "Pairwise Analysis Attempts vs. Sharing Degree",
            "Fig05b_analysis_attempts_vs_m",
            "complexity",
        ),
        (
            "average_analysis_cycles",
            "Average Analysis Cycles",
            "Pairwise Analysis Cycles vs. Sharing Degree",
            "Fig06_analysis_cycles_vs_m",
            "complexity",
        ),
        (
            "p95_analysis_cycles",
            "P95 Analysis Cycles",
            "Pairwise P95 Analysis Cycles vs. Sharing Degree",
            "Fig06b_p95_analysis_cycles_vs_m",
            "appendix",
        ),
    ]
    for metric, ylabel, title, stem, subdirectory in specs:
        rows, fig = _plot_policy_metric_vs_m(
            data, "pairwise_edge", metric, ylabel, title
        )
        save_plot_data(rows, stem, context)
        save_figure(fig, context.plots_dir / subdirectory, stem, context)


def _plot_two_policy_metric(
    data: pd.DataFrame,
    metric: str,
    ylabel: str,
    title: str,
) -> tuple[pd.DataFrame, plt.Figure]:
    policies = [("directional", "Directional"), ("pairwise_edge", "Pairwise")]
    rows = pd.concat([_policy_m_rows(data, policy) for policy, _ in policies])
    fig, axes = plt.subplots(1, 2, figsize=(10.0, 4.4), sharey=True)
    for ax, (policy, label) in zip(axes, policies):
        policy_rows = rows[rows["policy"] == policy]
        for fault in FAULT_ORDER:
            group = policy_rows[policy_rows["fault_model"] == fault].sort_values("m")
            ax.plot(
                group["m"],
                group[metric],
                marker="o",
                linewidth=1.7,
                markersize=4.5,
                color=FAULT_COLORS[fault],
                label=FAULT_LABELS[fault],
            )
        ax.set_title(label)
        ax.set_xlabel("Shared Line Number m")
        ax.set_xticks([0, 1, 2])
        _grid_y(ax)
    axes[0].set_ylabel(ylabel)
    handles, labels = axes[0].get_legend_handles_labels()
    fig.legend(
        handles,
        labels,
        loc="lower center",
        bbox_to_anchor=(0.5, 0.01),
        ncols=4,
    )
    fig.suptitle(title, y=0.97)
    fig.subplots_adjust(left=0.08, right=0.99, bottom=0.20, top=0.80, wspace=0.04)
    return rows.sort_values(["policy", "fault_order", "m"]), fig


def plot_fig07_and_fig08(data: pd.DataFrame, context: PlotContext) -> None:
    specs = [
        (
            "peak_matrix_size",
            "Peak Matrix Size (cells)",
            "Peak Repair-Matrix Size vs. Sharing Degree",
            "Fig07_matrix_size_vs_m",
            "complexity",
        ),
        (
            "average_matrix_size",
            "Average Matrix Size (cells)",
            "Average Repair-Matrix Size vs. Sharing Degree",
            "Fig07b_average_matrix_size_vs_m",
            "appendix",
        ),
        (
            "peak_CAM_requirement",
            "Peak CAM Requirement (entries)",
            "Peak CAM Requirement vs. Sharing Degree",
            "Fig08_CAM_requirement_vs_m",
            "complexity",
        ),
        (
            "average_CAM_requirement",
            "Average Active CAM Requirement (entries)",
            "Average Active CAM Requirement vs. Sharing Degree",
            "Fig08b_average_CAM_requirement_vs_m",
            "appendix",
        ),
    ]
    for metric, ylabel, title, stem, subdirectory in specs:
        rows, fig = _plot_two_policy_metric(data, metric, ylabel, title)
        save_plot_data(rows, stem, context)
        save_figure(fig, context.plots_dir / subdirectory, stem, context)


def _baseline_rows(data: pd.DataFrame) -> pd.DataFrame:
    baseline = data[data["configuration"] == "Base"].copy()
    duplicates = baseline.duplicated("fault_model", keep=False)
    if duplicates.any():
        raise ValueError("More than one No Sharing baseline exists for a fault model")
    return baseline


def compute_efficiencies(
    main: pd.DataFrame, context: PlotContext
) -> tuple[pd.DataFrame, pd.DataFrame]:
    baseline = _baseline_rows(main)[
        [
            "fault_model",
            "repair_rate",
            "peak_hardware_cost_proxy_bits",
            "average_analysis_cycles",
        ]
    ].rename(
        columns={
            "repair_rate": "baseline_repair_rate_lookup",
            "peak_hardware_cost_proxy_bits": "baseline_peak_hardware_cost",
            "average_analysis_cycles": "baseline_average_analysis_cycles",
        }
    )
    keys = ["D1", "D2", "P1", "P2", "G1"]
    if context.include_unverified_global_m2:
        keys.append("G2")
    rows = _ordered_rows(main, keys).merge(baseline, on="fault_model", how="left")
    rows["repair_gain_pp_from_baseline"] = 100.0 * (
        rows["repair_rate"] - rows["baseline_repair_rate_lookup"]
    )

    hardware = rows.copy()
    hardware["delta_hardware_bits"] = (
        hardware["peak_hardware_cost_proxy_bits"]
        - hardware["baseline_peak_hardware_cost"]
    )
    hardware["hardware_efficiency_pp_per_1000_bits"] = np.where(
        hardware["delta_hardware_bits"] > 0,
        1000.0
        * hardware["repair_gain_pp_from_baseline"]
        / hardware["delta_hardware_bits"],
        np.nan,
    )
    hardware["efficiency_status"] = np.where(
        hardware["delta_hardware_bits"] > 0, "computed", "nonpositive_delta_hw"
    )

    latency = rows.copy()
    latency["delta_analysis_cycles"] = (
        latency["average_analysis_cycles"]
        - latency["baseline_average_analysis_cycles"]
    )
    latency["latency_efficiency_pp_per_100_cycles"] = np.where(
        latency["delta_analysis_cycles"] > 0,
        100.0
        * latency["repair_gain_pp_from_baseline"]
        / latency["delta_analysis_cycles"],
        np.nan,
    )
    latency["efficiency_status"] = np.where(
        latency["delta_analysis_cycles"] > 0,
        "computed",
        "nonpositive_delta_cycles",
    )
    return hardware, latency


def _efficiency_panels(
    data: pd.DataFrame,
    metric: str,
    ylabel: str,
    title: str,
) -> plt.Figure:
    fig, axes = plt.subplots(
        2, 2, figsize=(10.0, 7.1), sharey=True, constrained_layout=True
    )
    keys = [key for key in CONFIG_ORDER if key in set(data["configuration"])]
    for ax, fault in zip(axes.flat, FAULT_ORDER):
        group = data[data["fault_model"] == fault].set_index("configuration")
        values = [group.loc[key, metric] if key in group.index else np.nan for key in keys]
        x = np.arange(len(keys))
        ax.bar(
            x,
            values,
            color=[CONFIG_COLORS[key] for key in keys],
            edgecolor="white",
        )
        ax.set_xticks(x, keys)
        ax.axhline(0.0, color="0.35", linewidth=0.8)
        ax.set_title(FAULT_LABELS[fault])
        _grid_y(ax)
    fig.supylabel(ylabel)
    fig.suptitle(title)
    return fig


def plot_fig09_and_fig10(main: pd.DataFrame, context: PlotContext) -> None:
    hardware, latency = compute_efficiencies(main, context)

    stem = "Fig09_repair_gain_per_hardware_cost"
    save_plot_data(hardware, stem, context)
    fig = _efficiency_panels(
        hardware,
        "hardware_efficiency_pp_per_1000_bits",
        "Repair Gain (pp) per 1000 Additional Hardware Bits",
        "Hardware Efficiency of Spare Sharing",
    )
    save_figure(fig, context.plots_dir / "efficiency", stem, context)

    stem = "Fig10_repair_gain_per_analysis_cost"
    save_plot_data(latency, stem, context)
    fig = _efficiency_panels(
        latency,
        "latency_efficiency_pp_per_100_cycles",
        "Repair Gain (pp) per 100 Additional Analysis Cycles",
        "Latency Efficiency of Spare Sharing",
    )
    save_figure(fig, context.plots_dir / "efficiency", stem, context)


def plot_fig11_and_fig12(main: pd.DataFrame, context: PlotContext) -> None:
    keys = ["Base", "D1", "P1", "P2", "G1"]
    if context.include_unverified_global_m2:
        keys.append("G2")
    rows = _ordered_rows(main, keys)

    stem = "Fig11_spare_utilization"
    save_plot_data(rows, stem, context)
    fig = _grouped_bars_by_fault(
        rows,
        "spare_utilization_percent",
        keys,
        "Average Spare Utilization (%)",
        "Spare Utilization under Different Fault Distributions",
        (0.0, 100.0),
    )
    save_figure(fig, context.plots_dir / "core", stem, context)

    stem = "Fig12_unused_spare_fraction"
    save_plot_data(rows, stem, context)
    fig = _grouped_bars_by_fault(
        rows,
        "unused_spare_fraction_percent",
        keys,
        "Unused Spare Fraction (%)",
        "Residual Spare Capacity",
        (0.0, 100.0),
    )
    save_figure(fig, context.plots_dir / "appendix", stem, context)


def plot_fig13(main: pd.DataFrame, context: PlotContext) -> None:
    keys = ["D1", "P1", "P2", "G1"]
    if context.include_unverified_global_m2:
        keys.append("G2")
    rows = _ordered_rows(main, keys)
    specifications = [
        (
            "average_borrow_count",
            "Average Borrow Count",
            "Borrow Frequency under Different Fault Distributions",
            "Fig13_borrow_count",
        ),
        (
            "p95_borrow_count",
            "P95 Borrow Count",
            "Tail Borrow Count under Different Fault Distributions",
            "Fig13b_p95_borrow_count",
        ),
    ]
    for metric, ylabel, title, stem in specifications:
        save_plot_data(rows, stem, context)
        fig = _grouped_bars_by_fault(rows, metric, keys, ylabel, title)
        save_figure(fig, context.plots_dir / "appendix", stem, context)


def _plot_final_tradeoff(
    rows: pd.DataFrame, fault: str, stem: str, context: PlotContext
) -> None:
    group = rows[rows["fault_model"] == fault].copy()
    group = pareto_analysis(
        group, "peak_hardware_cost_proxy_bits", "repair_rate_percent"
    )
    save_plot_data(group, stem, context)
    fig, ax = plt.subplots(figsize=(7.7, 5.0), constrained_layout=True)
    frontier = group[group["pareto_optimal"]].sort_values(
        ["peak_hardware_cost_proxy_bits", "repair_rate_percent"]
    )
    if len(frontier) >= 2:
        ax.plot(
            frontier["peak_hardware_cost_proxy_bits"],
            frontier["repair_rate_percent"],
            color="0.25",
            linestyle="--",
            linewidth=1.0,
            label="Pareto frontier",
            zorder=1,
        )
    annotation_offsets = {
        "Base": (5, 5),
        "D1": (6, -14),
        "D2": (6, 6),
        "P1": (6, 6),
        "P2": (6, 6),
        "G1": (6, 8),
        "G2": (6, -14),
    }
    for _, row in group.iterrows():
        optimal = bool(row["pareto_optimal"])
        ax.scatter(
            row["peak_hardware_cost_proxy_bits"],
            row["repair_rate_percent"],
            color=CONFIG_COLORS[row["configuration"]],
            s=68 if optimal else 48,
            edgecolor="black" if optimal else "white",
            linewidth=1.1 if optimal else 0.5,
            zorder=3,
        )
        ax.annotate(
            f"{row['configuration']}  {row['average_analysis_cycles']:.0f} cyc",
            (row["peak_hardware_cost_proxy_bits"], row["repair_rate_percent"]),
            xytext=annotation_offsets[row["configuration"]],
            textcoords="offset points",
            fontsize=8,
        )
    ax.set_xlabel("Peak Hardware Cost Proxy (bits)")
    ax.set_ylabel("Repair Rate (%)")
    ax.set_title(f"Resource-Constrained Tradeoff: {FAULT_LABELS[fault]} Imbalance")
    ax.margins(x=0.08, y=0.15)
    ax.set_ylim(bottom=0.0)
    _grid_y(ax)
    if len(frontier) >= 2:
        ax.legend(loc="best")
    ax.text(
        0.99,
        0.02,
        "Labels report average analysis cycles",
        transform=ax.transAxes,
        ha="right",
        fontsize=7.5,
        color="0.35",
    )
    save_figure(fig, context.plots_dir / "core", stem, context)


def plot_fig14(main: pd.DataFrame, context: PlotContext) -> None:
    keys = CONFIG_ORDER if context.include_unverified_global_m2 else CONFIG_ORDER[:-1]
    rows = _ordered_rows(main, keys)
    _plot_final_tradeoff(
        rows, "moderate_imbalance", "Fig14a_moderate_tradeoff", context
    )
    _plot_final_tradeoff(rows, "strong_imbalance", "Fig14b_strong_tradeoff", context)


def generate_summary_tables(main: pd.DataFrame, context: PlotContext) -> None:
    keys = CONFIG_ORDER if context.include_unverified_global_m2 else CONFIG_ORDER[:-1]
    rows = _ordered_rows(main, keys)
    table1 = pd.DataFrame(
        {
            "Fault Model": rows["fault_label"],
            "Policy": rows["configuration_label"],
            "m": rows["m"],
            "Repair Rate (%)": rows["repair_rate_percent"],
            "Repair Gain (pp)": rows["sharing_gain_pp"],
            "Peak CAM": rows["peak_CAM_requirement"],
            "Peak Matrix": rows["peak_matrix_size"],
            "Peak HW Cost (bits)": rows["peak_hardware_cost_proxy_bits"],
            "Avg Candidates": rows["average_candidate_count"],
            "Avg Analysis Cycles": rows["average_analysis_cycles"],
            "P95 Analysis Cycles": rows["p95_analysis_cycles"],
            "Avg Spare Utilization (%)": rows["spare_utilization_percent"],
            "Avg Borrow Count": rows["average_borrow_count"],
        }
    )
    save_plot_data(table1, "Table01_main_results", context)

    baseline = _baseline_rows(main)[
        [
            "fault_model",
            "peak_hardware_cost_proxy_bits",
            "average_analysis_cycles",
            "average_candidate_count",
            "peak_matrix_size",
        ]
    ].rename(
        columns={
            "peak_hardware_cost_proxy_bits": "baseline_hw",
            "average_analysis_cycles": "baseline_cycles",
            "average_candidate_count": "baseline_candidates",
            "peak_matrix_size": "baseline_matrix",
        }
    )
    relative = rows.merge(baseline, on="fault_model", how="left")
    table2 = pd.DataFrame(
        {
            "Fault Model": relative["fault_label"],
            "Policy": relative["configuration_label"],
            "m": relative["m"],
            "Repair Gain (pp)": relative["sharing_gain_pp"],
            "Hardware Multiplier": relative["peak_hardware_cost_proxy_bits"]
            / relative["baseline_hw"],
            "Analysis Cycle Multiplier": relative["average_analysis_cycles"]
            / relative["baseline_cycles"],
            "Candidate Multiplier": relative["average_candidate_count"]
            / relative["baseline_candidates"],
            "Matrix Multiplier": relative["peak_matrix_size"]
            / relative["baseline_matrix"],
        }
    )
    save_plot_data(table2, "Table02_relative_cost", context)


def generate_global_m2_diagnostic(data: pd.DataFrame, context: PlotContext) -> None:
    suspicious = data[data["unverified_global_m2"]].copy()
    if suspicious.empty:
        context.warn("unverified Global Pool local=0/global=8 rows were not found")
        return
    baseline = _baseline_rows(data)[["fault_model", "repair_rate"]].rename(
        columns={"repair_rate": "no_sharing_repair_rate"}
    )
    diagnostic = suspicious.merge(baseline, on="fault_model", how="left")
    diagnostic["global_m2_repair_rate_percent"] = diagnostic["repair_rate"] * 100.0
    diagnostic["no_sharing_repair_rate_percent"] = (
        diagnostic["no_sharing_repair_rate"] * 100.0
    )
    diagnostic = diagnostic.sort_values("fault_order")
    stem = "global_pool_m2_diagnostic"
    save_plot_data(diagnostic, stem, context)

    fig, ax = plt.subplots(figsize=(7.8, 4.8), constrained_layout=True)
    x = np.arange(len(FAULT_ORDER))
    width = 0.36
    ax.bar(
        x - width / 2,
        diagnostic["no_sharing_repair_rate_percent"],
        width,
        color=CONFIG_COLORS["Base"],
        label="No Sharing",
    )
    ax.bar(
        x + width / 2,
        diagnostic["global_m2_repair_rate_percent"],
        width,
        color=CONFIG_COLORS["G2"],
        label="Global Pool m=2 (unverified)",
    )
    ax.set_xticks(x, [FAULT_LABELS[fault] for fault in FAULT_ORDER])
    ax.set_xlabel("Fault Distribution")
    ax.set_ylabel("Repair Rate (%)")
    ax.set_ylim(0.0, 100.0)
    ax.set_title("Diagnostic: Fully Global local=0 / global=8 Configuration")
    _grid_y(ax)
    ax.legend()
    ax.text(
        0.5,
        0.97,
        "Do not interpret as architecture evidence until implementation is verified.",
        transform=ax.transAxes,
        ha="center",
        va="top",
        fontsize=8,
        color="#9C2F2F",
    )
    save_figure(fig, context.plots_dir / "diagnostic", stem, context)


def generate_findings(main: pd.DataFrame, context: PlotContext) -> None:
    lines = [
        "Data-derived observations",
        "=========================",
        "",
        "Values below are calculated from the selected summary.csv; they are not assumed conclusions.",
        "",
    ]
    for fault in FAULT_ORDER:
        group = main[main["fault_model"] == fault]
        if group.empty:
            continue
        best = group.loc[group["repair_rate"].idxmax()]
        lines.append(
            f"- {FAULT_LABELS[fault]}: best included configuration is "
            f"{best['configuration_label']} at {best['repair_rate_percent']:.2f}% "
            f"repair rate ({best['sharing_gain_pp']:+.2f} pp vs baseline)."
        )
    lines.extend(
        [
            "",
            "The suspicious Global Pool local=0/global=8 configuration is excluded from these statements "
            f"(INCLUDE_UNVERIFIED_GLOBAL_M2={context.include_unverified_global_m2}).",
        ]
    )
    target = context.plots_dir / "diagnostic" / "analysis_findings.txt"
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text("\n".join(lines) + "\n", encoding="utf-8")


def generate_validation_report(
    csv_file: Path,
    raw: pd.DataFrame,
    selected: pd.DataFrame,
    context: PlotContext,
    rs: int,
    cs: int,
    storage_mode: str,
) -> None:
    report = [
        "RECAM plotting data validation",
        "==============================",
        "",
        f"Input: {csv_file}",
        f"Rows loaded: {len(raw)}",
        f"Rows selected: {len(selected)}",
        f"Filter: Rs={rs}, Cs={cs}, storage_mode={storage_mode}",
        f"INCLUDE_UNVERIFIED_GLOBAL_M2={context.include_unverified_global_m2}",
        f"Generated figure groups: {len(context.figure_stems)}",
        "",
        "Checks performed:",
        "- repair_rate and baseline_repair_rate in [0,1]",
        "- sharing_gain equals repair_rate - baseline_repair_rate",
        "- Rs and Cs are positive",
        "- average analysis cycles are non-negative",
        "- peak CAM/matrix values are no smaller than averages",
        "- duplicate complete configurations",
        "",
        "Warnings:",
    ]
    if context.warnings:
        report.extend(f"- {warning}" for warning in context.warnings)
    else:
        report.append("- None")
    report.extend(
        [
            "",
            "Global Pool m=2 handling:",
            "- local_rows=local_columns=0 and global_rows=global_columns=8 is always retained in the diagnostic output.",
            "- It is excluded from main plots by default and must not be used as evidence before simulator verification.",
        ]
    )
    target = context.plots_dir / "diagnostic" / "data_validation.txt"
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text("\n".join(report) + "\n", encoding="utf-8")


def generate_future_todo(context: PlotContext) -> None:
    content = """# Future experiment plots (data not currently available)

No placeholder values or interpolated measurements are generated.

## Required simulator sweeps/metrics

- The current fixed-condition input does not contain a `fault_number` × `Rs=Cs` sweep. Generate it separately with `make analyze_dynamic_spare_sharing`; that workflow now creates repair-rate, absolute-gain, and relative-gain heatmaps.
- Repair rate vs. fault number is generated by the same sweep workflow. Use at least `Rs=Cs=2` and `Rs=Cs=3` for the recommended comparison.
- Residual spare count after successful repair requires `avg_unused_rows_success`, `avg_unused_columns_success`, and `avg_unused_spares_success`.
- Local failure despite available group spare requires `local_failure_with_group_spare_remaining` and `total_local_failures`.
- Continuous imbalance analysis requires per-map `imbalance_cv = std([FA,FB,FC,FD]) / mean([FA,FB,FC,FD])` when the mean is positive.
- CAM vs. SRAM comparisons require matching `storage_mode=sram` experiments.  The current data contains CAM only.

When these data become available, keep the same validation, percentage-point terminology, provisioning-vs-active distinction, and per-figure CSV export used by `plot_results.py`.
"""
    context.plots_dir.mkdir(parents=True, exist_ok=True)
    (context.plots_dir / "FUTURE_EXPERIMENTS_TODO.md").write_text(
        content, encoding="utf-8"
    )


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Generate RECAM dynamic spare-sharing research plots."
    )
    parser.add_argument(
        "summary_csv",
        nargs="?",
        type=Path,
        default=DEFAULT_SUMMARY_CSV,
        help=f"input summary.csv (default: {DEFAULT_SUMMARY_CSV})",
    )
    parser.add_argument(
        "--plots-dir",
        type=Path,
        default=DEFAULT_PLOTS_DIR,
        help=f"figure output root (default: {DEFAULT_PLOTS_DIR})",
    )
    parser.add_argument(
        "--plot-data-dir",
        type=Path,
        default=DEFAULT_PLOT_DATA_DIR,
        help=f"processed CSV output root (default: {DEFAULT_PLOT_DATA_DIR})",
    )
    parser.add_argument("--rs", type=int, default=2, help="row spare count filter")
    parser.add_argument("--cs", type=int, default=2, help="column spare count filter")
    parser.add_argument(
        "--storage-mode", default="cam", help="storage-mode filter (default: cam)"
    )
    parser.add_argument(
        "--include-unverified-global-m2",
        action="store_true",
        default=INCLUDE_UNVERIFIED_GLOBAL_M2,
        help="include the suspicious local=0/global=8 configuration in main plots",
    )
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    csv_file = args.summary_csv.expanduser().resolve()
    context = PlotContext(
        plots_dir=args.plots_dir.expanduser().resolve(),
        data_dir=args.plot_data_dir.expanduser().resolve(),
        include_unverified_global_m2=args.include_unverified_global_m2,
    )

    configure_plot_style()
    raw = load_data(csv_file)
    validate_data(raw, context)
    selected = select_experiment(
        raw, args.rs, args.cs, args.storage_mode, context
    )
    prepared = prepare_labels(selected)
    main_configs = filter_main_configs(prepared, context)
    save_plot_data(prepared, "processed_summary", context)

    plot_fig01(main_configs, context)
    plot_fig02(prepared, context)
    plot_fig03(main_configs, context)
    plot_fig04(main_configs, context)
    plot_fig05_and_fig06(prepared, context)
    plot_fig07_and_fig08(prepared, context)
    plot_fig09_and_fig10(main_configs, context)
    plot_fig11_and_fig12(main_configs, context)
    plot_fig13(main_configs, context)
    plot_fig14(main_configs, context)
    generate_summary_tables(main_configs, context)
    generate_global_m2_diagnostic(prepared, context)
    generate_findings(main_configs, context)
    generate_future_todo(context)
    generate_validation_report(
        csv_file,
        raw,
        prepared,
        context,
        args.rs,
        args.cs,
        args.storage_mode,
    )

    print(f"Generated {len(context.figure_stems)} figures as PNG and PDF.")
    print(f"Figures: {context.plots_dir}")
    print(f"Processed data: {context.data_dir}")


if __name__ == "__main__":
    main()
