#!/usr/bin/env python3
"""Regression contract for R3 group repair-rate and imbalance plotters."""

from __future__ import annotations

import csv
import os
import struct
import subprocess
import sys
import tempfile
from pathlib import Path

os.environ.setdefault("MPLCONFIGDIR", "/tmp/r3-group-plot-test-matplotlib")
import matplotlib.pyplot as plt


ROOT = Path(__file__).resolve().parents[1]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from scripts.plot.r3_group import plot_fault_imbalance as imbalance_plot
from scripts.plot.r3_group import plot_group_repair_rate as repair_plot
from scripts.plot.r3_group.common import (COMPACT_LEGEND_LABELS, STYLE, add_legend,
                                          legend_below_axes, legend_within_canvas,
                                          make_single_panel_figure,
                                          reserve_dense_legend_space)


DATASET_CLASS = "QUICK_SWEEP / DEVELOPMENT / NON-FORMAL"
POLICIES = (
    "local_no_sharing",
    "directional_m1_local_first",
    "directional_m1_early",
    "directional_m1_global",
    "pairwise_row_m1_local_first",
    "pairwise_row_m1_early",
    "pairwise_row_m1_global",
    "two_pairwise_m1_local_first",
    "two_pairwise_m1_early",
    "two_pairwise_m1_pair_global",
    "single_hop_m1_local_first",
    "single_hop_m1_early",
    "single_hop_m1_global",
)
VIEWS = {
    "all": POLICIES,
    "topology_2_2": POLICIES[:7],
    "topology_1_4": (POLICIES[0], *POLICIES[7:]),
    "policy_early": (POLICIES[0], "directional_m1_early", "two_pairwise_m1_early", "single_hop_m1_early"),
    "policy_global": (POLICIES[0], "directional_m1_global", "two_pairwise_m1_pair_global", "single_hop_m1_global"),
}
REPAIR_STEMS = {
    "all": "fig_r3_repair_rate",
    "topology_2_2": "topology_2_2_repair_rate",
    "topology_1_4": "topology_1_4_repair_rate",
    "policy_early": "policy_early_repair_rate",
    "policy_global": "policy_global_repair_rate",
}
IMBALANCE_STEMS = {
    "all": "fig_r3_imbalance",
    "topology_2_2": "topology_2_2_imbalance",
    "topology_1_4": "topology_1_4_imbalance",
    "policy_early": "policy_early_imbalance",
    "policy_global": "policy_global_imbalance",
}
COMPACT_LABELS = {
    "local_no_sharing": "LOCAL",
    "directional_m1_local_first": "Dir. V2 LOCAL-FIRST",
    "directional_m1_early": "Dir. V2 EARLY",
    "directional_m1_global": "Dir. V2 GLOBAL",
    "directional_m1_group_global": "Dir. generic GROUP_GLOBAL (legacy)",
    "pairwise_row_m1_local_first": "PairRow-LOCAL",
    "pairwise_row_m1_early": "PairRow-EARLY",
    "pairwise_row_m1_global": "PairRow-GLOBAL",
    "two_pairwise_m1_local_first": "2Pair-LOCAL",
    "two_pairwise_m1_early": "2Pair-EARLY",
    "two_pairwise_m1_pair_global": "2Pair-GLOBAL",
    "single_hop_m1_local_first": "1Hop-LOCAL",
    "single_hop_m1_early": "1Hop-EARLY",
    "single_hop_m1_global": "1Hop-GLOBAL",
}


def assert_policy_contract() -> None:
    expected_baseline = ("local_no_sharing",)
    if repair_plot.BASELINE_RECAM != expected_baseline:
        raise AssertionError("repair plotter baseline is not a singleton tuple")
    if imbalance_plot.BASELINE_RECAM != expected_baseline:
        raise AssertionError("imbalance plotter baseline is not a singleton tuple")
    for module in (repair_plot, imbalance_plot):
        actual = {
            "all": module.BASELINE_RECAM + module.TOPOLOGY_2_2 + module.TOPOLOGY_1_4,
            "topology_2_2": module.BASELINE_RECAM + module.TOPOLOGY_2_2,
            "topology_1_4": module.BASELINE_RECAM + module.TOPOLOGY_1_4,
            "policy_early": module.BASELINE_RECAM + module.POLICY_EARLY,
            "policy_global": module.BASELINE_RECAM + module.POLICY_GLOBAL,
        }
        if actual != VIEWS:
            raise AssertionError(f"{module.__name__} policy membership changed: {actual}")


