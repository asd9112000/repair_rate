#!/usr/bin/env python3
"""Plot reusable group, SRAM, and device views for the moderate study."""

from __future__ import annotations

import argparse
import csv
import json
import os
from collections import defaultdict
from pathlib import Path

os.environ.setdefault("MPLCONFIGDIR", "/tmp/repair_rate_matplotlib")
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt


POLICY_LABELS = {
    "no_sharing": "No sharing",
    "directional_m1": "Directional m=1",
    "pairwise_edge_m1": "Pairwise/Edge m=1",
    "global_m1": "Global m=1",
}
POLICY_ORDER = [
    "no_sharing", "directional_m1", "pairwise_edge_m1", "global_m1",
]
CONFIG_LABELS = {
    "B0_CAM_NO_SHARING": "CAM / no sharing",
    "B1_CAM_SHARING": "CAM / sharing",
    "B2_SRAM_NO_SHARING": "SRAM / no sharing",
    "B3_SRAM_SHARING": "SRAM / sharing",
}


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("input_dir", type=Path)
    parser.add_argument("--output-dir", type=Path, required=True)
    return parser.parse_args()


def read_csv(path: Path) -> list[dict[str, str]]:
    with path.open(newline="", encoding="utf-8") as source:
        return [dict(row) for row in csv.DictReader(source)]


def save(figure: plt.Figure, output_dir: Path, stem: str) -> None:
    figure.tight_layout()
    for suffix in ("png", "pdf"):
        figure.savefig(output_dir / f"{stem}.{suffix}", dpi=180)
    plt.close(figure)


def means(rows: list[dict[str, str]], keys: tuple[str, ...], field: str):
    samples: dict[tuple[str, ...], list[float]] = defaultdict(list)
    for row in rows:
        samples[tuple(row[key] for key in keys)].append(float(row[field]))
    return {key: sum(values) / len(values) for key, values in samples.items()}


def plot_group(rows: list[dict[str, str]], output_dir: Path) -> None:
    rates = means(rows, ("sweep_rs", "policy_id", "sweep_fault_count"),
                  "repair_rate")
    spares = sorted({int(row["sweep_rs"]) for row in rows})
    present = {row["policy_id"] for row in rows}
    policies = [policy for policy in POLICY_ORDER if policy in present]
    figure, axes = plt.subplots(1, len(spares), figsize=(6.2 * len(spares), 4.5),
                                sharey=True, squeeze=False)
    for axis, spare in zip(axes[0], spares):
        for policy in policies:
            points = sorted((int(fault), value) for (rs, name, fault), value
                            in rates.items() if int(rs) == spare and name == policy)
            if points:
                axis.plot([x for x, _ in points], [y for _, y in points],
                          marker="o", label=POLICY_LABELS.get(policy, policy))
        axis.set_title(f"Rs=Cs={spare}")
        axis.set_xlabel("Faults per 4-SA group")
        axis.grid(alpha=0.3)
    axes[0][0].set_ylabel("Repair rate")
    axes[0][-1].legend(fontsize=8)
    save(figure, output_dir, "group_repair_rate_vs_fault_count")

    faults = sorted({int(row["sweep_fault_count"]) for row in rows})
    cost_rates = means(
        rows,
        ("policy_id", "sweep_fault_count", "spare_cell_equivalent_per_group"),
        "repair_rate")
    figure, axes = plt.subplots(2, 2, figsize=(11.5, 8.0), sharey=True,
                                squeeze=False)
    for axis, fault in zip(axes.flat, faults):
        for policy in policies:
            points = sorted((int(cost), value)
                            for (name, count, cost), value in cost_rates.items()
                            if int(count) == fault and name == policy)
            if points:
                axis.plot([x for x, _ in points], [y for _, y in points],
                          marker="o", label=POLICY_LABELS.get(policy, policy))
        axis.set_title(f"Faults/group={fault}")
        axis.set_xlabel("Spare-cell-equivalent / group")
        axis.grid(alpha=0.3)
    axes[0][0].set_ylabel("Repair rate")
    axes[1][0].set_ylabel("Repair rate")
    axes[0][1].legend(fontsize=8)
    save(figure, output_dir, "group_repair_rate_vs_spare_cost")


