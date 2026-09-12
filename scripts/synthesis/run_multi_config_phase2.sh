#!/usr/bin/env bash
set -euo pipefail

output_root=results/phase2/multi_config_synthesis
yosys_timeout_seconds=${YOSYS_TIMEOUT_SECONDS:-600}
sources=rtl/dss_2x2/analyzer/dss_analyzer_top.sv:rtl/dss_2x2/analyzer/shared_fault_collector.sv:rtl/dss_2x2/analyzer/shared_pivot_cam.sv:rtl/dss_2x2/analyzer/shared_fault_counter.sv:rtl/dss_2x2/analyzer/tagged_hybrid_store.sv:rtl/dss_2x2/analyzer/cam_reuse_temp_buffer.sv:rtl/dss_2x2/analyzer/config_must_view.sv:rtl/dss_2x2/analyzer/hybrid_delta_bank.sv:rtl/dss_2x2/analyzer/multi_config_analyzer_bank.sv:rtl/dss_2x2/analyzer/multi_config_analyzer_area_engine.sv:rtl/dss_2x2/analyzer/config_logical_view.sv:rtl/dss_2x2/analyzer/config_analyzer.sv:rtl/dss_2x2/analyzer/config_result_bank.sv:rtl/dss_2x2/analyzer/early_selector.sv:rtl/dss_2x2/analyzer/group_selector.sv:rtl/dss_2x2/analyzer/resource_ledger.sv:rtl/dss_2x2/analyzer/config_resource_decode.sv:rtl/dss_2x2/common/resource_tracker.sv:rtl/dss_2x2/analyzer/pending_repair_buffer.sv:rtl/dss_2x2/analyzer/repair_decode.sv:rtl/dss_2x2/analyzer/repair_decode_with_hybrid.sv:rtl/dss_2x2/analyzer/repair_decode_area_engine.sv

settings=${SETTINGS:-"conservative14 experimental7"}

for setting in $settings; do
    if [[ $setting != conservative14 && $setting != experimental7 ]]; then
        echo "Unsupported Phase 2 synthesis setting: $setting" >&2
        exit 2
    fi
    if [[ $setting == conservative14 ]]; then entries=14; else entries=7; fi
    report_dir="$output_root/$setting"
    parameters="ROW_W=10,COL_W=10,MAX_FAULTS=12,HYBRID_SHARED_ENTRIES=$entries,SHARED_ROWS=1,SHARED_COLS=1,MAX_BORROWS=3,PARALLEL_ORIENTATION=0"
    mkdir -p "$report_dir"
    printf 'PHASE=2\nCONFIGURATION=%s\nTOP=dss_analyzer_top\nPARAMS=%s\nCLK_PERIOD_NS=10.0\nTECHNOLOGY=YOSYS_STRUCTURAL\nLIBRARY=NONE\nSTATUS=RUNNING\n' \
        "$setting" "$parameters" > "$report_dir/metadata.txt"
    set +e
    TOP=dss_analyzer_top SOURCES="$sources" PARAMS="$parameters" FLOW=STRUCTURAL \
        REPORT_DIR="$report_dir" CLK_PERIOD=10.0 \
        timeout --signal=TERM "${yosys_timeout_seconds}s" \
        yosys -q -l "$report_dir/yosys.log" -c scripts/synthesis/synth.tcl
    status=$?
    set -e
    if [[ $status -eq 124 ]]; then
        printf 'STATUS=TIMEOUT\nTIMEOUT_SECONDS=%s\n' "$yosys_timeout_seconds" >> "$report_dir/metadata.txt"
        echo "Phase 2 Yosys structural synthesis timed out for $setting after ${yosys_timeout_seconds}s" >&2
        exit 124
    elif [[ $status -ne 0 ]]; then
        printf 'STATUS=FAIL\nEXIT_CODE=%s\n' "$status" >> "$report_dir/metadata.txt"
        echo "Phase 2 Yosys structural synthesis failed for $setting (exit $status)" >&2
        exit "$status"
    fi
    printf 'STATUS=PASS\n' >> "$report_dir/metadata.txt"
done

python3 scripts/parse_reports.py --input-root "$output_root" \
    --output results/phase2/multi_config_synthesis_results.csv \
    --module-breakdown-output results/phase2/multi_config_module_area_breakdown.csv
echo "Directional multi-config Phase 2 synthesis PASS"
