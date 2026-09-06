#!/usr/bin/env python3
"""Run canonical B0/B1/B2/B3 parameter points and merge long-format CSV."""

from __future__ import annotations

import argparse
import csv
import itertools
import shlex
import subprocess
from pathlib import Path


def integers(text: str) -> list[int]:
    values = [int(field) for field in text.split(",")]
    if not values or any(value <= 0 for value in values):
        raise argparse.ArgumentTypeError("requires positive comma-separated integers")
    return values


def strings(text: str) -> list[str]:
    values = [field.strip() for field in text.split(",") if field.strip()]
    if not values:
        raise argparse.ArgumentTypeError("requires a non-empty comma-separated list")
    return values


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--simulator", type=Path,
                        default=Path("build/bin/HierarchicalRECAM"))
    parser.add_argument("--output-dir", type=Path, required=True)
    parser.add_argument("--rs", type=integers, default=[2])
    parser.add_argument("--cs", type=integers, default=[2])
    parser.add_argument("--fault-counts", type=integers, default=[8, 12, 16, 20])
    parser.add_argument("--word-bits", type=integers, default=[256])
    parser.add_argument("--seeds", type=integers, default=[20260820])
    parser.add_argument("--sram-policies", type=strings, default=["chunked:2"])
    parser.add_argument("--groups", type=int, default=32)
    parser.add_argument("--memory-rows", type=int, default=512)
    parser.add_argument("--memory-columns", type=int, default=8192)
    parser.add_argument("--topology", choices=["directional", "edge", "global"],
                        default="edge")
    parser.add_argument("--shared-rows", type=int, default=1)
    parser.add_argument("--shared-columns", type=int, default=0)
    parser.add_argument("--max-borrows", type=int, default=1)
    parser.add_argument("--online-entries", type=int)
    parser.add_argument("--registered-sram-search", action="store_true")
    parser.add_argument("--overwrite", action="store_true")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    simulator = args.simulator.resolve()
    if not simulator.is_file():
        raise SystemExit(f"simulator does not exist: {simulator}")
    if args.groups <= 0 or args.memory_rows <= 0 or args.memory_columns <= 0:
        raise SystemExit("groups and memory dimensions must be positive")
    if args.output_dir.exists() and not args.overwrite:
        raise SystemExit(f"output exists; pass --overwrite: {args.output_dir}")
    args.output_dir.mkdir(parents=True, exist_ok=True)

    merged: list[dict[str, str]] = []
    axes = itertools.product(
        args.rs, args.cs, args.fault_counts, args.word_bits,
        args.seeds, args.sram_policies)
    for point, (rs, cs, faults, word_bits, seed, policy) in enumerate(axes):
        if args.memory_columns % word_bits:
            raise SystemExit(
                f"memory-columns={args.memory_columns} is not divisible by "
                f"word-bits={word_bits}")
        run_id = (
            f"p{point:04d}_r{rs}_c{cs}_f{faults}_w{word_bits}_"
            f"s{seed}_{policy.replace(':', '-')}")
        run_dir = args.output_dir / "runs" / run_id
        command = [
            str(simulator), str(rs), str(cs), "--canonical-four",
            "--groups", str(args.groups), "--fault-count", str(faults),
            "--seed", str(seed), "--memory-rows", str(args.memory_rows),
            "--memory-columns", str(args.memory_columns),
            "--data-word-bits", str(word_bits), "--topology", args.topology,
            "--shared-rows", str(args.shared_rows),
            "--shared-columns", str(args.shared_columns),
            "--max-borrows", str(args.max_borrows),
            "--sram-policy", policy, "--output-dir", str(run_dir),
        ]
        if args.online_entries is not None:
            command.extend(["--online-global-reuse-entries",
                            str(args.online_entries)])
        if args.registered_sram_search:
            command.append("--registered-sram-search")
        subprocess.run(command, check=True)
        (run_dir / "command.txt").write_text(
            shlex.join(command) + "\n", encoding="utf-8")

        with (run_dir / "canonical_four_summary.csv").open(
                newline="", encoding="utf-8") as source:
            for row in csv.DictReader(source):
                row = dict(row)
                row.update({
                    "run_id": run_id,
                    "sweep_fault_count": str(faults),
                    "sweep_seed": str(seed),
                    "sweep_word_bits": str(word_bits),
                    "sweep_sram_policy": policy,
                })
                merged.append(row)

    if not merged:
        raise SystemExit("sweep produced no rows")
    output_path = args.output_dir / "canonical_four_sweep.csv"
    fieldnames = [
        "run_id", "sweep_fault_count", "sweep_seed", "sweep_word_bits",
        "sweep_sram_policy",
    ] + [name for name in merged[0] if name not in {
        "run_id", "sweep_fault_count", "sweep_seed", "sweep_word_bits",
        "sweep_sram_policy"}]
    with output_path.open("w", newline="", encoding="utf-8") as output:
        writer = csv.DictWriter(output, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(merged)
    print(f"Wrote {len(merged)} canonical rows to {output_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
