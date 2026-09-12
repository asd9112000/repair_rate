#!/usr/bin/env bash
set -euo pipefail

repo_root=$(cd "$(dirname "$0")/../.." && pwd)
timeout_seconds=${TIMEOUT_SECONDS:-600}
compile_mode=${COMPILE_MODE:-ultra}
design=${DESIGN:-dss_analyzer_top}
elab_params=${ELAB_PARAMS:-}
report_tag=${REPORT_TAG:-area_conservative14_${design}_${compile_mode}}
clock_period_ns=${CLOCK_PERIOD_NS:-20.0}
input_delay_ns=${INPUT_DELAY_NS:-0.0}
output_delay_ns=${OUTPUT_DELAY_NS:-0.0}
report_dir="$repo_root/results/phase2/dc_tsmc018_slow/$report_tag"

mkdir -p "$report_dir"
printf '%s\n' \
    'PHASE=2' \
    "CONFIGURATION=$report_tag" \
    "TOP=$design" \
    "ELAB_PARAMS=${elab_params:-defaults}" \
    "CLOCK_PERIOD_NS=$clock_period_ns" \
    "INPUT_DELAY_NS=$input_delay_ns" \
    "OUTPUT_DELAY_NS=$output_delay_ns" \
    'OUTPUT_LOAD=0.05' \
    'TECHNOLOGY=TSMC018_ARM_CBDK' \
    'PVT=slow' \
    'LIBRARY=/cad/std_libraries/CBDK_TSMC018_Arm_f1.0/CIC/SynopsysDC/db/slow.db' \
    'TOOL=Synopsys Design Compiler W-2024.09-SP2' \
    "TIMEOUT_SECONDS=$timeout_seconds" \
    "COMPILE_MODE=$compile_mode" \
    'STATUS=RUNNING' > "$report_dir/metadata.txt"

set +e
REPO_ROOT="$repo_root" REPORT_DIR="$report_dir" DESIGN="$design" ELAB_PARAMS="$elab_params" \
    PARALLEL_ORIENTATION=0 COMPILE_MODE="$compile_mode" \
    CLOCK_PERIOD_NS="$clock_period_ns" INPUT_DELAY_NS="$input_delay_ns" OUTPUT_DELAY_NS="$output_delay_ns" \
    timeout --signal=TERM "${timeout_seconds}s" tcsh -f -c \
    'source /cad/synopsys/CIC/synthesis.cshrc; dc_shell -no_gui -f "$REPO_ROOT/scripts/synthesis/dc/run_dss_analyzer.tcl" -output_log_file "$REPORT_DIR/dc_shell.log"'
status=$?
set -e

if [[ $status -eq 0 ]]; then
    printf '%s\n' 'STATUS=PASS' >> "$report_dir/metadata.txt"
    echo "Phase 2 Design Compiler synthesis PASS"
elif [[ $status -eq 124 ]]; then
    printf '%s\n' 'STATUS=TIMEOUT' >> "$report_dir/metadata.txt"
    echo "Phase 2 Design Compiler synthesis TIMEOUT after ${timeout_seconds}s" >&2
    exit 124
else
    printf 'STATUS=FAIL\nEXIT_CODE=%s\n' "$status" >> "$report_dir/metadata.txt"
    echo "Phase 2 Design Compiler synthesis FAIL (exit $status)" >&2
    exit "$status"
fi
