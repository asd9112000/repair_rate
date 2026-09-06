#!/usr/bin/env python3
"""Plot group-level fault-model versus sharing-policy galleries from CSVs."""

from __future__ import annotations

import argparse
import os
import tempfile
from pathlib import Path

os.environ.setdefault(
    "MPLCONFIGDIR", str(Path(tempfile.gettempdir()) / "recam-matplotlib-cache")
)

import matplotlib

matplotlib.use("Agg")

import matplotlib.pyplot as plt
import pandas as pd


FAULT_ORDER = [
    "uniform",
    "moderate_imbalance",
    "strong_imbalance",
    "hotspot",
]
FAULT_LABELS = {
    "uniform": "Uniform",
    "moderate_imbalance": "Moderate",
    "strong_imbalance": "Strong",
    "hotspot": "Hotspot",
}
POLICY_ORDER = [
    "No Sharing",
    "Directional m=1",
    "Directional m=2",
    "Pairwise m=1",
    "Pairwise m=2",
    "Global Pool m=1",
    "Global Pool m=2",
]
POLICY_PREFIXES = {
    "directional": "Directional",
    "pairwise_edge": "Pairwise",
    "global_pool": "Global Pool",
}
POLICY_DISPLAY_LABELS = {
    "No Sharing": "No\nSharing",
    "Directional m=1": "Directional\nm=1",
    "Directional m=2": "Directional\nm=2",
    "Pairwise m=1": "Pairwise\nm=1",
    "Pairwise m=2": "Pairwise\nm=2",
    "Global Pool m=1": "Global Pool\nm=1",
    "Global Pool m=2": "Global Pool\nm=2*",
}
UNVERIFIED_POLICIES = frozenset({"Global Pool m=2"})
POLICY_COLORS = {
    "No Sharing": "#6B7280",
    "Directional m=1": "#4C78A8",
    "Directional m=2": "#72B7B2",
    "Pairwise m=1": "#F58518",
    "Pairwise m=2": "#E45756",
    "Global Pool m=1": "#54A24B",
    "Global Pool m=2": "#B279A2",
}
POLICY_MARKERS = {
    "No Sharing": "o",
    "Directional m=1": "s",
    "Directional m=2": "D",
    "Pairwise m=1": "^",
    "Pairwise m=2": "v",
    "Global Pool m=1": "P",
    "Global Pool m=2": "X",
}
REQUIRED_COLUMNS = {
    "seed",
    "fault_count",
    "fault_model",
    "policy",
    "storage_mode",
    "Rs",
    "Cs",
    "shared_rows",
    "shared_columns",
    "runs",
    "repair_rate",
}


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=(
            "Create fault-model tables and fault-count curves for every supported "
            "sharing policy."
        )
    )
    parser.add_argument(
        "inputs",
        nargs="+",
        type=Path,
        help="summary.csv files or directories searched recursively",
    )
    parser.add_argument("--output-dir", required=True, type=Path)
    parser.add_argument("--storage-mode", choices=("cam", "sram"), default="cam")
    return parser.parse_args()


def find_summaries(inputs: list[Path]) -> list[Path]:
    summaries: set[Path] = set()
    for item in inputs:
        path = item.expanduser().resolve()
        if path.is_file():
            summaries.add(path)
        elif path.is_dir():
            summaries.update(path.rglob("summary.csv"))
        else:
            raise FileNotFoundError(f"Input does not exist: {path}")
    if not summaries:
        raise FileNotFoundError("No summary.csv files were found")
    return sorted(summaries)


def policy_label(row: pd.Series) -> str | None:
    if row["policy"] == "no_sharing":
        return "No Sharing"
    prefix = POLICY_PREFIXES.get(row["policy"])
    if prefix is None:
        return None
    if row["shared_rows"] != row["shared_columns"]:
        return None
    shared = int(row["shared_rows"])
    return f"{prefix} m={shared}" if shared in (1, 2) else None


