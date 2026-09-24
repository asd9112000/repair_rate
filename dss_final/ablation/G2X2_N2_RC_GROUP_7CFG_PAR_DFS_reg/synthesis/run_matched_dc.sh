#!/usr/bin/env bash
set -euo pipefail

case_dir=$(cd "$(dirname "$0")/.." && pwd)
repo_root=$(cd "$case_dir/../.." && pwd)
report_dir="$case_dir/synthesis/reports"
top=recam_n2_preopt_group_top

mkdir -p "$report_dir"
exec > >(tee "$report_dir/runner.log") 2>&1
trap 'status=$?; if [[ $status -ne 0 ]]; then sed -i "s/^STATUS=RUNNING$/STATUS=FAILED_OR_BLOCKED/" "$report_dir/metadata.txt" 2>/dev/null || true; printf "TERMINAL_EXIT_CODE=%s\n" "$status" >> "$report_dir/metadata.txt"; fi' EXIT

(cd "$repo_root" && sha256sum -c "${case_dir#$repo_root/}/source_sha256.txt")
cp "$case_dir/source_manifest.txt" "$report_dir/functional_source_manifest.txt"
cp "$case_dir/source_sha256.txt" "$report_dir/functional_source_sha256.txt"
printf '%s\n' \
    'CASE_ID=G2X2_N2_RC_GROUP_7CFG_PAR_DFS_reg' \
    "TOP=$top" \
    'TOOL=Synopsys Design Compiler W-2024.09-SP2' \
    'TECHNOLOGY=TSMC018_ARM_CBDK' \
    'PVT=slow' \
    'CLOCK_PERIOD_NS=20.0' \
    'INPUT_DELAY_NS=0.0' \
    'OUTPUT_DELAY_NS=0.0' \
    'COMPILE_MODE=compile -map_effort low' \
    'NAND2X1_REFERENCE_AREA=9.979200' \
    "FUNCTIONAL_COMMIT=$(git -C "$repo_root" rev-parse HEAD)" \
    'STATUS=RUNNING' > "$report_dir/metadata.txt"

REPO_ROOT="$repo_root" RTL_CASE_DIR="$case_dir" REPORT_DIR="$report_dir" DESIGN="$top" \
    tcsh -f -c 'source /cad/synopsys/CIC/synthesis.cshrc; dc_shell -no_gui -f "$RTL_CASE_DIR/synthesis/run_matched_dc.tcl" -output_log_file "$REPORT_DIR/dc_shell.log"'
"$repo_root/scripts/synthesis/write_gate_count_report.sh" "$report_dir"
(cd "$repo_root" && sha256sum -c "${case_dir#$repo_root/}/source_sha256.txt")
sed -i 's/^STATUS=RUNNING$/STATUS=PASS/' "$report_dir/metadata.txt"
printf 'N2 pre-optimization GROUP matched DC synthesis PASS\n'
