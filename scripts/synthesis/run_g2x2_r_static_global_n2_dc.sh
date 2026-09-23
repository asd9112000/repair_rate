#!/usr/bin/env bash
set -euo pipefail
repo_root=$(cd "$(dirname "$0")/../.." && pwd)
case_name=recam_dss_g2x2_r_static_global_n2
report_dir="${G2X2_R_RESULT_ROOT:-$repo_root/results/g2x2_r_group/dc_tsmc018_slow}/${case_name}_20ns"
mkdir -p "$report_dir"
printf '%s\n' "TOP=recam_dss_g2x2_r_static_global_top" "CLOCK_PERIOD_NS=20.0" "INPUT_DELAY_NS=0.0" "OUTPUT_DELAY_NS=0.0" "GIT_HEAD=$(git -C "$repo_root" rev-parse HEAD)" "STATUS=RUNNING" > "$report_dir/metadata.txt"
REPO_ROOT="$repo_root" REPORT_DIR="$report_dir" DESIGN=recam_dss_g2x2_r_static_global_top CLOCK_PERIOD_NS=20.0 tcsh -f -c 'source /cad/synopsys/CIC/synthesis.cshrc; dc_shell -no_gui -f "$REPO_ROOT/scripts/synthesis/dc/run_recam_dss_g2x2_r_static_global_n2.tcl" -output_log_file "$REPORT_DIR/dc_shell.log"'
sed -i 's/^STATUS=RUNNING$/STATUS=PASS/' "$report_dir/metadata.txt"
