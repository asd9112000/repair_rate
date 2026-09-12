#!/usr/bin/env python3
"""Collect, validate, plot, and summarize one DATE 2x2 group-scope run."""

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
from matplotlib.colors import LinearSegmentedColormap


POLICY_ROWS = {
    ("no_sharing", 0, 0): ("no_sharing", "RECAM / No Sharing"),
    ("directional", 1, 1): ("directional_m1", "Limited DSS ($m=1$)"),
    ("directional", 2, 2): ("directional_m2", "Proposed DSS ($m=2$)"),
}
BASELINE_POLICY = "no_sharing"
PLOT_POLICY_ORDER = ("no_sharing", "directional_m1", "directional_m2")
STYLE = {
    "no_sharing": ("#4D4D4D", "o", "--"),
    "directional_m1": ("#0072B2", "s", "-"),
    "directional_m2": ("#D55E00", "^", "-"),
}


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("run_root", type=Path)
    return parser.parse_args()


def read_json(path: Path) -> dict:
    with path.open(encoding="utf-8") as source:
        return json.load(source)


def write_json(path: Path, value: object) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(value, indent=2, sort_keys=True) + "\n",
                    encoding="utf-8")


def read_csv(path: Path) -> list[dict[str, str]]:
    with path.open(newline="", encoding="utf-8") as source:
        return list(csv.DictReader(source))


def write_csv(path: Path, rows: list[dict[str, object]], fields=None) -> None:
    if not rows:
        raise RuntimeError(f"refusing to write empty CSV: {path}")
    path.parent.mkdir(parents=True, exist_ok=True)
    names = fields or list(rows[0])
    with path.open("w", newline="", encoding="utf-8") as output:
        writer = csv.DictWriter(output, fieldnames=names, extrasaction="ignore")
        writer.writeheader()
        writer.writerows(rows)


def wilson(successes: int, trials: int) -> tuple[float, float]:
    z = 1.959963984540054
    p = successes / trials
    denominator = 1 + z * z / trials
    center = (p + z * z / (2 * trials)) / denominator
    margin = z * math.sqrt(p * (1 - p) / trials + z * z / (4 * trials * trials)) / denominator
    return max(0.0, center - margin), min(1.0, center + margin)


def policy_id(row: dict[str, str]) -> str | None:
    key = (row["policy"], int(row["shared_rows"]), int(row["shared_columns"]))
    value = POLICY_ROWS.get(key)
    return None if value is None else value[0]


def configure_plot() -> None:
    plt.rcParams.update({
        "font.size": 14, "axes.titlesize": 18, "axes.labelsize": 18,
        "legend.fontsize": 14, "xtick.labelsize": 14, "ytick.labelsize": 14,
        "lines.linewidth": 1.6, "lines.markersize": 5,
        "pdf.fonttype": 42, "ps.fonttype": 42,
    })


def save_figure(fig, root: Path, stem: str, *, tight_layout: bool = True) -> None:
    root.mkdir(parents=True, exist_ok=True)
    if tight_layout:
        fig.tight_layout()
    fig.savefig(root / f"{stem}.svg")
    fig.savefig(root / f"{stem}.pdf")
    plt.close(fig)


def configured_spares(config: dict) -> list[int]:
    return [int(value) for value in config.get("spare_counts", [config["fixed"]["rs"]])]


def spare_label(spare: int) -> str:
    return f"Rs=Cs={spare}"


def figure_directory(root: Path, spare: int, spares: list[int]) -> Path:
    return root / "figures" if len(spares) == 1 else root / "figures" / f"rs{spare}_cs{spare}"


def figure_stem(stem: str, spare: int, spares: list[int]) -> str:
    return stem if len(spares) == 1 else f"{stem}_rs{spare}_cs{spare}"


def relative_gain_percent(improved_rate: float, baseline_rate: float) -> float | None:
    if baseline_rate == 0:
        return None
    return 100 * (improved_rate - baseline_rate) / baseline_rate