def load_selected(paths: list[Path], storage_mode: str) -> pd.DataFrame:
    frames: list[pd.DataFrame] = []
    for path in paths:
        frame = pd.read_csv(path)
        missing = sorted(REQUIRED_COLUMNS - set(frame.columns))
        if missing:
            raise ValueError(f"{path} is missing columns: {', '.join(missing)}")
        frame = frame.copy()
        frame["source_csv"] = str(path)
        frames.append(frame)

    data = pd.concat(frames, ignore_index=True)
    data = data[data["storage_mode"].astype(str).str.lower() == storage_mode]
    data["policy_label"] = data.apply(policy_label, axis=1)
    data = data[data["policy_label"].notna()].copy()
    data = data[data["fault_model"].isin(FAULT_ORDER)].copy()
    if data.empty:
        raise ValueError("No supported sharing-policy rows were found")

    data["validation_status"] = data["policy_label"].map(
        lambda label: "unverified diagnostic"
        if label in UNVERIFIED_POLICIES
        else "verified"
    )

    integer_columns = [
        "seed",
        "fault_count",
        "Rs",
        "Cs",
        "shared_rows",
        "shared_columns",
        "runs",
    ]
    for column in integer_columns:
        data[column] = pd.to_numeric(data[column], errors="raise").astype(int)
    data["repair_rate"] = pd.to_numeric(data["repair_rate"], errors="raise")
    if (~data["repair_rate"].between(0.0, 1.0)).any():
        raise ValueError("repair_rate must be within [0, 1]")

    keys = ["Rs", "Cs", "fault_count", "fault_model", "policy_label"]
    duplicates = data.duplicated(keys, keep=False)
    if duplicates.any():
        preview = data.loc[duplicates, keys].drop_duplicates().head(8)
        raise ValueError(
            "Duplicate experiment cells found; aggregate seeds separately:\n"
            + preview.to_string(index=False)
        )
    return data.sort_values(keys).reset_index(drop=True)


def case_pivot(group: pd.DataFrame) -> pd.DataFrame:
    pivot = group.pivot(
        index="fault_model", columns="policy_label", values="repair_rate"
    ).reindex(index=FAULT_ORDER, columns=POLICY_ORDER)
    missing = [
        f"{FAULT_LABELS[model]}/{policy}"
        for model in FAULT_ORDER
        for policy in POLICY_ORDER
        if pd.isna(pivot.loc[model, policy])
    ]
    if missing:
        raise ValueError("Incomplete table cells: " + ", ".join(missing))
    pivot.index = [FAULT_LABELS[value] for value in pivot.index]
    return pivot


def save_table_figure(
    pivot: pd.DataFrame,
    rs: int,
    cs: int,
    fault_count: int,
    runs: int,
    seed: int,
    output_dir: Path,
) -> None:
    cell_text = [
        [model] + [f"{100.0 * value:.2f}%" for value in row]
        for model, row in pivot.iterrows()
    ]
    column_labels = ["Fault Model"] + [
        POLICY_DISPLAY_LABELS[policy] for policy in POLICY_ORDER
    ]
    fig, ax = plt.subplots(figsize=(15.8, 3.9))
    ax.axis("off")
    table = ax.table(
        cellText=cell_text,
        colLabels=column_labels,
        cellLoc="center",
        colLoc="center",
        colWidths=[0.17] + [0.118] * len(POLICY_ORDER),
        bbox=[0.015, 0.14, 0.97, 0.68],
    )
    table.auto_set_font_size(False)
    table.set_fontsize(10)
    table.scale(1.0, 1.45)

    for column in range(len(column_labels)):
        cell = table[(0, column)]
        policy = POLICY_ORDER[column - 1] if column else None
        cell.set_facecolor(
            "#9A6700" if policy in UNVERIFIED_POLICIES else "#34495E"
        )
        cell.set_text_props(color="white", weight="bold")
    for row_index, (_, row) in enumerate(pivot.iterrows(), start=1):
        table[(row_index, 0)].set_facecolor("#EAF0F6")
        table[(row_index, 0)].set_text_props(weight="bold")
        verified = row.drop(labels=list(UNVERIFIED_POLICIES), errors="ignore")
        best = verified.max()
        for column_index, (policy, value) in enumerate(row.items(), start=1):
            cell = table[(row_index, column_index)]
            is_best = policy not in UNVERIFIED_POLICIES and value == best
            if is_best:
                cell.set_facecolor("#E8F5E9")
            elif policy in UNVERIFIED_POLICIES:
                cell.set_facecolor("#FFF4CE")
            else:
                cell.set_facecolor("white")
            if is_best:
                cell.set_text_props(weight="bold", color="#176B35")
    for cell in table.get_celld().values():
        cell.set_edgecolor("#AEB6BF")
        cell.set_linewidth(0.8)

    fig.suptitle(
        f"Repair Rate Comparison — Rs={rs}, Cs={cs}, Fault Count={fault_count}",
        fontsize=14,
        fontweight="bold",
        y=0.96,
    )
    ax.text(
        0.5,
        0.87,
        f"Runs={runs:,} per configuration · Seed={seed} · Best verified value highlighted",
        ha="center",
        va="center",
        fontsize=9.5,
        color="#4D5656",
        transform=ax.transAxes,
    )
    ax.text(
        0.5,
        0.055,
        "* Global Pool m=2 is shown for completeness, but remains an unverified diagnostic configuration.",
        ha="center",
        va="center",
        fontsize=9,
        color="#7A4E00",
        transform=ax.transAxes,
    )
    stem = f"repair_rate_table_rs{rs}_cs{cs}_fault{fault_count}"
    fig.savefig(output_dir / f"{stem}.png", dpi=300, bbox_inches="tight")
    fig.savefig(output_dir / f"{stem}.pdf", bbox_inches="tight")
    plt.close(fig)


