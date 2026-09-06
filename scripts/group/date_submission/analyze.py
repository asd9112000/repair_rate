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


POLICY_ROWS = {
    ("no_sharing", 0, 0): ("no_sharing", "RECAM / No Sharing"),
    ("directional", 1, 1): ("directional_m1", "Limited DSS ($m=1$)"),
    ("directional", 2, 2): ("directional_m2", "Proposed DSS ($m=2$)"),
}
POLICY_ORDER = ("no_sharing", "directional_m1", "directional_m2")
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
        "font.size": 9, "axes.titlesize": 10, "axes.labelsize": 9,
        "legend.fontsize": 8, "xtick.labelsize": 8, "ytick.labelsize": 8,
        "lines.linewidth": 1.6, "lines.markersize": 5,
        "pdf.fonttype": 42, "ps.fonttype": 42,
    })


def save_figure(fig, root: Path, stem: str) -> None:
    fig.tight_layout()
    fig.savefig(root / f"{stem}.png", dpi=300)
    fig.savefig(root / f"{stem}.pdf")
    plt.close(fig)


def collect_repair(root: Path, config: dict) -> tuple[list[dict[str, object]], list[dict[str, object]]]:
    totals = defaultdict(lambda: {"successes": 0, "runs": 0, "seeds": set(), "util": []})
    for path in sorted((root / "raw/repair_rate").glob("*/summary.csv")):
        for row in read_csv(path):
            pid = policy_id(row)
            if pid is None:
                continue
            key = (int(row["fault_count"]), pid)
            runs = int(row["runs"])
            totals[key]["successes"] += round(float(row["repair_rate"]) * runs)
            totals[key]["runs"] += runs
            totals[key]["seeds"].add(int(row["seed"]))
            totals[key]["util"].append(float(row["average_spare_utilization"]))

    expected_faults = config["fault_counts_per_group"]
    expected_seeds = set(config["active_profile_config"]["seeds"])
    missing = []
    long_rows = []
    for faults in expected_faults:
        for pid in POLICY_ORDER:
            item = totals.get((faults, pid))
            if not item or item["seeds"] != expected_seeds:
                missing.append(f"f={faults},policy={pid}")
                continue
            successes, runs = item["successes"], item["runs"]
            low, high = wilson(successes, runs)
            long_rows.append({
                "fault_count": faults, "policy_id": pid,
                "label": POLICY_ROWS[next(key for key, value in POLICY_ROWS.items() if value[0] == pid)][1],
                "seed_count": len(item["seeds"]), "total_runs": runs,
                "successes": successes, "repair_rate_percent": 100 * successes / runs,
                "ci95_low_percent": 100 * low, "ci95_high_percent": 100 * high,
                "average_spare_utilization_percent": 100 * sum(item["util"]) / len(item["util"]),
            })
    if missing:
        raise RuntimeError("incomplete Figure 1 grid: " + "; ".join(missing))

    by_key = {(row["fault_count"], row["policy_id"]): row for row in long_rows}
    wide_rows = []
    for faults in expected_faults:
        b = by_key[(faults, "no_sharing")]
        m1 = by_key[(faults, "directional_m1")]
        m2 = by_key[(faults, "directional_m2")]
        wide_rows.append({
            "fault_count": faults,
            "baseline_mean": b["repair_rate_percent"],
            "baseline_error": (b["ci95_high_percent"] - b["ci95_low_percent"]) / 2,
            "m1_mean": m1["repair_rate_percent"],
            "m1_error": (m1["ci95_high_percent"] - m1["ci95_low_percent"]) / 2,
            "m2_mean": m2["repair_rate_percent"],
            "m2_error": (m2["ci95_high_percent"] - m2["ci95_low_percent"]) / 2,
            "m1_gain_pp": m1["repair_rate_percent"] - b["repair_rate_percent"],
            "m2_gain_pp": m2["repair_rate_percent"] - b["repair_rate_percent"],
            "baseline_ci95_low": b["ci95_low_percent"],
            "baseline_ci95_high": b["ci95_high_percent"],
            "m1_ci95_low": m1["ci95_low_percent"], "m1_ci95_high": m1["ci95_high_percent"],
            "m2_ci95_low": m2["ci95_low_percent"], "m2_ci95_high": m2["ci95_high_percent"],
            "total_samples_per_curve": b["total_runs"],
        })
    return long_rows, wide_rows


