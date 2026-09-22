#!/usr/bin/env bash
set -euo pipefail

repo_root=$(cd "$(dirname "$0")/../.." && pwd)
timeout_seconds=${TIMEOUT_SECONDS:-600}
clock_period_ns=${CLOCK_PERIOD_NS:-20.0}
case_name=recam_dss_grid2x2_directional_rs2_cs2_m1_early_noscratch
result_root=${FINAL_EARLY_RESULT_ROOT:-"$repo_root/results/final_early/dc_tsmc018_slow"}
report_dir="$result_root/${case_name}_20ns"
source_list="$repo_root/scripts/synthesis/dc/${case_name}_sources.tcl"
mkdir -p "$report_dir"
exec > >(tee "$report_dir/runner.log") 2>&1
record_failure() {
    local exit_code=$?
    if [[ $exit_code -ne 0 ]]; then
        sed -i 's/^STATUS=RUNNING$/STATUS=FAILED_OR_BLOCKED/' "$report_dir/metadata.txt"
        echo "TERMINAL_EXIT_CODE=$exit_code" >> "$report_dir/metadata.txt"
    fi
}
trap record_failure EXIT
printf '%s\n' \
    'PHASE=FINAL_EARLY_D' \
    "CASE=$case_name" \
    "TOP=${case_name}_top" \
    "CLOCK_PERIOD_NS=$clock_period_ns" \
    'INPUT_DELAY_NS=0.0' \
    'OUTPUT_DELAY_NS=0.0' \
    'TECHNOLOGY=TSMC018_ARM_CBDK' \
    'PVT=slow' \
    'LIBRARY=/cad/std_libraries/CBDK_TSMC018_Arm_f1.0/CIC/SynopsysDC/db/slow.db' \
    'TOOL=Synopsys Design Compiler W-2024.09-SP2' \
    'REQUESTED_COMPILE=compile -map_effort low' \
    'NAND2X1_REFERENCE_AREA=9.979200' \
    'RESOURCE_POINT=RS2_CS2_M1' \
    'SEMANTIC_POLICY=MODEL_B2_DIRECTIONAL_STREAMING_EARLY' \
    'BOUNDARY=DECISION_RESOURCE_MANAGEMENT_METADATA' \
    "GIT_HEAD=$(git -C "$repo_root" rev-parse HEAD)" \
    "SOURCE_LIST_SHA256=$(sha256sum "$source_list" | awk '{print $1}')" \
    "RTL_SOURCE_SHA256=$(find "$repo_root/rtl/recam_dss_grid2x2_directional_rs2_cs2_m1_early_noscratch" -type f -name '*.sv' -print0 | sort -z | xargs -0 sha256sum | sha256sum | awk '{print $1}')" \
    'STATUS=RUNNING' > "$report_dir/metadata.txt"

REPO_ROOT="$repo_root" REPORT_DIR="$report_dir" DESIGN="${case_name}_top" \
    REQUESTED_LM_LICENSE_FILE="${LM_LICENSE_FILE-}" CLOCK_PERIOD_NS="$clock_period_ns" \
    timeout --signal=TERM "${timeout_seconds}s" \
    tcsh -f -c 'set requested_lm = "$REQUESTED_LM_LICENSE_FILE"; source /cad/synopsys/CIC/synthesis.cshrc; if ("$requested_lm" != "") setenv LM_LICENSE_FILE "$requested_lm"; dc_shell -no_gui -f "$REPO_ROOT/scripts/synthesis/dc/run_recam_dss_grid2x2_directional_rs2_cs2_m1_early_noscratch.tcl" -output_log_file "$REPORT_DIR/dc_shell.log"'
"$repo_root/scripts/synthesis/write_gate_count_report.sh" "$report_dir"
sed -i 's/^STATUS=RUNNING$/STATUS=PASS/' "$report_dir/metadata.txt"
echo "FINAL EARLY Design Compiler synthesis PASS: $report_dir"