def plot_sram(rows: list[dict[str, str]], output_dir: Path) -> None:
    spares = sorted({int(row["sweep_rs"]) for row in rows})
    frontier = means(
        rows,
        ("sweep_rs", "policy", "matrix_policy",
         "group_normalized_total_area_proxy"),
        "bira_cycles_per_fault")
    figure, axes = plt.subplots(1, len(spares), figsize=(6.2 * len(spares), 4.6),
                                squeeze=False)
    for axis, spare in zip(axes[0], spares):
        selected = sorted((policy, matrix, float(area), cycles)
                          for (rs, policy, matrix, area), cycles in frontier.items()
                          if int(rs) == spare)
        for policy, matrix, area, cycles in selected:
            label = f"{policy} / {matrix.replace('sram_', '')}"
            axis.scatter(area, cycles, s=55, label=label)
        axis.set_title(f"Rs=Cs={spare}")
        axis.set_xlabel("Normalized SRAM+comparator area proxy")
        axis.set_ylabel("BIRA cycles / detected fault")
        axis.grid(alpha=0.3)
        handles, labels = axis.get_legend_handles_labels()
        unique = dict(zip(labels, handles))
        axis.legend(unique.values(), unique.keys(), fontsize=7)
    save(figure, output_dir, "sram_latency_cost_frontier")

    scan_samples: dict[tuple[str, str], list[dict[str, str]]] = defaultdict(list)
    for row in rows:
        if row["matrix_policy"] == "sram_scan_rebuild":
            scan_samples[(row["sweep_rs"], row["policy"])].append(row)
    scan_rows = []
    for key, samples in sorted(scan_samples.items(), key=lambda item: (int(item[0][0]), item[0][1])):
        averaged = dict(samples[0])
        for field in (
            "group_address_physical_bits", "group_hybrid_physical_bits",
            "group_runtime_physical_bits", "group_temp_buffer_bits",
            "group_matrix_bits", "group_comparator_bits",
        ):
            averaged[field] = str(sum(float(row[field]) for row in samples) / len(samples))
        scan_rows.append(averaged)
    labels = [f"R{row['sweep_rs']} {row['policy']}" for row in scan_rows]
    storage = [sum(float(row[field]) for field in (
        "group_address_physical_bits", "group_hybrid_physical_bits",
        "group_runtime_physical_bits", "group_temp_buffer_bits",
        "group_matrix_bits")) for row in scan_rows]
    comparator = [float(row["group_comparator_bits"]) for row in scan_rows]
    figure, axis = plt.subplots(figsize=(max(8.0, len(labels) * 1.05), 4.8))
    xs = list(range(len(labels)))
    axis.bar(xs, storage, label="Storage + matrix bits")
    axis.bar(xs, comparator, bottom=storage, label="Comparator bits")
    axis.set_xticks(xs, labels, rotation=35, ha="right")
    axis.set_ylabel("Modeled bits per 4-SA group")
    axis.legend()
    axis.grid(axis="y", alpha=0.3)
    save(figure, output_dir, "sram_modeled_bit_breakdown")


