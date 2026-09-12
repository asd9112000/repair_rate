#!/usr/bin/env bash
set -euo pipefail

root=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)
cd "$root"

v2_common=(
    rtl/dss_v2/common/dss_v2_params_pkg.sv
    rtl/dss_v2/common/dss_v2_types_pkg.sv
    rtl/dss_v2/adapter/dss_legacy_config_adapter.sv
    rtl/dss_v2/adapter/dss_v2_legacy_ledger_diagnostic_adapter.sv
    rtl/dss_v2/topology/dss_topology_2x2_directional.sv
    rtl/dss_v2/resource/dss_v2_resource_feasibility.sv
    rtl/dss_v2/resource/dss_v2_resource_ledger.sv
)
legacy_allocator=(
    rtl/recam/recam_dss_group_allocator.sv
    rtl/recam/recam_resource_aware_fallback_selector.sv
    rtl/recam/recam_physical_resource_ledger_checker.sv
    rtl/recam/recam_config_resource_action_decoder.sv
)

run_equivalence() {
    local vectors=$1
    scripts/simulation/run_verilator_test.sh recam_dss_v2_early_equivalence_test_top \
        tb/dss_v2/recam_dss_v2_early_equivalence_test_top.sv \
        tb/dss_v2/recam_dss_v2_early_equivalence_test.cpp \
        -Wno-PINCONNECTEMPTY -Wno-UNUSED "${v2_common[@]}" \
        rtl/dss_v2/top/recam_dss_v2_early_core.sv "${legacy_allocator[@]}" -- "$vectors"
}

run_equivalence 50
run_equivalence 1000

# Regression protection for each closed V2 foundation and the extracted core.
scripts/simulation/run_verilator_test.sh dss_legacy_config_adapter \
    rtl/dss_v2/adapter/dss_legacy_config_adapter.sv \
    tb/dss_v2/dss_legacy_config_adapter_test.cpp \
    rtl/dss_v2/common/dss_v2_params_pkg.sv rtl/dss_v2/common/dss_v2_types_pkg.sv
scripts/simulation/run_verilator_test.sh dss_v2_resource_feasibility \
    rtl/dss_v2/resource/dss_v2_resource_feasibility.sv \
    tb/dss_v2/dss_v2_resource_feasibility_test.cpp \
    rtl/dss_v2/common/dss_v2_params_pkg.sv rtl/dss_v2/common/dss_v2_types_pkg.sv \
    rtl/dss_v2/topology/dss_topology_2x2_directional.sv
scripts/simulation/run_verilator_test.sh dss_v2_resource_ledger \
    rtl/dss_v2/resource/dss_v2_resource_ledger.sv \
    tb/dss_v2/dss_v2_resource_ledger_test.cpp \
    rtl/dss_v2/common/dss_v2_params_pkg.sv rtl/dss_v2/common/dss_v2_types_pkg.sv
scripts/simulation/run_verilator_test.sh dss_v2_early_single_sa_wrapper \
    tb/dss_v2/dss_v2_early_single_sa_wrapper.sv \
    tb/dss_v2/dss_v2_early_single_sa_wrapper_test.cpp \
    rtl/dss_v2/common/dss_v2_params_pkg.sv rtl/dss_v2/common/dss_v2_types_pkg.sv \
    rtl/dss_v2/adapter/dss_legacy_config_adapter.sv \
    rtl/dss_v2/topology/dss_topology_2x2_directional.sv \
    rtl/dss_v2/resource/dss_v2_resource_feasibility.sv
scripts/simulation/run_verilator_test.sh recam_dss_v2_early_core \
    rtl/dss_v2/top/recam_dss_v2_early_core.sv \
    tb/dss_v2/recam_dss_v2_early_core_test.cpp "${v2_common[@]}"

# Frozen Phase 3H/I allocator regression remains the legacy protection gate.
scripts/simulation/run_recam_phase3hi_functional.sh