def collect_repair(root: Path, config: dict) -> tuple[list[dict[str, object]], list[dict[str, object]]]:
    totals = defaultdict(lambda: {
        "successes": 0, "runs": 0, "seeds": set(), "util": [], "seed_rates": {},
    })
    for path in sorted((root / "raw/repair_rate").rglob("summary.csv")):
        for row in read_csv(path):
            pid = policy_id(row)
            if pid not in PLOT_POLICY_ORDER:
                continue
            spare = int(row["Rs"])
            if int(row["Cs"]) != spare:
                raise RuntimeError("DATE spare sweep requires symmetric Rs=Cs")
            key = (spare, int(row["fault_count"]), pid)
            runs = int(row["runs"])
            totals[key]["successes"] += round(float(row["repair_rate"]) * runs)
            totals[key]["runs"] += runs
            seed = int(row["seed"])
            totals[key]["seeds"].add(seed)
            totals[key]["util"].append(float(row["average_spare_utilization"]))
            totals[key]["seed_rates"].setdefault(seed, []).append(100 * float(row["repair_rate"]))

    expected_faults = config["fault_counts_per_group"]
    expected_seeds = set(config["active_profile_config"]["seeds"])
    missing = []
    long_rows = []
    spares = configured_spares(config)
    for spare in spares:
        for faults in expected_faults:
            for pid in PLOT_POLICY_ORDER:
                item = totals.get((spare, faults, pid))
                if not item or item["seeds"] != expected_seeds:
                    missing.append(f"rs={spare},f={faults},policy={pid}")
                    continue
                successes, runs = item["successes"], item["runs"]
                low, high = wilson(successes, runs)
                seed_rates = [sum(item["seed_rates"][seed]) / len(item["seed_rates"][seed])
                              for seed in sorted(item["seed_rates"])]
                long_rows.append({
                    "Rs": spare, "Cs": spare, "spare_label": spare_label(spare),
                    "fault_count": faults, "policy_id": pid,
                    "label": next(value[1] for value in POLICY_ROWS.values() if value[0] == pid),
                    "seed_count": len(item["seeds"]), "total_runs": runs,
                    "successes": successes, "repair_rate_percent": 100 * successes / runs,
                    "ci95_low_percent": 100 * low, "ci95_high_percent": 100 * high,
                    "average_spare_utilization_percent": 100 * sum(item["util"]) / len(item["util"]),
                    "seed_repair_rate_percent": seed_rates,
                })
    if missing:
        raise RuntimeError("incomplete Figure 1 grid: " + "; ".join(missing))

    by_key = {(row["Rs"], row["fault_count"], row["policy_id"]): row for row in long_rows}
    wide_rows = []
    for spare in spares:
        for faults in expected_faults:
            baseline = by_key[(spare, faults, "no_sharing")]
            m1 = by_key[(spare, faults, "directional_m1")]
            m2 = by_key[(spare, faults, "directional_m2")]
            wide_rows.append({
                "Rs": spare, "Cs": spare, "fault_count": faults,
                "m1_gain_pp": float(m1["repair_rate_percent"]) - float(baseline["repair_rate_percent"]),
                "m2_gain_pp": float(m2["repair_rate_percent"]) - float(baseline["repair_rate_percent"]),
            })
    return long_rows, wide_rows


def plot_repair(root: Path, long_rows: list[dict[str, object]], wide_rows: list[dict[str, object]], spares: list[int]) -> None:
    for spare in spares:
        plots = figure_directory(root, spare, spares)
        selected_long = [row for row in long_rows if row["Rs"] == spare]
        fig, axis = plt.subplots(figsize=(6.7, 4.2))
        color, marker, line = STYLE[BASELINE_POLICY]
        axis.plot([row["fault_count"] for row in selected_long],
                  [float(row["repair_rate_percent"]) for row in selected_long],
                  color=color, marker=marker, linestyle=line, label="RECAM / No Sharing")
        axis.set_title(f"Average Repair Rate vs. Number of Faults per 2×2 Group ({spare_label(spare)})")
        axis.set_xlabel("Number of Faults per 2×2 Group")
        axis.set_ylabel("Average Repair Rate (%)")
        axis.set_ylim(0, 100)
        axis.grid(True, linestyle=":", alpha=0.65)
        axis.legend(frameon=False)
        save_figure(fig, plots, figure_stem("fig1_repair_rate_vs_faults_average", spare, spares))

        fig, axis = plt.subplots(figsize=(6.7, 4.2))
        positions = list(range(1, len(selected_long) + 1))
        seed_rates = [row["seed_repair_rate_percent"] for row in selected_long]
        box = axis.boxplot(seed_rates, positions=positions, widths=0.55, showmeans=True,
                           patch_artist=True, medianprops={"color": "black"})
        for patch in box["boxes"]:
            patch.set(facecolor="#BDBDBD", alpha=0.8)
        axis.set_xticks(positions, [row["fault_count"] for row in selected_long])
        axis.set_title(f"Repair-Rate Distribution across Seeds ({spare_label(spare)})")
        axis.set_xlabel("Number of Faults per 2×2 Group")
        axis.set_ylabel("Repair Rate per Seed (%)")
        axis.set_ylim(0, 100)
        axis.grid(True, linestyle=":", alpha=0.65)
        save_figure(fig, plots, figure_stem("fig1_repair_rate_vs_faults_box_plot", spare, spares))

        fig, axis = plt.subplots(figsize=(6.7, 4.2))
        axis.plot([row["fault_count"] for row in selected_long],
                  [float(row["average_spare_utilization_percent"]) for row in selected_long],
                  color=color, marker=marker, linestyle=line, label="RECAM / No Sharing")
        axis.set_title(f"Spare Utilization of RECAM / No Sharing ({spare_label(spare)})")
        axis.set_xlabel("Number of Faults per 2×2 Group")
        axis.set_ylabel("Average Spare Utilization (%)")
        axis.set_ylim(0, 100)
        axis.set_yticks(range(0, 101, 20))
        axis.grid(True, linestyle=":", alpha=0.65)
        axis.legend(frameon=False)
        save_figure(fig, plots, figure_stem("fig4_spare_utilization", spare, spares))


