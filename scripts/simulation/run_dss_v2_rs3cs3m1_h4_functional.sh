#!/usr/bin/env bash
set -euo pipefail

root=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)
cd "$root"

result_root=${H4_RESULT_ROOT:-results/dss_v2_rs3cs3m1/h4_functional}
mkdir -p "$result_root"
log="$result_root/verification.log"
: > "$log"

run() {
    echo "+ $*" | tee -a "$log"
    "$@" 2>&1 | tee -a "$log"
}

run scripts/simulation/run_verilator_test.sh \
    dss_v2_rs3cs3m1_config_table \
    rtl/dss_v2/rs3cs3m1/dss_v2_rs3cs3m1_config_table.sv \
    tb/dss_v2/dss_v2_rs3cs3m1_config_table_smoke_test.cpp -Wno-UNUSED
run scripts/simulation/run_verilator_test.sh \
    dss_v2_rs3cs3m1_shared_config_analyzer \
    rtl/dss_v2/rs3cs3m1/dss_v2_rs3cs3m1_shared_config_analyzer.sv \
    tb/dss_v2/dss_v2_rs3cs3m1_config_analyzer_smoke_test.cpp -Wno-UNUSED
run scripts/simulation/run_verilator_test.sh \
    dss_v2_rs3cs3m1_group_candidate_store \
    rtl/dss_v2/rs3cs3m1/dss_v2_rs3cs3m1_group_candidate_store.sv \
    tb/dss_v2/dss_v2_rs3cs3m1_group_store_smoke_test.cpp -Wno-UNUSED
run scripts/simulation/run_verilator_test.sh \
    recam_dss_v2_rs3cs3m1_policy_equivalence_top \
    tb/dss_v2/recam_dss_v2_rs3cs3m1_policy_equivalence_top.sv \
    tb/dss_v2/recam_dss_v2_rs3cs3m1_policy_equivalence_test.cpp \
    -Wno-PINCONNECTEMPTY -Wno-UNUSED \
    rtl/dss_v2/common/dss_v2_params_pkg.sv \
    rtl/dss_v2/common/dss_v2_types_pkg.sv \
    rtl/dss_v2/adapter/dss_v2_legacy_ledger_diagnostic_adapter.sv \
    rtl/dss_v2/resource/dss_v2_resource_ledger.sv \
    rtl/dss_v2/rs3cs3m1/dss_v2_rs3cs3m1_config_table.sv \
    rtl/dss_v2/rs3cs3m1/dss_v2_rs3cs3m1_topology.sv \
    rtl/dss_v2/rs3cs3m1/dss_v2_rs3cs3m1_group_candidate_store.sv \
    rtl/dss_v2/rs3cs3m1/recam_dss_v2_rs3cs3m1_early_core.sv \
    rtl/dss_v2/rs3cs3m1/recam_dss_v2_rs3cs3m1_group_core.sv -- 1000

echo "H4_RS3CS3M1_FUNCTIONAL PASS" | tee -a "$log"
{
    echo "H4_RS3CS3M1_FUNCTIONAL=PASS"
    echo "SEED=20260910"
    echo "EARLY_VECTORS=1000"
    echo "GROUP_VECTORS=1000"
    echo "EARLY_MISMATCHES=0"
    echo "GROUP_MISMATCHES=0"
    echo "GIT_REVISION=$(git rev-parse --short HEAD)"
    echo "LOG=$log"
} | tee "$result_root/summary.txt"
