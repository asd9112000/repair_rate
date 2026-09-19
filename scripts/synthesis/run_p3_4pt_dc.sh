#!/usr/bin/env bash
# Run the frozen P3-4PT-SYNTH 20 ns/slow four-point matrix.  Each child
# process elaborates exactly one public top and one dedicated source manifest.
set -euo pipefail

repo_root=$(cd "$(dirname "$0")/../.." && pwd)
timeout_seconds=${TIMEOUT_SECONDS:-600}
clock_period_ns=${CLOCK_PERIOD_NS:-20.0}
result_root=${P3_4PT_SYNTH_RESULT_ROOT:-"$repo_root/results/p3_4pt_synth"}
requested_id=${1:-ALL}

if [[ $requested_id == ALL ]]; then
    for synth_id in SYN_A SYN_B_OPT1 SYN_C SYN_D_OPT1; do
        bash "$0" "$synth_id"
    done
    exit 0
fi

case $requested_id in
    SYN_A)
        design=recam_dss_canonical_rs2_streaming_early_top
        source_manifest="$repo_root/scripts/synthesis/dc/p3_4pt_syn_a_sources.tcl"
        semantic_policy=NORMALIZED_STREAMING_EARLY
        topology=GRID2X2_DIRECTIONAL
        resource_point=RS2_CS2_M1
        global_class=NA
        ;;
    SYN_B_OPT1)
        design=recam_dss_grid2x2_directional_rs2_cs2_m1_normalized_group_global_noscratch_opt1_classcollapsed_integrated_top
        source_manifest="$repo_root/scripts/synthesis/dc/p3_4pt_syn_b_opt1_sources.tcl"
        semantic_policy=NORMALIZED_GROUP_GLOBAL_NOSCRATCH
        topology=GRID2X2_DIRECTIONAL
        resource_point=RS2_CS2_M1
        global_class=OPT1
        ;;
    SYN_C)
        design=recam_dss_line1x4_rs2_cs2_m1_normalized_streaming_early_top
        source_manifest="$repo_root/scripts/synthesis/dc/p3_4pt_syn_c_sources.tcl"
        semantic_policy=NORMALIZED_STREAMING_EARLY
        topology=LINE1X4_SINGLE_HOP
        resource_point=RS2_CS2_M1
        global_class=NA
        ;;
    SYN_D_OPT1)
        design=recam_dss_line1x4_rs2_cs2_m1_normalized_global_opt1_classcollapsed_top
        source_manifest="$repo_root/scripts/synthesis/dc/p3_4pt_syn_d_opt1_sources.tcl"
        semantic_policy=NORMALIZED_GLOBAL
        topology=LINE1X4_SINGLE_HOP
        resource_point=RS2_CS2_M1
        global_class=OPT1
        ;;
    *)
        echo "usage: $0 [ALL|SYN_A|SYN_B_OPT1|SYN_C|SYN_D_OPT1]" >&2
        exit 2
        ;;
esac

report_dir="$result_root/$requested_id"
source_manifest_rel=${source_manifest#"$repo_root"/}
mkdir -p "$report_dir"
cp "$source_manifest" "$report_dir/source_manifest.tcl"
exec > >(tee "$report_dir/runner.log") 2>&1

record_failure() {
    local exit_code=$?
    if [[ $exit_code -ne 0 ]]; then
        sed -i 's/^STATUS=RUNNING$/STATUS=FAILED_OR_BLOCKED/' "$report_dir/metadata.txt"
        printf 'TERMINAL_EXIT_CODE=%s\n' "$exit_code" >> "$report_dir/metadata.txt"
    fi
}
trap record_failure EXIT

printf '%s\n' \
    'PHASE=P3-4PT-SYNTH' \
    "SYNTH_ID=$requested_id" \
    "TOP=$design" \
    "SEMANTIC_POLICY=$semantic_policy" \
    "TOPOLOGY=$topology" \
    "RESOURCE_POINT=$resource_point" \
    "GLOBAL_CLASS=$global_class" \
    "CLOCK_PERIOD_NS=$clock_period_ns" \
    'INPUT_DELAY_NS=0.0' \
    'OUTPUT_DELAY_NS=0.0' \
    'TECHNOLOGY=TSMC018_ARM_CBDK' \
    'PVT=slow' \
    'LIBRARY=/cad/std_libraries/CBDK_TSMC018_Arm_f1.0/CIC/SynopsysDC/db/slow.db' \
    'TOOL=Synopsys Design Compiler W-2024.09-SP2' \
    'COMPILE_MODE=compile -map_effort low' \
    'NAND2X1_REFERENCE_AREA=9.979200' \
    "GIT_HEAD=$(git -C "$repo_root" rev-parse HEAD)" \
    "SOURCE_MANIFEST=$source_manifest_rel" \
    "SOURCE_MANIFEST_SHA256=$(sha256sum "$source_manifest" | awk '{print $1}')" \
    "SYNTHESIS_SCRIPT_SHA256=$(sha256sum "$repo_root/scripts/synthesis/dc/run_p3_4pt_synth.tcl" | awk '{print $1}')" \
    "RTL_TREE_SHA256=$(git -C "$repo_root" ls-tree -r HEAD -- rtl | sha256sum | awk '{print $1}')" \
    'STATUS=RUNNING' > "$report_dir/metadata.txt"

REPO_ROOT="$repo_root" REPORT_DIR="$report_dir" DESIGN="$design" SOURCE_MANIFEST="$source_manifest" \
    REQUESTED_LM_LICENSE_FILE="${LM_LICENSE_FILE-}" CLOCK_PERIOD_NS="$clock_period_ns" \
    timeout --signal=TERM "${timeout_seconds}s" \
    tcsh -f -c 'set requested_lm = "$REQUESTED_LM_LICENSE_FILE"; source /cad/synopsys/CIC/synthesis.cshrc; if ("$requested_lm" != "") setenv LM_LICENSE_FILE "$requested_lm"; dc_shell -no_gui -f "$REPO_ROOT/scripts/synthesis/dc/run_p3_4pt_synth.tcl" -output_log_file "$REPORT_DIR/dc_shell.log"'
"$repo_root/scripts/synthesis/write_gate_count_report.sh" "$report_dir"
sed -i 's/^STATUS=RUNNING$/STATUS=PASS/' "$report_dir/metadata.txt"
echo "P3-4PT-SYNTH $requested_id Design Compiler synthesis PASS"
