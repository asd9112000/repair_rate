#!/usr/bin/env bash
set -euo pipefail

verilator -Wall --lint-only --top-module generic_cam \
    rtl/primitives/cam/generic_cam.sv
verilator -Wall --lint-only --top-module sram_like \
    rtl/primitives/sram/sram_like.sv
verilator -Wall --lint-only --top-module sync_fifo \
    rtl/primitives/fifo/sync_fifo.sv
verilator -Wall --lint-only --top-module pway_search \
    rtl/primitives/search/pway_search.sv

if [[ -f rtl/dss_2x2/cam/dss_cam_top.sv ]]; then
    verilator -Wall -Wno-fatal --lint-only \
        rtl/primitives/cam/generic_cam.sv \
        rtl/dss_2x2/common/fault_cam.sv \
        rtl/dss_2x2/common/fault_collector.sv \
        rtl/dss_2x2/common/candidate_analyzer.sv \
        rtl/dss_2x2/common/resource_tracker.sv \
        rtl/dss_2x2/common/sharing_controller.sv \
        rtl/dss_2x2/common/solution_selector.sv \
        rtl/dss_2x2/common/solution_store.sv \
        rtl/dss_2x2/cam/dss_cam_top.sv
fi

echo "RTL lint PASS"
