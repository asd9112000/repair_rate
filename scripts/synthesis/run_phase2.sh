#!/usr/bin/env bash
set -euo pipefail

output_root=results/phase2/synthesis
configuration=dss_cam_2x2_directional
report_dir="$output_root/$configuration"
sources=rtl/primitives/cam/generic_cam.sv:rtl/dss_2x2/common/fault_cam.sv:rtl/dss_2x2/common/fault_collector.sv:rtl/dss_2x2/common/candidate_analyzer.sv:rtl/dss_2x2/common/resource_tracker.sv:rtl/dss_2x2/common/sharing_controller.sv:rtl/dss_2x2/common/solution_selector.sv:rtl/dss_2x2/common/solution_store.sv:rtl/dss_2x2/cam/dss_cam_top.sv
parameters=ROW_ADDR_WIDTH=10,COL_ADDR_WIDTH=10,FAULT_ENTRY_NUM=12,RS=2,CS=2,SHARED_ROWS=1,SHARED_COLS=1,MAX_BORROWS=3,TOPOLOGY=1

mkdir -p "$report_dir"
printf 'PHASE=2\nCONFIGURATION=%s\nTOP=dss_cam_top\nPARAMS=%s\nCLK_PERIOD_NS=10.0\nTECHNOLOGY=YOSYS_ELABORATION\nLIBRARY=NONE\nSTATUS=ELABORATION_ONLY\n' \
    "$configuration" "$parameters" > "$report_dir/metadata.txt"

TOP=dss_cam_top SOURCES="$sources" PARAMS="$parameters" FLOW=ELABORATION \
    REPORT_DIR="$report_dir" CLK_PERIOD=10.0 \
    yosys -q -l "$report_dir/yosys.log" -c scripts/synthesis/synth.tcl

python3 scripts/parse_reports.py \
    --input-root "$output_root" \
    --output results/phase2/synthesis_results.csv \
    --module-breakdown-output results/phase2/module_area_breakdown.csv

echo "Phase 2 Yosys elaboration sanity PASS"
