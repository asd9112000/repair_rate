#!/usr/bin/env python3
"""Convert Yosys JSON plus provenance metadata into the RTL result schema."""

from __future__ import annotations

import argparse
import csv
import json
from collections import Counter
from pathlib import Path


FIELDS = [
    "phase",
    "architecture",
    "configuration",
    "module",
    "ENTRY_NUM",
    "ADDR_WIDTH",
    "RS",
    "CS",
    "P",
    "FIFO_DEPTH",
    "technology",
    "library",
    "clock_period_ns",
    "total_cell_area",
    "combinational_area",
    "sequential_area",
    "cell_count",
    "critical_path_ns",
    "frequency_mhz",
    "status",
    "cell_types",
    "source_json",
]


def parse_metadata(path: Path) -> dict[str, str]:
    result: dict[str, str] = {}
    if not path.exists():
        return result
    for raw_line in path.read_text(encoding="utf-8").splitlines():
        if "=" not in raw_line:
            continue
        key, value = raw_line.split("=", 1)
        result[key.strip()] = value.strip()
    return result


def parse_parameters(text: str) -> dict[str, str]:
    result: dict[str, str] = {}
    for assignment in text.split(","):
        if "=" not in assignment:
            continue
        key, value = assignment.split("=", 1)
        result[key.strip()] = value.strip()
    return result


def select_module(netlist: dict, requested_top: str) -> tuple[str, dict]:
    modules = netlist.get("modules", {})
    if requested_top in modules:
        return requested_top, modules[requested_top]
    for name, module in modules.items():
        if module.get("attributes", {}).get("top") in (1, "1"):
            return name, module
    if len(modules) == 1:
        return next(iter(modules.items()))
    raise ValueError(f"cannot identify top module {requested_top!r}")


def collect_row(json_path: Path, input_root: Path) -> dict[str, str | int]:
    metadata = parse_metadata(json_path.parent / "metadata.txt")
    parameters = parse_parameters(metadata.get("PARAMS", ""))
    netlist = json.loads(json_path.read_text(encoding="utf-8"))
    module_name, module = select_module(netlist, metadata.get("TOP", json_path.stem))
    type_counts = Counter(
        cell.get("type", "UNKNOWN") for cell in module.get("cells", {}).values()
    )
    top = metadata.get("TOP", module_name)
    return {
        "phase": metadata.get("PHASE", ""),
        "architecture": "PRIMITIVE" if metadata.get("PHASE") == "1" else "RTL",
        "configuration": metadata.get("CONFIGURATION", json_path.parent.name),
        "module": top,
        "ENTRY_NUM": parameters.get("ENTRY_NUM", ""),
        "ADDR_WIDTH": parameters.get("ADDR_WIDTH", parameters.get("KEY_WIDTH", "")),
        "RS": parameters.get("RS", ""),
        "CS": parameters.get("CS", ""),
        "P": parameters.get("P", ""),
        "FIFO_DEPTH": parameters.get("FIFO_DEPTH", parameters.get("DEPTH", "")),
        "technology": metadata.get("TECHNOLOGY", ""),
        "library": metadata.get("LIBRARY", ""),
        "clock_period_ns": metadata.get("CLK_PERIOD_NS", ""),
        "total_cell_area": "",
        "combinational_area": "",
        "sequential_area": "",
        "cell_count": sum(type_counts.values()),
        "critical_path_ns": "",
        "frequency_mhz": "",
        "status": metadata.get("STATUS", "UNSPECIFIED"),
        "cell_types": ";".join(
            f"{cell_type}:{count}" for cell_type, count in sorted(type_counts.items())
        ),
        "source_json": str(json_path.relative_to(input_root.parent.parent)),
    }


def collect_module_breakdown(json_path: Path, input_root: Path) -> list[dict[str, str | int]]:
    """Return direct, pre-flatten cell counts for each elaborated module."""
    metadata = parse_metadata(json_path.parent / "metadata.txt")
    netlist = json.loads(json_path.read_text(encoding="utf-8"))
    rows: list[dict[str, str | int]] = []
    for module_name, module in sorted(netlist.get("modules", {}).items()):
        type_counts = Counter(
            cell.get("type", "UNKNOWN") for cell in module.get("cells", {}).values()
        )
        rows.append(
            {
                "phase": metadata.get("PHASE", ""),
                "configuration": metadata.get("CONFIGURATION", json_path.parent.name),
                "top_module": metadata.get("TOP", ""),
                "module": module_name,
                "direct_cell_count": sum(type_counts.values()),
                "cell_types": ";".join(
                    f"{cell_type}:{count}"
                    for cell_type, count in sorted(type_counts.items())
                ),
                "technology": metadata.get("TECHNOLOGY", ""),
                "library": metadata.get("LIBRARY", ""),
                "status": metadata.get("STATUS", "UNSPECIFIED"),
                "source_json": str(json_path.relative_to(input_root.parent.parent)),
            }
        )
    return rows


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--input-root", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--module-breakdown-output", type=Path)
    args = parser.parse_args()

    json_paths = sorted(
        path for path in args.input_root.glob("*/*.json")
        if not path.name.endswith(".hierarchy.json")
    )
    if not json_paths:
        raise SystemExit(f"no Yosys JSON reports found below {args.input_root}")
    rows = [collect_row(path, args.input_root) for path in json_paths]
    configurations = [str(row["configuration"]) for row in rows]
    if len(configurations) != len(set(configurations)):
        raise SystemExit("duplicate synthesis configuration in parsed reports")

    args.output.parent.mkdir(parents=True, exist_ok=True)
    with args.output.open("w", encoding="utf-8", newline="") as output_file:
        writer = csv.DictWriter(output_file, fieldnames=FIELDS)
        writer.writeheader()
        writer.writerows(rows)

    if args.module_breakdown_output:
        breakdown_paths = sorted(args.input_root.glob("*/*.hierarchy.json"))
        if not breakdown_paths:
            raise SystemExit("no pre-flatten hierarchy JSON reports found")
        breakdown_rows = [
            row
            for path in breakdown_paths
            for row in collect_module_breakdown(path, args.input_root)
        ]
        breakdown_fields = [
            "phase", "configuration", "top_module", "module", "direct_cell_count",
            "cell_types", "technology", "library", "status", "source_json",
        ]
        args.module_breakdown_output.parent.mkdir(parents=True, exist_ok=True)
        with args.module_breakdown_output.open(
            "w", encoding="utf-8", newline=""
        ) as output_file:
            writer = csv.DictWriter(output_file, fieldnames=breakdown_fields)
            writer.writeheader()
            writer.writerows(breakdown_rows)
    print(f"parsed {len(rows)} synthesis reports into {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