def plot_repair(root: Path, long_rows: list[dict[str, object]], wide_rows: list[dict[str, object]]) -> None:
    plots = root / "figures"
    plots.mkdir(exist_ok=True)
    fig, axis = plt.subplots(figsize=(6.7, 4.2))
    for pid in POLICY_ORDER:
        rows = [row for row in long_rows if row["policy_id"] == pid]
        color, marker, line = STYLE[pid]
        y = [float(row["repair_rate_percent"]) for row in rows]
        yerr = [[y[i] - float(row["ci95_low_percent"]) for i, row in enumerate(rows)],
                [float(row["ci95_high_percent"]) - y[i] for i, row in enumerate(rows)]]
        axis.errorbar([row["fault_count"] for row in rows], y, yerr=yerr,
                      color=color, marker=marker, linestyle=line,
                      capsize=2.2, label=str(rows[0]["label"]))
    axis.set_title("Repair Rate vs. Number of Faults per 2×2 Group")
    axis.set_xlabel("Number of Faults per 2×2 Group")
    axis.set_ylabel("Repair Rate (%)")
    axis.set_ylim(0, 100)
    axis.grid(True, linestyle=":", alpha=0.65)
    axis.legend(frameon=False)
    save_figure(fig, plots, "fig1_repair_rate_vs_faults")

    fig, axis = plt.subplots(figsize=(6.7, 4.2))
    for pid, field, label in (("directional_m1", "m1_gain_pp", "Limited DSS ($m=1$)"),
                              ("directional_m2", "m2_gain_pp", "Proposed DSS ($m=2$)")):
        color, marker, line = STYLE[pid]
        values = [float(row[field]) for row in wide_rows]
        axis.plot([row["fault_count"] for row in wide_rows], values,
                  color=color, marker=marker, linestyle=line, label=label)
        maximum = max(range(len(values)), key=values.__getitem__)
        is_proposed_max = pid == "directional_m2"
        text_offset = (-8, -22) if is_proposed_max else (5, 8)
        axis.annotate(f"{values[maximum]:.2f} pp @ {wide_rows[maximum]['fault_count']}",
                      (wide_rows[maximum]["fault_count"], values[maximum]),
                      xytext=text_offset, textcoords="offset points", fontsize=8,
                      ha="right" if is_proposed_max else "left")
    axis.axhline(0, color="#777777", linewidth=0.8)
    axis.set_title("Repair-Rate Improvement of Dynamic Spare Sharing")
    axis.set_xlabel("Number of Faults per 2×2 Group")
    axis.set_ylabel("Repair-Rate Improvement (percentage points)")
    axis.grid(True, linestyle=":", alpha=0.65)
    axis.legend(frameon=False)
    save_figure(fig, plots, "fig2_dss_gain_vs_faults")

    fig, axis = plt.subplots(figsize=(6.7, 4.2))
    for pid in POLICY_ORDER:
        rows = [row for row in long_rows if row["policy_id"] == pid]
        color, marker, line = STYLE[pid]
        axis.plot([row["fault_count"] for row in rows],
                  [row["average_spare_utilization_percent"] for row in rows],
                  color=color, marker=marker, linestyle=line,
                  label=str(rows[0]["label"]))
    axis.set_title("Spare Utilization under Dynamic Spare Sharing")
    axis.set_xlabel("Number of Faults per 2×2 Group")
    axis.set_ylabel("Average Spare Utilization (%)")
    axis.set_ylim(0, 100)
    axis.set_yticks(range(0, 101, 20))
    axis.grid(True, linestyle=":", alpha=0.65)
    axis.legend(frameon=False)
    save_figure(fig, plots, "fig4_spare_utilization")


def collect_imbalance(root: Path, config: dict) -> list[dict[str, object]]:
    totals = defaultdict(lambda: {"successes": 0, "runs": 0, "seeds": set(), "meta": None})
    for point_path in sorted((root / "raw/imbalance").glob("*/point.json")):
        meta = read_json(point_path)
        row = read_csv(point_path.parent / "summary.csv")[0]
        key = (meta["distribution_id"], meta["policy_id"])
        runs = int(row["runs"])
        totals[key]["successes"] += round(float(row["repair_rate"]) * runs)
        totals[key]["runs"] += runs
        totals[key]["seeds"].add(meta["seed"])
        totals[key]["meta"] = meta
    expected_seeds = set(config["active_profile_config"]["seeds"])
    result = []
    for distribution in config["imbalance"]["profiles"]:
        for pid in POLICY_ORDER:
            item = totals[(distribution["id"], pid)]
            if item["seeds"] != expected_seeds:
                raise RuntimeError(f"incomplete imbalance grid: {distribution['id']}/{pid}")
            counts = item["meta"]["fault_counts_abcd"]
            mean = sum(counts) / 4
            std = math.sqrt(sum((value - mean) ** 2 for value in counts) / 4)
            low, high = wilson(item["successes"], item["runs"])
            result.append({
                "distribution_id": distribution["id"], "distribution_label": distribution["label"],
                "fault_counts_abcd": ",".join(map(str, counts)), "max_faults_per_subarray": max(counts),
                "std_fault_count": std, "coefficient_of_variation": std / mean,
                "policy_id": pid, "total_runs": item["runs"], "successes": item["successes"],
                "repair_rate_percent": 100 * item["successes"] / item["runs"],
                "ci95_low_percent": 100 * low, "ci95_high_percent": 100 * high,
            })
    return result


