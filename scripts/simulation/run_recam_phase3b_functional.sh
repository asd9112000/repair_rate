#!/usr/bin/env bash
set -euo pipefail

repo_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)
cd "$repo_root"

rtl_files=(
    rtl/recam/recam_shared_config_analyzer.sv
    rtl/recam/recam_2r2c_analyzer.sv
    rtl/recam/recam_2r2c_matrix_builder.sv
    rtl/recam/recam_2r2c_pattern_analyzer.sv
    rtl/recam/recam_2r2c_pattern_encoder.sv
    tb/recam/recam_phase3b_verification_top.sv
)

verilator --lint-only --Wall -Wno-fatal "${rtl_files[@]}" \
    --top-module recam_phase3b_verification_top

scripts/simulation/run_verilator_test.sh \
    recam_phase3b_verification_top \
    tb/recam/recam_phase3b_verification_top.sv \
    tb/recam/recam_shared_config_analyzer_test.cpp \
    rtl/recam/recam_shared_config_analyzer.sv \
    rtl/recam/recam_2r2c_analyzer.sv \
    rtl/recam/recam_2r2c_matrix_builder.sv \
    rtl/recam/recam_2r2c_pattern_analyzer.sv \
    rtl/recam/recam_2r2c_pattern_encoder.sv
