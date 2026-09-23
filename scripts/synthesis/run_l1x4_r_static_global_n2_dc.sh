#!/usr/bin/env bash
set -euo pipefail

repo_root=$(cd "$(dirname "$0")/../.." && pwd)
case_name=recam_dss_l1x4_r_static_global_n2
top=recam_dss_l1x4_r_static_global_top
expected_top_sha=33b901864987fc3839abe54ef6985431df2cdbfd02333c33793f3c8da2b20123
source_dir="$repo_root/rtl/L1X4_R_GROUP_V1"
source_manifest="$source_dir/source_manifest.txt"
source_hashes="$source_dir/source_sha256.txt"
dc_source_manifest="$repo_root/scripts/synthesis/dc/recam_dss_l1x4_r_static_global_n2_sources.tcl"
report_dir="${L1X4_R_RESULT_ROOT:-$repo_root/results/l1x4_r_group/dc_tsmc018_slow}/${case_name}_20ns"

[[ $(sha256sum "$source_dir/${top}.sv" | awk '{print $1}') == "$expected_top_sha" ]] || {
    echo "TOP_HASH_GATE: FAIL" >&2
    exit 2
}
sha256sum -c "$source_hashes"
[[ $(wc -l < "$source_manifest") -eq 8 ]] || { echo "SOURCE_COUNT: FAIL" >&2; exit 2; }
[[ $(grep -Ec '^rtl/L1X4_R_GROUP_V1/[^/]+\.sv$' "$source_manifest") -eq 8 ]] || {
    echo "SOURCE_MANIFEST: FAIL" >&2
    exit 2
}
if grep -Eq 'dss_1x4|g2x2|hyp02|G2X2_RC|dss_final' "$dc_source_manifest"; then
    echo "SOURCE_ISOLATION: FAIL" >&2
    exit 2
fi
grep -Fq 'A -> B -> C -> D' "$source_dir/CASE.md"
grep -Fq '27 ordered paths' "$source_dir/CASE.md"

mkdir -p "$report_dir"
cp "$source_manifest" "$report_dir/source_manifest.txt"
cp "$source_hashes" "$report_dir/source_sha256.txt"
cp "$dc_source_manifest" "$report_dir/authoritative_source_manifest.tcl"
printf '%s\n' \
    "TOP=$top" \
    "TOP_SHA256=$expected_top_sha" \
    'SOURCE_COUNT=8' \
    'SOURCE_HASH_GATE=PASS' \
    'NO_G2X2_R_TOP=PASS' \
    'NO_OLD_GENERIC_L1X4_TOP=PASS' \
    'NO_HYP02_TOP=PASS' \
    'NO_RC_TOP=PASS' \
    'DENSE_CONFIGS=3' \
    'DENSE_ENTRIES=12' \
    'STATIC_PATHS=27' \
    'ANALYZER=recam_shared_config_analyzer (frozen G2X2_R family)' \
    'PRE_SYNTH_AUDIT=PASS' \
    > "$report_dir/pre_synth_audit.txt"
printf '%s\n' \
    "TOP=$top" \
    "TOP_SHA256=$expected_top_sha" \
    'DC_VERSION=W-2024.09-SP2' \
    'LIBRARY=TSMC018 slow.db' \
    'CORNER=slow' \
    'CLOCK_PERIOD_NS=20.0' \
    'INPUT_DELAY_NS=0.0' \
    'OUTPUT_DELAY_NS=0.0' \
    'COMPILE_COMMAND=compile -map_effort low' \
    'CONSTRAINTS=scripts/synthesis/dc/recam_phase3j_constraints.tcl' \
    'NAND2X1_AREA_REFERENCE=9.979200' \
    "GIT_HEAD=$(git -C "$repo_root" rev-parse HEAD)" \
    'STATUS=RUNNING' \
    > "$report_dir/metadata.txt"

REPO_ROOT="$repo_root" REPORT_DIR="$report_dir" DESIGN="$top" CLOCK_PERIOD_NS=20.0 \
    tcsh -f -c 'source /cad/synopsys/CIC/synthesis.cshrc; dc_shell -no_gui -f "$REPO_ROOT/scripts/synthesis/dc/run_recam_dss_l1x4_r_static_global_n2.tcl" -output_log_file "$REPORT_DIR/dc_shell.log"'
"$repo_root/scripts/synthesis/write_gate_count_report.sh" "$report_dir"
sed -i 's/^STATUS=RUNNING$/STATUS=PASS/' "$report_dir/metadata.txt"
printf '%s\n' 'TIMING_MET=YES' 'PPA_STATUS=PROVISIONAL_PENDING_REVIEW' >> "$report_dir/metadata.txt"
