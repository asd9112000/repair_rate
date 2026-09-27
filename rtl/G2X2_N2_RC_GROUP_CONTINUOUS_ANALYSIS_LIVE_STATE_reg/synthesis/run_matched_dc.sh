#!/usr/bin/env bash
set -euo pipefail

case_dir=$(cd "$(dirname "$0")/.." && pwd)
repo_root=$(cd "$case_dir/../.." && pwd)
case_rel=${case_dir#"$repo_root"/}
top=recam_dss_hyp02_static_global_live_state_top

if [[ $# -ne 2 || $1 != --period-ns ]]; then
    echo "usage: $0 --period-ns {10|15|20|25}" >&2
    exit 2
fi
case $2 in
    10|10.0) period_ns=10.0; period_label=10ns ;;
    15|15.0) period_ns=15.0; period_label=15ns ;;
    20|20.0) period_ns=20.0; period_label=20ns ;;
    25|25.0) period_ns=25.0; period_label=25ns ;;
    *) echo "unsupported period: $2" >&2; exit 2 ;;
esac

report_dir="$case_dir/synthesis/by_period/$period_label"
(cd "$repo_root" && sha256sum -c "$case_rel/synthesis/source_sha256.txt")
mkdir -p "$report_dir"
cp "$case_dir/synthesis/source_manifest.txt" "$report_dir/functional_source_manifest.txt"
cp "$case_dir/synthesis/source_sha256.txt" "$report_dir/functional_source_sha256.txt"
REPO_ROOT="$repo_root" RTL_CASE_DIR="$case_dir" REPORT_DIR="$report_dir" DESIGN="$top" PERIOD_NS="$period_ns" \
    tcsh -f -c 'source /cad/synopsys/CIC/synthesis.cshrc; dc_shell -no_gui -f "$RTL_CASE_DIR/synthesis/run_matched_dc.tcl" -output_log_file "$REPORT_DIR/dc_shell.log"'
(cd "$repo_root" && sha256sum -c "$case_rel/synthesis/source_sha256.txt")
