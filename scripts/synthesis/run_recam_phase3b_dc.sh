#!/usr/bin/env bash
set -euo pipefail

repo_root=$(cd "$(dirname "$0")/../.." && pwd)
timeout_seconds=${TIMEOUT_SECONDS:-600}
clock_period_ns=${CLOCK_PERIOD_NS:-20.0}
input_delay_ns=${INPUT_DELAY_NS:-0.0}
output_delay_ns=${OUTPUT_DELAY_NS:-0.0}
report_tag=${REPORT_TAG:-phase3b_shared}
report_dir="$repo_root/results/phase3b/dc_tsmc018_slow/$report_tag"
timing_driven=${TIMING_DRIVEN:-1}
compile_mode=timing_driven_fast
if [[ $timing_driven == 0 ]]; then compile_mode=structural_report; fi

mkdir -p "$report_dir"
printf '%s\n' \
    'PHASE=3B' \
    'TOP=recam_shared_config_analyzer' \
    "CLOCK_PERIOD_NS=$clock_period_ns" \
    "INPUT_DELAY_NS=$input_delay_ns" \
    "OUTPUT_DELAY_NS=$output_delay_ns" \
    'TECHNOLOGY=TSMC018_ARM_CBDK' \
    'PVT=slow' \
    'LIBRARY=/cad/std_libraries/CBDK_TSMC018_Arm_f1.0/CIC/SynopsysDC/db/slow.db' \
    'TOOL=Synopsys Design Compiler W-2024.09-SP2' \
    "TIMEOUT_SECONDS=$timeout_seconds" \
    "COMPILE_MODE=$compile_mode" \
    'STATUS=RUNNING' > "$report_dir/metadata.txt"

set +e
REPO_ROOT="$repo_root" REPORT_DIR="$report_dir" \
    TIMING_DRIVEN="$timing_driven" \
    CLOCK_PERIOD_NS="$clock_period_ns" INPUT_DELAY_NS="$input_delay_ns" \
    OUTPUT_DELAY_NS="$output_delay_ns" \
    timeout --signal=TERM "${timeout_seconds}s" tcsh -f -c \
    'source /cad/synopsys/CIC/synthesis.cshrc; dc_shell -no_gui -f "$REPO_ROOT/scripts/synthesis/dc/run_recam_phase3b.tcl" -output_log_file "$REPORT_DIR/dc_shell.log"'
status=$?
set -e
if [[ $status -eq 0 ]]; then
    "$repo_root/scripts/synthesis/write_gate_count_report.sh" "$report_dir"
    sequential_cells=$(awk -F= '$1 == "SEQUENTIAL_CELL_COUNT" {print $2}' "$report_dir/gate_count.rpt")
    if [[ $sequential_cells != 0 ]]; then
        printf 'STATUS=FAIL\nFAILURE_REASON=UNEXPECTED_SEQUENTIAL_CELLS\n' >> "$report_dir/metadata.txt"
        exit 3
    fi
    echo 'STATUS=PASS' >> "$report_dir/metadata.txt"
elif [[ $status -eq 124 ]]; then
    echo 'STATUS=TIMEOUT' >> "$report_dir/metadata.txt"
    exit 124
else
    printf 'STATUS=FAIL\nEXIT_CODE=%s\n' "$status" >> "$report_dir/metadata.txt"
    exit "$status"
fi
echo "Phase 3B Design Compiler synthesis PASS: $report_dir"
