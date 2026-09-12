#!/usr/bin/env bash
set -euo pipefail

repo_root=$(cd "$(dirname "$0")/../.." && pwd)
timeout_seconds=${TIMEOUT_SECONDS:-600}
clock_period_ns=${CLOCK_PERIOD_NS:-20.0}
result_root=${PHASE4E_RESULT_ROOT:-"$repo_root/results/phase4e/dc_tsmc018_slow"}
report_dir="$result_root/v2_specialized_early_20ns"
mkdir -p "$report_dir"
printf '%s\n' 'PHASE=4E' 'TOP=recam_dss_v2_early_top' "CLOCK_PERIOD_NS=$clock_period_ns" \
    'INPUT_DELAY_NS=0.0' 'OUTPUT_DELAY_NS=0.0' 'TECHNOLOGY=TSMC018_ARM_CBDK' 'PVT=slow' \
    'LIBRARY=/cad/std_libraries/CBDK_TSMC018_Arm_f1.0/CIC/SynopsysDC/db/slow.db' \
    'TOOL=Synopsys Design Compiler W-2024.09-SP2' 'COMPILE_MODE=timing_driven_fast' \
    'BOUNDARY=V2_EARLY_DECISION_RESOURCE_MANAGEMENT' > "$report_dir/metadata.txt"
# Preserve a caller-supplied license search list.  The CAD setup script sets a
# default single server, while an interactive approved environment may supply
# an ordered multi-server list required for the DC feature.
REPO_ROOT="$repo_root" REPORT_DIR="$report_dir" DESIGN=recam_dss_v2_early_top \
    CLOCK_PERIOD_NS="$clock_period_ns" timeout --signal=TERM "${timeout_seconds}s" \
    tcsh -f -c 'set requested_lm = "$LM_LICENSE_FILE"; source /cad/synopsys/CIC/synthesis.cshrc; setenv LM_LICENSE_FILE "$requested_lm"; dc_shell -no_gui -f "$REPO_ROOT/scripts/synthesis/dc/run_recam_phase4e.tcl" -output_log_file "$REPORT_DIR/dc_shell.log"'
"$repo_root/scripts/synthesis/write_gate_count_report.sh" "$report_dir"
echo 'STATUS=PASS' >> "$report_dir/metadata.txt"
echo 'Phase 4E V2 EARLY Design Compiler synthesis PASS'