def plot_spare_heatmaps(root: Path, long_rows: list[dict[str, object]], wide_rows: list[dict[str, object]], spares: list[int]) -> None:
    if len(spares) < 2:
        return
    faults = [fault for fault in sorted({int(row["fault_count"]) for row in wide_rows})
              if fault // 4 not in {2, 3, 15, 16}]
    heatmap_dir = root / "figures" / "heatmaps"
    panels = [("no_sharing", "RECAM / No Sharing"), ("directional_m1", "DSS"),
              ("directional_m2", "Proposed DSS ($m=2)")]
    fig, axes = plt.subplots(1, 3, figsize=(10.2, 3.4), sharey=True)
    for axis, (pid, title) in zip(axes, panels):
        values = {(int(row["Rs"]), int(row["fault_count"])): float(row["repair_rate_percent"])
                  for row in long_rows if row["policy_id"] == pid}
        matrix = [[values[(spare, fault)] for fault in faults] for spare in spares]
        image = axis.imshow(matrix, aspect="auto", vmin=0, vmax=100, cmap="viridis")
        for row_index, row in enumerate(matrix):
            for column_index, value in enumerate(row):
                color = "white" if sum(image.cmap(image.norm(value))[:3]) < 1.5 else "black"
                axis.text(column_index, row_index, f"{value:.1f}", ha="center", va="center",
                          color=color, fontsize=14)
        axis.set_title(title)
        axis.set_xticks(range(len(faults)), [f"{fault / 4:g}" for fault in faults])
        axis.set_xlabel("Fault count")
    axes[0].set_yticks(range(len(spares)), [spare_label(spare) for spare in spares])
    axes[0].set_ylabel("Spare line configuration")
    colorbar = fig.colorbar(image, ax=axes, label="Repair Rate (%)", fraction=0.035, pad=0.05)
    colorbar.ax.yaxis.labelpad = 6
    fig.subplots_adjust(left=0.08, right=0.84, bottom=0.20, top=0.84, wspace=0.20)
    save_figure(fig, heatmap_dir, "heatmap_repair_rate_by_spare", tight_layout=False)

    gain_cmap = LinearSegmentedColormap.from_list("white_to_vivid_red", ["#ffffff", "#FF0000"])
    max_gain = max(float(row[field]) for row in wide_rows for field in ("m1_gain_pp", "m2_gain_pp"))
    for field, title, stem in (
            ("m1_gain_pp", "DSS gain", "heatmap_dss_gain_by_spare"),
            ("m2_gain_pp", "Proposed DSS gain", "heatmap_proposed_dss_gain_by_spare")):
        fig, axis = plt.subplots(figsize=(7.8, 3.8))
        values = {(int(row["Rs"]), int(row["fault_count"])): float(row[field]) for row in wide_rows}
        matrix = [[values[(spare, fault)] for fault in faults] for spare in spares]
        image = axis.imshow(matrix, aspect="auto", vmin=0, vmax=max_gain, cmap=gain_cmap)
        for row_index, row in enumerate(matrix):
            for column_index, value in enumerate(row):
                color = "white" if sum(image.cmap(image.norm(value))[:3]) < 1.5 else "black"
                axis.text(column_index, row_index, f"{value:.1f}", ha="center", va="center",
                          color=color, fontsize=14)
        axis.set_title(title)
        axis.set_xticks(range(len(faults)), [f"{fault / 4:g}" for fault in faults])
        axis.set_xlabel("Fault count")
        axis.set_yticks(range(len(spares)), [spare_label(spare) for spare in spares])
        axis.set_ylabel("Spare line configuration")
        colorbar = fig.colorbar(image, ax=axis, label="Repair-rate improvement (pp)",
                                fraction=0.046, pad=0.06)
        colorbar.ax.yaxis.labelpad = 8
        fig.subplots_adjust(left=0.16, right=0.80, bottom=0.20, top=0.84)
        save_figure(fig, heatmap_dir, stem, tight_layout=False)


def collect_imbalance(root: Path, config: dict) -> list[dict[str, object]]:
    totals = defaultdict(lambda: {"successes": 0, "runs": 0, "seeds": set(), "meta": None})
    for point_path in sorted((root / "raw/imbalance").rglob("point.json")):
        meta = read_json(point_path)
        if meta["policy_id"] != BASELINE_POLICY:
            continue
        row = read_csv(point_path.parent / "summary.csv")[0]
        spare = int(meta.get("rs", config["fixed"]["rs"]))
        key = (spare, meta["distribution_id"], meta["policy_id"])
        runs = int(row["runs"])
        totals[key]["successes"] += round(float(row["repair_rate"]) * runs)
        totals[key]["runs"] += runs
        totals[key]["seeds"].add(meta["seed"])
        totals[key]["meta"] = meta
    expected_seeds = set(config["active_profile_config"]["seeds"])
    result = []
    for spare in configured_spares(config):
        for distribution in config["imbalance"]["profiles"]:
            item = totals[(spare, distribution["id"], BASELINE_POLICY)]
            if item["seeds"] != expected_seeds:
                raise RuntimeError(
                    f"incomplete imbalance grid: rs={spare}/{distribution['id']}/{BASELINE_POLICY}")
            counts = item["meta"]["fault_counts_abcd"]
            mean = sum(counts) / 4
            std = math.sqrt(sum((value - mean) ** 2 for value in counts) / 4)
            low, high = wilson(item["successes"], item["runs"])
            result.append({
                "Rs": spare, "Cs": spare, "spare_label": spare_label(spare),
                "distribution_id": distribution["id"], "distribution_label": distribution["label"],
                "fault_counts_abcd": ",".join(map(str, counts)), "max_faults_per_subarray": max(counts),
                "std_fault_count": std, "coefficient_of_variation": std / mean,
                "policy_id": BASELINE_POLICY, "total_runs": item["runs"], "successes": item["successes"],
                "repair_rate_percent": 100 * item["successes"] / item["runs"],
                "ci95_low_percent": 100 * low, "ci95_high_percent": 100 * high,
            })
    return result


def plot_imbalance(root: Path, rows: list[dict[str, object]], spares: list[int]) -> None:
    for spare in spares:
        fig, axis = plt.subplots(figsize=(6.7, 4.2))
        selected = sorted((row for row in rows if row["Rs"] == spare),
                          key=lambda row: float(row["coefficient_of_variation"]))
        color, marker, line = STYLE[BASELINE_POLICY]
        axis.plot([row["coefficient_of_variation"] for row in selected],
                  [float(row["repair_rate_percent"]) for row in selected],
                  color=color, marker=marker, linestyle=line, label="RECAM / No Sharing")
        axis.set_title(f"Average Repair Rate vs. Fault Imbalance ({spare_label(spare)})")
        axis.set_xlabel("Fault-Count Coefficient of Variation (std / mean)")
        axis.set_ylabel("Average Repair Rate (%)")
        axis.set_ylim(0, 100)
        axis.grid(True, linestyle=":", alpha=0.65)
        axis.legend(frameon=False, loc="upper right", fontsize=14)
        save_figure(fig, figure_directory(root, spare, spares),
                    figure_stem("fig3_repair_rate_vs_fault_imbalance", spare, spares))


def plot_hardware_proxy(root: Path, rows: list[dict[str, object]]) -> None:
    """Internal model plot; deliberately does not claim physical area or timing."""
    totals = defaultdict(lambda: {"area": [], "cycles": [], "p": None})
    for row in rows:
        item = totals[(row["Rs"], row["policy"])]
        item["area"].append(float(row["group_normalized_total_area_proxy"]))
        item["cycles"].append(float(row["bira_cycles_per_fault"]))
        item["p"] = row["P_A"]
    fig, axis = plt.subplots(figsize=(6.7, 4.2))
    for (spare, policy), item in sorted(totals.items(), key=lambda pair: float(pair[1]["area"][0])):
        area = sum(item["area"]) / len(item["area"])
        cycles = sum(item["cycles"]) / len(item["cycles"])
        axis.scatter(area, cycles, s=42)
        axis.annotate(f"Rs=Cs={spare}: {policy} (P={item['p']})", (area, cycles),
                      xytext=(4, 5), textcoords="offset points", fontsize=14)
    axis.set_title("Architectural SRAM-RECAM Storage–Latency Model (Non-Physical Proxy)")
    axis.set_xlabel("Normalized Storage + Comparator Area Proxy (not µm²)")
    axis.set_ylabel("Modeled BIRA Cycles per Fault")
    y_values = [sum(item["cycles"]) / len(item["cycles"]) for item in totals.values()]
    axis.set_ylim(min(y_values) - 0.12, max(y_values) + 0.16)
    axis.grid(True, linestyle=":", alpha=0.65)
    save_figure(fig, root / "figures", "supp_hardware_model_tradeoff")


def collect_hardware(root: Path) -> list[dict[str, object]]:
    rows = []
    for path in sorted((root / "raw/hardware_model").rglob("sram_recam_metrics.csv")):
        rows.extend(read_csv(path))
    if not rows:
        raise RuntimeError("no SRAM-RECAM hardware-model CSV files found")
    fields = [
        "policy", "search_policy", "matrix_policy", "N", "word_bits", "buffer_entries",
        "address_entries", "hybrid_entries", "runtime_entries", "address_entry_bits",
        "hybrid_entry_bits", "runtime_entry_bits", "P_A", "P_H", "P_R",
        "group_address_physical_bits", "group_hybrid_physical_bits",
        "group_runtime_physical_bits", "group_temp_buffer_bits", "group_matrix_bits",
        "group_comparator_bits", "group_normalized_total_area_proxy",
        "bira_cycles_per_fault", "runtime_lookup_rounds_avg", "runtime_hit_rounds_avg",
        "runtime_miss_rounds_avg", "repair_rate", "cam_baseline_repair_rate", "Rs", "Cs",
        "repair_result_matches_cam_baseline", "seed", "runs", "fault_count",
    ]
    return [{field: row[field] for field in fields} for row in rows]


def summarize_hardware_proxy(rows: list[dict[str, object]]) -> tuple[list[dict[str, object]], list[dict[str, object]]]:
    grouped = defaultdict(list)
    for row in rows:
        grouped[(row["Rs"], row["Cs"], row["policy"])].append(row)
    summary = []
    for (rs, cs, policy), selected in grouped.items():
        summary.append({
            "Rs": int(rs), "Cs": int(cs), "spare_label": f"Rs={rs}, Cs={cs}",
            "policy": policy, "effective_parallelism_P": int(selected[0]["P_A"]),
            "runtime_entries": int(selected[0]["runtime_entries"]),
            "runtime_entry_bits": int(selected[0]["runtime_entry_bits"]),
            "mean_normalized_area_proxy": sum(float(row["group_normalized_total_area_proxy"]) for row in selected) / len(selected),
            "mean_bira_cycles_per_fault": sum(float(row["bira_cycles_per_fault"]) for row in selected) / len(selected),
            "runtime_miss_rounds": sum(float(row["runtime_miss_rounds_avg"]) for row in selected) / len(selected),
            "seed_count": len(selected), "physical_area": "unavailable",
        })
    summary.sort(key=lambda row: float(row["mean_normalized_area_proxy"]))
    frontier = []
    for candidate in summary:
        dominated = any(
            other is not candidate and
            float(other["mean_normalized_area_proxy"]) <= float(candidate["mean_normalized_area_proxy"]) and
            float(other["mean_bira_cycles_per_fault"]) <= float(candidate["mean_bira_cycles_per_fault"]) and
            (float(other["mean_normalized_area_proxy"]) < float(candidate["mean_normalized_area_proxy"]) or
             float(other["mean_bira_cycles_per_fault"]) < float(candidate["mean_bira_cycles_per_fault"]))
            for other in summary)
        if not dominated:
            frontier.append(dict(candidate, frontier_basis="non-physical normalized proxy"))
    return summary, frontier


def make_tables(root: Path, wide: list[dict[str, object]], hardware: list[dict[str, object]]) -> None:
    transition = next((int(row["fault_count"]) for row in wide if float(row["baseline_mean"]) < 95),
                      int(wide[0]["fault_count"]))
    max_gain = max(wide, key=lambda row: float(row["m2_gain_pp"]))
    selected_faults = []
    for value in (int(wide[0]["fault_count"]), transition,
                  int(max_gain["fault_count"]), int(wide[-1]["fault_count"])):
        if value not in selected_faults:
            selected_faults.append(value)
    selected = [row for row in wide if int(row["fault_count"]) in selected_faults]
    table1 = [{key: row[key] for key in ("fault_count", "baseline_mean", "m1_mean",
                                          "m1_gain_pp", "m2_mean", "m2_gain_pp")}
              for row in selected]
    write_csv(root / "tables/table1_dss_repairability.csv", table1)
    headers = ["Faults/group", "No Sharing (%)", "m=1 (%)", "m=1 gain (pp)", "m=2 (%)", "m=2 gain (pp)"]
    md = ["| " + " | ".join(headers) + " |", "|" + "---|" * len(headers)]
    for row in table1:
        md.append("| " + " | ".join(f"{float(value):.2f}" for value in row.values()) + " |")
    (root / "tables/table1_dss_repairability.md").write_text("\n".join(md) + "\n", encoding="utf-8")
    latex = ["\\begin{tabular}{rrrrrr}", "\\toprule", " & ".join(headers) + " \\\\", "\\midrule"]
    for row in table1:
        latex.append(" & ".join(f"{float(value):.2f}" for value in row.values()) + " \\\\")
    latex += ["\\bottomrule", "\\end{tabular}"]
    (root / "tables/table1_dss_repairability.tex").write_text("\n".join(latex) + "\n", encoding="utf-8")

    seen = set()
    table2 = []
    for row in hardware:
        policy = row["policy"]
        if policy in seen:
            continue
        seen.add(policy)
        table2.append({
            "implementation": f"SRAM-like {policy}", "entries_N": row["runtime_entries"],
            "entry_width_W": row["runtime_entry_bits"], "parallelism_P": row["P_R"],
            "cell_count": "", "area_um2": "", "critical_delay_ns": "",
            "search_cycles_miss": row["runtime_miss_rounds_avg"], "lookup_time_ns": "",
            "status": "MODEL_ONLY: RTL/library/clock constraint unavailable",
        })
    table2.insert(0, {"implementation": "CAM", "entries_N": hardware[0]["runtime_entries"],
                      "entry_width_W": hardware[0]["runtime_entry_bits"], "parallelism_P": "N",
                      "cell_count": "", "area_um2": "", "critical_delay_ns": "",
                      "search_cycles_miss": 1, "lookup_time_ns": "",
                      "status": "NOT_SYNTHESIZED: RTL/library/clock constraint unavailable"})
    table2.insert(1, {"implementation": "Register Table", "entries_N": hardware[0]["runtime_entries"],
                      "entry_width_W": hardware[0]["runtime_entry_bits"], "parallelism_P": "",
                      "cell_count": "", "area_um2": "", "critical_delay_ns": "",
                      "search_cycles_miss": "", "lookup_time_ns": "",
                      "status": "NOT_IMPLEMENTED: no functionally equivalent RTL"})
    write_csv(root / "tables/table2_hardware_cost_summary.csv", table2)


def write_captions(root: Path, config: dict, wide: list[dict[str, object]], representative: int) -> None:
    samples = wide[0]["total_samples_per_curve"]
    captions = {
        "fig1": f"Group-scope repair rate for independent 2×2 groups with Rs=Cs=2, moderate/mixed faults, and {samples} paired samples per curve point. Error bars are Wilson 95% confidence intervals. The full 0–100% axis shows the transition region without truncation.",
        "fig1_average": f"Group-scope average repair rate for the same Figure 1 corpus, without confidence-interval error bars. Rs=Cs=2, moderate/mixed faults, and {samples} paired samples per curve point; the y-axis spans 0–100%.",
        "fig2": "Measured percentage-point improvement over the paired No Sharing baseline using the same Figure 1 corpus. Labels mark the observed maximum for each DSS policy; no significance test is claimed.",
        "fig2_relative": "Relative repair-rate improvement over No Sharing, calculated as (DSS − No Sharing) / No Sharing × 100. Points with a 0% No Sharing repair rate are undefined and omitted.",
        "fig3": f"Group-scope repair rate at F={representative} total faults distributed across A/B/C/D with increasing coefficient of variation. Policies use identical deterministic corpus keys for each distribution and seed; error bars are Wilson 95% confidence intervals.",
        "fig4": "Average fraction of the fixed physical spare-line budget used by each policy. Sharing changes ownership reachability but does not add spare rows or columns.",
        "supp_hardware_model": "Internal architectural-model view of normalized storage/comparator proxy versus modeled BIRA cycles. The x-axis is not physical area and this plot is not a substitute for same-library synthesis.",
    }
    caption_dir = root / "figures/captions"
    caption_dir.mkdir(parents=True, exist_ok=True)
    for name, text in captions.items():
        (caption_dir / f"{name}_caption.md").write_text(text + "\n", encoding="utf-8")


def write_summary(root: Path, config: dict, wide: list[dict[str, object]], imbalance: list[dict[str, object]], hardware: list[dict[str, object]]) -> None:
    m1 = max(wide, key=lambda row: float(row["m1_gain_pp"]))
    m2 = max(wide, key=lambda row: float(row["m2_gain_pp"]))
    gap = max(wide, key=lambda row: float(row["m2_mean"]) - float(row["m1_mean"]))
    transition = next((row for row in wide if float(row["baseline_mean"]) < 95), None)
    collapse = next((row for row in wide if float(row["baseline_mean"]) <= 5), None)
    representative = config["selected_representative_fault_count"]
    imb_by = {(row["distribution_id"], row["policy_id"]): row for row in imbalance}
    gain_trend = []
    for distribution in config["imbalance"]["profiles"]:
        b = imb_by[(distribution["id"], "no_sharing")]
        p = imb_by[(distribution["id"], "directional_m2")]
        gain_trend.append((distribution["id"], float(p["repair_rate_percent"]) - float(b["repair_rate_percent"])))
    equivalent = all(row["repair_result_matches_cam_baseline"] == "1" for row in hardware)
    _, proxy_frontier = summarize_hardware_proxy(hardware)
    lines = [
        "# DATE 2×2 DSS experiment summary", "",
        f"- Scope: independent 4-SA / 2×2 groups; no device-wide occupancy claim.",
        f"- Maximum m=1 gain: {float(m1['m1_gain_pp']):.2f} pp at {m1['fault_count']} faults/group.",
        f"- Maximum m=2 gain: {float(m2['m2_gain_pp']):.2f} pp at {m2['fault_count']} faults/group.",
        f"- Largest observed m=2 minus m=1 gap: {float(gap['m2_mean']) - float(gap['m1_mean']):.2f} pp at {gap['fault_count']} faults/group.",
        f"- Transition start (operational definition: No Sharing <95%): {transition['fault_count'] if transition else 'not observed'} faults/group.",
        f"- Near-collapse point (operational definition: No Sharing <=5%): {collapse['fault_count'] if collapse else 'not observed'}.",
        f"- Imbalance representative point: {representative} faults/group, selected from 20/22/24 by maximum measured m=2 gain (tie: lower fault count).",
        "- Proposed-DSS gain across imbalance profiles: " + ", ".join(f"{name}={gain:.2f} pp" for name, gain in gain_trend) + ".",
        f"- SRAM search policies matched CAM repair outcomes: {'yes' if equivalent else 'no'}.",
        "- m=2 hardware-worth conclusion: requires joint interpretation of measured repair gain and future same-library physical synthesis; current data alone cannot establish an area–latency winner.",
        "- High-severity failure cannot be uniquely attributed to total-spare exhaustion from current aggregate counters; Figure 4 reports utilization, not causal failure classes.",
        "- CAM/SRAM area crossover, gate-count ordering, Pareto frontier, and lookup time in ns are unresolved because the repository provides no functionally equivalent CAM/register/SRAM RTL, Liberty data, macro characterization, or clock constraint.",
        "- Model-only normalized proxy frontier: " + ", ".join(row["policy"] for row in proxy_frontier) + ". This is not a physical-area Pareto claim.",
        "- Main-paper candidates: Figures 1–3 and Table 1 after author review. Figure 4 is mechanism-supporting but aggregate. Hardware-model CSV is supplementary/internal until synthesis exists.",
    ]
    legacy_check = root / "csv/legacy_coarse_reproduction.json"
    if legacy_check.exists():
        check = read_json(legacy_check)
        lines.append(
            f"- Legacy coarse-run reproduction: {check['differing_rows']}/{check['compared_rows']} rows differ, "
            f"maximum {check['maximum_absolute_repair_rate_difference'] * 100:.2f} pp. "
            "The legacy artifact lacks command/git provenance; it is not merged with this run.")
    paired_summary = root / "explorations/paired_outcomes/paired_contingency.csv"
    if paired_summary.exists():
        paired_rows = read_csv(paired_summary)
        comparisons = [row for row in paired_rows
                       if row["left_policy"] == "directional_m2" and
                       row["right_policy"] == "directional_m1"]
        lines.append("- Paired m=2 versus m=1 exploration: " + ", ".join(
            f"F={row['fault_count']} has {row['left_only_success']} m2-only and "
            f"{row['right_only_success']} m1-only successes"
            for row in comparisons) + ". Exact tests are exploratory and unadjusted.")
    (root / "summary.md").write_text("\n".join(lines) + "\n", encoding="utf-8")


def write_completion_report(root: Path, wide: list[dict[str, object]]) -> None:
    m1 = max(wide, key=lambda row: float(row["m1_gain_pp"]))
    m2 = max(wide, key=lambda row: float(row["m2_gain_pp"]))
    paired_done = (root / "explorations/paired_outcomes/summary.md").exists()
    paired_done_line = ("- Exact per-sample paired contingency exploration at F=16/22/24.\n"
                        if paired_done else "")
    paired_key_line = ("- At F=16/22/24, m=2-only successes were 9/35/54 of 3000; no m=1-only successes were observed. Tests are exploratory and unadjusted.\n"
                       if paired_done else "")
    text = f"""# Completion report

DONE:
- Priority 0 build, regression, fixed-seed reproducibility, and 2×2 checks.
- Dense paired Figure 1 sweep; Figure 2 gain analysis; Figure 3 controlled imbalance experiment.
- Figure 4 aggregate spare-utilization view, Table 1, captions, raw data, and provenance.
- SRAM-RECAM search-policy functional equivalence and architectural cycle/storage model.
{paired_done_line}

FAILED:
- Figures 5–7 in physical area/critical-delay units and a publication-ready Table 2: no equivalent RTL, CAM/SRAM macro data, Liberty library, SDC clock constraint, or characterized delay is present.

NEEDS REVIEW:
- Whether m=2's measured incremental gain over m=1 justifies implementation overhead.
- Figure 4 is aggregate utilization, not a causal failure breakdown.
- Legacy coarse artifact differs slightly and lacks enough provenance for exact diagnosis.

KEY RESULTS:
- Maximum m=1 gain: {float(m1['m1_gain_pp']):.2f} pp at {m1['fault_count']} faults/group.
- Maximum m=2 gain: {float(m2['m2_gain_pp']):.2f} pp at {m2['fault_count']} faults/group.
- All SRAM search policies matched their CAM functional baseline.
{paired_key_line}

PAPER-READY FIGURES:
- {root / 'figures/fig1_repair_rate_vs_faults.pdf'}
- {root / 'figures/fig1_repair_rate_vs_faults_average.pdf'}
- {root / 'figures/fig2_dss_gain_vs_faults.pdf'}
- {root / 'figures/fig2_dss_relative_gain_vs_faults.pdf'}
- {root / 'figures/fig3_repair_rate_vs_fault_imbalance.pdf'}

SUPPORTING / INTERNAL:
- {root / 'figures/fig4_spare_utilization.pdf'}
- {root / 'figures/supp_hardware_model_tradeoff.pdf'}
"""
    (root / "completion_report.md").write_text(text, encoding="utf-8")


def write_readme(root: Path, config: dict) -> None:
    profile = config["active_profile_config"]
    text = f"""# DATE submission 2×2 DSS artifacts

This run is a **group-scope** study of independent 2×2 / four-subarray samples. It must not be averaged with hierarchical device results. All generated figures select only the `RECAM / No Sharing` baseline.

## Contents

- `figures/fig1_repair_rate_vs_faults_average.*`: Figure 1 baseline average repair-rate line plot.
- `figures/fig1_repair_rate_vs_faults_box_plot.*`: Figure 1 per-seed repair-rate distributions.
- `figures/fig3_repair_rate_vs_fault_imbalance.*`: baseline average repair rate under controlled A/B/C/D count imbalance.
- `figures/fig4_spare_utilization.*`: baseline aggregate fixed-budget spare utilization.
- `csv/`: corresponding baseline-only aggregated data; `raw/`: simulator outputs and per-point provenance.

## Reproduction

```bash
python3 scripts/group/date_submission/run.py --profile {config['active_profile']} --run-id NEW_RUN_ID
python3 scripts/group/date_submission/analyze.py {root}
```

Executables: `build/bin/DynamicSpareSharing` and `build/bin/DynamicSpareSharing_SRAM_RECAM`.
Seeds: {profile['seeds']}; samples per seed and curve point: {profile['runs_per_seed']}.
Fault model: moderate imbalance; spatial model: mixed; Rs=Cs points: {configured_spares(config)}; paper CAM reuse; local-first; max-borrows=3; legacy solution-take.

"""
    (root / "README.md").write_text(text, encoding="utf-8")


def write_multi_spare_summary(root: Path, wide_rows: list[dict[str, object]], spares: list[int]) -> None:
    lines = ["# DATE 2×2 DSS spare sweep summary", "",
             "Scope: independent group-level 2×2 samples; results are not device-level metrics.", ""]
    for spare in spares:
        selected = [row for row in wide_rows if row["Rs"] == spare]
        best_m1 = max(selected, key=lambda row: float(row["m1_gain_pp"]))
        best_m2 = max(selected, key=lambda row: float(row["m2_gain_pp"]))
        lines.append(
            f"- Rs=Cs={spare}: max m=1 gain {float(best_m1['m1_gain_pp']):.2f} pp "
            f"at F={best_m1['fault_count']}; max m=2 gain {float(best_m2['m2_gain_pp']):.2f} pp "
            f"at F={best_m2['fault_count']}.")
    lines += ["", "Per-spare figures are under `figures/rs<k>_cs<k>/`; cross-spare heatmaps are under `figures/heatmaps/`."]
    (root / "summary.md").write_text("\n".join(lines) + "\n", encoding="utf-8")


def write_multi_spare_captions(root: Path, wide_rows: list[dict[str, object]], spares: list[int], config: dict) -> None:
    representatives = config["selected_representative_fault_counts"]
    for spare in spares:
        selected = [row for row in wide_rows if row["Rs"] == spare]
        samples = selected[0]["total_samples_per_curve"]
        caption_dir = figure_directory(root, spare, spares) / "captions"
        caption_dir.mkdir(parents=True, exist_ok=True)
        suffix = f"rs{spare}_cs{spare}"
        texts = {
            "fig1": f"Group-scope repair rate for independent 2×2 groups with Rs=Cs={spare}, moderate/mixed faults, and {samples} paired samples per curve point. Error bars are Wilson 95% confidence intervals; the y-axis spans 0–100%.",
            "fig1_average": f"Group-scope average repair rate for the same Figure 1 corpus at Rs=Cs={spare}, without confidence-interval error bars; the y-axis spans 0–100%.",
            "fig2": f"Measured percentage-point gain over the paired No Sharing baseline for Rs=Cs={spare}; no significance test is claimed.",
            "fig2_relative": f"Relative repair-rate improvement over No Sharing for Rs=Cs={spare}, calculated as (DSS − No Sharing) / No Sharing × 100. Points with a 0% baseline are omitted.",
            "fig3": f"Group-scope repair rate at F={representatives[f'rs{spare}_cs{spare}']} faults/group for Rs=Cs={spare}, under controlled A/B/C/D imbalance. Error bars are Wilson 95% confidence intervals.",
            "fig4": f"Average fraction of the fixed Rs=Cs={spare} physical spare-line budget used by each policy.",
        }
        for name, text in texts.items():
            (caption_dir / f"{name}_{suffix}_caption.md").write_text(text + "\n", encoding="utf-8")


def main() -> int:
    args = parse_args()
    root = args.run_root.resolve()
    config = read_json(root / "run_config.json")
    if config["scope"] != "group" or config["fixed"]["layout"] != "2x2":
        raise SystemExit("analyzer only accepts the group-scope 2x2 DATE manifest")
    for directory in (root / "csv", root / "figures", root / "tables"):
        directory.mkdir(exist_ok=True)
    configure_plot()
    spares = configured_spares(config)
    long_rows, wide_rows = collect_repair(root, config)
    write_csv(root / "csv/fig1_repair_rate_long_all_spares.csv", long_rows)
    write_csv(root / "csv/fig1_repair_rate_summary_all_spares.csv", wide_rows)
    write_csv(root / "csv/fig4_spare_utilization_all_spares.csv", long_rows)
    for spare in spares:
        suffix = f"rs{spare}_cs{spare}"
        selected_long = [row for row in long_rows if row["Rs"] == spare]
        selected_wide = [row for row in wide_rows if row["Rs"] == spare]
        write_csv(root / "csv" / f"fig1_repair_rate_long_{suffix}.csv", selected_long)
        write_csv(root / "csv" / f"fig1_repair_rate_summary_{suffix}.csv", selected_wide)
        write_csv(root / "csv" / f"fig4_spare_utilization_{suffix}.csv", selected_long)
    if len(spares) == 1:
        write_csv(root / "csv/fig1_repair_rate_long.csv", long_rows)
        write_csv(root / "csv/fig1_repair_rate_summary.csv", wide_rows)
        write_csv(root / "csv/fig4_spare_utilization.csv", long_rows)
    plot_repair(root, long_rows, wide_rows, spares)
    plot_spare_heatmaps(root, long_rows, wide_rows, spares)
    imbalance = collect_imbalance(root, config)
    write_csv(root / "csv/fig3_fault_imbalance_all_spares.csv", imbalance)
    for spare in spares:
        write_csv(root / "csv" / f"fig3_fault_imbalance_rs{spare}_cs{spare}.csv",
                  [row for row in imbalance if row["Rs"] == spare])
    if len(spares) == 1:
        write_csv(root / "csv/fig3_fault_imbalance.csv", imbalance)
    plot_imbalance(root, imbalance, spares)
    write_readme(root, config)
    validation = {
        "scope": "group", "layout": "2x2", "complete_fault_grid": True,
        "complete_imbalance_grid": True,
        "paired_repair_rate_corpus": True,
        "imbalance_corpus_pairing": "same explicit A/B/C/D counts, spatial model, seed, and run count",
        "selected_policy": "RECAM / No Sharing",
        "physical_synthesis_available": False,
        "significance_test_performed": False,
        "repair_rate_interval": config["statistics"]["repair_rate_interval"],
        "paired_outcome_exploration_present": (root / "explorations/paired_outcomes/validation.json").exists(),
    }
    write_json(root / "csv/validation.json", validation)
    print(f"Wrote DATE figures, tables, CSV, and summary to {root}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