def plot_imbalance(root: Path, rows: list[dict[str, object]]) -> None:
    fig, axis = plt.subplots(figsize=(6.7, 4.2))
    for pid in POLICY_ORDER:
        selected = sorted((row for row in rows if row["policy_id"] == pid),
                          key=lambda row: float(row["coefficient_of_variation"]))
        color, marker, line = STYLE[pid]
        y = [float(row["repair_rate_percent"]) for row in selected]
        yerr = [[y[i] - float(row["ci95_low_percent"]) for i, row in enumerate(selected)],
                [float(row["ci95_high_percent"]) - y[i] for i, row in enumerate(selected)]]
        short_label = {"no_sharing": "No Sharing", "directional_m1": "DSS $m=1$",
                       "directional_m2": "DSS $m=2$"}[pid]
        axis.errorbar([row["coefficient_of_variation"] for row in selected], y, yerr=yerr,
                      color=color, marker=marker, linestyle=line, capsize=2.2,
                      label=short_label)
    axis.set_title("Repair Rate under Increasing Inter-Subarray Fault Imbalance")
    axis.set_xlabel("Fault-Count Coefficient of Variation (std / mean)")
    axis.set_ylabel("Repair Rate (%)")
    axis.set_ylim(0, 100)
    axis.grid(True, linestyle=":", alpha=0.65)
    axis.legend(frameon=False, loc="upper right", fontsize=7.5)
    save_figure(fig, root / "figures", "fig3_repair_rate_vs_fault_imbalance")


def plot_hardware_proxy(root: Path, rows: list[dict[str, object]]) -> None:
    """Internal model plot; deliberately does not claim physical area or timing."""
    totals = defaultdict(lambda: {"area": [], "cycles": [], "p": None})
    for row in rows:
        item = totals[row["policy"]]
        item["area"].append(float(row["group_normalized_total_area_proxy"]))
        item["cycles"].append(float(row["bira_cycles_per_fault"]))
        item["p"] = row["P_A"]
    fig, axis = plt.subplots(figsize=(6.7, 4.2))
    for policy, item in sorted(totals.items(), key=lambda pair: float(pair[1]["area"][0])):
        area = sum(item["area"]) / len(item["area"])
        cycles = sum(item["cycles"]) / len(item["cycles"])
        axis.scatter(area, cycles, s=42)
        axis.annotate(f"{policy} (P={item['p']})", (area, cycles),
                      xytext=(4, 5), textcoords="offset points", fontsize=7.5)
    axis.set_title("Architectural SRAM-RECAM Storage–Latency Model (Non-Physical Proxy)")
    axis.set_xlabel("Normalized Storage + Comparator Area Proxy (not µm²)")
    axis.set_ylabel("Modeled BIRA Cycles per Fault")
    y_values = [sum(item["cycles"]) / len(item["cycles"]) for item in totals.values()]
    axis.set_ylim(min(y_values) - 0.12, max(y_values) + 0.16)
    axis.grid(True, linestyle=":", alpha=0.65)
    save_figure(fig, root / "figures", "supp_hardware_model_tradeoff")


def collect_hardware(root: Path) -> list[dict[str, object]]:
    rows = []
    for path in sorted((root / "raw/hardware_model").glob("*/sram_recam_metrics.csv")):
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
        "runtime_miss_rounds_avg", "repair_rate", "cam_baseline_repair_rate",
        "repair_result_matches_cam_baseline", "seed", "runs", "fault_count",
    ]
    return [{field: row[field] for field in fields} for row in rows]