def generate_tables(data: pd.DataFrame, output_dir: Path) -> int:
    table_dir = output_dir / "tables"
    data_dir = output_dir / "table_data"
    table_dir.mkdir(parents=True, exist_ok=True)
    data_dir.mkdir(parents=True, exist_ok=True)
    count = 0
    for (rs, cs, fault_count), group in data.groupby(
        ["Rs", "Cs", "fault_count"], sort=True
    ):
        pivot = case_pivot(group)
        runs = int(group["runs"].iloc[0])
        seed = int(group["seed"].iloc[0])
        stem = f"repair_rate_table_rs{rs}_cs{cs}_fault{fault_count}"
        export = pivot.copy() * 100.0
        export.index.name = "Fault Model"
        export.to_csv(data_dir / f"{stem}.csv", float_format="%.4f")
        save_table_figure(pivot, rs, cs, fault_count, runs, seed, table_dir)
        count += 1
    return count


def generate_curves(data: pd.DataFrame, output_dir: Path) -> int:
    curve_dir = output_dir / "curves"
    curve_dir.mkdir(parents=True, exist_ok=True)
    count = 0
    for (rs, cs), group in data.groupby(["Rs", "Cs"], sort=True):
        fig, axes = plt.subplots(2, 2, figsize=(12.5, 7.6), sharex=True, sharey=True)
        for ax, model in zip(axes.flat, FAULT_ORDER):
            model_rows = group[group["fault_model"] == model]
            for policy in POLICY_ORDER:
                rows = model_rows[model_rows["policy_label"] == policy].sort_values(
                    "fault_count"
                )
                ax.plot(
                    rows["fault_count"],
                    100.0 * rows["repair_rate"],
                    label=POLICY_DISPLAY_LABELS[policy].replace("\n", " "),
                    color=POLICY_COLORS[policy],
                    marker=POLICY_MARKERS[policy],
                    linestyle="--" if policy in UNVERIFIED_POLICIES else "-",
                    linewidth=2.0,
                    markersize=5,
                )
            ax.set_title(FAULT_LABELS[model], fontweight="bold")
            ax.grid(True, alpha=0.28)
            ax.set_ylim(-2, 102)
        axes[1, 0].set_xlabel("Fault Count")
        axes[1, 1].set_xlabel("Fault Count")
        axes[0, 0].set_ylabel("Repair Rate (%)")
        axes[1, 0].set_ylabel("Repair Rate (%)")
        handles, labels = axes[0, 0].get_legend_handles_labels()
        fig.legend(
            handles,
            labels,
            loc="lower center",
            bbox_to_anchor=(0.5, 0.04),
            ncol=4,
            frameon=False,
        )
        fig.suptitle(
            f"Repair Rate vs. Fault Count by Policy — Rs={rs}, Cs={cs}",
            fontsize=14,
            fontweight="bold",
        )
        fig.text(
            0.5,
            0.012,
            "* Global Pool m=2 is an unverified diagnostic configuration.",
            ha="center",
            fontsize=8.5,
            color="#7A4E00",
        )
        fig.tight_layout(rect=[0, 0.15, 1, 0.94])
        stem = f"repair_rate_curves_rs{rs}_cs{cs}"
        fig.savefig(curve_dir / f"{stem}.png", dpi=300, bbox_inches="tight")
        fig.savefig(curve_dir / f"{stem}.pdf", bbox_inches="tight")
        plt.close(fig)
        count += 1
    return count


def main() -> None:
    args = parse_args()
    output_dir = args.output_dir.expanduser().resolve()
    output_dir.mkdir(parents=True, exist_ok=True)
    summaries = find_summaries(args.inputs)
    data = load_selected(summaries, args.storage_mode)
    data.to_csv(output_dir / "selected_results_long.csv", index=False)
    table_count = generate_tables(data, output_dir)
    curve_count = generate_curves(data, output_dir)
    print(f"Loaded {len(summaries)} summary CSV file(s).")
    print(f"Generated {table_count} table figure(s) and {curve_count} curve figure(s).")
    print(f"Output: {output_dir}")


if __name__ == "__main__":
    main()
