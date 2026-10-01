#!/usr/bin/env bash
set -euo pipefail

repo_root=$(cd "$(dirname "$0")/../../.." && pwd)
archive_dir="$repo_root/dss_latency/N2_CONTINUOUS_ANALYSIS/full_sixcase_timing_sweep"
tcl="$archive_dir/run_matched_ca_live_period.tcl"

if [[ $# -ne 2 || $1 != --case || $2 != G2X2_R_EARLY && $2 != G2X2_R_GROUP && $2 != L1X4_R_EARLY && $2 != L1X4_R_GROUP ]]; then
    echo "usage: $0 --case {G2X2_R_EARLY|G2X2_R_GROUP|L1X4_R_EARLY|L1X4_R_GROUP}" >&2
    exit 2
fi

case_id=$2
case "$case_id" in
    G2X2_R_EARLY)
        design=recam_dss_g2x2_r_static_early_live_state_top
        case_kind=EARLY
        live_dir="$repo_root/rtl/G2X2_N2_R_EARLY_CONTINUOUS_ANALYSIS_LIVE_STATE_reg"
        canonical_dir="$repo_root/rtl/G2X2_N2_R_EARLY"
        ;;
    G2X2_R_GROUP)
        design=recam_dss_g2x2_r_static_global_live_state_top
        case_kind=GROUP
        live_dir="$repo_root/rtl/G2X2_N2_R_GROUP_CONTINUOUS_ANALYSIS_LIVE_STATE_reg"
        canonical_dir="$repo_root/rtl/G2X2_N2_R_GROUP_reg"
        ;;
    L1X4_R_EARLY)
        design=recam_dss_l1x4_r_static_early_live_state_top
        case_kind=EARLY
        live_dir="$repo_root/rtl/L1X4_N2_R_EARLY_CONTINUOUS_ANALYSIS_LIVE_STATE_reg"
        canonical_dir="$repo_root/rtl/L1X4_N2_R_EARLY"
        ;;
    L1X4_R_GROUP)
        design=recam_dss_l1x4_r_static_global_live_state_top
        case_kind=GROUP
        live_dir="$repo_root/rtl/L1X4_N2_R_GROUP_CONTINUOUS_ANALYSIS_LIVE_STATE_reg"
        canonical_dir="$repo_root/rtl/L1X4_N2_R_GROUP_reg"
        ;;
esac

for period_ns in 10.0 15.0 25.0; do
    period_label=${period_ns%.0}ns
    report_dir="$archive_dir/raw/$case_id/$period_label"
    mkdir -p "$report_dir"
    (cd "$repo_root" && sha256sum -c "$archive_dir/provenance/SOURCE_SHA256SUMS")
    REPO_ROOT="$repo_root" LIVE_DIR="$live_dir" CANONICAL_DIR="$canonical_dir" \
        REPORT_DIR="$report_dir" DESIGN="$design" CASE_KIND="$case_kind" PERIOD_NS="$period_ns" TCL_FILE="$tcl" \
        tcsh -f -c 'source /cad/synopsys/CIC/synthesis.cshrc; dc_shell -no_gui -f "$TCL_FILE" -output_log_file "$REPORT_DIR/dc_shell.log"' \
        > "$report_dir/runner.log" 2>&1
    test -f "$report_dir/metadata.txt"
    grep -qx 'STATUS=PASS' "$report_dir/metadata.txt"
    for report in area.rpt timing.rpt qos.rpt constraints.rpt hierarchy_area.rpt runner.log dc_shell.log; do
        test -f "$report_dir/$report" || { echo "missing $report_dir/$report" >&2; exit 1; }
    done
    (cd "$repo_root" && sha256sum -c "$archive_dir/provenance/SOURCE_SHA256SUMS")
done
