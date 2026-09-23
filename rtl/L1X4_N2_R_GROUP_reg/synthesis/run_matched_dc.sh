#!/usr/bin/env bash
set -euo pipefail

case_dir=$(cd "$(dirname "$0")/.." && pwd)
top=recam_dss_l1x4_r_static_global_top
report_dir="$case_dir/synthesis/reports"

(cd "${case_dir}/../.." && sha256sum -c "${case_dir#$PWD/}/source_sha256.txt")
mkdir -p "$report_dir"
cp "$case_dir/source_manifest.txt" "$report_dir/functional_source_manifest.txt"
cp "$case_dir/source_sha256.txt" "$report_dir/functional_source_sha256.txt"
RTL_CASE_DIR="$case_dir" REPORT_DIR="$report_dir" DESIGN="$top" \
    tcsh -f -c 'source /cad/synopsys/CIC/synthesis.cshrc; dc_shell -no_gui -f "$RTL_CASE_DIR/synthesis/run_matched_dc.tcl" -output_log_file "$REPORT_DIR/dc_shell.log"'
(cd "${case_dir}/../.." && sha256sum -c "${case_dir#$PWD/}/source_sha256.txt")