def plot_device(rows: list[dict[str, str]], output_dir: Path) -> None:
    spares = sorted({int(row["sweep_rs"]) for row in rows})
    for field, ylabel, stem in (
        ("repair_rate", "Repairable groups / modeled groups",
         "device_repair_rate_vs_fault_count"),
        ("max_solution_after_bist_cycles", "Maximum latency after BIST (cycles)",
         "device_latency_after_bist_vs_fault_count"),
    ):
        values = means(rows, ("sweep_rs", "configuration_id", "sweep_fault_count"),
                       field)
        figure, axes = plt.subplots(1, len(spares),
                                    figsize=(6.2 * len(spares), 4.5),
                                    sharey=False, squeeze=False)
        for axis, spare in zip(axes[0], spares):
            styles = {
                "B0_CAM_NO_SHARING": ("o", "-"),
                "B1_CAM_SHARING": ("o", "-"),
                "B2_SRAM_NO_SHARING": ("x", "--"),
                "B3_SRAM_SHARING": ("x", "--"),
            }
            colors = {
                "B0_CAM_NO_SHARING": "tab:blue",
                "B1_CAM_SHARING": "tab:orange",
                "B2_SRAM_NO_SHARING": "tab:blue",
                "B3_SRAM_SHARING": "tab:orange",
            }
            for config in CONFIG_LABELS:
                points = sorted((int(fault), value)
                                for (rs, name, fault), value in values.items()
                                if int(rs) == spare and name == config)
                if points:
                    marker, linestyle = styles[config]
                    axis.plot([x for x, _ in points], [y for _, y in points],
                              marker=marker, linestyle=linestyle,
                              color=colors[config], label=CONFIG_LABELS[config])
            axis.set_title(f"Rs=Cs={spare}")
            axis.set_xlabel("Faults per 4-SA group")
            axis.set_ylabel(ylabel)
            axis.grid(alpha=0.3)
        axes[0][-1].legend(fontsize=8)
        save(figure, output_dir, stem)

    cam_rows = [row for row in rows if row["configuration_id"] in
                ("B0_CAM_NO_SHARING", "B1_CAM_SHARING")]
    reserved = means(
        cam_rows, ("sweep_rs", "configuration_id", "sweep_fault_count"),
        "global_reuse_entries_reserved")
    overflow = means(
        cam_rows, ("sweep_rs", "configuration_id", "sweep_fault_count"),
        "global_reuse_overflow_count")
    figure, axes = plt.subplots(1, len(spares), figsize=(6.2 * len(spares), 4.5),
                                squeeze=False)
    for axis, spare in zip(axes[0], spares):
        for config in ("B0_CAM_NO_SHARING", "B1_CAM_SHARING"):
            points = sorted((int(fault), value,
                             overflow[(rs, name, fault)])
                            for (rs, name, fault), value in reserved.items()
                            if int(rs) == spare and name == config)
            if points:
                axis.plot([x for x, _, _ in points], [y for _, y, _ in points],
                          marker="o", label=CONFIG_LABELS[config] + " reserved")
                axis.plot([x for x, _, _ in points], [y for _, _, y in points],
                          marker="x", linestyle="--",
                          label=CONFIG_LABELS[config] + " overflow")
        axis.set_title(f"Rs=Cs={spare}")
        axis.set_xlabel("Faults per 4-SA group")
        axis.set_ylabel("Global reuse entries / requests")
        axis.grid(alpha=0.3)
        axis.legend(fontsize=7)
    save(figure, output_dir, "device_global_reuse_pressure")

    hardware = means(
        rows, ("sweep_rs", "bira_storage"), "bira_modeled_bits_per_engine")
    figure, axis = plt.subplots(figsize=(7.2, 4.6))
    width = 0.36
    xs = list(range(len(spares)))
    cam = [hardware[(str(spare), "CAM")] for spare in spares]
    sram = [hardware[(str(spare), "SRAM")] for spare in spares]
    axis.bar([x - width / 2 for x in xs], cam, width, label="CAM RECAM")
    axis.bar([x + width / 2 for x in xs], sram, width, label="SRAM RECAM")
    axis.set_xticks(xs, [f"Rs=Cs={spare}" for spare in spares])
    axis.set_ylabel("Modeled bits per BIRA engine")
    axis.grid(axis="y", alpha=0.3)
    axis.legend()
    save(figure, output_dir, "device_bira_modeled_bits_per_engine")


def main() -> int:
    args = parse_args()
    input_dir = args.input_dir.resolve()
    output_dir = args.output_dir.resolve()
    output_dir.mkdir(parents=True, exist_ok=True)
    group = read_csv(input_dir / "group_summary.csv")
    sram = read_csv(input_dir / "sram_summary.csv")
    device = read_csv(input_dir / "device_summary.csv")
    plot_group(group, output_dir)
    plot_sram(sram, output_dir)
    plot_device(device, output_dir)
    manifest = {
        "scope_rule": "group-level and device-level repair rates are plotted separately",
        "figures": sorted(path.name for path in output_dir.glob("*.png")),
        "source_csv": ["group_summary.csv", "sram_summary.csv", "device_summary.csv"],
    }
    (output_dir / "plot_manifest.json").write_text(
        json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    print(f"Wrote moderate-study plots to {output_dir}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
