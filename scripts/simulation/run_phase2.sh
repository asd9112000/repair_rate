#!/usr/bin/env bash
set -euo pipefail

runner=scripts/simulation/run_verilator_test.sh

"$runner" candidate_analyzer \
    rtl/dss_2x2/common/candidate_analyzer.sv \
    tb/dss_2x2/candidate_analyzer_test.cpp

"$runner" dss_cam_top \
    rtl/dss_2x2/cam/dss_cam_top.sv \
    tb/dss_2x2/dss_cam_top_test.cpp \
    -f scripts/simulation/dss_phase2.f

echo "Phase 2 DSS-CAM Verilator regression PASS"
