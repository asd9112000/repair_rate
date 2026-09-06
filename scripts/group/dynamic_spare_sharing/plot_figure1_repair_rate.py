#!/usr/bin/env python3
"""Plot Figure 1 from paired 2x2 DynamicSpareSharing group sweeps."""

from __future__ import annotations

import argparse
import csv
import math
from collections import defaultdict
from pathlib import Path

import matplotlib.pyplot as plt


POLICIES = {
    ("no_sharing", 0, 0): ("no_sharing", "RECAM / No Sharing"),
    ("directional", 1, 1): ("directional_m1", "Limited DSS ($m=1$)"),
    ("directional", 2, 2): ("directional_m2", "Proposed DSS ($m=2$)"),
}


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Plot repair rate for no sharing and 2x2 directional DSS.")
    parser.add_argument("input_root", type=Path,
                        help="directory containing per-seed summary.csv files")
    parser.add_argument("--output-dir", type=Path, required=True)
    parser.add_argument("--rs", type=int, default=2)
    parser.add_argument("--cs", type=int, default=2)
    parser.add_argument("--fault-model", default="moderate_imbalance")
    parser.add_argument("--spatial-model", default="mixed")
    return parser.parse_args()


def wilson_interval(successes: int, trials: int) -> tuple[float, float]:
    if trials == 0:
        return (0.0, 0.0)
    z = 1.959963984540054
    p = successes / trials
    denominator = 1.0 + z * z / trials
    center = (p + z * z / (2.0 * trials)) / denominator
    margin = z * math.sqrt(
        p * (1.0 - p) / trials + z * z / (4.0 * trials * trials)
    ) / denominator
    return (max(0.0, center - margin), min(1.0, center + margin))


def main() -> int:
    args = parse_args()
    summaries = sorted(args.input_root.rglob("summary.csv"))
    if not summaries:
        raise SystemExit(f"no summary.csv found under {args.input_root}")

    totals: dict[tuple[str, int], dict[str, object]] = defaultdict(
        lambda: {"successes": 0, "runs": 0, "seeds": set()})
    for summary in summaries:
        with summary.open(newline="", encoding="utf-8") as source:
            for row in csv.DictReader(source):
                if (row["layout"] != "2x2" or
                        int(row["Rs"]) != args.rs or
                        int(row["Cs"]) != args.cs or
                        row["fault_model"] != args.fault_model or
                        row["fault_spatial_model"] != args.spatial_model or
                        row["storage_mode"] != "cam"):
                    continue
                policy_key = (row["policy"], int(row["shared_rows"]),
                              int(row["shared_columns"]))
                if policy_key not in POLICIES:
                    continue
                policy_id, _ = POLICIES[policy_key]
                fault_count = int(row["fault_count"])
                runs = int(row["runs"])
                successes = round(float(row["repair_rate"]) * runs)
                aggregate = totals[(policy_id, fault_count)]
                aggregate["successes"] = int(aggregate["successes"]) + successes
                aggregate["runs"] = int(aggregate["runs"]) + runs
                seeds = aggregate["seeds"]
                assert isinstance(seeds, set)
                seeds.add(int(row["seed"]))

    expected_ids = {value[0] for value in POLICIES.values()}
    observed_ids = {key[0] for key in totals}
    if observed_ids != expected_ids:
        raise SystemExit(
            f"expected policies {sorted(expected_ids)}, found {sorted(observed_ids)}")

    args.output_dir.mkdir(parents=True, exist_ok=True)
    plot_rows: list[dict[str, object]] = []
    labels = {policy_id: label for policy_id, label in POLICIES.values()}
    for (policy_id, fault_count), aggregate in sorted(totals.items()):
        successes = int(aggregate["successes"])
        runs = int(aggregate["runs"])
        low, high = wilson_interval(successes, runs)
        seeds = aggregate["seeds"]
        assert isinstance(seeds, set)
        plot_rows.append({
            "policy_id": policy_id,
            "label": labels[policy_id],
            "fault_count_per_group": fault_count,
            "seed_count": len(seeds),
            "total_runs": runs,
            "successes": successes,
            "repair_rate_percent": 100.0 * successes / runs,
            "ci95_low_percent": 100.0 * low,
            "ci95_high_percent": 100.0 * high,
        })

    data_path = args.output_dir / "figure1_plot_data.csv"
    with data_path.open("w", newline="", encoding="utf-8") as output:
        writer = csv.DictWriter(output, fieldnames=list(plot_rows[0]))
        writer.writeheader()
        writer.writerows(plot_rows)

    style = {
        "no_sharing": ("#4D4D4D", "o", "--"),
        "directional_m1": ("#0072B2", "s", "-"),
        "directional_m2": ("#D55E00", "^", "-"),
    }
    fig, axis = plt.subplots(figsize=(6.6, 4.4))
    for policy_id in ("no_sharing", "directional_m1", "directional_m2"):
        selected = [row for row in plot_rows if row["policy_id"] == policy_id]
        color, marker, line_style = style[policy_id]
        x = [int(row["fault_count_per_group"]) for row in selected]
        y = [float(row["repair_rate_percent"]) for row in selected]
        yerr = [
            [max(0.0, value - float(row["ci95_low_percent"]))
             for value, row in zip(y, selected)],
            [max(0.0, float(row["ci95_high_percent"]) - value)
             for value, row in zip(y, selected)],
        ]
        axis.errorbar(x, y, yerr=yerr, label=labels[policy_id], color=color,
                      marker=marker, linestyle=line_style, linewidth=1.8,
                      markersize=5.5, capsize=2.5)

    axis.set_xlabel("Number of Faults per 2×2 Group")
    axis.set_ylabel("Repair Rate (%)")
    axis.set_ylim(-2, 104)
    axis.grid(True, linestyle=":", linewidth=0.7, alpha=0.7)
    axis.legend(frameon=False)
    fig.tight_layout()
    fig.savefig(args.output_dir / "figure1_repair_rate_vs_faults.png", dpi=300)
    fig.savefig(args.output_dir / "figure1_repair_rate_vs_faults.pdf")
    plt.close(fig)
    print(f"Wrote Figure 1 and plot data to {args.output_dir}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
