#!/usr/bin/env python3
"""Plot canonical-four repair rate, after-BIST latency, and hardware bits."""

from __future__ import annotations

import argparse
import csv
from collections import defaultdict
from pathlib import Path

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt


ORDER = [
    "B0_CAM_NO_SHARING", "B1_CAM_SHARING",
    "B2_SRAM_NO_SHARING", "B3_SRAM_SHARING",
]


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("input", type=Path)
    parser.add_argument("--output-dir", type=Path, required=True)
    return parser.parse_args()


def plot_lines(rows: list[dict[str, str]], field: str, ylabel: str,
               stem: str, output_dir: Path) -> None:
    grouped: dict[str, list[tuple[int, float]]] = defaultdict(list)
    for row in rows:
        grouped[row["configuration_id"]].append(
            (int(row["sweep_fault_count"]), float(row[field])))
    figure, axis = plt.subplots(figsize=(7.2, 4.5))
    for config in ORDER:
        points = sorted(grouped.get(config, []))
        if not points:
            continue
        # Average repeated seeds at each x value.
        samples: dict[int, list[float]] = defaultdict(list)
        for x, y in points:
            samples[x].append(y)
        xs = sorted(samples)
        ys = [sum(samples[x]) / len(samples[x]) for x in xs]
        axis.plot(xs, ys, marker="o", label=config)
    axis.set_xlabel("Faults per 4-SA group")
    axis.set_ylabel(ylabel)
    axis.grid(True, alpha=0.3)
    axis.legend(fontsize=8)
    figure.tight_layout()
    for suffix in ("png", "pdf"):
        figure.savefig(output_dir / f"{stem}.{suffix}", dpi=180)
    plt.close(figure)


def main() -> int:
    args = parse_args()
    with args.input.open(newline="", encoding="utf-8") as source:
        rows = list(csv.DictReader(source))
    required = {
        "configuration_id", "sweep_fault_count", "repair_rate",
        "max_solution_after_bist_cycles", "bira_modeled_bits_per_engine",
    }
    missing = required.difference(rows[0] if rows else {})
    if missing:
        raise SystemExit(f"missing CSV columns: {sorted(missing)}")
    args.output_dir.mkdir(parents=True, exist_ok=True)
    with (args.output_dir / "plot_data.csv").open(
            "w", newline="", encoding="utf-8") as output:
        writer = csv.DictWriter(output, fieldnames=list(rows[0]))
        writer.writeheader()
        writer.writerows(rows)
    plot_lines(rows, "repair_rate", "Repair rate",
               "repair_rate_vs_fault_count", args.output_dir)
    plot_lines(rows, "max_solution_after_bist_cycles",
               "Maximum latency after BIST (cycles)",
               "latency_after_bist_vs_fault_count", args.output_dir)
    plot_lines(rows, "bira_modeled_bits_per_engine",
               "Modeled bits per BIRA engine",
               "hardware_bits_vs_fault_count", args.output_dir)
    print(f"Wrote canonical-four plots to {args.output_dir}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
