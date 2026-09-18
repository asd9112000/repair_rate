#!/usr/bin/env bash
set -euo pipefail

repo_root=$(cd "$(dirname "$0")/../.." && pwd)
# The isolated K=7/35-candidate analyzer has a materially larger DesignWare
# mapping workload than the accepted K=5 target.  The first two clean target
# attempts progressed normally but were terminated by this wrapper at 600 s and
# 3600 s respectively during DC mapping.  Retain an explicit override while
# providing a four-hour characterization budget by default.
timeout_seconds=${TIMEOUT_SECONDS:-14400}
clock_period_ns=${CLOCK_PERIOD_NS:-20.0}
result_root=${H5_RESULT_ROOT:-"$repo_root/results/dss_v2_rs3cs3m1/h5_hardware"}

run_point() {
    local policy=$1
    local design=$2
    local manifest=$3
    local boundary=$4
    local report_dir="$result_root/$policy"
    mkdir -p "$report_dir"
    {
        echo "PHASE=H5"
        echo "HARDWARE_ID=3_3_1_${policy}"
        echo "TOP=$design"
        echo "CLOCK_PERIOD_NS=$clock_period_ns"
        echo "TIMEOUT_SECONDS=$timeout_seconds"
        echo "INPUT_DELAY_NS=0.0"
        echo "OUTPUT_DELAY_NS=0.0"
        echo "TECHNOLOGY=TSMC018_ARM_CBDK"
        echo "PVT=slow"
        echo "LIBRARY=/cad/std_libraries/CBDK_TSMC018_Arm_f1.0/CIC/SynopsysDC/db/slow.db"
        echo "TOOL=Synopsys Design Compiler W-2024.09-SP2"
        echo "COMPILE_MODE=timing_driven_fast"
        echo "BOUNDARY=$boundary"
        echo "RTL_GIT_REVISION=$(git -C "$repo_root" rev-parse --short HEAD)"
        echo "RTL_SOURCE_SHA256=$(find "$repo_root/rtl/dss_v2/rs3cs3m1" -type f -name '*.sv' -print0 | sort -z | xargs -0 sha256sum | sha256sum | awk '{print $1}')"
    } > "$report_dir/metadata.txt"
    local requested_lm=${LM_LICENSE_FILE:-}
    REQUESTED_LM_LICENSE_FILE="$requested_lm" REPO_ROOT="$repo_root" REPORT_DIR="$report_dir" DESIGN="$design" \
        SOURCE_MANIFEST="$manifest" H5_POLICY="$policy" CLOCK_PERIOD_NS="$clock_period_ns" \
        timeout --signal=TERM "${timeout_seconds}s" tcsh -f -c '
set requested_lm = "$REQUESTED_LM_LICENSE_FILE"
source /cad/synopsys/CIC/synthesis.cshrc
if ( "$requested_lm" != "" ) then
    setenv LM_LICENSE_FILE "$requested_lm"
endif
dc_shell -no_gui -f "$REPO_ROOT/scripts/synthesis/dc/run_recam_rs3cs3m1_h5.tcl" -output_log_file "$REPORT_DIR/dc_shell.log"
'
    "$repo_root/scripts/synthesis/write_gate_count_report.sh" "$report_dir"
    echo "STATUS=PASS" >> "$report_dir/metadata.txt"
}

run_point EARLY recam_dss_v2_rs3cs3m1_early_top scripts/synthesis/dc/recam_rs3cs3m1_h5_early_sources.tcl V2_RS3CS3M1_EARLY_DECISION_RESOURCE_MANAGEMENT
run_point GROUP recam_dss_v2_rs3cs3m1_group_top scripts/synthesis/dc/recam_rs3cs3m1_h5_group_sources.tcl V2_RS3CS3M1_GROUP_NOSCRATCH_DECISION_RESOURCE_MANAGEMENT
echo "H5 RS3CS3M1 Design Compiler synthesis PASS"
