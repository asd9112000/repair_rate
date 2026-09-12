#!/usr/bin/env bash
set -euo pipefail
repo_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)
cd "$repo_root"
rtl_files=(rtl/recam/recam_early_solution_selector.sv rtl/recam/recam_group_compressed_solution_selector.sv tb/recam/recam_phase3e_comparison_top.sv)
verilator --lint-only --Wall -Wno-fatal "${rtl_files[@]}" --top-module recam_phase3e_comparison_top
scripts/simulation/run_verilator_test.sh recam_phase3e_comparison_top tb/recam/recam_phase3e_comparison_top.sv tb/recam/recam_group_compressed_solution_selector_test.cpp rtl/recam/recam_early_solution_selector.sv rtl/recam/recam_group_compressed_solution_selector.sv
