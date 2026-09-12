#!/usr/bin/env bash
set -euo pipefail
repo_root=$(cd "$(dirname "$0")/../.." && pwd)
timeout_seconds=${TIMEOUT_SECONDS:-600}
clock_period_ns=${CLOCK_PERIOD_NS:-20.0}
input_delay_ns=${INPUT_DELAY_NS:-0.0}
output_delay_ns=${OUTPUT_DELAY_NS:-0.0}
result_root="$repo_root/results/phase3d/dc_tsmc018_slow"
run_design() {
    local design=$1 tag=$2 report_dir="$result_root/$2"
    mkdir -p "$report_dir"
    printf '%s\n' 'PHASE=3D' "TOP=$design" "CLOCK_PERIOD_NS=$clock_period_ns" "INPUT_DELAY_NS=$input_delay_ns" "OUTPUT_DELAY_NS=$output_delay_ns" 'TECHNOLOGY=TSMC018_ARM_CBDK' 'PVT=slow' 'LIBRARY=/cad/std_libraries/CBDK_TSMC018_Arm_f1.0/CIC/SynopsysDC/db/slow.db' 'TOOL=Synopsys Design Compiler W-2024.09-SP2' "TIMEOUT_SECONDS=$timeout_seconds" 'COMPILE_MODE=timing_driven_fast' 'STATUS=RUNNING' > "$report_dir/metadata.txt"
    set +e
    REPO_ROOT="$repo_root" REPORT_DIR="$report_dir" DESIGN="$design" CLOCK_PERIOD_NS="$clock_period_ns" INPUT_DELAY_NS="$input_delay_ns" OUTPUT_DELAY_NS="$output_delay_ns" timeout --signal=TERM "${timeout_seconds}s" tcsh -f -c 'source /cad/synopsys/CIC/synthesis.cshrc; dc_shell -no_gui -f "$REPO_ROOT/scripts/synthesis/dc/run_recam_phase3d.tcl" -output_log_file "$REPORT_DIR/dc_shell.log"'
    local status=$?
    set -e
    if [[ $status -eq 0 ]]; then
        "$repo_root/scripts/synthesis/write_gate_count_report.sh" "$report_dir"
        echo 'STATUS=PASS' >> "$report_dir/metadata.txt"
    else
        printf 'STATUS=FAIL\nEXIT_CODE=%s\n' "$status" >> "$report_dir/metadata.txt"
        return "$status"
    fi
}
run_design recam_early_solution_selector "${EARLY_ONLY_REPORT_TAG:-phase3d_early_only_20ns}"
run_design recam_phase3d_integrated_top "${INTEGRATED_REPORT_TAG:-phase3d_early_integrated_20ns}"
echo "Phase 3D Design Compiler synthesis PASS"
