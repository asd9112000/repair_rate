#!/usr/bin/env bash
set -euo pipefail

repo_root=$(cd "$(dirname "$0")/../.." && pwd)
timeout_seconds=${TIMEOUT_SECONDS:-600}
compile_mode=${COMPILE_MODE:-structural_report}
clock_period_ns=${CLOCK_PERIOD_NS:-20.0}
input_delay_ns=${INPUT_DELAY_NS:-0.0}
output_delay_ns=${OUTPUT_DELAY_NS:-0.0}

run_block() {
    local variant=$1
    local design=$2
    local params=$3
    local tag="blocks_${variant}_${design}_${compile_mode}"
    DESIGN="$design" ELAB_PARAMS="$params" REPORT_TAG="$tag" \
        TIMEOUT_SECONDS="$timeout_seconds" COMPILE_MODE="$compile_mode" \
        CLOCK_PERIOD_NS="$clock_period_ns" INPUT_DELAY_NS="$input_delay_ns" \
        OUTPUT_DELAY_NS="$output_delay_ns" \
        "$repo_root/scripts/synthesis/run_multi_config_phase2_dc.sh"
}

# Major state/compute boundaries of the AREA architecture.  Each uses the same
# source manifest and target library as the integrated top; the top itself is
# separately elaborated/link-checked by run_multi_config_phase2_dc.sh.
run_block conservative14 shared_fault_collector "HYBRID_SHARED_ENTRIES=14"
run_block conservative14 multi_config_analyzer_area_engine "HYBRID_ENTRY_NUM=14"
run_block conservative14 multi_config_group_selector ""
run_block conservative14 multi_config_early_selector ""
run_block conservative14 pending_repair_buffer "HYBRID_ENTRIES=14"
run_block conservative14 repair_decode_area_engine "ENTRY_NUM=14"

# Experimental shared-Hybrid sizing variant required by the handoff.  Only the
# blocks whose storage or datapath width depends on Hybrid capacity are rerun.
run_block measured7 shared_fault_collector "HYBRID_SHARED_ENTRIES=7"
run_block measured7 multi_config_analyzer_area_engine "HYBRID_ENTRY_NUM=7"
run_block measured7 pending_repair_buffer "HYBRID_ENTRIES=7"
run_block measured7 repair_decode_area_engine "ENTRY_NUM=7"

python3 "$repo_root/scripts/synthesis/parse_dc_phase2_reports.py" \
    --input-root "$repo_root/results/phase2/dc_tsmc018_slow" \
    --output "$repo_root/results/phase2/dc_tsmc018_slow/phase2_dc_results.csv"

echo "Phase 2 Design Compiler per-major-block synthesis PASS"
