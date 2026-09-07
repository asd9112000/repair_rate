#!/usr/bin/env bash
set -euo pipefail

output_root=results/phase1/synthesis
mkdir -p "$output_root"

run_synthesis() {
    local configuration=$1
    local top=$2
    local sources=$3
    local parameters=$4
    local report_dir="$output_root/$configuration"
    mkdir -p "$report_dir"
    printf 'PHASE=1\nCONFIGURATION=%s\nTOP=%s\nPARAMS=%s\nCLK_PERIOD_NS=10.0\nTECHNOLOGY=GENERIC_YOSYS\nLIBRARY=NONE\nSTATUS=SANITY_ONLY\n' \
        "$configuration" "$top" "$parameters" > "$report_dir/metadata.txt"
    TOP="$top" SOURCES="$sources" PARAMS="$parameters" \
        REPORT_DIR="$report_dir" CLK_PERIOD=10.0 \
        yosys -q -l "$report_dir/yosys.log" -c scripts/synthesis/synth.tcl
}

run_synthesis generic_cam_e4 generic_cam \
    rtl/primitives/cam/generic_cam.sv \
    ENTRY_NUM=4,KEY_WIDTH=8,DATA_WIDTH=8
run_synthesis generic_cam_e8 generic_cam \
    rtl/primitives/cam/generic_cam.sv \
    ENTRY_NUM=8,KEY_WIDTH=8,DATA_WIDTH=8
run_synthesis sram_like_e8 sram_like \
    rtl/primitives/sram/sram_like.sv \
    ENTRY_NUM=8,DATA_WIDTH=16
run_synthesis sync_fifo_d4 sync_fifo \
    rtl/primitives/fifo/sync_fifo.sv \
    DEPTH=4,DATA_WIDTH=16
run_synthesis pway_search_e8_p1 pway_search \
    rtl/primitives/search/pway_search.sv \
    ENTRY_NUM=8,ENTRY_WIDTH=8,P=1
run_synthesis pway_search_e8_p4 pway_search \
    rtl/primitives/search/pway_search.sv \
    ENTRY_NUM=8,ENTRY_WIDTH=8,P=4

python3 scripts/parse_reports.py \
    --input-root "$output_root" \
    --output results/phase1/synthesis_results.csv

echo "Phase 1 Yosys synthesis sanity PASS"
