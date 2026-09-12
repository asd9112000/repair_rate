#!/usr/bin/env python3
"""Run the focused DATE Figure 2 fault-imbalance sweep.

The spelling is retained for compatibility with the requested command name.
Each point fixes the group total F=4*mu and uses
(mu-sigma, mu-sigma, mu+sigma, mu+sigma), so the population standard
deviation of A/B/C/D fault counts is exactly sigma.
"""

from __future__ import annotations

import argparse
import datetime as dt
import json
import shlex
import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[3]
SIMULATOR = ROOT / "build/bin/DynamicSpareSharing"
ANALYZER = Path(__file__).with_name("analzye_imbalance.py")
POLICIES = (
    ("no_sharing", "none", 0),
    ("directional_m1", "directional", 1),
)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--run-id")
    parser.add_argument("--output-dir", type=Path)
    parser.add_argument("--rs", type=int, default=2)
    parser.add_argument("--cs", type=int, default=2)
    parser.add_argument("--fault-counts", type=int, nargs="+",
                        default=[16, 20, 24, 28, 32, 36])
    parser.add_argument("--runs", type=int, default=300,
                        help="Monte Carlo samples per seed and point (default: 300)")
    parser.add_argument("--seeds", type=int, nargs="+", default=[20260820])
    parser.add_argument("--spatial", choices=("uniform", "mixed", "clustered"),
                        default="mixed")
    parser.add_argument("--no-build", action="store_true")
    return parser.parse_args()


def write_json(path: Path, value: object) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(value, indent=2, sort_keys=True) + "\n", encoding="utf-8")


def run_logged(command: list[str], directory: Path, metadata: dict) -> None:
    directory.mkdir(parents=True)
    (directory / "command.txt").write_text(shlex.join(command) + "\n", encoding="utf-8")
    write_json(directory / "point.json", metadata)
    print(f"[{metadata['point_id']}]", flush=True)
    with (directory / "run.log").open("w", encoding="utf-8") as log:
        subprocess.run(command, cwd=ROOT, stdout=log, stderr=subprocess.STDOUT, check=True)


def counts_for_sigma(fault_count: int, sigma: int) -> tuple[int, int, int, int]:
    mean = fault_count // 4
    return (mean - sigma, mean - sigma, mean + sigma, mean + sigma)


def main() -> int:
    args = parse_args()
    if args.rs != args.cs or args.rs <= 0:
        raise SystemExit("Figure 2 requires positive symmetric Rs=Cs")
    if args.runs <= 0 or not args.seeds:
        raise SystemExit("--runs and --seeds must be non-empty positive values")
    if any(fault_count <= 0 or fault_count % 4 != 0 for fault_count in args.fault_counts):
        raise SystemExit("--fault-counts must be positive multiples of four")
    if len(set(args.fault_counts)) != len(args.fault_counts):
        raise SystemExit("--fault-counts must not contain duplicates")

    timestamp = dt.datetime.now(dt.timezone.utc).strftime("%Y%m%dT%H%M%SZ")
    run_id = args.run_id or f"figure2_imbalance_{timestamp}"
    output_root = (args.output_dir or ROOT / "reports/group/date_submission_2x2" / run_id).resolve()
    if output_root.exists():
        raise SystemExit(f"output directory already exists: {output_root}")
    output_root.mkdir(parents=True)

    if not args.no_build:
        subprocess.run(["make", "dynamic_sharing_b"], cwd=ROOT, check=True)
    if not SIMULATOR.is_file():
        raise SystemExit(f"missing simulator binary: {SIMULATOR}")

    config = {
        "scope": "group",
        "layout": "2x2",
        "figure": "Figure 2: Repair Rate vs Fault Imbalance",
        "rs": args.rs,
        "cs": args.cs,
        "fault_counts": args.fault_counts,
        "runs_per_seed": args.runs,
        "seeds": args.seeds,
        "spatial_model": args.spatial,
        "policies": [policy[0] for policy in POLICIES],
        "distribution": "(mu-sigma, mu-sigma, mu+sigma, mu+sigma)",
        "stddev_definition": "population standard deviation across A/B/C/D",
        "variance_definition": "population variance across A/B/C/D (= sigma^2)",
        "run_id": run_id,
    }
    write_json(output_root / "run_config.json", config)

    common = [
        "--layout", "2x2", "--spatial", args.spatial,
        "--memory-rows", "512", "--memory-columns", "8192",
        "--runs", str(args.runs), "--paper-cam-reuse", "--local-first",
        "--max-borrows", "3", "--solution-take", "legacy", "--summary-only",
    ]
    for fault_count in args.fault_counts:
        mean = fault_count // 4
        for sigma in range(mean + 1):
            counts = counts_for_sigma(fault_count, sigma)
            for seed in args.seeds:
                for policy_id, topology, shared_lines in POLICIES:
                    point_id = f"f{fault_count}_sigma{sigma}_s{seed}_{policy_id}"
                    point_dir = output_root / "raw" / point_id
                    command = [
                        str(SIMULATOR), str(args.rs), str(args.cs),
                        "--fault-model", "user", "--fault-counts", ",".join(map(str, counts)),
                        "--fault-count", str(fault_count), "--seed", str(seed),
                        "--topology", topology, "--shared-lines", str(shared_lines),
                        *common, "--output-dir", str(point_dir),
                    ]
                    run_logged(command, point_dir, {
                        "kind": "figure2_imbalance", "point_id": point_id,
                        "scope": "group", "layout": "2x2", "rs": args.rs, "cs": args.cs,
                        "fault_count": fault_count, "mean_fault_count": mean,
                        "std_fault_count": sigma, "normalized_std_fault_count": sigma / mean,
                        "fault_counts_abcd": counts, "seed": seed, "runs": args.runs,
                        "policy_id": policy_id,
                    })

    subprocess.run([sys.executable, str(ANALYZER), str(output_root)], cwd=ROOT, check=True)
    print(f"Completed Figure 2 imbalance run: {output_root}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
