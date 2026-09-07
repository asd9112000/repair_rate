#!/usr/bin/env python3
"""End-to-end regression for DynamicSpareSharing remap integration output."""

from __future__ import annotations

from pathlib import Path
import subprocess
import sys
import tempfile


ROOT = Path(__file__).resolve().parents[1]
SIMPLIFIED = ROOT / "tests/fixtures/dynamic_remap_simplified.faults"
ORGANIZED = ROOT / "tests/fixtures/dynamic_remap_organized.faults"
SHARING_FIXTURE = ROOT / "tests/fixtures/dynamic_sharing_seed_20260820.faults"


def run(command: list[str]) -> subprocess.CompletedProcess[str]:
    completed = subprocess.run(
        command,
        cwd=ROOT,
        check=False,
        capture_output=True,
        text=True,
    )
    if completed.returncode != 0:
        raise AssertionError(
            "command failed:\n"
            + " ".join(command)
            + "\nstdout:\n"
            + completed.stdout
            + "\nstderr:\n"
            + completed.stderr
        )
    return completed


def records(path: Path) -> list[str]:
    return [
        line
        for line in path.read_text(encoding="utf-8").splitlines()
        if line and not line.startswith("#")
    ]


def verify_simplified_input_and_buffer_maps(
    simulator: Path,
    validator: Path,
) -> None:
    with tempfile.TemporaryDirectory(prefix="dynamic-remap-flat-") as temp:
        output = Path(temp)
        completed = run(
            [
                str(simulator),
                "2",
                "2",
                "--simplified-fault-file",
                str(SIMPLIFIED),
                "--topology",
                "none",
                "--write-remap-tables",
                "--output-dir",
                str(output),
            ]
        )
        if "buffmap_entries=4" not in completed.stdout:
            raise AssertionError("dynamic runner did not report four BUFFMAP entries")

        full = output / "RemapTable.txt"
        simplified = output / "RemapTable_simplified.txt"
        if not full.is_file() or not simplified.is_file():
            raise AssertionError("dynamic runner omitted a remap output file")

        full_lines = full.read_text(encoding="utf-8").splitlines()
        full_records = records(full)
        simple_records = records(simplified)
        if sum(line.startswith("OPTION ") for line in full_records) != 1:
            raise AssertionError("expected one canonical remap option")
        if sum(line.startswith("PE ") for line in full_records) != 4:
            raise AssertionError("remap option does not contain four PEs")
        if sum(line.startswith("MAP ") for line in full_records) != 16:
            raise AssertionError("expected sixteen spare-line MAP entries")
        buffer_lines = [
            line for line in full_records if line.startswith("BUFFMAP ")
        ]
        if len(buffer_lines) != 4:
            raise AssertionError("expected one BUFFMAP entry per subarray")
        expected_buffer_lines = {
            "BUFFMAP 2 3 4 5 0 4 4 3",
            "BUFFMAP 2 3 4 5 1 14 14 3",
            "BUFFMAP 2 3 4 5 2 24 24 3",
            "BUFFMAP 2 3 4 5 3 34 34 3",
        }
        if set(buffer_lines) != expected_buffer_lines:
            raise AssertionError("BUFFMAP physical addresses or latency changed")
        if "# DYNAMIC_POLICY no_sharing" not in full_lines:
            raise AssertionError("full remap log omitted its dynamic policy")
        if any(
            line.startswith(("OPTION ", "PE ", "END_PE", "END_OPTION"))
            for line in simple_records
        ):
            raise AssertionError("simplified remap output contains wrapper records")
        if len(simple_records) != 20 or set(simple_records) != {
            line
            for line in full_records
            if line.startswith(("MAP ", "BUFFMAP "))
        }:
            raise AssertionError("simplified remap output differs from full MAP data")

        validation = run([str(validator), str(ORGANIZED), str(full)])
        if "REMAP VALIDATION PASSED" not in validation.stdout:
            raise AssertionError("coverage validator rejected dynamic remap output")


