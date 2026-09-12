#!/usr/bin/env python3
"""Summarize terminal Design Compiler Phase 2 reports into one CSV."""

from __future__ import annotations

import argparse
import csv
import re
from pathlib import Path


def metadata(path: Path) -> dict[str, str]:
    values: dict[str, str] = {}
    for line in path.read_text(encoding="utf-8").splitlines():
        if "=" in line:
            key, value = line.split("=", 1)
            values[key] = value
    return values


def value(text: str, pattern: str) -> str:
    match = re.search(pattern, text, re.MULTILINE)
    return match.group(1) if match else ""


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--input-root", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    rows = []
    for meta_path in sorted(args.input_root.glob("*/metadata.txt")):
        info = metadata(meta_path)
        if info.get("STATUS") != "PASS":
            continue
        report_dir = meta_path.parent
        area_path = report_dir / "area.rpt"
        qor_path = report_dir / "qos.rpt"
        if not area_path.is_file() or not qor_path.is_file():
            continue
        area = area_path.read_text(encoding="utf-8", errors="replace")
        qor = qor_path.read_text(encoding="utf-8", errors="replace")
        rows.append({
            "configuration": info.get("CONFIGURATION", report_dir.name),
            "top": info.get("TOP", ""),
            "elab_params": info.get("ELAB_PARAMS", ""),
            "compile_mode": info.get("COMPILE_MODE", ""),
            "clock_period_ns": info.get("CLOCK_PERIOD_NS", ""),
            "input_delay_ns": info.get("INPUT_DELAY_NS", ""),
            "output_delay_ns": info.get("OUTPUT_DELAY_NS", ""),
            "cell_count": value(area, r"^Number of cells:\s+([0-9.]+)"),
            "combinational_area": value(area, r"^Combinational area:\s+([0-9.]+)"),
            "sequential_area": value(area, r"^Noncombinational area:\s+([0-9.]+)"),
            "cell_area": value(area, r"^Total cell area:\s+([0-9.]+)"),
            "design_area": value(area, r"^Total area:\s+([0-9.]+)"),
            "critical_path_ns": value(qor, r"^\s*Critical Path Length:\s+([0-9.-]+)"),
            "critical_path_slack_ns": value(qor, r"^\s*Critical Path Slack:\s+([0-9.-]+)"),
            "violating_nets": value(qor, r"^\s*Nets With Violations:\s+([0-9.]+)"),
            "status": info["STATUS"],
            "report_dir": str(report_dir),
        })
    if not rows:
        raise SystemExit("no terminal PASS DC reports found")
    args.output.parent.mkdir(parents=True, exist_ok=True)
    with args.output.open("w", encoding="utf-8", newline="") as output:
        writer = csv.DictWriter(output, fieldnames=rows[0].keys())
        writer.writeheader()
        writer.writerows(rows)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
