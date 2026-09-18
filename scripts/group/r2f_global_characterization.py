#!/usr/bin/env python3
"""Non-formal scaling characterization for exact 1x4 Single-Hop GLOBAL."""
import csv
import math
import statistics
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "results/date2026/repair_rate/r2f_global_characterization"
POINTS = [(2, fault) for fault in (8, 12, 16, 20)] + [(3, 12)]
SAMPLES = (10, 50, 100)
METRICS = ("global_runtime_us", "global_search_nodes_visited",
           "global_complete_assignments_checked", "global_legal_complete_assignments",
           "global_raw_cartesian_product_size", "global_partial_assignments_pruned")

def p95(values):
    return sorted(values)[max(0, math.ceil(.95 * len(values)) - 1)]

def main():
    if OUT.exists():
        raise SystemExit(f"fresh output required: {OUT}")
    (OUT / "raw").mkdir(parents=True)
    rows = []
    for n, faults in POINTS:
        for samples in SAMPLES:
            name = f"n{n}_f{faults}_g{samples}"
            directory = OUT / "raw" / name
            command = [str(ROOT / "build/bin/DynamicSpareSharing"), str(n), str(n),
                       "--fault-model", "multinomial_uniform", "--fault-count", str(faults),
                       "--runs", str(samples), "--seed", "20260921", "--spatial", "mixed",
                       "--layout", "1x4", "--topology", "neighbor", "--shared-rows", "1",
                       "--shared-columns", "0", "--solution-take",
                       "one_by_four_single_hop_global_v1", "--paper-cam-reuse",
                       "--hybrid-cam-entry-width-bits", "20", "--summary-only",
                       "--output-dir", str(directory)]
            completed = subprocess.run(command, cwd=ROOT, capture_output=True, text=True)
            (OUT / "raw" / f"{name}.log").write_text(
                completed.stdout + completed.stderr, encoding="utf-8")
            if completed.returncode:
                raise RuntimeError(name)
            with (directory / "paired_policy_results_v1.csv").open(newline="", encoding="utf-8") as source:
                data = list(csv.DictReader(source))
            summary = {"classification": "CHARACTERIZATION_ONLY_NOT_FORMAL",
                       "RS": n, "CS": n, "share_row": 1, "F_GROUP": faults,
                       "groups": samples}
            for metric in METRICS:
                values = [int(row[metric]) for row in data]
                summary[f"mean_{metric}"] = statistics.mean(values)
                summary[f"median_{metric}"] = statistics.median(values)
                summary[f"p95_{metric}"] = p95(values)
                summary[f"max_{metric}"] = max(values)
            worst = max(data, key=lambda row: int(row["global_runtime_us"]))
            summary.update({"worst_group_index": worst["group_id"],
                            "worst_runtime_us": worst["global_runtime_us"],
                            "worst_candidate_counts": "|".join(worst[f"global_candidate_count_{sa}"] for sa in "ABCD"),
                            "worst_fault_counts": "unavailable_in_policy_sidecar"})
            rows.append(summary)
    with (OUT / "summary.csv").open("w", newline="", encoding="utf-8") as target:
        writer = csv.DictWriter(target, fieldnames=list(rows[0]))
        writer.writeheader(); writer.writerows(rows)
    print(OUT)

if __name__ == "__main__":
    main()