def assert_dense_legend_contract() -> None:
    if COMPACT_LEGEND_LABELS != COMPACT_LABELS:
        raise AssertionError("compact R3 legend labels changed")
    figure, axis = make_single_panel_figure()
    reserve_dense_legend_space(figure)
    for index, policy in enumerate(POLICIES):
        axis.plot((0, 1), (index, index), label=STYLE[policy][0])
    legend = add_legend(figure, axis, dense=True)
    try:
        if legend.get_frame_on():
            raise AssertionError("dense legend must be frameless")
        if legend._ncols != 2:
            raise AssertionError("dense single-column legend must start at two columns")
        if not legend_within_canvas(figure, legend):
            raise AssertionError("dense legend exceeds the fixed figure canvas")
        if not legend_below_axes(figure, legend, axis):
            raise AssertionError("dense legend overlaps the plotted axes")
    finally:
        plt.close(figure)


def write_csv(path: Path, fields: list[str], rows: list[dict[str, object]]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", newline="", encoding="utf-8") as target:
        writer = csv.DictWriter(target, fieldnames=fields)
        writer.writeheader()
        writer.writerows(rows)


def create_analysis_fixture(root: Path) -> None:
    repair_rows: list[dict[str, object]] = []
    imbalance_rows: list[dict[str, object]] = []
    for rs in (2, 3):
        for policy_index, policy in enumerate(POLICIES):
            successes = 90 + policy_index
            repair_rows.append({
                "dataset_class": DATASET_CLASS, "RS": rs, "policy": policy,
                "F_GROUP": 8, "groups": 100, "successes": successes,
                "repair_rate": successes / 100, "wilson95_low": 0.80,
                "wilson95_high": 0.99,
            })
            for imbalance_bin in range(1, 6):
                imbalance_rows.append({
                    "dataset_class": DATASET_CLASS, "RS": rs, "policy": policy,
                    "imbalance_bin": imbalance_bin, "groups": 20,
                    "successes": min(20, 12 + policy_index + imbalance_bin),
                    "repair_rate": 0.0,
                })
    write_csv(root / "data" / "repair_rate_summary.csv",
              ["dataset_class", "RS", "policy", "F_GROUP", "groups", "successes",
               "repair_rate", "wilson95_low", "wilson95_high"], repair_rows)
    write_csv(root / "data" / "imbalance_summary.csv",
              ["dataset_class", "RS", "policy", "imbalance_bin", "groups", "successes",
               "repair_rate"], imbalance_rows)


def assert_dataset_classification(root: Path) -> None:
    for source in ("repair_rate_summary.csv", "imbalance_summary.csv"):
        with (root / "data" / source).open(newline="", encoding="utf-8") as input_file:
            classes = {row["dataset_class"] for row in csv.DictReader(input_file)}
        if classes != {DATASET_CLASS}:
            raise AssertionError(f"{source} dataset classification changed: {classes}")


def assert_figure_set(root: Path, metric: str, stems: dict[str, str]) -> None:
    for comparison, stem in stems.items():
        directory = root / "figures" / metric / (
            f"{comparison}_{metric}" if comparison != "all" else f"all_{metric}")
        for suffix in ("", "_rs2", "_rs3"):
            for extension, format_directory in ((".pdf", "pdf"), (".svg", "svg"),
                                                (".png", None)):
                output = (directory / format_directory if format_directory else directory) / f"{stem}{suffix}{extension}"
                if not output.is_file() or output.stat().st_size == 0:
                    raise AssertionError(f"missing or empty figure: {output}")
        for suffix, expected_size in (("", (4260, 1920)),
                                      ("_rs2", (2070, 1920)),
                                      ("_rs3", (2070, 1920))):
            png = directory / f"{stem}{suffix}.png"
            with png.open("rb") as image:
                header = image.read(24)
            if header[:8] != b"\x89PNG\r\n\x1a\n":
                raise AssertionError(f"not a PNG: {png}")
            size = struct.unpack(">II", header[16:24])
            if size != expected_size:
                raise AssertionError(
                    f"fixed figure canvas changed for {png}: {size} != {expected_size}")


def main() -> int:
    assert_policy_contract()
    assert_dense_legend_contract()
    with tempfile.TemporaryDirectory(prefix="r3-group-plot-") as temporary:
        analysis_root = Path(temporary) / "analysis"
        create_analysis_fixture(analysis_root)
        assert_dataset_classification(analysis_root)
        for plotter in ("plot_group_repair_rate.py", "plot_fault_imbalance.py"):
            subprocess.run([
                sys.executable, str(ROOT / "scripts" / "plot" / "r3_group" / plotter),
                "--analysis-root", str(analysis_root),
            ], cwd=ROOT, check=True)
        assert_figure_set(analysis_root, "repair_rate", REPAIR_STEMS)
        assert_figure_set(analysis_root, "imbalance", IMBALANCE_STEMS)
    print("R3 group plotting regression passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
