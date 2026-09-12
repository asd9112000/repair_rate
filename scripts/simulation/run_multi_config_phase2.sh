#!/usr/bin/env bash
set -euo pipefail
runner=scripts/simulation/run_verilator_test.sh

make test_directional_multi_config_analyzer
mkdir -p results/phase2/golden
build/tests/directional_multi_config_analyzer_test \
    --emit-rtl-golden results/phase2/golden/directional_multi_config.vec

"$runner" lowest_pattern_encoder rtl/dss_2x2/analyzer/lowest_pattern_encoder.sv tb/dss_2x2/lowest_pattern_encoder_test.cpp
"$runner" rc_transpose_wrapper rtl/dss_2x2/analyzer/rc_transpose_wrapper.sv tb/dss_2x2/rc_transpose_wrapper_test.cpp
"$runner" shared_fault_collector rtl/dss_2x2/analyzer/shared_fault_collector.sv tb/dss_2x2/shared_fault_collector_test.cpp \
    rtl/dss_2x2/analyzer/shared_pivot_cam.sv rtl/dss_2x2/analyzer/shared_fault_counter.sv \
    rtl/dss_2x2/analyzer/tagged_hybrid_store.sv rtl/dss_2x2/analyzer/cam_reuse_temp_buffer.sv
"$runner" config_must_view rtl/dss_2x2/analyzer/config_must_view.sv tb/dss_2x2/config_must_view_test.cpp
"$runner" multi_config_analyzer_bank rtl/dss_2x2/analyzer/multi_config_analyzer_bank.sv \
    tb/dss_2x2/multi_config_analyzer_bank_test.cpp rtl/dss_2x2/analyzer/config_analyzer.sv
"$runner" multi_config_analyzer_area_engine rtl/dss_2x2/analyzer/multi_config_analyzer_area_engine.sv \
    tb/dss_2x2/multi_config_analyzer_area_engine_test.cpp \
    rtl/dss_2x2/analyzer/config_logical_view.sv rtl/dss_2x2/analyzer/config_candidate_evaluator.sv
"$runner" config_result_bank rtl/dss_2x2/analyzer/config_result_bank.sv tb/dss_2x2/config_result_bank_test.cpp
"$runner" multi_config_early_selector rtl/dss_2x2/analyzer/early_selector.sv tb/dss_2x2/early_selector_test.cpp \
    rtl/dss_2x2/analyzer/resource_ledger.sv rtl/dss_2x2/common/resource_tracker.sv
"$runner" multi_config_group_selector rtl/dss_2x2/analyzer/group_selector.sv tb/dss_2x2/group_selector_test.cpp \
    rtl/dss_2x2/analyzer/resource_ledger.sv rtl/dss_2x2/common/resource_tracker.sv
"$runner" repair_decode rtl/dss_2x2/analyzer/repair_decode.sv tb/dss_2x2/repair_decode_test.cpp
"$runner" repair_decode_with_hybrid rtl/dss_2x2/analyzer/repair_decode_with_hybrid.sv \
    tb/dss_2x2/repair_decode_with_hybrid_test.cpp
"$runner" pending_repair_buffer rtl/dss_2x2/analyzer/pending_repair_buffer.sv tb/dss_2x2/pending_repair_buffer_test.cpp
"$runner" dss_analyzer_top rtl/dss_2x2/analyzer/dss_analyzer_top.sv tb/dss_2x2/dss_analyzer_top_golden_test.cpp \
    -f scripts/simulation/dss_multi_config_phase2.f -- results/phase2/golden/directional_multi_config.vec
"$runner" dss_analyzer_top rtl/dss_2x2/analyzer/dss_analyzer_top.sv tb/dss_2x2/dss_analyzer_top_golden_test.cpp \
    -GPARALLEL_ORIENTATION=0 \
    -f scripts/simulation/dss_multi_config_phase2.f -- results/phase2/golden/directional_multi_config.vec --area

echo "Directional multi-config Phase 2 regression PASS"
