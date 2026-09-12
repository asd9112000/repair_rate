#!/usr/bin/env bash
set -euo pipefail
repo_root=$(cd "$(dirname "$0")/../.." && pwd)
timeout_seconds=${TIMEOUT_SECONDS:-600}
clock_period_ns=${CLOCK_PERIOD_NS:-20.0}
result_root=${PHASE3J_RESULT_ROOT:-"$repo_root/results/phase3j/dc_tsmc018_slow"}
run_design() {
    local design=$1 report_dir="$result_root/$2"
    mkdir -p "$report_dir"
    printf '%s\n' 'PHASE=3J' "TOP=$design" "CLOCK_PERIOD_NS=$clock_period_ns" 'INPUT_DELAY_NS=0.0' 'OUTPUT_DELAY_NS=0.0' 'TECHNOLOGY=TSMC018_ARM_CBDK' 'PVT=slow' 'LIBRARY=/cad/std_libraries/CBDK_TSMC018_Arm_f1.0/CIC/SynopsysDC/db/slow.db' 'TOOL=Synopsys Design Compiler W-2024.09-SP2' 'COMPILE_MODE=timing_driven_fast' > "$report_dir/metadata.txt"
    REPO_ROOT="$repo_root" REPORT_DIR="$report_dir" DESIGN="$design" CLOCK_PERIOD_NS="$clock_period_ns" timeout --signal=TERM "${timeout_seconds}s" tcsh -f -c 'source /cad/synopsys/CIC/synthesis.cshrc; dc_shell -no_gui -f "$REPO_ROOT/scripts/synthesis/dc/run_recam_phase3j.tcl" -output_log_file "$REPORT_DIR/dc_shell.log"'
    "$repo_root/scripts/synthesis/write_gate_count_report.sh" "$report_dir"
    echo 'STATUS=PASS' >> "$report_dir/metadata.txt"
}
run_design recam_phase3j_full_early_top phase3j_full_early_20ns
run_design recam_phase3j_full_group_top phase3j_full_group_20ns
echo 'Phase 3J Design Compiler synthesis PASS'
