#!/usr/bin/env bash
set -euo pipefail
repo_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd); cd "$repo_root"
verilator --lint-only --Wall -Wno-fatal rtl/recam/recam_early_solution_selector.sv --top-module recam_early_solution_selector
scripts/simulation/run_verilator_test.sh recam_early_solution_selector tb/recam/recam_early_solution_selector_test.cpp rtl/recam/recam_early_solution_selector.sv
scripts/simulation/run_verilator_test.sh recam_phase3d_integration_top tb/recam/recam_phase3d_integration_top.sv tb/recam/recam_phase3d_integration_test.cpp rtl/recam/recam_early_solution_selector.sv rtl/recam/recam_role_aware_config_scheduler.sv rtl/recam/recam_shared_config_analyzer.sv
