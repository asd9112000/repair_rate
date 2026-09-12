#!/usr/bin/env bash
set -euo pipefail

repo_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)
cd "$repo_root"

rtl_files=(
    rtl/recam/recam_shared_config_analyzer.sv
    rtl/recam/recam_role_aware_config_scheduler.sv
)

verilator --lint-only --Wall -Wno-fatal "${rtl_files[@]}" \
    --top-module recam_role_aware_config_scheduler

scripts/simulation/run_verilator_test.sh \
    recam_role_aware_config_scheduler \
    tb/recam/recam_role_aware_config_scheduler_test.cpp \
    rtl/recam/recam_role_aware_config_scheduler.sv \
    rtl/recam/recam_shared_config_analyzer.sv
