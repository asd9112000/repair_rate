#!/usr/bin/env python3
"""Aggregate and plot the focused DATE Figure 2 fault-imbalance sweep."""

from __future__ import annotations

import argparse
import csv
import json
import math
import os
from collections import defaultdict
from pathlib import Path

os.environ.setdefault("MPLCONFIGDIR", "/tmp/date-submission-matplotlib")
import matplotlib.pyplot as plt
from matplotlib.colors import TwoSlopeNorm


POLICIES = {
    "no_sharing": ("RECAM / No Sharing", "#8c4a3c", "o", "--"),
    "directional_m1": ("DSS", "#1f4e79", "s", "-"),
}
HEATMAP_COLOR_LIMIT_PP = 10.0


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("run_root", type=Path)
    return parser.parse_args()


def read_json(path: Path) -> dict:
    return json.loads(path.read_text(encoding="utf-8"))


def wilson(successes: int, trials: int) -> tuple[float, float]:
    z = 1.959963984540054
    p = successes / trials
    denominator = 1 + z * z / trials
    center = (p + z * z / (2 * trials)) / denominator
    margin = z * math.sqrt(p * (1 - p) / trials + z * z / (4 * trials * trials)) / denominator
    return max(0.0, center - margin), min(1.0, center + margin)


def plot_case(axis, rows: list[dict[str, object]], fault_count: int) -> None:
    for policy, (label, color, marker, line) in POLICIES.items():
        selected = [row for row in rows if row["fault_count"] == fault_count and row["policy_id"] == policy]
        y = [float(row["repair_rate_percent"]) for row in selected]
        yerr = [[max(0.0, value - float(row["ci95_low_percent"]))
                 for value, row in zip(y, selected)],
                [max(0.0, float(row["ci95_high_percent"]) - value)
                 for value, row in zip(y, selected)]]
        axis.errorbar([row["std_fault_count"] for row in selected], y, yerr=yerr,
                      color=color, marker=marker, linestyle=line, capsize=2.2, label=label)
    axis.set_title(f"Total faults F={fault_count}")
    axis.set_xlim(0, fault_count // 4)
    axis.set_ylim(0, 100)
    axis.grid(True, linestyle=":", alpha=0.65)
    axis.set_xlabel("Fault-count standard deviation $\\sigma$")


def save_gain_heatmap(figure_dir: Path, rows: list[dict[str, object]], fault_counts: list[int],
                      stem: str, title: str) -> None:
    rates = {(int(row["fault_count"]), int(row["std_fault_count"]), row["policy_id"]):
             float(row["repair_rate_percent"]) for row in rows}
    selected_sigmas = sorted({sigma for fault_count, sigma, _ in rates if fault_count in fault_counts})
    variances = [sigma * sigma for sigma in selected_sigmas]
    matrix = []
    for fault_count in fault_counts:
        values = []
        for sigma in selected_sigmas:
            baseline = rates.get((fault_count, sigma, "no_sharing"))
            directional = rates.get((fault_count, sigma, "directional_m1"))
            values.append(math.nan if baseline is None or directional is None else directional - baseline)
        matrix.append(values)
    cmap = plt.get_cmap("coolwarm").copy()
    cmap.set_bad("#F2F2F2")
    fig, axis = plt.subplots(figsize=(8.3, 2.3 + 0.55 * len(fault_counts)))
    image = axis.imshow(matrix, aspect="auto", cmap=cmap,
                        norm=TwoSlopeNorm(vmin=-HEATMAP_COLOR_LIMIT_PP, vcenter=0,
                                         vmax=HEATMAP_COLOR_LIMIT_PP),
                        interpolation="nearest")
    axis.set_title(title, fontsize=16)
    axis.set_xticks(range(len(variances)), variances)
    axis.set_yticks(range(len(fault_counts)), fault_counts)
    # axis.set_xlabel("Fault-count variance $\\sigma^2$", fontsize=14)
    # axis.set_ylabel("Total fault count Ng", fontsize=14)
    axis.set_xlabel("Fault-count variance $\\sigma^2$", fontsize=18)
    axis.set_ylabel("Total fault count Ng", fontsize=18)
    axis.tick_params(axis="both", labelsize=14)

    for row_index, row in enumerate(matrix):
        for column_index, value in enumerate(row):
            if math.isnan(value):
                continue
            text_color = "white" if abs(value) > 0.55 * HEATMAP_COLOR_LIMIT_PP else "black"
            axis.text(column_index, row_index, f"{value:.1f}", ha="center", va="center",
                      fontsize=14,  color=text_color)
    axis.set_xticks([index - 0.5 for index in range(len(variances) + 1)], minor=True)
    axis.set_yticks([index - 0.5 for index in range(len(fault_counts) + 1)], minor=True)
    axis.grid(which="minor", color="white", linewidth=1.0)
    axis.tick_params(which="minor", bottom=False, left=False)
    colorbar = fig.colorbar(image, ax=axis, label="DSS - No Sharing repair rate (pp)")
    colorbar.set_label("DSS - No Sharing repair rate (pp)", fontsize=18)
    colorbar.ax.tick_params(labelsize=14)
    colorbar.ax.yaxis.labelpad = 8
    fig.tight_layout()
    fig.savefig(figure_dir / f"{stem}.svg")
    fig.savefig(figure_dir / f"{stem}.pdf")
    plt.close(fig)

def main() -> int:
    root = parse_args().run_root.resolve()
    config = read_json(root / "run_config.json")
    totals = defaultdict(lambda: {"successes": 0, "runs": 0, "seeds": set(), "meta": None})
    for point_path in sorted((root / "raw").rglob("point.json")):
        meta = read_json(point_path)
        policy = meta["policy_id"]
        if policy not in POLICIES:
            continue
        with (point_path.parent / "summary.csv").open(newline="", encoding="utf-8") as source:
            row = next(csv.DictReader(source))
        key = (meta["fault_count"], meta["std_fault_count"], policy)
        totals[key]["successes"] += round(float(row["repair_rate"]) * int(row["runs"]))
        totals[key]["runs"] += int(row["runs"])
        totals[key]["seeds"].add(meta["seed"])
        totals[key]["meta"] = meta

    expected_seeds = set(config["seeds"])
    rows = []
    for fault_count in config["fault_counts"]:
        for sigma in range(fault_count // 4 + 1):
            for policy in POLICIES:
                item = totals.get((fault_count, sigma, policy))
                if not item or item["seeds"] != expected_seeds:
                    raise RuntimeError(f"incomplete point: F={fault_count}, sigma={sigma}, {policy}")
                low, high = wilson(item["successes"], item["runs"])
                meta = item["meta"]
                rows.append({
                    "fault_count": fault_count, "mean_fault_count": meta["mean_fault_count"],
                    "std_fault_count": sigma, "normalized_std_fault_count": meta["normalized_std_fault_count"],
                    "fault_counts_abcd": ",".join(map(str, meta["fault_counts_abcd"])),
                    "policy_id": policy, "total_runs": item["runs"], "successes": item["successes"],
                    "repair_rate_percent": 100 * item["successes"] / item["runs"],
                    "ci95_low_percent": 100 * low, "ci95_high_percent": 100 * high,
                })

    csv_dir = root / "csv"
    csv_dir.mkdir(exist_ok=True)
    with (csv_dir / "fig2_repair_rate_vs_fault_imbalance.csv").open("w", newline="", encoding="utf-8") as output:
        writer = csv.DictWriter(output, fieldnames=list(rows[0]))
        writer.writeheader()
        writer.writerows(rows)

    figure_dir = root / "figures"
    figure_dir.mkdir(exist_ok=True)
    fig, axes = plt.subplots(2, 3, figsize=(10.5, 6.7), sharey=True)
    for axis, fault_count in zip(axes.flat, config["fault_counts"]):
        plot_case(axis, rows, fault_count)
    axes[0, 0].set_ylabel("Repair Rate (%)")
    axes[1, 0].set_ylabel("Repair Rate (%)")
    handles, labels = axes[0, 0].get_legend_handles_labels()
    fig.legend(handles, labels, loc="upper center", bbox_to_anchor=(0.5, 0.955),
               ncol=2, frameon=False)
    fig.suptitle("Figure 2. Repair Rate vs. Inter-Subarray Fault Imbalance", y=0.995)
    fig.tight_layout(rect=(0, 0, 1, 0.89))
    fig.savefig(figure_dir / "fig2_repair_rate_vs_fault_imbalance.svg")
    fig.savefig(figure_dir / "fig2_repair_rate_vs_fault_imbalance.pdf")
    plt.close(fig)
    for fault_count in config["fault_counts"]:
        fig, axis = plt.subplots(figsize=(6.7, 4.2))
        plot_case(axis, rows, fault_count)
        axis.set_ylabel("Repair Rate (%)")
        axis.legend(frameon=False)
        fig.tight_layout()
        fig.savefig(figure_dir / f"fig2_repair_rate_vs_fault_imbalance_f{fault_count}.svg")
        fig.savefig(figure_dir / f"fig2_repair_rate_vs_fault_imbalance_f{fault_count}.pdf")
        plt.close(fig)
    save_gain_heatmap(figure_dir, rows, config["fault_counts"],
                      "heatmap_repair_rate_gain_vs_fault_variance",
                      "DSS Repair-Rate Gain by Fault Variance and Total Fault Count")
    for fault_count in config["fault_counts"]:
        save_gain_heatmap(figure_dir, rows, [fault_count],
                          f"heatmap_repair_rate_gain_f{fault_count}",
                          f"DSS Repair-Rate Gain (F={fault_count})")
    print(f"Wrote Figure 2 CSV and figures to {root}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