def verify_runtime_sram_manifest(simulator: Path) -> None:
    with tempfile.TemporaryDirectory(prefix="dynamic-remap-runtime-") as temp:
        output = Path(temp)
        run(
            [
                str(simulator),
                "2",
                "2",
                "--simplified-fault-file",
                str(SIMPLIFIED),
                "--topology",
                "none",
                "--write-remap-tables",
                "--runtime-repair-storage",
                "sram-serial",
                "--runtime-data-read-cycles",
                "0",
                "--runtime-data-write-cycles",
                "1",
                "--runtime-mux-cycles",
                "1",
                "--output-dir",
                str(output),
            ]
        )
        manifest = output / "RuntimeRepairTable.csv"
        if not manifest.is_file():
            raise AssertionError("runtime SRAM manifest was not written")
        rows = [
            line
            for line in manifest.read_text(encoding="utf-8").splitlines()
            if line and not line.startswith("#")
        ]
        expected = [
            "0,2,3,4,5,0,4,4,sram_serial,0,1,2,2",
            "0,2,3,4,5,1,14,14,sram_serial,1,2,3,3",
            "0,2,3,4,5,2,24,24,sram_serial,2,3,4,4",
            "0,2,3,4,5,3,34,34,sram_serial,3,4,5,5",
        ]
        if rows[1:] != expected:
            raise AssertionError("runtime SRAM slot or read/write latency changed")
        buffmaps = [
            line
            for line in records(output / "RemapTable.txt")
            if line.startswith("BUFFMAP ")
        ]
        if [line.rsplit(" ", 1)[1] for line in buffmaps] != ["2", "3", "4", "5"]:
            raise AssertionError("BUFFMAP did not expose runtime read-hit latency")


def verify_integrated_generator_remains_available(simulator: Path) -> None:
    with tempfile.TemporaryDirectory(prefix="dynamic-remap-generated-") as temp:
        output = Path(temp)
        run(
            [
                str(simulator),
                "2",
                "2",
                "--fault-count",
                "4",
                "--fault-model",
                "uniform",
                "--runs",
                "1",
                "--seed",
                "77123",
                "--topology",
                "none",
                "--write-remap-tables",
                "--output-dir",
                str(output),
            ]
        )
        full = output / "RemapTable.txt"
        if not full.is_file() or "OPTION 0 0 0" not in full.read_text(
            encoding="utf-8"
        ):
            raise AssertionError("integrated generator did not produce remap output")


def verify_group_selected_sharing_remap(
    simulator: Path,
    validator: Path,
) -> None:
    with tempfile.TemporaryDirectory(prefix="dynamic-remap-edge-") as temp:
        output = Path(temp)
        completed = run(
            [
                str(simulator),
                "2",
                "2",
                "--fault-file",
                str(SHARING_FIXTURE),
                "--topology",
                "edge",
                "--shared-lines",
                "1",
                "--local-first",
                "--buffer",
                "2",
                "--write-remap-tables",
                "--output-dir",
                str(output),
            ]
        )
        if "success_groups=12 failed_groups=20" not in completed.stdout:
            raise AssertionError("edge-sharing remap success accounting changed")
        full = output / "RemapTable.txt"
        lines = full.read_text(encoding="utf-8").splitlines()
        if sum(line.startswith("OPTION ") for line in lines) != 12:
            raise AssertionError("edge-sharing output omitted a successful option")
        failures = [
            line
            for line in lines
            if line.startswith("#FAILED SUBARRAY GROUP ") and "<" not in line
        ]
        if len(failures) != 20:
            raise AssertionError("edge-sharing output omitted a failed group")
        validation = run([str(validator), str(SHARING_FIXTURE), str(full)])
        if "valid / invalid        : 12 / 0" not in validation.stdout:
            raise AssertionError("selected edge-sharing remap is not executable")


def verify_compressed_group_remap(
    simulator: Path,
    validator: Path,
) -> None:
    with tempfile.TemporaryDirectory(prefix="compressed-remap-edge-") as temp:
        output = Path(temp)
        run(
            [
                str(simulator),
                "2",
                "2",
                "--fault-file",
                str(SHARING_FIXTURE),
                "--topology",
                "edge",
                "--shared-lines",
                "1",
                "--max-borrows",
                "3",
                "--buffer",
                "2",
                "--solution-take",
                "group",
                "--write-remap-tables",
                "--output-dir",
                str(output),
            ]
        )
        full = output / "RemapTable.txt"
        validation = run([str(validator), str(SHARING_FIXTURE), str(full)])
        if "REMAP VALIDATION PASSED" not in validation.stdout or \
                " / 0" not in validation.stdout:
            raise AssertionError(
                "GROUP_COMPRESSED selected a remap that does not cover faults"
            )


def main() -> None:
    if len(sys.argv) != 3:
        raise SystemExit(
            "usage: dynamic_remap_output_test.py DYNAMIC_SIMULATOR VALIDATOR"
        )
    simulator = Path(sys.argv[1]).resolve()
    validator = Path(sys.argv[2]).resolve()
    verify_simplified_input_and_buffer_maps(simulator, validator)
    verify_runtime_sram_manifest(simulator)
    verify_integrated_generator_remains_available(simulator)
    verify_group_selected_sharing_remap(simulator, validator)
    verify_compressed_group_remap(simulator, validator)
    print("Dynamic remap output integration test passed")


if __name__ == "__main__":
    main()