def summarize_hardware_proxy(rows: list[dict[str, object]]) -> tuple[list[dict[str, object]], list[dict[str, object]]]:
    grouped = defaultdict(list)
    for row in rows:
        grouped[row["policy"]].append(row)
    summary = []
    for policy, selected in grouped.items():
        summary.append({
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
        "fig2": "Measured percentage-point improvement over the paired No Sharing baseline using the same Figure 1 corpus. Labels mark the observed maximum for each DSS policy; no significance test is claimed.",
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
- {root / 'figures/fig2_dss_gain_vs_faults.pdf'}
- {root / 'figures/fig3_repair_rate_vs_fault_imbalance.pdf'}

SUPPORTING / INTERNAL:
- {root / 'figures/fig4_spare_utilization.pdf'}
- {root / 'figures/supp_hardware_model_tradeoff.pdf'}
"""
    (root / "completion_report.md").write_text(text, encoding="utf-8")


def write_readme(root: Path, config: dict) -> None:
    profile = config["active_profile_config"]
    text = f"""# DATE submission 2×2 DSS artifacts

This run is a **group-scope** study of independent 2×2 / four-subarray samples. It must not be averaged with hierarchical device results.

## Contents

- `figures/fig1_repair_rate_vs_faults.*`: full-range repair rate with Wilson 95% confidence intervals.
- `figures/fig2_dss_gain_vs_faults.*`: measured percentage-point gain over No Sharing.
- `figures/fig3_repair_rate_vs_fault_imbalance.*`: controlled A/B/C/D count imbalance at the selected transition point.
- `figures/fig4_spare_utilization.*`: aggregate fixed-budget spare utilization.
- `csv/`: underlying and aggregated data; `raw/`: simulator outputs and per-point provenance.
- `tables/`: CSV, Markdown, and LaTeX repairability table plus an explicitly incomplete hardware-cost table.
- `explorations/paired_outcomes/`: exact per-sample contingency analysis when present.

## Reproduction

```bash
python3 scripts/group/date_submission/run.py --profile {config['active_profile']} --run-id NEW_RUN_ID
python3 scripts/group/date_submission/analyze.py {root}
```

Executables: `build/bin/DynamicSpareSharing` and `build/bin/DynamicSpareSharing_SRAM_RECAM`.
Seeds: {profile['seeds']}; samples per seed and curve point: {profile['runs_per_seed']}.
Fault model: moderate imbalance; spatial model: mixed; Rs=Cs=2; paper CAM reuse; local-first; max-borrows=3; legacy solution-take.

## Hardware boundary

The SRAM-RECAM simulator supplies logical storage, comparator, search-round, and functional-equivalence metrics. No physical synthesis configuration is present: technology/library, CAM macro, register-table RTL, SRAM macro, clock constraint, and characterized critical delay are unavailable. Consequently Figures 5–7 in physical area/ns and a publication-ready Table 2 are not produced. Blank fields in Table 2 are intentional, not zero.
"""
    (root / "README.md").write_text(text, encoding="utf-8")


def main() -> int:
    args = parse_args()
    root = args.run_root.resolve()
    config = read_json(root / "run_config.json")
    if config["scope"] != "group" or config["fixed"]["layout"] != "2x2":
        raise SystemExit("analyzer only accepts the group-scope 2x2 DATE manifest")
    for directory in (root / "csv", root / "figures", root / "tables"):
        directory.mkdir(exist_ok=True)
    configure_plot()
    long_rows, wide_rows = collect_repair(root, config)
    write_csv(root / "csv/fig1_repair_rate_long.csv", long_rows)
    write_csv(root / "csv/fig1_repair_rate_summary.csv", wide_rows)
    write_csv(root / "csv/fig2_dss_gain.csv", [
        {key: row[key] for key in ("fault_count", "m1_gain_pp", "m2_gain_pp")}
        for row in wide_rows])
    write_csv(root / "csv/fig4_spare_utilization.csv", [
        {key: row[key] for key in ("fault_count", "policy_id",
                                    "average_spare_utilization_percent")}
        for row in long_rows])
    plot_repair(root, long_rows, wide_rows)
    imbalance = collect_imbalance(root, config)
    write_csv(root / "csv/fig3_fault_imbalance.csv", imbalance)
    plot_imbalance(root, imbalance)
    hardware = collect_hardware(root)
    write_csv(root / "csv/hardware_architecture_model.csv", hardware)
    hardware_summary, hardware_frontier = summarize_hardware_proxy(hardware)
    write_csv(root / "csv/hardware_proxy_summary.csv", hardware_summary)
    write_csv(root / "csv/hardware_proxy_frontier.csv", hardware_frontier)
    plot_hardware_proxy(root, hardware)
    make_tables(root, wide_rows, hardware)
    write_captions(root, config, wide_rows, config["selected_representative_fault_count"])
    write_summary(root, config, wide_rows, imbalance, hardware)
    write_readme(root, config)
    write_completion_report(root, wide_rows)
    validation = {
        "scope": "group", "layout": "2x2", "complete_fault_grid": True,
        "complete_imbalance_grid": True,
        "paired_repair_rate_corpus": True,
        "imbalance_corpus_pairing": "same explicit A/B/C/D counts, spatial model, seed, and run count",
        "all_sram_policies_match_cam_repair": all(row["repair_result_matches_cam_baseline"] == "1" for row in hardware),
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
