#!/usr/bin/env bash
set -euo pipefail

repo_root=$(cd "$(dirname "$0")/../.." && pwd)
timeout_seconds=${TIMEOUT_SECONDS:-600}
clock_period_ns=${CLOCK_PERIOD_NS:-20.0}
input_delay_ns=${INPUT_DELAY_NS:-0.0}
output_delay_ns=${OUTPUT_DELAY_NS:-0.0}
result_root="$repo_root/results/phase3a/dc_tsmc018_slow"
timing_driven=${TIMING_DRIVEN:-0}

run_design() {
    local design=$1
    local report_dir
    if [[ -n ${REPORT_TAG:-} ]]; then
        report_dir="$result_root/$REPORT_TAG"
    else
        report_dir="$result_root/${REPORT_TAG_PREFIX:-}$design"
    fi
    mkdir -p "$report_dir"
    printf '%s\n' \
        'PHASE=3A' \
        "TOP=$design" \
        "CLOCK_PERIOD_NS=$clock_period_ns" \
        "INPUT_DELAY_NS=$input_delay_ns" \
        "OUTPUT_DELAY_NS=$output_delay_ns" \
        'TECHNOLOGY=TSMC018_ARM_CBDK' \
        'PVT=slow' \
        'LIBRARY=/cad/std_libraries/CBDK_TSMC018_Arm_f1.0/CIC/SynopsysDC/db/slow.db' \
        'TOOL=Synopsys Design Compiler W-2024.09-SP2' \
        "TIMEOUT_SECONDS=$timeout_seconds" \
        "COMPILE_MODE=$([[ $timing_driven == 1 ]] && echo timing_driven || echo structural_report)" \
        'STATUS=RUNNING' > "$report_dir/metadata.txt"

    set +e
    REPO_ROOT="$repo_root" REPORT_DIR="$report_dir" DESIGN="$design" \
        TIMING_DRIVEN="$timing_driven" \
        TIMING_DRIVEN_FAST="${TIMING_DRIVEN_FAST:-0}" \
        CLOCK_PERIOD_NS="$clock_period_ns" INPUT_DELAY_NS="$input_delay_ns" \
        OUTPUT_DELAY_NS="$output_delay_ns" \
        timeout --signal=TERM "${timeout_seconds}s" tcsh -f -c \
        'source /cad/synopsys/CIC/synthesis.cshrc; dc_shell -no_gui -f "$REPO_ROOT/scripts/synthesis/dc/run_recam_phase3a.tcl" -output_log_file "$REPORT_DIR/dc_shell.log"'
    local status=$?
    set -e
    if [[ $status -eq 0 ]]; then
        "$repo_root/scripts/synthesis/write_gate_count_report.sh" "$report_dir"
        local sequential_cells
        sequential_cells=$(awk -F= '$1 == "SEQUENTIAL_CELL_COUNT" {print $2}' "$report_dir/gate_count.rpt")
        if [[ $sequential_cells != 0 ]]; then
            printf 'STATUS=FAIL\nFAILURE_REASON=UNEXPECTED_SEQUENTIAL_CELLS\n' >> "$report_dir/metadata.txt"
            return 3
        fi
        echo 'STATUS=PASS' >> "$report_dir/metadata.txt"
    elif [[ $status -eq 124 ]]; then
        echo 'STATUS=TIMEOUT' >> "$report_dir/metadata.txt"
        return 124
    else
        printf 'STATUS=FAIL\nEXIT_CODE=%s\n' "$status" >> "$report_dir/metadata.txt"
        return "$status"
    fi
}

if [[ -n ${DESIGN_LIST:-} ]]; then
    for design in $DESIGN_LIST; do run_design "$design"; done
else
    run_design recam_2r2c_matrix_builder
    run_design recam_2r2c_pattern_analyzer
    run_design recam_2r2c_pattern_encoder
    run_design recam_2r2c_analyzer
fi
echo "Phase 3A Design Compiler synthesis PASS"
