#!/usr/bin/env bash
set -euo pipefail

repo_root=$(cd "$(dirname "$0")/../.." && pwd)
cd "$repo_root"

scripts/simulation/run_verilator_test.sh recam_2r2c_analyzer \
    rtl/recam/recam_2r2c_analyzer.sv \
    tb/recam/recam_2r2c_analyzer_test.cpp \
    rtl/recam/recam_2r2c_matrix_builder.sv \
    rtl/recam/recam_2r2c_pattern_analyzer.sv \
    rtl/recam/recam_2r2c_pattern_encoder.sv

echo "Phase 3A RECAM functional verification PASS"
